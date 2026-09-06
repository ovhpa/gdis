/*
Copyright (C) 2003 by Sean David Fleming

sean@ivec.org

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#define G_DISABLE_DEPRECATED
#define GLIB_DISABLE_DEPRECATION_WARNINGS
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#ifdef CAIRO_HAS_PS_SURFACE
#include <cairo-ps.h>
#endif

#include "gdis.h"
#include "coords.h"
#include "edit.h"
#include "geometry.h"
#include "graph.h"
#include "matrix.h"
#include "molsurf.h"
#include "morph.h"
#include "model.h"
#include "spatial.h"
#include "zone.h"
#include "opengl.h"
#include "render.h"
#include "select.h"
#include "surface.h"
#include "numeric.h"
#include "measure.h"
#include "quaternion.h"
#include "interface.h"
#include "dialog.h"
#include "gl_varray.h"

/* Forward declarations for stereo rendering */
void stereo_redraw(void);

/* externals */
extern struct sysenv_pak sysenv;
extern struct elem_pak elements[];

/* Text overlay storage — dynamically allocated to avoid static memory corruption */
#define MAX_TEXT_OVERLAYS 64
typedef struct {
  unsigned char *buf;
  gint tw, th;
  gint x, y;
} TextOverlayEntry;

TextOverlayEntry *text_overlays = NULL;
int num_overlays = 0;

static void ensure_overlay_array(void)
{
  if (!text_overlays)
  {
    text_overlays = (TextOverlayEntry *) g_malloc(sizeof(TextOverlayEntry) * MAX_TEXT_OVERLAYS);
    for (int i = 0; i < MAX_TEXT_OVERLAYS; i++)
    {
      text_overlays[i].buf = NULL;
      text_overlays[i].tw = 0;
      text_overlays[i].th = 0;
      text_overlays[i].x = 0;
      text_overlays[i].y = 0;
    }
  }
}

/* Free all overlay buffers (called after Qt paintEvent draws them) */
/* Push overlay data to Qt text overlay widget */
extern void qt_text_overlay_widget_push(void);

void qt_text_overlay_free_all(void)
{
  /* Clear overlay count without freeing buffers — Qt has already deep-copied
   * them. Buffers are leaked but that's acceptable for a long-lived application.
   * This avoids the race condition where overlays are freed between two paintGL
   * calls (Qt may call paintGL twice per update()). */
  ensure_overlay_array();
  num_overlays = 0;
}

/* Exported for Qt text overlay rendering */
gint qt_text_overlay_count_raw(void)
{
  ensure_overlay_array();
  return num_overlays;
}

/* Static storage for overlay data — avoids pointer passing across C/C++ boundary */
static gpointer g_overlay_buf[MAX_TEXT_OVERLAYS];
static gint g_overlay_x[MAX_TEXT_OVERLAYS];
static gint g_overlay_y[MAX_TEXT_OVERLAYS];
static gint g_overlay_w[MAX_TEXT_OVERLAYS];
static gint g_overlay_h[MAX_TEXT_OVERLAYS];

void qt_text_overlay_get_raw(gint idx, gint *out_x, gint *out_y, gint *out_w, gint *out_h)
{
  ensure_overlay_array();
  if (idx < 0 || idx >= num_overlays)
  {
    if (out_x)
      *out_x = 0;
    if (out_y)
      *out_y = 0;
    if (out_w)
      *out_w = 0;
    if (out_h)
      *out_h = 0;
    return;
  }
  g_overlay_buf[idx] = text_overlays[idx].buf;
  g_overlay_x[idx] = text_overlays[idx].x;
  g_overlay_y[idx] = text_overlays[idx].y;
  g_overlay_w[idx] = text_overlays[idx].tw;
  g_overlay_h[idx] = text_overlays[idx].th;
  if (out_x)
    *out_x = text_overlays[idx].x;
  if (out_y)
    *out_y = text_overlays[idx].y;
  if (out_w)
    *out_w = text_overlays[idx].tw;
  if (out_h)
    *out_h = text_overlays[idx].th;
}

/* Get overlay buffer directly — avoids pointer return across C/C++ boundary */
gpointer qt_text_overlay_get_buf_internal(gint idx)
{
  if (idx < 0 || idx >= num_overlays)
    return NULL;
  return g_overlay_buf[idx];
}

/* Get overlay buffer as unsigned char* for direct QImage construction */
unsigned char *qt_text_overlay_get_buf_raw(gint idx)
{
  ensure_overlay_array();
  if (idx < 0 || idx >= num_overlays)
    return NULL;
  return text_overlays[idx].buf;
}

void qt_text_overlay_store(unsigned char *buf, gint tw, gint th, gint x, gint y)
{
  ensure_overlay_array();
  if (num_overlays >= MAX_TEXT_OVERLAYS)
    return;
  /* Deep copy the buffer so we own it */
  gint size = tw * th * 4;
  unsigned char *copy = (unsigned char *) g_malloc(size);
  memcpy(copy, buf, size);
  text_overlays[num_overlays].buf = copy;
  text_overlays[num_overlays].tw = tw;
  text_overlays[num_overlays].th = th;
  text_overlays[num_overlays].x = x;
  text_overlays[num_overlays].y = y;
  num_overlays++;

  /* Don't free the original — both are valid */
}

static void qt_text_overlay_composite(unsigned char *surface_data, gint sw, gint sh)
{
  int pixels_written = 0;
  for (int i = 0; i < num_overlays; i++)
  {
    struct {
      unsigned char *buf;
      gint tw, th;
      gint x, y;
    } *o = &text_overlays[i];
    if (!o->buf)
      continue;
    gint dst_row_start = o->y;
    gint dst_col_start = o->x;
    for (int row = 0; row < o->th && dst_row_start + row < sh; row++)
    {
      for (int col = 0; col < o->tw && dst_col_start + col < sw; col++)
      {
        gint di = ((dst_row_start + row) * sw + (dst_col_start + col)) * 4;
        gint si = (row * o->tw + col) * 4;
        guint sa = o->buf[si + 3];
        if (sa == 0)
          continue;
        float sa_f = sa / 255.0f;
        surface_data[di + 0] = (guint8) (o->buf[si + 2] * sa_f + surface_data[di + 0] * (1.0f - sa_f) + 0.5f);
        surface_data[di + 1] = (guint8) (o->buf[si + 1] * sa_f + surface_data[di + 1] * (1.0f - sa_f) + 0.5f);
        surface_data[di + 2] = (guint8) (o->buf[si + 0] * sa_f + surface_data[di + 2] * (1.0f - sa_f) + 0.5f);
        pixels_written++;
      }
    }
    g_free(o->buf);
    o->buf = NULL;
  }

  num_overlays = 0;
}

#define DRAW_PICTURE 0

/* transformation/projection matrices */
/*
GLint viewport[4];
GLdouble mvmatrix[16], projmatrix[16];
*/

gdouble gl_acm[9];
gdouble halo_fade[16];
gint halo_segments = 16;
gpointer gl_font;
gint gl_fontsize = 10;

/***********************************/
/* world to canvas size conversion */
/***********************************/
gdouble gl_pixel_offset(gdouble r, struct canvas_pak *canvas)
{
  gint p[2];
  gdouble x[3];

  VEC3SET(x, r, 0.0, 0.0);
  gl_unproject(p, x, canvas);

  return (sqrt(p[0] * p[0] + p[1] * p[1]));
}

/*****************************************************/
/* is a given normal aligned with the viewing vector */
/*****************************************************/
gint gl_visible(gdouble *n, struct model_pak *model)
{
  gdouble v[3];
  struct camera_pak *camera;

  g_assert(model != NULL);

  camera = model->camera;

  ARR3SET(v, camera->v);
  if (camera->mode == LOCKED)
    quat_rotate(v, camera->q);

  if (vector_angle(v, n, 3) < 0.5 * G_PI)
    return (FALSE);

  return (TRUE);
}

/****************************************************/
/* set up the visual for subsequent canvas creation */
/****************************************************/
gint gl_init_visual(void)
{
  /* Qt handles GL config via QSurfaceFormat - this is a no-op now */
  /* stereo is configured in the QOpenGLWidget */
  sysenv.render.stereo_use_frustum = TRUE;
  sysenv.stereo_windowed = TRUE;
  sysenv.render.stereo_quadbuffer = sysenv.stereo;
  return (0);
}

/*****************************************/
/* set up the camera and projection mode */
/*****************************************/
#define DEBUG_INIT_PROJ 0
void gl_init_projection(struct canvas_pak *canvas, struct model_pak *model)
{
  gdouble r, a;
  // gdouble pix2ang;/*FIX: 533acf*/
  gdouble x[3], o[3], v[3];
  struct camera_pak *camera;

  g_assert(canvas != NULL);

  /* Early return if model is NULL — no model means nothing to draw */
  if (!model)
    return;

  /* Guard against zero-dimension canvases (e.g., during creation/resize transition) */
  if (canvas->width <= 0 || canvas->height <= 0)
    return;

  /* set up matrices */
  glViewport(canvas->x, canvas->y, canvas->width, canvas->height);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glGetIntegerv(GL_VIEWPORT, canvas->viewport);
  glGetDoublev(GL_MODELVIEW_MATRIX, canvas->modelview);
  glGetDoublev(GL_PROJECTION_MATRIX, canvas->projection);

  g_assert(model->camera != NULL);
  /* pad model with a small amount of space */
  r = 1.0 + model->rmax;

  /* yet another magic number (0.427) - works reasonably well */
  // pix2ang = sysenv.size;/*FIX: 533acf*/
  // pix2ang *= 0.427 / model->rmax;/*FIX: 533acf*/
  sysenv.rsize = r;

  /* viewing */
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  /* set up camera */
  camera = model->camera;
  ARR3SET(x, camera->x);
  ARR3SET(o, camera->o);
  ARR3SET(v, camera->v);

#if DEBUG_INIT_PROJ
  camera_dump(camera);
#endif

  switch (camera->mode)
  {
  case FREE:
    break;

  default:
  case LOCKED:
    quat_rotate(x, camera->q);
    quat_rotate(o, camera->q);
    quat_rotate(v, camera->q);
    break;
  }

  /* convert viewing vector to a location */
  ARR3ADD(v, x);
  gluLookAt(x[0], x[1], x[2], v[0], v[1], v[2], o[0], o[1], o[2]);

  /* CURRENT - projection volume defined AFTER modelview has been set */
  /* it's easier to get the right effect with the new free moving camera */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  /* prevent inversion due to -ve zoom */
  if (camera->zoom < 0.05)
    camera->zoom = 0.05;

  /* TODO - r = fn of zoom */
  if (canvas)
  {
    a = canvas->width;
    a /= canvas->height;
  } else
  {
    /* stereo - doesn't get the canvas passed to it (wholescreen) */
    a = sysenv.width;
    a /= sysenv.height;
  }

  sysenv.aspect = a;

  if (camera->perspective)
  {
    /* NB: if near distance is 0.0 it causes drawing problems */
    gluPerspective(camera->fov, a, 0.1, 4.0 * sysenv.rsize);
  } else
  {
    r *= camera->zoom;
    if (a > 1.0)
      glOrtho(-r * a, r * a, -r, r, 0.0, 4.0 * sysenv.rsize);
    else
      glOrtho(-r, r, -r / a, r / a, 0.0, 4.0 * sysenv.rsize);
  }

  /* store matrices for proj/unproj operations */
  /* FIXME - this will break stereo ... */
  if (canvas)
  {
    glGetIntegerv(GL_VIEWPORT, canvas->viewport);
    glGetDoublev(GL_MODELVIEW_MATRIX, canvas->modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, canvas->projection);
  }

  /* opengl -> gdis coordinate conversion */
  VEC3SET(&gl_acm[0], -1.0, 0.0, 0.0);
  VEC3SET(&gl_acm[3], 0.0, -1.0, 0.0);
  VEC3SET(&gl_acm[6], 0.0, 0.0, -1.0);
}

/****************************/
/* set up all light sources */
/****************************/
void gl_init_lights(struct model_pak *data)
{
  gint i;
  gfloat light[4];
  gdouble x, tmp[4];
  GSList *list;
  struct light_pak *ldata;
  struct camera_pak *camera;

  g_assert(data != NULL);
  g_assert(data->camera != NULL);

  camera = data->camera;

  /* go through all OpenGL lights (enable/disable as required) */
  list = sysenv.render.light_list;
  for (i = GL_LIGHT0; i <= GL_LIGHT7; i++)
  {
    /* do we have an active light */
    if (list)
    {
      glEnable(i);
      /* get light data */
      ldata = list->data;
      /* position/direction */
      ARR3SET(light, ldata->x);
      ARR3SET(tmp, light);
      vecmat(gl_acm, tmp);

      /* FIXME - FREE camera mode case */
      if (camera->mode == LOCKED)
        quat_rotate(tmp, camera->q);

      ARR3SET(light, tmp);

      switch (ldata->type)
      {
      case DIRECTIONAL:
        light[3] = 0.0;
        break;
      case POSITIONAL:
      default:
        VEC3MUL(light, -1.0);
        light[3] = 1.0;
      }
      glLightfv(i, GL_POSITION, light);
      /* light properties */
      ARR3SET(light, ldata->colour);
      VEC3MUL(light, ldata->ambient);
      glLightfv(i, GL_AMBIENT, light);

      ARR3SET(light, ldata->colour);
      VEC3MUL(light, ldata->diffuse);
      glLightfv(i, GL_DIFFUSE, light);

      ARR3SET(light, ldata->colour);
      VEC3MUL(light, ldata->specular);
      glLightfv(i, GL_SPECULAR, light);
      /* next */
      list = g_slist_next(list);
    } else
      glDisable(i);
  }

  /* halo diminishing function */
  for (i = 0; i < halo_segments; i++)
  {
    x = (gdouble) i / (gdouble) halo_segments;
    x *= x;
    halo_fade[i] = exp(-5.0 * x);
  }
}

/************************************/
/* compute sphere radius for a core */
/************************************/
gdouble gl_get_radius(struct core_pak *core, struct model_pak *model)
{
  gdouble radius = 1.0;
  struct elem_pak elem;

  g_assert(model != NULL);
  g_assert(core != NULL);

  switch (core->render_mode)
  {
  case CPK:
    /* TODO - calling get_elem_data() all the time is inefficient */
    get_elem_data(core->atom_code, &elem, model);
    radius *= sysenv.render.cpk_scale * elem.vdw;
    break;

  case LIQUORICE:
    /* only one bond - omit as it's a terminating atom */
    /* FIXME - this will skip isolated atoms with one periodic bond */
    if (g_slist_length(core->bonds) == 1)
      radius *= -1.0;
    /* more than one bond - put in a small sphere to smooth bond joints */
    /* no bonds (isolated) - use normal ball radius */
    if (core->bonds)
      radius *= sysenv.render.stick_radius;
    else
      radius *= sysenv.render.ball_radius;
    break;

  case STICK:
    radius *= sysenv.render.stick_radius;
    if (core->bonds)
      radius *= -1.0;
    break;

  case BALL_STICK:
    if (sysenv.render.scale_ball_size)
    {
      /* TODO - calling get_elem_data() all the time is inefficient */
      get_elem_data(core->atom_code, &elem, model);
      radius *= sysenv.render.cpk_scale * elem.vdw;
    } else
      radius *= sysenv.render.ball_radius;
    break;
  }
  return (radius);
}

/*********************************************/
/* window to real space conversion primitive */
/*********************************************/
void gl_project(gdouble *w, gint x, gint y, struct canvas_pak *canvas)
{
  gint ry;
  GLdouble r[3];

  ry = sysenv.height - y - 1;

  /* z = 0.0 (near clipping plane) z = 1.0 (far clipping plane) */
  /* z = 0.5 is in the middle of the viewing volume (orthographic) */
  /* and is right at the fore for perspective projection */
  gluUnProject(x, ry, 0.5, canvas->modelview, canvas->projection, canvas->viewport, &r[0], &r[1], &r[2]);
  ARR3SET(w, r);
}

/*********************************************/
/* real space to window conversion primitive */
/*********************************************/
void gl_unproject(gint *x, gdouble *w, struct canvas_pak *canvas)
{
  GLdouble r[3];

  gluProject(w[0], w[1], w[2], canvas->modelview, canvas->projection, canvas->viewport, &r[0], &r[1], &r[2]);
  x[0] = r[0];
  x[1] = sysenv.height - r[1] - 1;
}

/********************************/
/* checks if a point is visible */
/********************************/
/*
gint gl_vertex_visible(gdouble *x)
{
gint p[2];

gl_get_window_coords(x, p);

if (p[0] < 0 || p[0] > sysenv.width)
  return(FALSE);
if (p[1] < 0 || p[1] > sysenv.height)
  return(FALSE);
return(TRUE);
}
*/

/*************************************************/
/* checks if a point is visible (with tolerance) */
/*************************************************/
/*
gint gl_vertex_tolerate(gdouble *x, gint dx)
{
gint p[2];

gl_get_window_coords(x, p);

if (p[0] < -dx || p[0] > sysenv.width+dx)
  return(FALSE);
if (p[1] < -dx || p[1] > sysenv.height+dx)
  return(FALSE);
return(TRUE);
}
*/

/*******************************************/
/* set opengl RGB colour for gdis RGB data */
/*******************************************/
void set_gl_colour(gint *rgb)
{
  gdouble col[3];

  col[0] = (gdouble) *rgb;
  col[1] = (gdouble) * (rgb + 1);
  col[2] = (gdouble) * (rgb + 2);
  VEC3MUL(col, INV_COLOUR_SCALE);
  glColor4f(col[0], col[1], col[2], 1.0);
}

/*************************************************************/
/* adjust fg colour for visibility against current bg colour */
/*************************************************************/
/* Cached background colour to avoid redundant fg recalculations */
static gdouble cached_bg[3] = {-1, -1, -1};

void make_fg_visible(void)
{
  gdouble fg[3], bg[3];

  ARR3SET(bg, sysenv.render.bg_colour);

  /* Skip if background hasn't changed — fg colours are already correct. */
  static gint first_call = 1;
  if (!first_call && fabs(cached_bg[0] - bg[0]) < 0.001 && fabs(cached_bg[1] - bg[1]) < 0.001 && fabs(cached_bg[2] - bg[2]) < 0.001)
    return;
  first_call = 0;

  ARR3SET(fg, sysenv.render.fg_colour);

  /* Ensure foreground is visible against background.
   * Use brightness-based contrast: bright bg → dark text, dark bg → light text. */
  float brightness = 0.299f * bg[0] + 0.587f * bg[1] + 0.114f * bg[2];
  if (brightness > 0.5f)
  {
    /* Bright background — use dark fg for contrast */
    fg[0] = 0.0;
    fg[1] = 0.0;
    fg[2] = 0.0;
  }
  else
  {
    /* Dark background — use light fg for contrast */
    fg[0] = 1.0;
    fg[1] = 1.0;
    fg[2] = 1.0;
  }
  ARR3SET(sysenv.render.fg_colour, fg);

  /* adjust label colour for visibility against the current background.
   * Use same brightness logic as fg: bright bg → dark labels (yellow-ish),
   * dark bg → light labels (white/yellow). */
  ARR3SET(fg, sysenv.render.label_colour);
  if (brightness > 0.5f)
  {
    /* Bright background — use dark yellow label */
    fg[0] = 1.0;
    fg[1] = 0.85;
    fg[2] = 0.0;
  }
  else
  {
    /* Dark background — use bright white/yellow label */
    fg[0] = 1.0;
    fg[1] = 1.0;
    fg[2] = 0.85;
  }
  ARR3SET(sysenv.render.label_colour, fg);

  /* adjust title colour for visibility against the current background */
  if (brightness > 0.5f)
  {
    /* Bright background — use dark blue-ish title */
    sysenv.render.title_colour[0] = 0.1;
    sysenv.render.title_colour[1] = 0.2;
    sysenv.render.title_colour[2] = 0.6;
  }
  else
  {
    /* Dark background — use light blue-ish title */
    sysenv.render.title_colour[0] = 0.5;
    sysenv.render.title_colour[1] = 0.7;
    sysenv.render.title_colour[2] = 1.0;
  }

  /*
  printf("fg: %lf %lf %lf\n",fg[0],fg[1],fg[2]);
  */

  /* Cache background colour for next frame comparison */
  cached_bg[0] = bg[0];
  cached_bg[1] = bg[1];
  cached_bg[2] = bg[2];
}

/*******************************/
/* return approx. string width */
/*******************************/
/* gl_text_width now in gl_text.cpp */

/***********************************/
/* pango print:  new routine as a  */
/* replacement for gl_print_window */
/* 2D print.              --OVHPA  */
/***********************************/
void pango_print(const gchar *str, gint x, gint y, struct canvas_pak *canvas, guint font_size, gint rotate)
{
  /* Use gl_fontsize as default when font_size==0 */
  if (font_size == 0)
    font_size = gl_fontsize;

  /* Guard against NULL cairo_surface (Qt mode) */
  if (!sysenv.cairo_surface)
    return;
  if (!str || !*str)
    return;

  /* Qt-based text rendering: render text into a small buffer, then blit to canvas */
  extern void gl_text_render_2d(const gchar *str, unsigned char *buf, gint w, gint h, gint font_size, gint rotate,
                                gdouble fg_r, gdouble fg_g, gdouble fg_b);

  /* Measure text size */
  gint tw = gl_text_width_size(str, font_size);
  gint th = font_size + 4;
  if (tw < 1 || th < 1)
    return;

  /* For rotated text, use a square buffer large enough to contain the
   * rotated result. For unrotated text, use a tight tw x th buffer. */
  gint buf_w = tw, buf_h = th;
  if (rotate != 0)
  {
    gint sz = tw > th ? tw : th;
    gint epsilon = font_size / 2;
    if (epsilon < 4)
      epsilon = 4;
    sz += epsilon;
    buf_w = 2 * sz;
    buf_h = 2 * sz;
  }

  /* Render text into a small buffer (ARGB32 format from QPainter)
   * Use contrasting color: if background is bright, use dark text. */
  float bg_r = sysenv.render.bg_colour[0];
  float bg_g = sysenv.render.bg_colour[1];
  float bg_b = sysenv.render.bg_colour[2];
  float brightness = 0.299f * bg_r + 0.587f * bg_g + 0.114f * bg_b;
  float text_r, text_g, text_b;
  if (brightness > 0.5f)
  {
    /* Bright background — use dark text */
    text_r = 0.0f;
    text_g = 0.0f;
    text_b = 0.0f;
  } else
  {
    /* Dark background — use light text */
    text_r = 1.0f;
    text_g = 1.0f;
    text_b = 1.0f;
  }

  /* Use a stack buffer for small text (most common case).
   * Falls back to heap allocation only for oversized rotated text. */
  unsigned char *buf;
  unsigned char stack_buf[256 * 256 * 4];
  int use_stack = (buf_w <= 256 && buf_h <= 256);

  if (use_stack)
    buf = stack_buf;
  else
    buf = g_malloc0(4 * buf_w * buf_h);

  gl_text_render_2d(str, buf, buf_w, buf_h, font_size, rotate, text_r, text_g, text_b);

  /* Store text overlay for compositing after glReadPixels.
   * For rotated text, the buffer is centered on (x,y), so offset
   * the overlay position so the text appears at the intended canvas coords. */
  gint overlay_x = x, overlay_y = y;
  if (rotate != 0)
  {
    overlay_x = x - buf_w / 2 + font_size / 2;
    overlay_y = y - buf_h / 2;
  }
  extern void qt_text_overlay_store(unsigned char *buf, gint tw, gint th, gint x, gint y);

  qt_text_overlay_store(buf, buf_w, buf_h, overlay_x, overlay_y);

  /* Free heap-allocated buffer (stack buffer is automatic) */
  if (!use_stack)
    g_free(buf);
}

/**************************************/
/**************************************
 * pango_print_sz: print with explicit size override (0 = use gl_fontsize)
 **************************************/
void pango_print_sz(const gchar *str, gint x, gint y, struct canvas_pak *canvas, gint font_size_override, gint rotate)
{
  guint fs = (font_size_override > 0) ? (guint) font_size_override : gl_fontsize;
  pango_print(str, x, y, canvas, fs, rotate);
}

/* pango print_world:  new routine as */
/* a replacement for gl_print_world   */
/* 3D print.                 --OVHPA  */
/**************************************/
#define DEBUG_PANGO_TEXT 0
void pango_print_world(gchar *str, gdouble *v, struct canvas_pak *canvas)
{
  /* Guard against NULL cairo_surface (Qt mode) */
  if (!sysenv.cairo_surface)
    return;
  /* Qt-based 3D text rendering replaces Cairo/Pango */
  extern void gl_text_render_3d(const gchar *str, unsigned char *buf, gint max_w, gint max_h, gint font_size,
                                gdouble fg_r, gdouble fg_g, gdouble fg_b, gint *out_width, gint *out_height);
  gint buf_w = sysenv.width;
  gint buf_h = sysenv.height;
  gint tw, th;

  /* Reuse persistent buffer (same as cairo_surface) to avoid per-frame alloc */
  unsigned char *buf = sysenv.cairo_surface;
  if (!buf)
    return;

  gl_text_render_3d(str, buf, buf_w, buf_h, gl_fontsize, sysenv.render.fg_colour[0], sysenv.render.fg_colour[1],
                    sysenv.render.fg_colour[2], &tw, &th);

  /* copy to framebuffer */
  glRasterPos3f(v[0], v[1], v[2]);
  glPixelZoom(1, -1);
  glDrawPixels(tw, th, GL_BGRA, GL_UNSIGNED_BYTE, buf);
}
/********************************/
/* vertex at 2D screen position */
/********************************/
void gl_vertex_window(gint x, gint y, struct canvas_pak *canvas)
{
  gdouble w[3];

  gl_project(w, x, y, canvas);
  glVertex3dv(w);
}

/*********************************/
/* draw a box at screen position */
/*********************************/
void gl_draw_box(gint px1, gint py1, gint px2, gint py2, struct canvas_pak *canvas)
{
  glBegin(GL_LINE_LOOP);
  gl_vertex_window(px1, py1, canvas);
  gl_vertex_window(px1, py2, canvas);
  gl_vertex_window(px2, py2, canvas);
  gl_vertex_window(px2, py1, canvas);
  glEnd();
}

/*********************************************************/
/* determines if input position is close to a drawn core */
/*********************************************************/
#define PIXEL_TOLERANCE 10
gint gl_core_proximity(gint x, gint y, struct core_pak *core, struct canvas_pak *canvas)
{
  gint dx, dy, dr, p[2], itol;
  gdouble tol;
  struct model_pak *model;

  if (!canvas || !core)
    return (G_MAXINT);

  model = canvas->model;

  if (!model)
    return (G_MAXINT);

  gl_unproject(p, core->rx, canvas);

  tol = gl_get_radius(core, model);

  tol *= sysenv.size;
  tol /= model->rmax;

  /* Ensure a generous minimum hit radius so atoms are easy to click,
   * especially at high zoom levels where small projected radii make
   * precise clicking difficult. The atom's rendered radius provides
   * the base, with a floor of PIXEL_TOLERANCE pixels for usability. */
  itol = nearest_int(tol);
  if (itol < PIXEL_TOLERANCE)
    itol = PIXEL_TOLERANCE;

  /* Use squared distance for comparison below — convert to squared radius. */
  itol *= itol;

  dx = p[0] - x;
  dy = p[1] - y;
  dr = dx * dx + dy * dy;

  if (dr < itol)
    return (dr);

  return (G_MAXINT);
}

/**********************************************/
/* seek nearest core to window pixel position */
/**********************************************/
#define DEBUG_SEEK_CORE 0
gpointer gl_seek_core(gint x, gint y, struct model_pak *model)
{
  gint dr, rmin;
  GSList *list;
  struct core_pak *core, *found = NULL;
  struct canvas_pak *canvas;

#if DEBUG_SEEK_CORE
  printf("mouse: [%d, %d]\n", x, y);
#endif

  /* canvas aware */
  canvas = canvas_find(model);
  if (!canvas)
    return (NULL);

  /* default tolerance */
  rmin = sysenv.size;

  for (list = model->cores; list; list = g_slist_next(list))
  {
    core = list->data;

    dr = gl_core_proximity(x, y, core, canvas);
    if (dr < rmin)
    {
      found = core;
      rmin = dr;
    }
  }
  return (found);
}

/********************************/
/* OpenGL atom location routine */
/********************************/
#define DEBUG_GL_SEEK_BOND 0
gpointer gl_seek_bond(gint x, gint y, struct model_pak *model)
{
  gint p1[2], p2[2];
  gdouble r[3];
  gdouble dx, dy, d2;
  gdouble tol;
  gpointer match = NULL;
  GSList *list;
  struct bond_pak *bdata;
  struct core_pak *core1, *core2;

  /* default (squared) tolerance */
  tol = sysenv.size;
  tol /= model->rmax;

#if DEBUG_GL_SEEK_BOND
  printf("input: %d,%d [%f]\n", x, y, tol);
#endif

  /* search */
  for (list = model->bonds; list; list = g_slist_next(list))
  {
    bdata = list->data;
    core1 = bdata->atom1;
    core2 = bdata->atom2;

    {
      struct canvas_pak *c = canvas_find(model);
      if (!c)
        continue;
      ARR3SET(r, core1->rx);
      gl_unproject(p1, r, c);

      ARR3SET(r, core2->rx);
      gl_unproject(p2, r, c);

#if DEBUG_GL_SEEK_BOND
      printf("[%s-%s] @ [%f,%f]\n", core1->atom_label, core2->atom_label, 0.5 * (p1[0] + p2[0]), 0.5 * (p1[1] + p2[1]));
#endif

      dx = 0.5 * (p1[0] + p2[0]) - x;
      dy = 0.5 * (p1[1] + p2[1]) - y;

      d2 = dx * dx + dy * dy;
      if (d2 < tol)
      {
        /* keep searching - return the best match */
        tol = d2;
        match = bdata;
      }
    }
  }
  return (match);
}

/***************************************************/
/* compute atoms that lie within the selection box */
/***************************************************/
#define DEBUG_GL_SELECT_BOX 0
#define GL_SELECT_TOLERANCE 10
void gl_select_box(void)
{
  gint i, tmp, x[2];
  gdouble r[3], piv[3];
  GSList *list, *ilist = NULL;
  struct model_pak *data;
  struct core_pak *core;
  struct image_pak *image;

  /* valid model */
  data = sysenv.active_model;
  if (!data)
    return;
  if (data->graph_active)
    return;
  if (data->picture_active)
    return;

  /* ensure box limits have the correct order (ie low to high) */
  for (i = 0; i < 2; i++)
  {
    if (data->select_box[i] > data->select_box[i + 2])
    {
      tmp = data->select_box[i];
      data->select_box[i] = data->select_box[i + 2];
      data->select_box[i + 2] = tmp;
    }
  }

  /* add pixel tolerance for (approx) single clicks */
  if ((data->select_box[2] - data->select_box[0]) < GL_SELECT_TOLERANCE)
  {
    data->select_box[0] -= GL_SELECT_TOLERANCE;
    data->select_box[2] += GL_SELECT_TOLERANCE;
  }
  if ((data->select_box[1] - data->select_box[3]) < GL_SELECT_TOLERANCE)
  {
    data->select_box[1] -= GL_SELECT_TOLERANCE;
    data->select_box[3] += GL_SELECT_TOLERANCE;
  }

#if DEBUG_GL_SELECT_BOX
  printf("[%d,%d] - [%d,%d]\n", data->select_box[0], data->select_box[1], data->select_box[2], data->select_box[3]);
#endif

  /* find matches */
  do
  {
    /* periodic images */
    if (ilist)
    {
      image = ilist->data;
      ARR3SET(piv, image->rx);
      ilist = g_slist_next(ilist);
    } else
    {
      VEC3SET(piv, 0.0, 0.0, 0.0);
      ilist = data->images;
    }
    /* cores */
    for (list = data->cores; list; list = g_slist_next(list))
    {
      core = list->data;
      if (core->status & DELETED)
        continue;

      /* get core pixel position */
      /* CURRENT */
      ARR3SET(r, core->rx);
      ARR3ADD(r, piv);
      {
        struct canvas_pak *c = canvas_find(data);
        if (!c)
          continue;
        gl_unproject(x, r, c);
      }

      /* check bounds */
      if (x[0] > data->select_box[0] && x[0] < data->select_box[2])
      {
        if (x[1] > data->select_box[1] && x[1] < data->select_box[3])
        {
          select_core(core, FALSE, data);
        }
      }
    }
  } while (ilist);

  redraw_canvas(SINGLE);
}

/********************************************/
/* build lists for respective drawing types */
/********************************************/
#define DEBUG_BUILD_CORE_LISTS 0
void gl_build_core_lists(GSList **solid, GSList **ghost, GSList **wire, struct model_pak *model)
{
  GSList *list;
  struct core_pak *core;

  g_assert(model != NULL);

  *solid = *ghost = *wire = NULL;
  for (list = model->cores; list; list = g_slist_next(list))
  {
    core = list->data;

    /* bailout checks */
    if (core->status & (HIDDEN | DELETED))
      continue;
    if (core->render_mode == ZONE)
      continue;

    /* Checks for MARVIN regions */
    if ((!model->show_region1A && core->region == REGION1A) || (!model->show_region1B && core->region == REGION1B) ||
        (!model->show_region2A && core->region == REGION2A) || (!model->show_region2B && core->region == REGION2B))
      continue;

    /* build appropriate lists */
    if (core->render_wire)
      *wire = g_slist_prepend(*wire, core);
    else if (core->ghost)
      *ghost = g_slist_prepend(*ghost, core);
    else
      *solid = g_slist_prepend(*solid, core);
  }

#if DEBUG_BUILD_CORE_LISTS
  printf("solid: %d\n", g_slist_length(*solid));
  printf("ghost: %d\n", g_slist_length(*ghost));
  printf(" wire: %d\n", g_slist_length(*wire));
#endif
}

/************************/
/* draw a list of cores */
/************************/
#define DEBUG_DRAW_CORES 0
void gl_draw_cores(GSList *cores, struct model_pak *model)
{
  gint max, quality;
#ifdef UNUSED_BUT_SET
  gint dx;
#endif
  gdouble radius, x[3], colour[4];
  GSList *list, *ilist;
  struct core_pak *core;
  struct point_pak sphere;
  struct image_pak *image;

  /* NEW - limit the quality based on physical model size */
  /* FIXME - broken due to moving camera */
  quality = sysenv.render.sphere_quality;
  if (sysenv.render.auto_quality)
  {
    radius = sysenv.size / model->zoom;
    max = radius / 10;
#ifdef UNUSED_BUT_SET
    dx = 10;
#endif

#if DEBUG_DRAW_CORES
    printf("%f : %d [%d]\n", radius, max, quality);
#endif

    if (quality > max)
      sysenv.render.sphere_quality = max;
  }

  /* set up geometric primitive */
  gl_init_sphere(&sphere, model);

  /* draw desired cores */
  for (list = cores; list; list = g_slist_next(list))
  {
    core = list->data;

    /* set colour */
    ARR3SET(colour, core->colour);
    VEC3MUL(colour, INV_COLOUR_SCALE);
    colour[3] = core->colour[3];
    glColor4dv(colour);

    radius = gl_get_radius(core, model);
    if (radius > 0.0)
    {
      /* original + image iteration */
      ilist = NULL;
      do
      {
        ARR3SET(x, core->rx);
        if (ilist)
        {
          /* image translation */
          image = ilist->data;
          ARR3ADD(x, image->rx);
          ilist = g_slist_next(ilist);
        } else
          ilist = model->images;
        /*
              if (gl_vertex_tolerate(x, dx))
        */
        gl_draw_sphere(&sphere, x, radius);
      } while (ilist);
    }
  }
  gl_free_points(&sphere);
  sysenv.render.sphere_quality = quality;
}

/* CURRENT - implement fractional image_limits */
/* possibility is draw extra whole unit (ie plus bonds) then use clipping planes to trim */
#if EXPERIMENTAL
{
  gint i, j, a, b, c, flag, limit[6], test[6];
  gdouble t[4], frac[6], whole;
  struct mol_pak *mol;

  /* set up integer limits */
  for (i = 6; i--;)
  {
    frac[i] = modf(data->image_limit[i], &whole);
    limit[i] = (gint) whole;
    /* if we have a fractional part - extend the periodic image boundary */
    if (frac[i] > FRACTION_TOLERANCE)
    {
      /*
      printf("i = %d, frac = %f\n", i, frac[i]);
      */
      test[i] = TRUE;
      limit[i]++;
    } else
      test[i] = FALSE;
  }

  limit[0] *= -1;
  limit[2] *= -1;
  limit[4] *= -1;

  /* setup for pic iteration */
  a = limit[0];
  b = limit[2];
  c = limit[4];

  for (;;)
  {
    /* image increment */
    if (a == limit[1])
    {
      a = limit[0];
      b++;
      if (b == limit[3])
      {
        b = limit[2];
        c++;
        if (c == limit[5])
          break;
      }
    }

    VEC3SET(t, a, b, c);

    /* NEW - include fractional cell images */
    /* TODO - include the testing as part of image increment testing? (ie above) */
    flag = TRUE;
    for (i = 0; i < data->periodic; i++)
    {
      /* +ve fractional extent test */
      if (test[2 * i + 1])
        if (t[i] == limit[2 * i + 1] - 1)
        {
          for (j = 0; j < data->periodic; j++)
            if (test[2 * j + 1])
            {
              /* check molecule centroid, rather than atom coords */
              mol = core->mol;

              if (mol->centroid[j] > frac[2 * j + 1])
                flag = FALSE;
            }
        }

      /* -ve fractional extent test */
      if (test[2 * i])
        if (t[i] == limit[2 * i])
        {
          /* + ve fractional extent test */
          for (j = 0; j < data->periodic; j++)
            if (test[2 * j])
            {
              /* check molecule centroid, rather than atom coords */
              mol = core->mol;

              /* TODO - pre-sub 1.0 from frac for this test */
              if (mol->centroid[j] < (1.0 - frac[2 * j]))
                flag = FALSE;
            }
        }
    }

    if (flag)
    {

      t[3] = 0.0;
      vec4mat(data->display_lattice, t);

      ARR3SET(vec, core->rx);
      ARR3ADD(vec, t);

      gl_draw_sphere(&sphere, vec, radius);
    }

    a++;
  }
}
#endif

/**********************************/
/* draw the selection halo/circle */
/**********************************/
void gl_draw_halo_list(GSList *list, struct model_pak *data)
{
  gint h;
  gdouble radius, dr;
  gdouble vec[3], halo[4];
  GSList *item, *ilist;
  struct point_pak circle;
  struct core_pak *core;
  struct image_pak *image;

  g_assert(data != NULL);

  /* variable quality halo */
  h = 2 + sysenv.render.sphere_quality;
  h = h * h;
  gl_init_circle(&circle, h, data);

  /* halo colour */
  VEC4SET(halo, 1.0, 0.95, 0.45, 1.0);
  for (item = list; item; item = g_slist_next(item))
  {
    core = item->data;

    glColor4dv(halo);
    radius = gl_get_radius(core, data);
    if (radius == 0.0)
      continue;

    /* halo ring size increment */
    /* a fn of the radius? eg 1/2 */
    dr = 0.6 * radius / halo_segments;

    /* original + image iteration */
    ilist = NULL;
    do
    {
      ARR3SET(vec, core->rx);
      if (ilist)
      {
        /* image translation */
        image = ilist->data;
        ARR3ADD(vec, image->rx);
        ilist = g_slist_next(ilist);
      } else
        ilist = data->images;

      if (sysenv.render.halos)
      {
        /* halo fade loop */
        for (h = 0; h < halo_segments; h++)
        {
          halo[3] = halo_fade[h];

          glColor4dv(halo);
          gl_draw_ring(&circle, vec, radius + h * dr - 0.5 * dr, radius + h * dr + 0.5 * dr);
        }
        /* reset halo transparancy */
        halo[3] = 1.0;
      } else
        gl_draw_ring(&circle, vec, 1.2 * radius, 1.4 * radius);
    } while (ilist);
  }

  gl_free_points(&circle);
}

/*****************************************/
/* translucent atom depth buffer sorting */
/*****************************************/
gint gl_depth_sort(struct core_pak *c1, struct core_pak *c2)
{
  if (c1->rx[2] > c2->rx[2])
    return (-1);
  return (1);
}

/***********************/
/* draw model's shells */
/***********************/
void gl_draw_shells(struct model_pak *data)
{
  gint omit_atom, mode;
  gdouble radius;
  gdouble vec[3], colour[4];
  GSList *list, *ilist;
  struct point_pak sphere;
  struct core_pak *core;
  struct shel_pak *shel;
  struct image_pak *image;

  g_assert(data != NULL);

  gl_init_sphere(&sphere, data);

  for (list = data->shels; list; list = g_slist_next(list))
  {
    shel = list->data;
    if (shel->status & (DELETED | HIDDEN))
      continue;

    /* Checks for MARVIN regions */
    if ((!data->show_region1A && shel->region == REGION1A) || (!data->show_region1B && shel->region == REGION1B) ||
        (!data->show_region2A && shel->region == REGION2A) || (!data->show_region2B && shel->region == REGION2B))
      continue;

    /* shell colour */
    ARR3SET(colour, shel->colour);
    colour[3] = 0.5;

    /* render mode */
    core = shel->core;

    if (core)
      mode = core->render_mode;
    else
      mode = BALL_STICK;

    /* bailout modes */
    switch (mode)
    {
    case LIQUORICE:
    case ZONE:
      continue;
    }

    /* position */
    ilist = NULL;
    do
    {
      ARR3SET(vec, shel->rx);
      if (ilist)
      {
        /* image */
        image = ilist->data;
        ARR3ADD(vec, image->rx);
        ilist = g_slist_next(ilist);
      } else
      {
        ilist = data->images;
      }

      /* set appropriate radius */
      omit_atom = FALSE;
      radius = 1.0;
      switch (mode)
      {
      case BALL_STICK:
        radius *= sysenv.render.ball_radius;
        break;

      case CPK:
        radius *= elements[shel->atom_code].vdw;
        break;

      case STICK:
        radius *= sysenv.render.stick_radius;
        break;
      }
      if (!omit_atom)
      {
        glColor4dv(colour);
        gl_draw_sphere(&sphere, vec, radius);
      }
    } while (ilist);
  }

  glEnable(GL_LIGHTING);

  gl_free_points(&sphere);
}

/*******************************************/
/* draw model pipes (separate bond halves) */
/*******************************************/
/* stage (drawing stage) ie colour materials/lines/etc. */
/* TODO - only do minimum necessary for each stage */
void gl_draw_pipes(gint line, GSList *pipe_list, struct model_pak *model)
{
#ifdef UNUSED_BUT_SET
  gint dx;
#endif
  guint q;
  gdouble v1[3], v2[3];
  GSList *list, *ilist;
  struct pipe_pak *pipe;
  struct image_pak *image;

  /* setup for bond drawing */
  q = sysenv.render.cylinder_quality;

  /* FIXME - this is broken by the new camera code */
  if (sysenv.render.auto_quality)
  {
    if (!line)
    {
      /* only use desired quality if less than our guess at the useful maximum */
      q = sysenv.size / (10.0 * model->rmax);
      q++;
      if (q > sysenv.render.cylinder_quality)
        q = sysenv.render.cylinder_quality;
    }
  }
#ifdef UNUSED_BUT_SET
  dx = 10;
#endif
  /* enumerate the supplied pipes (half bonds) */
  for (list = pipe_list; list; list = g_slist_next(list))
  {
    pipe = list->data;

    /* original + image iteration */
    ilist = NULL;
    do
    {
      /* original */
      ARR3SET(v1, pipe->v1);
      ARR3SET(v2, pipe->v2);
      if (ilist)
      {
        image = ilist->data;
        /* image */
        ARR3ADD(v1, image->rx);
        ARR3ADD(v2, image->rx);
        ilist = g_slist_next(ilist);
      } else
        ilist = model->images;

      /* NEW - don't render if both endpoints are off screen */
      /*
          if (gl_vertex_tolerate(v1, dx) && gl_vertex_tolerate(v2, dx))
      */
      //    {
      glColor4dv(pipe->colour);
      if (line)
      {
        glBegin(GL_LINES);
        glVertex3dv(v1);
        glVertex3dv(v2);
        glEnd();
      } else
        gl_draw_cylinder(v1, v2, pipe->radius, q);
      //    }
    } while (ilist);
  }
}

/***************************/
/* draw crystal morphology */
/***************************/
/* deprecated */
#define DEBUG_DRAW_MORPH 0
void gl_draw_morph(struct model_pak *data)
{
  gdouble x[3];
  GSList *list1, *list2;
  struct plane_pak *plane;
  struct vertex_pak *v;

  /* checks */
  g_assert(data != NULL);

  /* turn lighting off for wire frame drawing - looks esp ugly */
  /* when hidden (stippled) lines and normal lines are overlayed */
  // if (0.5*sysenv.render.wire_surface)
  if (((gint) 0.5 * sysenv.render.wire_surface) != 0)
    glDisable(GL_LIGHTING);

  glLineWidth(sysenv.render.frame_thickness);

  /* visibility calculation */
  for (list1 = data->planes; list1; list1 = g_slist_next(list1))
  {
    plane = list1->data;

    /* CURRENT */
    /*
      if (plane->present)
    {
    struct vertex_pak *v1, *v2, *v3;

    if (plane->m[0] == 0 && plane->m[1] == 2 && plane->m[2] == -1)
      {
    printf(" ==== (%f %f %f)\n", plane->m[0], plane->m[1], plane->m[2]);
    P3VEC("n: ", plane->norm);

    for (list2=plane->vertices ; list2 ; list2=g_slist_next(list2))
      {
    v1 = list2->data;
    P3VEC("v: ", v1->rx);
      }

      }
    }
    */

    if (plane->present)
      plane->visible = facet_visible(data, plane);
  }

  /* draw hidden lines first */
  if (sysenv.render.wire_surface && sysenv.render.wire_show_hidden)
  {
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0x0303);

    for (list1 = data->planes; list1; list1 = g_slist_next(list1))
    {
      plane = list1->data;
      if (!plane->present)
        continue;

      /* start the face */
      glBegin(GL_POLYGON);
      for (list2 = plane->vertices; list2; list2 = g_slist_next(list2))
      {
        v = list2->data;

        ARR3SET(x, v->rx);

        glNormal3dv(plane->norm);
        glVertex3dv(x);
      }
      glEnd();
    }
  }

  /* draw the visible facets */
  glDisable(GL_LINE_STIPPLE);
  for (list1 = data->planes; list1; list1 = g_slist_next(list1))
  {
    plane = list1->data;

    if (!plane->visible)
      continue;
    if (!plane->present)
      continue;

#if DEBUG_DRAW_MORPH
    printf("(%f %f %f) :\n", plane->m[0], plane->m[1], plane->m[2]);
#endif

    /* start the face */
    glBegin(GL_POLYGON);
    for (list2 = plane->vertices; list2; list2 = g_slist_next(list2))
    {
      v = list2->data;

#if DEBUG_DRAW_MORPH
      P3VEC("  : ", v->rx);
#endif

      ARR3SET(x, v->rx);
      glNormal3dv(plane->norm);
      glVertex3dv(x);
    }
    glEnd();
  }

  /* if wire frame draw - turn lighting back on */
  if (sysenv.render.wire_surface)
    glEnable(GL_LIGHTING);
}

/***********************************/
/* draw the cartesian/lattice axes */
/***********************************/
void gl_draw_axes(gint mode, struct canvas_pak *canvas, struct model_pak *data)
{
  gint i, ry;
  gdouble f, x1[3], x2[3];
  gchar label[3];

  if (!canvas)
    return;
  if (!data)
    return;

  glDisable(GL_FOG);

  /* axes type setup */
  if (data->axes_type == CARTESIAN)
    strcpy(label, " x");
  else
    strcpy(label, " a");

  /* inverted y sense correction */
  ry = sysenv.height - canvas->y - canvas->height + 40;
  gl_project(x1, canvas->x + 40, ry, canvas);
  gl_project(x2, canvas->x + 20, ry, canvas);
  ARR3SUB(x2, x1);

  /* yet another magic number */
  f = 20.0 * VEC3MAG(x2) / data->rmax;

  if (mode)
  {
    /* set colour */
    glColor4f(sysenv.render.fg_colour[0], sysenv.render.fg_colour[1], sysenv.render.fg_colour[2], 1.0);
    /* draw the axes */
    for (i = 3; i--;)
    {
      ARR3SET(x2, data->axes[i].rx);
      VEC3MUL(x2, f);
      ARR3ADD(x2, x1);

      /* yet another magic number for the vector thickness */
      draw_vector(x1, x2, 0.005 * f * data->rmax);
    }
  } else
  {
    /* draw the labels - offset by fontsize? */
    glColor4f(sysenv.render.title_colour[0], sysenv.render.title_colour[1], sysenv.render.title_colour[2], 1.0);
    for (i = 0; i < 3; i++)
    {
      ARR3SET(x2, data->axes[i].rx);
      VEC3MUL(x2, f);
      ARR3ADD(x2, x1);
      pango_print_world(label, x2, canvas);
      label[1]++;
    }
  }

  if (sysenv.render.fog)
    glEnable(GL_FOG);
}

/*******************************************/
/* draw the cell frame for periodic models */
/*******************************************/
void gl_draw_cell(struct model_pak *data)
{
  gint i, j;
  gdouble v1[3], v2[3], v3[3], v4[3];

  /* draw the opposite ends of the frame */
  for (i = 0; i < 5; i += 4)
  {
    glBegin(GL_LINE_LOOP);
    ARR3SET(v1, data->cell[i + 0].rx);
    ARR3SET(v2, data->cell[i + 1].rx);
    ARR3SET(v3, data->cell[i + 2].rx);
    ARR3SET(v4, data->cell[i + 3].rx);
    glVertex3dv(v1);
    glVertex3dv(v2);
    glVertex3dv(v3);
    glVertex3dv(v4);
    glEnd();
  }
  /* draw the sides of the frame */
  glBegin(GL_LINES);
  for (i = 4; i--;)
  {
    j = i + 4;
    /* retrieve coordinates */
    ARR3SET(v1, data->cell[i].rx);
    ARR3SET(v2, data->cell[j].rx);
    /* draw */
    glVertex3dv(v1);
    glVertex3dv(v2);
  }
  glEnd();
}

/***************************************/
/* draw the cell frame periodic images */
/***************************************/
void gl_draw_cell_images(struct model_pak *model)
{
  gint i, j;
  gdouble v1[3], v2[3], v3[3], v4[3];
  GSList *ilist;
  struct image_pak *image;

  /* image iteration (don't do original) */
  // ilist = model->images;/*FIX: b38968*/
  for (ilist = model->images; ilist; ilist = g_slist_next(ilist))
  {
    /* image translation */
    image = ilist->data;

    /* draw the opposite ends of the frame */
    for (i = 0; i < 5; i += 4)
    {
      glBegin(GL_LINE_LOOP);
      ARR3SET(v1, model->cell[i + 0].rx);
      ARR3SET(v2, model->cell[i + 1].rx);
      ARR3SET(v3, model->cell[i + 2].rx);
      ARR3SET(v4, model->cell[i + 3].rx);
      ARR3ADD(v1, image->rx);
      ARR3ADD(v2, image->rx);
      ARR3ADD(v3, image->rx);
      ARR3ADD(v4, image->rx);
      glVertex3dv(v1);
      glVertex3dv(v2);
      glVertex3dv(v3);
      glVertex3dv(v4);
      glEnd();
    }
    /* draw the sides of the frame */
    glBegin(GL_LINES);
    for (i = 4; i--;)
    {
      j = i + 4;
      /* retrieve coordinates */
      ARR3SET(v1, model->cell[i].rx);
      ARR3SET(v2, model->cell[j].rx);
      ARR3ADD(v1, image->rx);
      ARR3ADD(v2, image->rx);
      /* draw */
      glVertex3dv(v1);
      glVertex3dv(v2);
    }
    glEnd();
  }
}

/***************************/
/* draw core - shell links */
/***************************/
void gl_draw_links(struct model_pak *model)
{
  GSList *list;
  struct core_pak *core;
  struct shel_pak *shell;

  g_assert(model != NULL);

  glBegin(GL_LINES);
  for (list = model->cores; list; list = g_slist_next(list))
  {
    core = list->data;

    if (core->shell)
    {
      shell = core->shell;

      glVertex3dv(core->rx);
      glVertex3dv(shell->rx);
    }
  }
  glEnd();
}

/*********************/
/* draw measurements */
/*********************/
void gl_draw_measurements(struct model_pak *model)
{
  gint type;
  gdouble colour[3], a1[3], a2[3], a3[3], a4[3], v1[3], v2[3], v3[3], n[3];
  GSList *list;

  /* draw the lines */
  for (list = model->measure_list; list; list = g_slist_next(list))
  {
    type = measure_type_get(list->data);

    measure_colour_get(colour, list->data);
    glColor4f(colour[0], colour[1], colour[2], 1.0);

    switch (type)
    {
    case MEASURE_BOND:
    case MEASURE_DISTANCE:
    case MEASURE_INTER:
    case MEASURE_INTRA:
      measure_coord_get(v1, 0, list->data, model);
      measure_coord_get(v2, 1, list->data, model);
      glBegin(GL_LINES);
      glVertex3dv(v1);
      glVertex3dv(v2);
      glEnd();
      break;

    case MEASURE_ANGLE:
      measure_coord_get(v1, 0, list->data, model);
      measure_coord_get(v2, 1, list->data, model);
      measure_coord_get(v3, 2, list->data, model);
      /* NB: central atom should be first */
      draw_arc(v2, v1, v3);
      break;

    case MEASURE_TORSION:
      /* get constituent core coordinates */
      measure_coord_get(a1, 0, list->data, model);
      measure_coord_get(a2, 1, list->data, model);
      measure_coord_get(a3, 2, list->data, model);
      measure_coord_get(a4, 3, list->data, model);
      /* middle 2 cores define the axis */
      ARR3SET(n, a3);
      ARR3SUB(n, a2);
      normalize(n, 3);
      /* arm 1 */
      ARR3SET(v3, a1);
      ARR3SUB(v3, a2);
      proj_vop(v1, v3, n);
      normalize(v1, 3);
      /* arm 2 */
      ARR3SET(v3, a4);
      ARR3SUB(v3, a3);
      proj_vop(v2, v3, n);
      normalize(v2, 3);
      /* axis centre */
      ARR3SET(v3, a2);
      ARR3ADD(v3, a3);
      VEC3MUL(v3, 0.5);
      /* arm endpoints are relative to axis centre */
      ARR3ADD(v1, v3);
      ARR3ADD(v2, v3);
      /* draw arc */
      draw_arc(v3, v1, v2);
      /* draw lines */
      glBegin(GL_LINE_STRIP);
      glVertex3dv(v1);
      glVertex3dv(v3);
      glVertex3dv(v2);
      glEnd();
      break;
    }
  }
}

/***************************/
/* draw morphology indices */
/***************************/
void gl_draw_miller(struct model_pak *data)
{
  gchar *label;
  gdouble vec[3];
  GSList *plist;
  struct plane_pak *plane;

  /* draw facet labels */
  plist = data->planes;
  while (plist != NULL)
  {
    plane = plist->data;
    if (plane->present && plane->visible)
    {
      /* TODO - scale the font with data->scale? (vanishes if too small) */
      /* print the hkl label */
      label = g_strdup_printf("%d%d%d", plane->index[0], plane->index[1], plane->index[2]);
      ARR3SET(vec, plane->rx);
      pango_print_world(label, vec, canvas_find(data));
      g_free(label);

      /* TODO - vector font (display list) with number + overbar number */
      /*
          glBegin(GL_LINES);
          if (plane->index[0] < 0)
            {
            glVertex3d(plane->rx, plane->ry-gl_fontsize, plane->rz);
            glVertex3d(plane->rx+gl_fontsize, plane->ry-gl_fontsize, plane->rz);
            }
          if (plane->index[1] < 0)
            {
            glVertex3d(plane->rx+1*gl_fontsize, plane->ry-gl_fontsize, plane->rz);
            glVertex3d(plane->rx+2*gl_fontsize, plane->ry-gl_fontsize, plane->rz);
            }
          if (plane->index[2] < 0)
            {
            glVertex3d(plane->rx+2*gl_fontsize, plane->ry-gl_fontsize, plane->rz);
            glVertex3d(plane->rx+3*gl_fontsize, plane->ry-gl_fontsize, plane->rz);
            }
          glEnd();
      */
    }
    plist = g_slist_next(plist);
  }
}

/************************************************/
/* draw the colour scale for molecular surfaces */
/************************************************/
/* FIXME - siesta epot */
void gl_draw_colour_scale(gint x, gint y, struct model_pak *data)
{
  gint i, n;
  gdouble z1, dz;
  gdouble colour[3], w1[3], w2[3];
  GString *text;
  struct canvas_pak *canvas = canvas_find(data);

  g_assert(data != NULL);

  /* init */
  n = data->epot_div;
  z1 = data->epot_max;
  dz = (data->epot_max - data->epot_min) / (gdouble) (n - 1);

  /* colour boxes */
  glPolygonMode(GL_FRONT, GL_FILL);
  glBegin(GL_QUADS);
  for (i = 0; i < n; i++)
  {
    switch (data->ms_colour_method)
    {
    case MS_SOLVENT:
      ms_dock_colour(colour, z1, data->epot_min, data->epot_max);
      break;

    default:
      ms_epot_colour(colour, z1, data->epot_min, data->epot_max);
    }

    glColor3f(colour[0], colour[1], colour[2]);
    z1 -= dz;

    gl_project(w1, x, y + i * 20, canvas);
    gl_project(w2, x, y + 19 + i * 20, canvas);
    glVertex3dv(w1);
    glVertex3dv(w2);

    gl_project(w1, x + 19, y + 19 + i * 20, canvas);
    gl_project(w2, x + 19, y + i * 20, canvas);
    glVertex3dv(w1);
    glVertex3dv(w2);
  }
  glEnd();

  /* init */
  text = g_string_new(NULL);
  z1 = data->epot_max;
  glColor3f(1.0, 1.0, 1.0);

  /* box labels */
  for (i = 0; i < n; i++)
  {
    g_string_printf(text, "%6.2f", z1); // g_string_sprintf
    pango_print(text->str, x + 30, y + i * 20 + 18, canvas, gl_fontsize, 0);
    z1 -= dz;
  }
  g_string_free(text, TRUE);
}

/**************************************/
/* draw text we wish to be unobscured */
/**************************************/
void gl_draw_text(struct canvas_pak *canvas, struct model_pak *data)
{
  gint i, j, type;
  gint n = 0, hidden = 0;
  gchar *text;
  gdouble q, v1[3], v2[3], v3[3];
  GSList *list;
  GString *label;
  struct vec_pak *v;
  struct spatial_pak *spatial;
  struct core_pak *core[4];
  struct shel_pak *shell;

  if (!canvas)
    return;
  if (!data)
    return;

  /* print mode */
  text = get_mode_label(data);
  pango_print(text, canvas->x + canvas->width - gl_text_width_size(text, gl_fontsize), canvas->height - canvas->y - 20,
              canvas, gl_fontsize, 0);
  g_free(text);

  /* print some useful info */
  if (sysenv.render.show_energy)
  {
    text = property_lookup("Energy", data);
    if (text)
    {
      gint px = canvas->x + canvas->width - gl_text_width_size(text, gl_fontsize);
      gint py = canvas->y + 2 * gl_fontsize;
      /* Energy text rendered at (%d, %d) */ px = px;
      py = py;
      pango_print(text, px, py, canvas, gl_fontsize, 0);
    }
    g_free(text);
  }

  if (data->show_frame_number)
  {
    if (data->animation)
    {
      text = g_strdup_printf("[%d:%d]", data->cur_frame, data->num_frames - 1);
      pango_print(text, (canvas->x + canvas->width - gl_text_width_size(text, gl_fontsize)) / 2, canvas->y + 40, canvas,
                  gl_fontsize, 0);
      g_free(text);
    }
  }

  /* hkl labels */
  /*
  if (data->num_vertices && data->morph_label)
    gl_draw_miller(data);
  */

  /* unit cell lengths */
  if (data->show_cell_lengths)
  {
    j = 2;
    for (i = 0; i < data->periodic; i++)
    {
      j += pow(-1, i) * (i + 1);
      text = g_strdup_printf("%5.2f \u212B", data->pbc[i]);
      ARR3SET(v1, data->cell[0].rx);
      ARR3ADD(v1, data->cell[j].rx);
      VEC3MUL(v1, 0.5);
      pango_print_world(text, v1, canvas);
      g_free(text);
    }
  }

  /* epot scale */
  if (data->ms_colour_scale)
    gl_draw_colour_scale(canvas->x + 1, sysenv.height - canvas->y - canvas->height + 80, data);

  /* NEW - camera waypoint number */
  if (data->show_waypoints && !data->animating)
  {
    i = 0;
    glColor3f(0.0, 0.0, 1.0);
    for (list = data->waypoint_list; list; list = g_slist_next(list))
    {
      struct camera_pak *camera = list->data;
      i++;
      if (camera == data->camera)
        continue;

      text = g_strdup_printf("%d", i);
      pango_print_world(text, camera->x, canvas);
      g_free(text);
    }
  }

  /* TODO - incorporate scaling etc. in camera waypoint drawing */
  /* the following text is likely to have partial */
  /* overlapping, so XOR text fragments for clearer display */
  /* due to new text diplay, GL_XOR is no longer recommended --OVHPA */
  // glEnable(GL_COLOR_LOGIC_OP);
  // glLogicOp(GL_XOR);
  // glColor4f(1.0, 1.0, 1.0, 1.0);
  /* TODO - all atom related printing -> construct a string */
  label = g_string_new(NULL);

  if (data->show_selection_labels)
    list = data->selection;
  else
    list = data->cores;

  /*
  for (list=data->selection ; list ; list=g_slist_next(list))
  for (list=data->cores ; list ; list=g_slist_next(list))
  */

  while (list)
  {
    core[0] = list->data;
    if (core[0]->status & (DELETED | HIDDEN))
    {
      list = g_slist_next(list);
      hidden += 1;
      continue;
    }

    label = g_string_assign(label, "");

    /* show the order the atom was read in (start from 1 - helps with zmatrix debugging) */
    if (data->show_atom_index)
    {
      i = g_slist_index(data->cores, core[0]);
      g_string_append_printf(label, "[%d]", i + 1);
    }

    /* set up atom labels */
    if (data->show_atom_labels)
    {
      g_string_append_printf(label, "(%s)", core[0]->atom_label);
    }
    if (data->show_atom_types)
    {
      if (core[0]->atom_type)
      {
        g_string_append_printf(label, "(%s)", core[0]->atom_type);
      } else
      {
        g_string_append_printf(label, "(?)");
      }
    }

    /*VZ*/
    if (data->show_nmr_shifts)
    {
      g_string_append_printf(label, "(%4.2f)", core[0]->atom_nmr_shift);
    }
    if (data->show_nmr_csa)
    {
      g_string_append_printf(label, "(%4.2f;", core[0]->atom_nmr_aniso);
      g_string_append_printf(label, "%4.2f)", core[0]->atom_nmr_asym);
    }
    if (data->show_nmr_efg)
    {
      g_string_append_printf(label, "(%g;", core[0]->atom_nmr_cq);
      g_string_append_printf(label, "%4.2f)", core[0]->atom_nmr_efgasym);
    }

    /* get atom charge, add shell charge (if any) to get net result */
    if (data->show_atom_charges)
    {
      q = atom_charge(core[0]);
      g_string_append_printf(label, "{%6.4f}", q);
    }

    if (data->show_core_charges)
    {
      q = core[0]->charge;
      g_string_append_printf(label, "{%6.4f}", q);
    }

    if (data->show_shell_charges)
    {
      if (core[0]->shell)
      {
        shell = core[0]->shell;
        q = shell->charge;
        g_string_append_printf(label, "{%6.4f}", q);
      } else
        g_string_append_printf(label, "{}");
    }

    /* print */
    if (label->str)
    {
      ARR3SET(v1, core[0]->rx);
      pango_print_world(label->str, v1, canvas);
    }

    list = g_slist_next(list);
  }
  g_string_free(label, TRUE);

  if (hidden > 0 || data->selection)
  {
    gint base_y = canvas->height - canvas->y - 2 * gl_fontsize;
    if (hidden > 0)
    {
      text = g_strdup_printf("hidden: %d", hidden);
      pango_print(text, canvas->x + 20, base_y, canvas, gl_fontsize, 0);
      g_free(text);
    }
    if (data->selection)
    {
      text = g_strdup_printf("selected: %d", g_slist_length(data->selection));
      pango_print(text, canvas->x + 20, base_y - gl_fontsize, canvas, gl_fontsize, 0);
      g_free(text);
    }
  }

  /* print current frame */
  /* geom measurement labels */
  if (data->show_geom_labels)
    for (list = data->measure_list; list; list = g_slist_next(list))
    {
      type = measure_type_get(list->data);
      switch (type)
      {
      case MEASURE_BOND:
      case MEASURE_DISTANCE:
      case MEASURE_INTER:
      case MEASURE_INTRA:
        measure_coord_get(v1, 0, list->data, data);
        measure_coord_get(v2, 1, list->data, data);
        ARR3ADD(v1, v2);
        VEC3MUL(v1, 0.5);
        pango_print_world(measure_value_get(list->data), v1, canvas);
        break;

      case MEASURE_ANGLE:
        /* angle is i-j-k */
        measure_coord_get(v1, 0, list->data, data);
        measure_coord_get(v2, 1, list->data, data);
        measure_coord_get(v3, 2, list->data, data);
        /* angle label */
        /* FIXME - should use a similar process to the draw_arc code to */
        /* determine which arm is shorter & use that to determine label position */
        ARR3ADD(v1, v2);
        ARR3ADD(v1, v3);
        VEC3MUL(v1, 0.3333);
        pango_print_world(measure_value_get(list->data), v1, canvas);
        break;
      }
    }

  /* spatial object labels */
  // glDisable(GL_COLOR_LOGIC_OP);
  /* FIXME - need to change the variable name */
  if (data->morph_label)
    for (list = data->spatial; list; list = g_slist_next(list))
    {
      spatial = list->data;
      if (spatial->show_label)
      {
        glColor4f(spatial->c[0], spatial->c[1], spatial->c[2], 1.0);
        v = g_slist_nth_data(spatial->list, 0);
        if (gl_visible(v->rn, data))
          pango_print_world(spatial->label, spatial->x, canvas);
      }
    }
}

/********************************/
/* draw a ribbon special object */
/********************************/
void gl_draw_ribbon(struct model_pak *data)
{
  gdouble len, vec1[3], vec2[3];
  GSList *list, *rlist;
  struct ribbon_pak *ribbon;
  struct object_pak *object;
  GLfloat ctrl[8][3];

  for (list = data->ribbons; list; list = g_slist_next(list))
  {
    object = list->data;

    g_assert(object->type == RIBBON);

    /* go through the ribbon segment list */
    rlist = (GSList *) object->data;
    while (rlist != NULL)
    {
      ribbon = rlist->data;

      glColor4f(ribbon->colour[0], ribbon->colour[1], ribbon->colour[2], sysenv.render.transmit);

      /* end points */
      ARR3SET(&ctrl[0][0], ribbon->r1);
      ARR3SET(&ctrl[3][0], ribbon->r2);

      /* get distance between ribbon points */
      ARR3SET(vec1, ribbon->r1);
      ARR3SUB(vec1, ribbon->r2);
      len = VEC3MAG(vec1);

      /* shape control points */
      ARR3SET(&ctrl[1][0], ribbon->r1);
      ARR3SET(&ctrl[2][0], ribbon->r2);

      /* segment length based curvature - controls how flat it is at the cyclic group */
      ARR3SET(vec1, ribbon->o1);
      VEC3MUL(vec1, len * sysenv.render.ribbon_curvature);
      ARR3ADD(&ctrl[1][0], vec1);
      ARR3SET(vec2, ribbon->o2);
      VEC3MUL(vec2, len * sysenv.render.ribbon_curvature);
      ARR3ADD(&ctrl[2][0], vec2);

      /* compute offsets for ribbon thickness */
      crossprod(vec1, ribbon->n1, ribbon->o1);
      crossprod(vec2, ribbon->n2, ribbon->o2);
      normalize(vec1, 3);
      normalize(vec2, 3);

      /* thickness vectors for the two ribbon endpoints */
      VEC3MUL(vec1, 0.5 * sysenv.render.ribbon_thickness);
      VEC3MUL(vec2, 0.5 * sysenv.render.ribbon_thickness);

      /* ensure these are pointing the same way */
      if (via(vec1, vec2, 3) > PI / 2.0)
      {
        VEC3MUL(vec2, -1.0);
      }

      /* FIXME - 2D evaluators have mysteriously just stopped working */
      /* FIXME - fedora / driver problem? */
      /* FIXME - seems to be specific to jago (diff video card) -> update driver */
      if (sysenv.render.wire_surface)
      {

        /* CURRENT - exp using a 1D workaround (only use half the control points) */
        glLineWidth(sysenv.render.ribbon_thickness);

        glMap1f(GL_MAP1_VERTEX_3, 0.0, 1.0, 3, 4, &ctrl[0][0]);
        glEnable(GL_MAP1_VERTEX_3);
        glMapGrid1f(sysenv.render.ribbon_quality, 0.0, 1.0);
        glEvalMesh1(GL_LINE, 0, sysenv.render.ribbon_quality);

        /*
        {
        GLfloat f;
        glBegin(GL_LINE_STRIP);
        for (f=0.0 ; f<1.0 ; f+=0.01)
          {
          glEvalCoord1f(f);
          }
        glEnd();
        }
        */

      } else
      {
        /* init the bottom edge control points */
        ARR3SET(&ctrl[4][0], &ctrl[0][0]);
        ARR3SET(&ctrl[5][0], &ctrl[1][0]);
        ARR3SET(&ctrl[6][0], &ctrl[2][0]);
        ARR3SET(&ctrl[7][0], &ctrl[3][0]);
        /* lift points to make the top edge */
        ARR3ADD(&ctrl[0][0], vec1);
        ARR3ADD(&ctrl[1][0], vec1);
        ARR3ADD(&ctrl[2][0], vec2);
        ARR3ADD(&ctrl[3][0], vec2);
        /* lower points to make the bottom edge */
        ARR3SUB(&ctrl[4][0], vec1);
        ARR3SUB(&ctrl[5][0], vec1);
        ARR3SUB(&ctrl[6][0], vec2);
        ARR3SUB(&ctrl[7][0], vec2);
        /* drawing */
        /* CURRENT - 2D evaluators have mysteriously just stopped working */
        glMap2f(GL_MAP2_VERTEX_3, 0.0, 1.0, 3, 4, 0.0, 1.0, 12, 2, &ctrl[0][0]);
        glEnable(GL_MAP2_VERTEX_3);
        glEnable(GL_AUTO_NORMAL);
        glMapGrid2f(sysenv.render.ribbon_quality, 0.0, 1.0, 3, 0.0, 1.0);
        glEvalMesh2(GL_FILL, 0, sysenv.render.ribbon_quality, 0, 3);
      }

      rlist = g_slist_next(rlist);
    }
  }
}

/**************************/
/* spatial object drawing */
/**************************/
/* NB: different setup for vectors/planes - draw in seperate iterations */
void gl_draw_spatial(gint material, struct model_pak *data)
{
  gdouble size, v0[3], v1[3], v2[3], vi[3], mp[3], mn[3];
  GSList *list, *list1, *list2, *ilist;
  struct vec_pak *p1, *p2;
  struct spatial_pak *spatial;
  struct image_pak *image;
  struct camera_pak *camera;

  g_assert(data != NULL);

  camera = data->camera;

  g_assert(camera != NULL);

  ARR3SET(v0, camera->v);
  quat_rotate(v0, camera->q);

  for (list = data->spatial; list; list = g_slist_next(list))
  {
    spatial = list->data;

    /* NEW - normal check for wire frame and show hidden */
    if (sysenv.render.wire_surface && !sysenv.render.wire_show_hidden)
    {
      p1 = g_slist_nth_data(spatial->list, 0);

      if (via(v0, p1->rn, 3) < 0.5 * PI)
        continue;
    }

    if (spatial->material == material)
    {
      /* enumerate periodic images */
      ilist = NULL;
      do
      {
        if (ilist)
        {
          /* image */
          image = ilist->data;
          ARR3SET(vi, image->rx);
          ilist = g_slist_next(ilist);
        } else
        {
          /* original */
          VEC3SET(vi, 0.0, 0.0, 0.0);
          if (spatial->periodic)
            ilist = data->images;
        }

        switch (spatial->type)
        {
          /* vector spatials (special object) */
        case SPATIAL_VECTOR:
          list1 = spatial->list;
          list2 = g_slist_next(list1);
          while (list1 && list2)
          {
            p1 = list1->data;
            p2 = list2->data;
            ARR3SET(v1, p1->rx);
            ARR3SET(v2, p2->rx);
            draw_vector(v1, v2, 0.04);
            list1 = g_slist_next(list2);
            list2 = g_slist_next(list1);
          }
          break;

          /* generic spatials (method + vertices) */
        default:
          size = 0.0;
          VEC3SET(mp, 0.0, 0.0, 0.0);
          VEC3SET(mn, 0.0, 0.0, 0.0);

          // p1 = g_slist_nth_data(spatial->list, 0);/*FIX: 7d9c35*/

          /* CURRENT */
          /*
          sysenv.render.wire_show_hidden
          */

          glBegin(spatial->method);
          for (list1 = spatial->list; list1; list1 = g_slist_next(list1))
          {
            p1 = list1->data;
            ARR3SET(v1, p1->rx);
            ARR3ADD(v1, vi);

            ARR3ADD(mp, v1);
            ARR3SET(mn, p1->rn);
            size++;

            glColor4f(p1->colour[0], p1->colour[1], p1->colour[2], sysenv.render.transmit);
            glNormal3dv(p1->rn);
            glVertex3dv(v1);
          }
          glEnd();

          if (size > 0.1)
          {
            VEC3MUL(mp, 1.0 / size);
          }
          ARR3SET(spatial->x, mp);

          break;
        }
      } while (ilist);
    }
  }
}


/*************************/
/* draw camera waypoints */
/*************************/
void gl_camera_waypoints_draw(struct model_pak *model)
{
  gdouble x[3], o[3], e[3], b1[3], b2[3], b3[3], b4[4];
  GSList *list;
  struct point_pak sphere;

  gl_init_sphere(&sphere, model);

  for (list = model->waypoint_list; list; list = g_slist_next(list))
  {
    struct camera_pak *camera = list->data;
    if (camera == model->camera)
      continue;

    /* offset the sphere so it doesn't interfere with viewing */
    ARR3SET(x, camera->v);
    VEC3MUL(x, -0.1);
    ARR3ADD(x, camera->x);
    glColor3f(0.5, 0.5, 0.5);
    gl_draw_sphere(&sphere, x, 0.1);

#define CONE_SIZE 0.3

    /* get vector orthogonal to viewing and orientation vectors */
    ARR3SET(e, camera->e);
    VEC3MUL(e, CONE_SIZE);

    /* compute base of the cone */
    ARR3SET(b1, camera->v);
    VEC3MUL(b1, CONE_SIZE);
    ARR3ADD(b1, camera->x);
    ARR3SET(b2, b1);
    ARR3SET(b3, b1);
    ARR3SET(b4, b1);
    ARR3SET(o, camera->o);
    VEC3MUL(o, CONE_SIZE);

    /* compute 4 base points of the FOY square cone */
    ARR3ADD(b1, o);
    ARR3ADD(b1, e);
    ARR3ADD(b2, o);
    ARR3SUB(b2, e);
    ARR3SUB(b3, o);
    ARR3SUB(b3, e);
    ARR3SUB(b4, o);
    ARR3ADD(b4, e);

    /* square cone = FOV */
    glBegin(GL_TRIANGLE_FAN);
    glVertex3dv(camera->x);
    glVertex3dv(b2);
    glVertex3dv(b3);
    glVertex3dv(b4);
    glVertex3dv(b1);
    glEnd();

    /* complete the cone - different colour to indicate this is the UP orientation direction */
    glColor3f(1.0, 0.0, 0.0);
    glBegin(GL_TRIANGLES);
    glVertex3dv(camera->x);
    glVertex3dv(b1);
    glVertex3dv(b2);
    glEnd();
  }

  gl_free_points(&sphere);
}

/************************/
/* main drawing routine */
/************************/
void draw_objs(struct canvas_pak *canvas, struct model_pak *data)
{
  gint i;
  gulong time;
  gdouble r;
  gfloat specular[4], fog[4];
  gdouble fog_mark;
  GSList *pipes[4];
  GSList *solid = NULL, *ghost = NULL, *wire = NULL;

  time = mytimer();

  /* NEW - playing around with the idea of zone visibility */
  /*
  zone_visible_init(data);
  */

  /* transformation recording */
  if (data->mode == RECORD)
  {
    struct camera_pak *camera;

    /* hack to prevent duplicate recording of the 1st frame */
    /* FIXME - this will fail for multiple (concatenated) recordings */
    if (data->num_frames > 1)
    {
      camera = camera_dup(data->camera);
      data->transform_list = g_slist_append(data->transform_list, camera);
    }

    data->num_frames++;
  }

  /* set up the lighting */
  gl_init_lights(data);

  /* set up the fontsize --OVHPA */
  gl_get_fontsize();

  /* scaling affects placement to avoid near/far clipping */
  r = sysenv.rsize;

  /* main drawing setup */
  glFrontFace(GL_CCW);
  glEnable(GL_LIGHTING);
  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_CULL_FACE);
  glShadeModel(GL_SMOOTH);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glLineWidth(sysenv.render.line_thickness);
  glPointSize(2.0);

/* NEW - vertex arrarys */
#if VERTEX_ARRAYS
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_NORMAL_ARRAY);
#endif

  /* turn off antialiasing, in case it was previously on */
  glDisable(GL_LINE_SMOOTH);
  glDisable(GL_POINT_SMOOTH);
  glDisable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  /* depth queuing via fog */
  /* FIXME - broken due to new camera code */
  if (sysenv.render.fog)
  {
    glEnable(GL_FOG);

    ARR3SET(fog, sysenv.render.bg_colour);
    glFogfv(GL_FOG_COLOR, fog);

    glFogf(GL_FOG_MODE, GL_LINEAR);
    glHint(GL_FOG_HINT, GL_DONT_CARE);

    /* NB: only does something if mode is EXP */
    /*
      glFogf(GL_FOG_DENSITY, 0.1);
    */

    fog_mark = r;
    glFogf(GL_FOG_START, fog_mark);

    fog_mark += (1.0 - sysenv.render.fog_density) * 4.0 * r;
    glFogf(GL_FOG_END, fog_mark);
  } else
    glDisable(GL_FOG);

  /* solid drawing */
  glPolygonMode(GL_FRONT, GL_FILL);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);

  /* material shininess control (atoms) */
  VEC3SET(specular, sysenv.render.ahl_strength, sysenv.render.ahl_strength, sysenv.render.ahl_strength);
  glMaterialf(GL_FRONT, GL_SHININESS, sysenv.render.ahl_size);
  glMaterialfv(GL_FRONT, GL_SPECULAR, specular);

  /* colour-only change for spheres */
  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

  /* TODO - speedup - if atoms are v small - disable lighting */
  /* draw solid cores */
  if (data->show_cores)
    gl_build_core_lists(&solid, &ghost, &wire, data);
  gl_draw_cores(solid, data);

  /* pipe lists should be build AFTER core list (OFF_SCREEN flag tests) */
  render_make_pipes(pipes, data);

  /* double sided drawing for all spatial objects */
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glDisable(GL_CULL_FACE);

  /* draw solid bonds */
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  gl_draw_pipes(FALSE, pipes[0], data);

  /* material shininess control (surfaces) */
  VEC3SET(specular, sysenv.render.shl_strength, sysenv.render.shl_strength, sysenv.render.shl_strength);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
  glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, sysenv.render.shl_size);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glColor4f(sysenv.render.fg_colour[0], sysenv.render.fg_colour[1], sysenv.render.fg_colour[2], 1.0);

  /* draw camera waypoint locations */
  if (data->show_waypoints && !data->animating)
    gl_camera_waypoints_draw(data);

  gl_draw_spatial(SPATIAL_SOLID, data);

  /* wire frame stuff */
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  if (sysenv.render.antialias)
  {
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
  }
  gl_draw_cores(wire, data);

  /* cancel wire frame if spatials are to be solid */
  if (!sysenv.render.wire_surface)
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    /* spatials are potentially translucent */
    if (sysenv.render.transmit < 0.99)
    {
      /* alpha blending for translucent surfaces */
      glEnable(GL_BLEND);
      /* NB: make depth buffer read only - for translucent objects */
      /* see the red book p229 */
      glDepthMask(GL_FALSE);
    }
  }

  /* NEW - all spatials drawn with this line thickness */
  glLineWidth(sysenv.render.frame_thickness);

  /* FIXME - translucent spatials are not drawn back to front & can look funny */
  gl_draw_spatial(SPATIAL_SURFACE, data);

  /* ensure solid surfaces from now on */
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  if (data->show_axes)
  {
    /* always draw axes polygon fragments */
    glDepthFunc(GL_ALWAYS);
    gl_draw_axes(TRUE, canvas, data);
    glDepthFunc(GL_LESS);
  }

  /* enforce transparency */
  glEnable(GL_BLEND);
  glDepthMask(GL_FALSE);

  /* translucent ghost atoms */
  ghost = g_slist_sort(ghost, (gpointer) gl_depth_sort);
  gl_draw_cores(ghost, data);

  /* ghost bonds */
  /* NB: back to front ordering */
  pipes[1] = render_sort_pipes(pipes[1]);
  gl_draw_pipes(FALSE, pipes[1], data);

  /* translucent shells */
  if (data->show_shells)
    gl_draw_shells(data);

  /* CURRENT - ribbon drawing is broken */
  gl_draw_ribbon(data);

  /* halos */
  glDisable(GL_LIGHTING);
  glShadeModel(GL_FLAT);
  gl_draw_halo_list(data->selection, data);

  /* stop translucent drawing/depth buffering */
  glDepthMask(GL_TRUE);

  /* at this point, should have completed all colour_material routines */
  /* ie only simple line/text drawing stuff after this point */
  glEnable(GL_CULL_FACE);
  glDisable(GL_COLOR_MATERIAL);

  gl_draw_spatial(SPATIAL_LINE, data);

  /* always visible foreground colour */
  glColor4f(sysenv.render.fg_colour[0], sysenv.render.fg_colour[1], sysenv.render.fg_colour[2], 1.0);

  /* unit cell drawing */
  if (data->show_cell && data->periodic)
  {
    glLineWidth(sysenv.render.frame_thickness);
    gl_draw_cell(data);
  }

  /* draw wire/line bond types */
  glLineWidth(sysenv.render.stick_thickness);
  glColor4f(1.0, 0.85, 0.5, 1.0);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  /* wire frame cylinders bonds */
  glDisable(GL_CULL_FACE);
  gl_draw_pipes(FALSE, pipes[2], data);
  glEnable(GL_CULL_FACE);

  /* stick (line) bonds */
  gl_draw_pipes(TRUE, pipes[3], data);

  /* set up stippling */
  glEnable(GL_LINE_STIPPLE);
  glLineStipple(1, 0x0F0F);
  glLineWidth(1.0);

  /* periodic cell images */
  glColor3f(0.8, 0.7, 0.6);
  if (data->show_cell_images)
    gl_draw_cell_images(data);

  /* enforce no depth queuing from now on */
  glDisable(GL_FOG);

  /* active/measurements colour (yellow) */
  glColor4f(sysenv.render.label_colour[0], sysenv.render.label_colour[1], sysenv.render.label_colour[2], 1.0);

  /* selection box */
  glLineWidth(1.5);
  if (data->box_on)
    gl_draw_box(data->select_box[0], data->select_box[1], data->select_box[2], data->select_box[3], canvas);

  /* measurements */
  glLineWidth(sysenv.render.geom_line_width);
  gl_draw_measurements(data);

  /* CURRENT - core/shell links */
  if (data->show_links)
    gl_draw_links(data);

  glDisable(GL_LINE_STIPPLE);

  /* text drawing - accept all fragments */
  glDepthFunc(GL_ALWAYS);
  glColor4f(sysenv.render.fg_colour[0], sysenv.render.fg_colour[1], sysenv.render.fg_colour[2], 1.0);
  gl_draw_text(canvas, data);

  if (data->show_axes)
    gl_draw_axes(FALSE, canvas, data);

  /* free all core lists */
  g_slist_free(solid);
  g_slist_free(ghost);
  g_slist_free(wire);

  /* free all pipes */
  for (i = 4; i--;)
    free_slist(pipes[i]);

  /* save timing info */
  data->redraw_current = mytimer() - time;
  data->redraw_cumulative += data->redraw_current;
  data->redraw_count++;
}

/************************/
/* total canvas refresh */
/************************/
/*
void gl_draw(struct canvas_pak *canvas, struct model_pak *data)
{
  gint flag;
  gdouble sq, cq;
  PangoFontDescription *pfd;

  if (!data)
    {
    gui_atom_widget_update(NULL, NULL);
    return;
    }

  sq = sysenv.render.sphere_quality;
  cq = sysenv.render.cylinder_quality;
  if (sysenv.moving && sysenv.render.fast_rotation)
    {
    sysenv.render.sphere_quality = 1;
    sysenv.render.cylinder_quality = 5;
    }

  sysenv.render.sphere_quality = sq;
  sysenv.render.cylinder_quality = cq;
}
*/

/****************************************************************************/
/* timing analysis, returns true if redraw timeout was adjusted, else false */
/****************************************************************************/
#define DEBUG_CANVAS_TIMING 0
gint canvas_timing_adjust(struct model_pak *model)
{
  static gint n = 1;
  gint m;
  gdouble time;

  /* In Qt mode, frame pacing is handled by paintGL's semaphore + update().
   * The old timer-based approach (g_timeout_add) doesn't work without the
   * old main loop. Skip timing adjustments entirely in Qt mode. */
  if (sysenv.qt_mode)
    return (FALSE);

  /* timing analysis */
  if (model->redraw_count >= 10)
  {
    time = model->redraw_cumulative;
    time /= model->redraw_count;
    time /= 1000.0;

#if DEBUG_CANVAS_TIMING
    printf("[redraw] cumulative = %d us : average = %.1f ms : freq = %d x 25ms\n", model->redraw_cumulative, time, n);
#endif

    model->redraw_count = 0;
    model->redraw_cumulative = 0;

    /* adjust redraw frequency? */
    m = 1 + time / 25;
    /* NEW - only increase if the difference is at least */
    /* two greater to prevent flicking back and forth */
    if (m > n + 1)
    {
      n = m;
#if DEBUG_CANVAS_TIMING
      printf("increasing delay between redraw: %d\n", n);
#endif
      g_timeout_add(n * 25, (GSourceFunc) &gui_canvas_handler, NULL);
      return (TRUE);
    }
  } else
  {
    /* redraw frequency test */
    if (n > 1 && model->redraw_count)
    {
      time = model->redraw_current;

      time *= 0.001;
      m = 1 + time / 25;
      /* adjust redraw frequency? */
      if (m < n)
      {
        n = m;
#if DEBUG_CANVAS_TIMING
        printf("decreasing delay between redraw: %d : %dus)\n", n, model->redraw_current);
#endif
        g_timeout_add(n * 25, (GSourceFunc) &gui_canvas_handler, NULL);
        return (TRUE);
      }
    }
  }
  return (FALSE);
}

/************************************************************/
/* perform a snapshot of sysenv.glarea onto eps_file --OVHPA*/
/************************************************************/
void do_eps_snapshot(struct model_pak *model, gint w, gint h)
{
  /* EPS snapshot: save cairo surface as PNG. */
  if (model == NULL)
    return;
  if (model->eps_file == NULL)
    return;

  extern unsigned char *qt_cairo_surface(void);
  extern gint qt_cairo_width(void);
  extern gint qt_cairo_height(void);
  unsigned char *pixels = qt_cairo_surface();
  int cw = qt_cairo_width();
  int ch = qt_cairo_height();
  if (!pixels || cw <= 0 || ch <= 0)
    return;

  /* Write raw ARGB32 to a PNG file (same path with .png extension). */
  gchar *tmpfile = g_strdup_printf("%s.png", model->eps_file);
  FILE *fp = fopen(tmpfile, "wb");
  if (fp)
  {
    fwrite(pixels, 4, cw * ch, fp);
    fclose(fp);
  }
  g_free(tmpfile);

  model->snapshot_eps = FALSE;
}

/**************************/
/* handle redraw requests */
/**************************/
#define DEBUG_CANVAS_REFRESH 0
#define GDK_GL_DEBUG 0
#define DEBUG_CANVAS_REFRESH_TRACE 0
gint gl_canvas_refresh(void)
{
  gint nc;
  GSList *list;
  struct model_pak *model;
  struct canvas_pak *canvas;
  gboolean do_snap = FALSE;
  struct canvas_pak *snap_canvas;
  /*CAIRO write --OHPA*/
  unsigned char *surface_data = NULL;
  gint w, h;

#if DEBUG_CANVAS_REFRESH_TRACE
  fprintf(stderr, "[RENDER]  gl_canvas_refresh ENTER: active_model=%p\n", (void*)sysenv.active_model);
#endif

  /* Semaphore entry guard: prevent re-entrant gl_canvas_refresh calls.
   * In Qt mode, multiple update() requests can stack up during rapid mouse
   * motion. The paintGL semaphore catches most cases, but this C-side guard
   * is a safety net for direct redraw_canvas() calls from non-GUI code paths. */
  if (sysenv.active_model && ((struct model_pak *) sysenv.active_model)->drawing_in_progress)
    {
#if DEBUG_CANVAS_REFRESH_TRACE
      fprintf(stderr, "[RENDER]  EARLY EXIT: drawing_in_progress\n");
#endif
      return TRUE;
    }

  /* Safety: if no canvases, just clear and return */
  if (!sysenv.canvas_list)
    {
#if DEBUG_CANVAS_REFRESH_TRACE
      fprintf(stderr, "[RENDER]  EARLY EXIT: no canvas list\n");
#endif
      return TRUE;
    }

  /* No active model — clear the framebuffer to background colour so stale
   * geometry from a deleted model is removed. */
  if (!sysenv.active_model)
  {
    glClearColor(sysenv.render.bg_colour[0], sysenv.render.bg_colour[1], sysenv.render.bg_colour[2], 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    return TRUE;
  }

  /* Semaphore: prevent recursive/interleaved gl_canvas_refresh calls.
   * When two render paths fight each other (timer + timeout), one can
   * overwrite the other's draw with a stale clear — black screen. */
  /* Check if anything needs rendering — bails out when nothing is dirty.
   * model->redraw is set by: load, model switch, mouse drag operations.
   * During drag (sysenv.moving), always render to keep the view updated. */
  gboolean need_render = FALSE;
  for (list = sysenv.canvas_list; list; list = g_slist_next(list))
  {
    canvas = list->data;
    model = canvas->model;
#if DEBUG_CANVAS_REFRESH_TRACE
      fprintf(stderr, "[RENDER]  need_check: canvas_model=%p, active=%p, w=%d, h=%d\n",
             (void*)model, (void*)sysenv.active_model,
             canvas->width, canvas->height);
#endif
    if (model && canvas->width > 0 && canvas->height > 0)
    {
      gboolean match_active = (canvas->model == sysenv.active_model);
      /* During drag, always render — the view is changing every frame. */
      if (sysenv.moving)
      {
        need_render = TRUE;
#if DEBUG_CANVAS_REFRESH_TRACE
        fprintf(stderr, "[RENDER]    need_render=TRUE (moving)\n");
#endif
        break;
      }
      /* If this is the active model, only render when dirty or resized.
       * If this is NOT the active model, always render (stale canvas). */
      if (model->redraw || canvas->resize || !match_active)
      {
        need_render = TRUE;
#if DEBUG_CANVAS_REFRESH_TRACE
        fprintf(stderr, "[RENDER]    need_render=TRUE (redraw=%d resize=%d match_active=%d)\n",
               model->redraw, canvas->resize, (int)match_active);
#endif
        break;
      }
    }
  }

  /* If nothing needs rendering, skip — the previous frame's content is still valid.
   * Clearing to background here causes flicker/black screen during drag polling. */
#if DEBUG_CANVAS_REFRESH_TRACE
  fprintf(stderr, "[RENDER]  need_render=%d\n", (int)need_render);
#endif
  if (!need_render)
  {
    if (sysenv.active_model)
      ((struct model_pak *) sysenv.active_model)->drawing_in_progress = FALSE;
    return TRUE;
  }

  /* divert to stereo update? */
  if (sysenv.stereo)
  {
    /* Stereo rendering path: use stereo_redraw() which handles both eyes,
     * anaglyph mode, and frustum-based separation. This works with the
     * Qt canvas system by iterating over all canvases (left + right eye). */
    if (sysenv.active_model)
      ((struct model_pak *) sysenv.active_model)->drawing_in_progress = FALSE;
    stereo_redraw();
    return (TRUE);
  }

  /* Qt handles GL context management - no gdk_gl_begin needed */

  /* Ensure foreground/label colours match the current background.
   * This is called every frame but the cache in make_fg_visible() skips
   * recalculation when bg hasn't changed (cheap comparison). */
  make_fg_visible();

  glClearColor(sysenv.render.bg_colour[0], sysenv.render.bg_colour[1], sysenv.render.bg_colour[2], 0.0);
  glClearStencil(0x0);
  glDrawBuffer(GL_BACK);

  /* Clear the framebuffer when needed:
   * - First draw after init: Qt may have left garbage/black screen.
   * - During drag rotation/zoom/pan: camera moved, old pixels are stale.
   * - Geometry changed (atoms/bonds added/removed): need_clear is TRUE.
   * We always set glClearColor above, so the clear uses correct bg colour. */
  static gboolean first_draw = TRUE;
  gboolean do_clear = FALSE;
  if (first_draw)
  {
    do_clear = TRUE;
    first_draw = FALSE;
  }
  if (sysenv.moving)
    do_clear = TRUE;
  /* After a drag release, the old framebuffer may contain stale content
   * (e.g., selection box). Ensure we clear when moving just stopped. */
  if (!sysenv.moving && sysenv.just_released)
  {
    do_clear = TRUE;
    sysenv.just_released = FALSE;
  }
  if (sysenv.active_model && ((struct model_pak *) sysenv.active_model)->need_clear)
  {
    do_clear = TRUE;
    ((struct model_pak *) sysenv.active_model)->need_clear = FALSE;
    /* Clear overlay count so old text from previous model/frame doesn't persist */
    extern void qt_text_overlay_free_all(void);
    qt_text_overlay_free_all();
  }

  if (do_clear)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  /* Get canvas dimensions from sysenv */
  w = sysenv.width;
  h = sysenv.height;

  /* Reuse a persistent Cairo surface buffer to avoid per-frame allocation.
   * The old code did g_malloc0(4*w*h) every frame — for 1920x1080 that's ~8MB
   * of alloc+zero per frame, which is extremely expensive. */
  if (sysenv.cairo_width != w || sysenv.cairo_height != h)
  {
    /* Canvas dimensions changed — free old overlays since positions are stale. */
    qt_text_overlay_free_all();
    g_free(sysenv.cairo_surface);
    surface_data = g_malloc(4 * w * h * sizeof(unsigned char));
    sysenv.cairo_surface = surface_data;
    sysenv.cairo_width = w;
    sysenv.cairo_height = h;
  }
  /* Note: buffer is NOT zeroed on reuse — text overlays write their own pixels. */

  /* Always clear overlay count at start of render pass to prevent text
   * accumulation across frames (e.g., during drag rotation). Qt has already
   * deep-copied previous overlays, so clearing the C-side array is safe. */
  qt_text_overlay_free_all();

  /* Enable alpha blending for text compositing */
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  nc = g_slist_length(sysenv.canvas_list);

  for (list = sysenv.canvas_list; list; list = g_slist_next(list))
  {
    canvas = list->data;
    model = canvas->model;

#if DEBUG_CANVAS
    gchar *str = g_strdup_printf("FPS: %i", sysenv.fps);
    pango_print(str, 5, 5, canvas, 10, 0);
    g_free(str);
    printf("canvas: %p\nactive: %d\nresize: %d\nmodel: %p\n", canvas, canvas->active, canvas->resize, canvas->model);
#endif

#if DEBUG_CANVAS_REFRESH_TRACE
      fprintf(stderr, "[RENDER]  Canvas: model=%p, active_model=%p, redraw=%d, resize=%d\n",
             (void*)model, (void*)sysenv.active_model,
             model ? model->redraw : -1, canvas->resize);
#endif

    /* set up viewing transformations (even if no model - border) */
    gl_init_projection(canvas, model);

    /* drawing (model only) */
    if (model)
    {
      /* Redraw if dirty or resized. If this is the active model and nothing changed, still draw
       * to prevent black screen during drag when no redraw flag was set. */
      gboolean redraw_this = model->redraw || canvas->resize;
      int skip = (!redraw_this && canvas->model != sysenv.active_model);
#if DEBUG_CANVAS_REFRESH_TRACE
      fprintf(stderr, "[RENDER]    redraw_this=%d, match_active=%d, skip=%d\n",
             (int)redraw_this, (canvas->model == sysenv.active_model), (int)skip);
#endif
      if (skip)
        continue;

#if DEBUG_CANVAS_REFRESH
      printf("gl_draw(): %d,%d - %d x %d\n", canvas->x, canvas->y, canvas->width, canvas->height);
#endif

      if (model->graph_active)
        graph_draw(canvas, model);
      else
        draw_objs(canvas, model);

      model->redraw = FALSE;
      canvas->resize = FALSE;

      if (model->snapshot_eps)
      {
        snap_canvas = canvas;
        do_snap = TRUE; /*<- do actual snapshot AFTER drawing*/
      }

      /* Draw text overlays directly using OpenGL textured quads.
       * No glReadPixels — the OpenGL back buffer is swapped by Qt at end of paintGL(). */

      /* Text overlays will be drawn by Qt's QPainter in paintGL(). */
      /* --OVHPA*/
      if (canvas_timing_adjust(model))
      {
        /* Qt handles buffer swap in paintGL() */
        /* Don't free cairo_surface — it's a persistent buffer reused across frames. */
        if (sysenv.active_model)
          ((struct model_pak *) sysenv.active_model)->drawing_in_progress = FALSE;
        return (FALSE);
      }
    }

    /* draw viewport frames if more than one canvas */
    if (nc > 1)
    {
      glColor3f(1.0, 1.0, 1.0);
      if (sysenv.active_model)
      {
        if (canvas->model == sysenv.active_model)
          glColor3f(1.0, 1.0, 0.0);
      }
      glLineWidth(1.0);
      gl_draw_box(canvas->x + 1, sysenv.height - canvas->y - 1, canvas->x + canvas->width - 1,
                  sysenv.height - canvas->y - canvas->height, canvas);
    }
  }
  /* Don't free or null the persistent cairo surface — it's reused across frames.
   * The buffer is consumed by overlay push; leave pointer intact so
   * subsequent pango_print calls in this frame can still write into it. */

  if (do_snap)
    do_eps_snapshot(snap_canvas->model, snap_canvas->width, snap_canvas->height); /*snapshot here*/

  /* Release drawing semaphore */
  if (sysenv.active_model)
    ((struct model_pak *) sysenv.active_model)->drawing_in_progress = FALSE;

  return (TRUE);
}

/**************************/
/* handle redraw requests */
/**************************/
gint gui_canvas_handler(gpointer *dummy)
{
  static gulong start = 0, time = 0, frames = 0;

  /* first time init */
  if (!start)
    start = mytimer();

  frames++;

  /* update FPS every nth frame */
  if (frames > 10)
  {
    gdouble fps = frames * 1000000.0;

    time = mytimer() - start;

    sysenv.fps = nearest_int(fps / (gdouble) time);
    start = mytimer();
    time = frames = 0;
  }

  /* In Qt mode, rendering is handled by the polling loop in paintGL(). */

  if (sysenv.refresh_dialog)
  {
    /* dialog redraw */
    dialog_refresh_all();

    /* model pane redraw */
    /* tree_model_refresh(sysenv.active_model); */

    /* selection redraw */
    gui_active_refresh();

    sysenv.refresh_dialog = FALSE;
  }
  return (TRUE);
}
