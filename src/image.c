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
#include <string.h>

#include "gdis.h"
#include "coords.h"
#include "edit.h"
#include "file.h"
#include "matrix.h"
#include "opengl.h"
#include "dialog.h"
#include "interface.h"
#include "gui_image.h"

// #include "folder.xpm"
// #include "disk.xpm"
// #include "arrow.xpm"
// #include "axes.xpm"
// #include "tools.xpm"
// #include "palette.xpm"
// #include "cross.xpm"
// #include "geom.xpm"
// #include "cell.xpm"
// #include "camera.xpm"
// #include "element.xpm"
// #include "tb_animate.xpm"
// #include "tb_diffraction.xpm"
// #include "tb_isosurface.xpm"
// #include "tb_surface.xpm"
// #include "canvas_single.xpm"
// #include "canvas_create.xpm"
// #include "canvas_delete.xpm"
/* NEW: include a button to create a new model */
// #include "plus.xpm"
/* NEW: include a control for plots and eps/png export tool -- OVHPA */
// #include "plots.xpm"
// #include "to_eps.xpm"
// #include "to_png.xpm"
/* NEW: tracking system --OVHPA */
// #include "track.xpm"
/* NEW: include buttons for aligning model */
// #include "xview.xpm"
// #include "yview.xpm"
// #include "zview.xpm"
// #include "aview.xpm"
// #include "bview.xpm"
// #include "cview.xpm"
// #include "rotate1.xpm"
// #include "rotate2.xpm"
// #include "rotate3.xpm"
// #include "select_all.xpm"

extern struct sysenv_pak sysenv;

/****************************************/
/* callback to schedule a canvas export */
/****************************************/
void image_export(gchar *name)
{
  g_assert(name != NULL);

  sysenv.snapshot = TRUE;
  sysenv.snapshot_filename = g_build_filename(sysenv.cwd, name, NULL);
}

/*************************************************/
/* add a filename to active model's picture list */
/*************************************************/
void image_import(const gchar *name)
{
  gchar *picture;
  struct model_pak *model;

  /* checks */
  /* TODO - check file validity */
  g_assert(name != NULL);

  model = sysenv.active_model;
  if (!model)
    return;

  picture = g_strdup(name);

  model->picture_list = g_slist_append(model->picture_list, picture);
  model->picture_active = picture;
  model->graph_active = NULL;

  /* updates */
  tree_model_add(model);
  redraw_canvas(SINGLE);
}

/**************************************/
/* retrieve a standard image's pixbuf */
/**************************************/
gpointer image_table_lookup(const gchar *name) { return (g_hash_table_lookup(sysenv.image_table, name)); }
