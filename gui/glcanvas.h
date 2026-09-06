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

#ifndef GLCANVAS_H
#define GLCANVAS_H

/* glib must be included before Qt to avoid C++ exception specifier conflicts */
#ifndef G_DISABLE_SINGLE_INCLUDES
#define G_DISABLE_SINGLE_INCLUDES
#endif
#include <glib.h>

#include <QOpenGLWidget>
#include <QOpenGLFunctions_2_1>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>

class GLCanvas : public QOpenGLWidget, protected QOpenGLFunctions_2_1
{
  Q_OBJECT

public:
  explicit GLCanvas(QWidget *parent = nullptr);
  ~GLCanvas() override;

  /* Set canvas index for multi-canvas mode */
  void setCanvasIndex(int idx);
  int canvasIndex() const { return m_canvasIndex; }

protected:
  void initializeGL() override;
  void paintGL() override;
  void resizeGL(int w, int h) override;
  QSize sizeHint() const override;
  void mousePressEvent(QMouseEvent *ev) override;
  void mouseMoveEvent(QMouseEvent *ev) override;
  void mouseReleaseEvent(QMouseEvent *ev) override;
  void wheelEvent(QWheelEvent *ev) override;
  void keyPressEvent(QKeyEvent *ev) override;
  void keyReleaseEvent(QKeyEvent *ev) override;

private:
  int m_canvasIndex;
  QPoint m_lastPos;
};

#endif /* GLCANVAS_H */
