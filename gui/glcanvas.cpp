/*
Copyright (C) 2026 by Okadome Valencia

hubert.valencia _at_ ovhpa.net

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

The GNU GPL can also be found at http://www.gnu.org
*/

/*
 * Qt OpenGL Canvas Widget for GDIS
 */

#include "glcanvas.h"
#include "gdis_api.h"
#include "pak.h"

extern struct sysenv_pak sysenv;

#include <QPainter>

/* C function declarations - gdis_api.h already provides these */
extern "C" {
typedef void (*gui_event_func)(void *, void *);
gui_event_func get_gui_press_handler(void);
gui_event_func get_gui_motion_handler(void);
gui_event_func get_gui_release_handler(void);
gui_event_func get_gui_scroll_handler(void);
void set_mouse_event_data(int x, int y, int button, int state);
void set_scroll_event_data(int delta);
void set_key_event_data(int key, int state);
void gui_key_release_event(void *, void *);

/* Canvas and rendering - declared with C linkage */
void canvas_resize(void);
void redraw_canvas(int);
gint gl_canvas_refresh(void);
void set_canvas_dimensions(int w, int h);

/* Direct event handlers (called from Qt) */
gint gui_press_event(void);
gint gui_motion_event(void);
gint gui_release_event(void);
gint gui_scroll_event(void);
}

/* Simple mouse event data storage */
static int g_mouse_x = 0, g_mouse_y = 0;
static int g_mouse_button = 0, g_mouse_state = 0;
static int g_scroll_delta = 0;

GLCanvas::GLCanvas(QWidget *parent) : QOpenGLWidget(parent), m_canvasIndex(0)
{
  setMinimumSize(50, 50);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  /* Try stereo first if configured, then fall back to non-stereo */
  QSurfaceFormat fmt;
  fmt.setVersion(2, 1);
  fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
  /* Qt handles stereo via surface format - default to non-stereo */
  fmt.setStereo(false);

  /* Allow fallback to single buffer if double buffer fails */
  fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
  fmt.setSwapInterval(0); /* Disable vsync for immediate updates */

  setFormat(fmt);

  /* Accept keyboard focus so key events reach the canvas */
  setFocusPolicy(Qt::StrongFocus);

  /* Create the canvas entry in sysenv.canvas_list */
  extern void canvas_create_entry(void);
  canvas_create_entry();

  /* Mark Qt mode so C core skips initialization */
  extern void set_qt_mode(int);
  set_qt_mode(1);

  /* Optimize: don't auto-refresh, only on explicit update() */
  setAutoFillBackground(false);
}

GLCanvas::~GLCanvas() = default;

void GLCanvas::setCanvasIndex(int idx) { m_canvasIndex = idx; }

void GLCanvas::initializeGL()
{
  if (!context()->isValid())
  {
    QSurfaceFormat fmt = format();
    fmt.setSwapBehavior(QSurfaceFormat::SingleBuffer);
    fmt.setStereo(false);
    setFormat(fmt);
    update();
    return;
  }

  initializeOpenGLFunctions();
  /* Background colour will be set correctly in paintGL via sysenv.render.bg_colour.
   * make_fg_visible() adapts fg colours to the current bg on first real draw. */
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

#define DEBUG_PAINTGL 0
void GLCanvas::paintGL()
{
  /* Semaphore: prevent re-entrant paintGL calls.
   * Qt may queue multiple update() requests during rapid mouse motion.
   * Only the first one actually renders; subsequent ones are skipped
   * to avoid double-clearing and interleaved draw operations. */
  static bool rendering = false;
  if (rendering)
    {
      return;
    }
  rendering = true;

  /* Clear drawing_in_progress at start of paintGL to prevent stale semaphore.
   * If a previous frame was interrupted (e.g., by canvas_timing_adjust early exit),
   * this flag would block all subsequent renders. */
  if (sysenv.active_model)
    ((struct model_pak *) sysenv.active_model)->drawing_in_progress = FALSE;

  /* Safety: if no canvas list yet (init phase), just return — will be drawn
   * once the model is loaded and paintGL is called again via update(). */
  extern gint qt_has_canvas_list(void);
  if (!qt_has_canvas_list())
  {
    // fprintf(stderr, "[PAINT]  Skipping: no canvas list\n");
    rendering = false;
    return;
  }

  /* gl_canvas_refresh returns TRUE if it rendered something, FALSE if skipped.
   * Only push overlays when we actually drew — otherwise the previous frame's
   * overlay data is still valid and freeing it would make text disappear. */
  gint did_render = gl_canvas_refresh();
#if 1
  // fprintf(stderr, "[PAINT]  gl_canvas_refresh returned %d\n", did_render);
#endif
  if (did_render)
  {
    /* Push overlays to Qt widget for rendering.
     * Overlays are NOT freed here — they persist across frames so the overlay
     * widget's deep-copied images remain valid even during a second paintGL call.
     * They're only cleared when canvas dimensions change (positions become stale). */
    extern void qt_push_text_overlays_to_widget(void);
    qt_push_text_overlays_to_widget();
  }

  rendering = false;
}

QSize GLCanvas::sizeHint() const { return QSize(200, 200); }

void GLCanvas::resizeGL(int w, int h)
{
  if (h == 0)
    h = 1;
  glViewport(0, 0, (GLint) w, (GLint) h);

  extern void set_canvas_dimensions(int w, int h);
  set_canvas_dimensions(w, h);

  canvas_resize();
  redraw_canvas(1); /* ALL */
}

void GLCanvas::mousePressEvent(QMouseEvent *ev)
{
  int button = 0;
  switch (ev->button())
  {
  case Qt::LeftButton:
    button = 1;
    break;
  case Qt::RightButton:
    button = 3;
    break;
  case Qt::MiddleButton:
    button = 2;
    break;
  default:
    button = 0;
    break;
  }
  int state = 0;
  if (ev->modifiers() & Qt::ShiftModifier)
    state |= 1;
  if (ev->modifiers() & Qt::ControlModifier)
    state |= 4;
  if (button == 1)
    state |= 256;
  else if (button == 2)
    state |= 512;
  else if (button == 3)
    state |= 1024;
  set_mouse_event_data(ev->pos().x(), ev->pos().y(), button, state);
  gui_press_event();
  update(); /* Trigger repaint after press (selection, box start, etc.) */
}

void GLCanvas::mouseMoveEvent(QMouseEvent *ev)
{
  int button = 0;
  Qt::MouseButtons btns = ev->buttons();
  if (btns & Qt::LeftButton)
    button = 1;
  else if (btns & Qt::RightButton)
    button = 3;
  else if (btns & Qt::MiddleButton)
    button = 2;
  int state = 0;
  if (ev->modifiers() & Qt::ShiftModifier)
    state |= 1;
  if (ev->modifiers() & Qt::ControlModifier)
    state |= 4;
  if (btns & Qt::LeftButton)
    state |= 256;
  else if (btns & Qt::MiddleButton)
    state |= 512;
  else if (btns & Qt::RightButton)
    state |= 1024;
  set_mouse_event_data(ev->pos().x(), ev->pos().y(), button, state);

  /* No drag — ignore. */
  if (button == 0)
    return;

  /* Call C-side transform directly (no deferral). The C code sets model->redraw=TRUE,
   * and redraw_canvas() calls qt_force_canvas_refresh(). paintGL's semaphore ensures
   * only one render per event loop iteration, even if multiple mouse-move events queue. */
  gui_motion_event();
  update(); /* Request repaint — coalesced by Qt into single paintGL call */
}

void GLCanvas::mouseReleaseEvent(QMouseEvent *ev)
{
  int button = 0;
  switch (ev->button())
  {
  case Qt::LeftButton:
    button = 1;
    break;
  case Qt::RightButton:
    button = 3;
    break;
  case Qt::MiddleButton:
    button = 2;
    break;
  default:
    button = 0;
    break;
  }
  int state = 0;
  if (ev->modifiers() & Qt::ShiftModifier)
    state |= 1;
  if (ev->modifiers() & Qt::ControlModifier)
    state |= 4;
  set_mouse_event_data(ev->pos().x(), ev->pos().y(), button, state);
  gui_release_event();
  update(); /* Trigger repaint after release (selection commit, etc.) */
}

void GLCanvas::wheelEvent(QWheelEvent *ev)
{
  set_scroll_event_data(ev->angleDelta().y());
  gui_scroll_event();
  update(); /* Request repaint after zoom */
}

void GLCanvas::keyPressEvent(QKeyEvent *ev)
{
  /* Handle Delete key for atom deletion */
  if (ev->key() == Qt::Key_Delete)
  {
    extern void select_delete(void);
    extern void redraw_canvas(gint);
    select_delete();
    redraw_canvas(1); /* ALL */
    update();
    return;
  }
  set_key_event_data(ev->key(), 1);
}

void GLCanvas::keyReleaseEvent(QKeyEvent *ev)
{
  set_key_event_data(ev->key(), 0);
  gui_key_release_event(NULL, NULL);
}
