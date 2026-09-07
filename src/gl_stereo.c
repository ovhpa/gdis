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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define GLIB_DISABLE_DEPRECATION_WARNINGS
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "gdis.h"
#include "coords.h"
#include "matrix.h"
#include "opengl.h"
#include "render.h"
#include "interface.h"

/* externals */
extern struct sysenv_pak sysenv;
extern struct elem_pak elements[];

/* defs */
#define SGI_STEREO_X 148
#define SGI_STEREO_Y 532
#define SGI_STEREO_WIDTH 982
#define SGI_STEREO_HEIGHT 491

enum {
  STEREO_ACTIVE,
  STEREO_PASSIVE,
};

/* globals */
gint stereo_x = 0, stereo_y = 0, stereo_width = 0, stereo_height = 0, stereo_depth = 0;

/*******************/
/* redraw handling */
/*******************/
#define DEBUG_STEREO_REDRAW 0
void stereo_redraw(void)
{
  gint x1, y1, x2, y2, dim;
#ifdef UNUSED_BUT_SET
  gint width, height;
  gdouble s, x, y;
#endif
  gdouble dist, eye, off;
  struct model_pak *model;
  struct canvas_pak left, right;
  struct camera_pak *camera = NULL;

  model = sysenv.active_model;
  if (model)
    camera = model->camera;

  glClearColor(sysenv.render.bg_colour[0], sysenv.render.bg_colour[1], sysenv.render.bg_colour[2], 1.0);
  glClearStencil(0x4);

#if DEBUG_STEREO_REDRAW
  printf(" --- STEREO ---\n");
  printf("x+y = %d + %d\n", stereo_x, stereo_y);
  printf("wxh = %d x %d\n", stereo_width, stereo_height);

  printf(" --- CANVAS ---\n");
  printf("x+y = %d + %d\n", sysenv.x, sysenv.y);
  printf("wxh = %d x %d\n", sysenv.width, sysenv.height);

  printf(" r  = %f\n", sysenv.rsize);
  printf("eye = %f\n", eye);
#endif

#ifdef __sgi
  /* TOP/BOTTOM viewports */
  x1 = SGI_STEREO_X;
  y1 = SGI_STEREO_Y;
  x2 = SGI_STEREO_X;
  y2 = 0;
#ifdef UNUSED_BUT_SET
  width = SGI_STEREO_WIDTH;
  height = SGI_STEREO_HEIGHT;
#endif
#else
  if (sysenv.stereo_fullscreen && !sysenv.render.stereo_quadbuffer)
  {
    /* fullscreen with no quad buffer -> assume dual head */

    dim = stereo_width / 2 < stereo_height ? stereo_width / 2 : stereo_height;

    x1 = (stereo_width / 2 - dim) / 2;
    y1 = (stereo_height - dim) / 2;

    x2 = stereo_width / 2 + x1;
    y2 = (stereo_height - dim) / 2;
#ifdef UNUSED_BUT_SET
    width = dim;
    height = dim;
#endif
  } else
  {
    x1 = x2 = stereo_x;
    y1 = y2 = stereo_y;
#ifdef UNUSED_BUT_SET
    width = stereo_width;
    height = stereo_height;
#endif
  }
#endif

#ifdef UNUSED_BUT_SET
  /* full canvas perspective projection */
  s = sysenv.size;
  x = sysenv.rsize * width / s;
  y = sysenv.rsize * height / s;
#endif

  /* NEW - fudge to make old code work with new canvas scheme */
  left.x = x1;
  right.x = x2;

  left.y = y1;
  right.y = y2;

  left.width = stereo_width;
  right.width = stereo_width;

  left.height = stereo_height;
  right.height = stereo_height;

  left.size = right.size = sysenv.size;

  left.active = right.active = TRUE;
  left.resize = right.resize = TRUE;
  left.model = right.model = model;

/* calculate distance to frustum (ie near) based on desired field of view */
/* wrong ... changing doesnt affect FOV, only correct/incorrect clipping  */
#define STEREO_FOV D2R * 70.0

/* playing with these seems to do nothing at all to the perspective FOV */
#define FAR_CLIP 100.0
#define NEAR_CLIP 0.5

  dist = sysenv.rsize / tan(0.5 * STEREO_FOV);
  eye = 0.01 * sysenv.render.stereo_eye_offset * dist;

  /* NEW */
  off = 0.01 * sysenv.render.stereo_parallax * dist;

  /* large eye sep - small parallax -> +ve para -> everything BEHIND */
  /* small eye sep - large parallax -> -ve papa -> some stuff in FRONT */

  /* always clear the canvas first (eliminates left over images if left/right eyes are turned off) */
  glDrawBuffer(GL_BACK);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  /* Anaglyph path: draw left eye with red mask, right eye with cyan mask */
  if (sysenv.render.stereo_anaglyph)
  {
    if (model)
    {
      gdouble eye, off;
      struct camera_pak *camera = model->camera;

      dist = sysenv.rsize / tan(0.5 * STEREO_FOV);
      eye = 0.01 * sysenv.render.stereo_eye_offset * dist;
      off = 0.01 * sysenv.render.stereo_parallax * dist;

      /* LEFT EYE */
      if (sysenv.render.stereo_anaglyph_blue_red)
        glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE); /* red */
      else
        glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE); /* red */
      glDepthMask(GL_TRUE);
      glDepthMask(GL_TRUE);

      gl_init_projection(&left, model);

      if (sysenv.render.stereo_use_frustum)
      {
        gdouble r;
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        r = sysenv.rsize;
        if (camera)
          r *= camera->zoom;
        if (sysenv.aspect > 1.0)
          glFrustum(-r * sysenv.aspect + off, r * sysenv.aspect + off, -r, r, NEAR_CLIP * dist,
                    dist + FAR_CLIP * sysenv.rsize);
        else
          glFrustum(-r + off, r + off, -r / sysenv.aspect, r / sysenv.aspect, NEAR_CLIP * dist,
                    dist + FAR_CLIP * sysenv.rsize);
        glTranslatef(0.0, 0.0, -sysenv.rsize * 0.5);
      }
      glTranslatef(+eye, 0.0, 0.0);
      draw_objs(NULL, model);
    }

    /* RIGHT EYE */
    if (sysenv.render.stereo_anaglyph_blue_red)
      glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE); /* blue */
    else
      glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE); /* cyan */
    glDepthMask(GL_TRUE);
    glDepthMask(GL_TRUE);

    if (model)
    {
      struct camera_pak *camera = model->camera;

      gl_init_projection(&right, model);

      if (sysenv.render.stereo_use_frustum)
      {
        gdouble r;
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        r = sysenv.rsize;
        if (camera)
          r *= camera->zoom;
        if (sysenv.aspect > 1.0)
          glFrustum(-r * sysenv.aspect - off, r * sysenv.aspect - off, -r, r, NEAR_CLIP * dist,
                    dist + FAR_CLIP * sysenv.rsize);
        else
          glFrustum(-r - off, r - off, -r / sysenv.aspect, r / sysenv.aspect, NEAR_CLIP * dist,
                    dist + FAR_CLIP * sysenv.rsize);
        glTranslatef(0.0, 0.0, -sysenv.rsize * 0.5);
      }
      glTranslatef(-eye, 0.0, 0.0);
      draw_objs(NULL, model);
    }

    /* Restore full color mask */
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    return;
  }

  if (sysenv.render.stereo_left)
  {
    if (model)
    {
      gl_init_projection(&left, model);

      /* CURRENT - with the new camera code things have been messed up a bit */
      /* easy solution - just stick to perspective view in the standard */
      /* gl_init_projection() and translate for each eye. But, there may be */
      /* viewing errors this way - proper way (via frustum) is more awkward */
      /* due to clipping and scaling problems */
      if (sysenv.render.stereo_use_frustum)
      {
        gdouble r;

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        r = sysenv.rsize;

        if (camera)
          r *= camera->zoom;
        if (sysenv.aspect > 1.0)
          glFrustum(-r * sysenv.aspect + off, r * sysenv.aspect + off, -r, r, NEAR_CLIP * dist,
                    dist + FAR_CLIP * sysenv.rsize);
        else
          glFrustum(-r + off, r + off, -r / sysenv.aspect, r / sysenv.aspect, NEAR_CLIP * dist,
                    dist + FAR_CLIP * sysenv.rsize);

        glTranslatef(0.0, 0.0, -sysenv.rsize * 0.5);
      }

      // CURRENT - reverse eye
      glTranslatef(+eye, 0.0, 0.0);

      glDrawBuffer(GL_BACK_LEFT);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      draw_objs(NULL, model);
    }
  }

  /* RIGHT */
  if (sysenv.render.stereo_right)
  {

    if (model)
    {
      /* only draw to right buffer if it exists, otherwise stay with same (left) one */

      gl_init_projection(&right, model);

      if (sysenv.render.stereo_use_frustum)
      {
        gdouble r;

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        r = sysenv.rsize;

        if (camera)
          r *= camera->zoom;

        if (sysenv.aspect > 1.0)
          glFrustum(-r * sysenv.aspect - off, r * sysenv.aspect - off, -r, r, NEAR_CLIP * dist,
                    dist + FAR_CLIP * sysenv.rsize);
        else
          glFrustum(-r - off, r - off, -r / sysenv.aspect, r / sysenv.aspect, NEAR_CLIP * dist,
                    dist + FAR_CLIP * sysenv.rsize);

        glTranslatef(0.0, 0.0, -sysenv.rsize * 0.5);
      }

      // CURRENT - reverse eye
      glTranslatef(-eye, 0.0, 0.0);

      if (sysenv.render.stereo_quadbuffer)
        glDrawBuffer(GL_BACK_RIGHT);

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      draw_objs(NULL, model);
    }
  }

  /* NEW - store matrices for proj/unproj operations */
  /* FIXME - doesn't seem to work */
  /*
  glGetIntegerv(GL_VIEWPORT, viewport);
  glGetDoublev(GL_MODELVIEW_MATRIX, mvmatrix);
  glGetDoublev(GL_PROJECTION_MATRIX, projmatrix);
  */
}

/********************************/
/* stereo window expose handler */
/********************************/
gint stereo_expose_event(gpointer w, gpointer event)
{
  /* Qt handles GL context - no gdk_gl_begin needed */
  stereo_redraw();
  /* Qt handles buffer swap */
  return (TRUE);
}

/*****************************/
/* init WINDOWED stereo mode */
/*****************************/
void stereo_init_window(struct canvas_pak *canvas)
{
  if (sysenv.stereo_fullscreen)
    return;

  stereo_x = canvas->x;
  stereo_y = canvas->y;
  stereo_width = canvas->width;
  stereo_height = canvas->height;
}

/*******************************/
/* init FULLSCREEN stereo mode */
/*******************************/
#define DEBUG_FULLSCREEN 0
void stereo_open_window(void)
{
  /* Qt handles fullscreen stereo via QOpenGLWidget */
  /* TODO: implement fullscreen stereo via Qt */
  if (!sysenv.stereo_fullscreen)
    return;
  printf("NOTE: Fullscreen stereo not yet implemented in Qt version.\n");
}
