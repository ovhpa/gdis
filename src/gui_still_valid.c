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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <time.h>
// #include <unistd.h>
#define G_DISABLE_DEPRECATED
#define GLIB_DISABLE_DEPRECATION_WARNINGS
/* for g_remove: */
#include <glib/gstdio.h>

#ifndef __WIN32
#include <sys/times.h>
#endif

#include "gdis.h"
#include "coords.h"
#include "edit.h"
#include "model.h"
#include "file.h"
#include "graph_internal.h"
#include "task.h"
#include "morph.h"
#include "model.h"
#include "module.h"
#include "matrix.h"
#include "measure.h"
#include "render.h"
#include "select.h"
#include "space.h"
#include "sginfo.h"
#include "spatial.h"
#include "opengl.h"
#include "quaternion.h"
#include "surface.h"
#include "gui_shorts.h"
#include "interface.h"
#include "dialog.h"
#include "zmatrix.h"
#include "gui_image.h"
#include "undo.h"
#include "numeric.h"
#include "parse.h"
#include "track.h"
#include "zone.h"
#include "mdi_pak.h"
#include "defect.h"
#include "plots.h"
#include "file_vasp.h"
#include "gui_vasp_internal.h"
#include "file_uspex.h"
#include "gui_uspex_internal.h"
#include "molsurf.h"

/* This gui_still_valid.c file is a catch-them-all of previous implementation that have remain valid under QT6 */
/* it means helper functions and methods that did not rely on any GUI */

/* definition corner */

struct dialog_pak {
  gint type;
  gpointer model;
  gpointer data;
  GHashTable *children;
  void (*refresh)(gpointer);
  void (*cleanup)(gpointer);
  gpointer *window;
};
struct qt_tree_node {
  struct model_pak *model;
  gpointer graph; /* NULL for model nodes, graph pointer for graph nodes */
  char *name;
  int type;              /* model periodicity for model nodes, graph type for graph nodes */
  const char *icon_name; /* XPM icon name for tree display (NULL for graphs) */
};

/* GLOBAL corner */

extern struct sysenv_pak sysenv;
extern struct elem_pak elements[];

static gpointer g_content_table = NULL;
static struct qt_tree_node qt_tree_nodes[4096];
static int qt_tree_count = 0;

gdouble render_animate_angle[3] = {0.0, 360.0, 5.0};
gdouble render_animate_frames = 100;
gint render_animate_overwrite = TRUE;
#define SCALE_MAG 10
#define PIX2ANG 0.03
/* this is an OpenGL limitation (ie shininess) */
#define MAX_HL 128
#define CEDIT (sysenv.cedit)

#define FWHM_GAUSSIAN 0.4246609
/* MEH = m*e*e/2*h*h */
#define MEH 0.026629795
enum { DIFF_XRAY, DIFF_NEUTRON, DIFF_ELECTRON, DIFF_GAUSSIAN, DIFF_LORENTZIAN, DIFF_PSEUDO_VOIGT };

struct mdi_pak mdi_data;

struct defect_pak defect;

struct vasp_calc_gui vasp_gui;
struct uspex_calc_gui uspex_gui;

gdouble ms_prad = 1.0;
gdouble ms_blur = 0.3;
gdouble ms_eden = 0.1;
gint ms_method = MS_MOLECULAR, ms_colour = MS_TOUCH;

/* task manager globals */
gdouble max_threads = -1;
gint task_show_running = 1;
gint task_show_queued = 0;
gint task_show_completed = 1;

gint selected_task = -1;
gint task_info_pid = 0;
gint task_thread_pid = -1;
GArray *psarray = NULL;
char *strptime(const char *, const char *, struct tm *);
#define EXIST(idx) ((idx) != -1)

struct model_pak surfdata;
gdouble sculpt_length = 20.0;

/* STUB corner */

/* Stub functions — no-op.
 * TODO: remove when event bridge is cleaned up to not call them. */
void gui_relation_update(gpointer data) {}
void gui_active_refresh(void) {}
void dialog_destroy_model(struct model_pak *model) {}
void dialog_destroy(gpointer data) {}
void dialog_destroy_type(gint type) {}
void dialog_refresh_all(void) {}
void canvas_select(gint x, gint y)
{
  gint ry;
  GSList *list;
  struct canvas_pak *canvas;

  /* height invert correction */
  ry = sysenv.height - y - 1;

  for (list = sysenv.canvas_list; list; list = g_slist_next(list))
  {
    canvas = list->data;
    if (x >= canvas->x && x < canvas->x + canvas->width)
    {
      if (ry >= canvas->y && ry < canvas->y + canvas->height)
      {
        if (canvas->model)
        {
          if (canvas->model != sysenv.active_model)
            tree_select_model(canvas->model);
        }
      }
    }
  }
}

/* Stub: canvas deletion handled by Qt tree model */
void canvas_delete(void) { /* No-op in Qt mode — tree model handles canvas lifecycle */ }

/* Stub: canvas creation handled by Qt GUI layer */
void canvas_create(void) {}

/* Stub: replaces removed set_canvas_dimensions */
void set_canvas_dimensions(int w, int h)
{
  sysenv.width = w;
  sysenv.height = h;
  sysenv.size = (w > h) ? h : w;
}

/* Stub: canvas creation is handled by Qt GUI layer. */
void canvas_new(gint x, gint y, gint w, gint h)
{
  struct canvas_pak *canvas;
  canvas = g_malloc(sizeof(struct canvas_pak));
  canvas->x = x;
  canvas->y = y;
  canvas->width = w;
  canvas->height = h;
  if (w > h)
    canvas->size = h;
  else
    canvas->size = w;
  canvas->active = FALSE;
  canvas->resize = TRUE;
  canvas->model = sysenv.active_model;
  sysenv.canvas_list = g_slist_prepend(sysenv.canvas_list, canvas);
}

#define DEBUG_CANVAS_RESIZE 0
void canvas_resize(void)
{
  gint i, j, n, rows, cols, width, height;
  GSList *list;
  struct canvas_pak *canvas;

  n = g_slist_length(sysenv.canvas_list);

  rows = cols = 1;
  switch (n)
  {
  case 2:
    rows = 1;
    cols = 2;
    break;
  case 3:
  case 4:
    rows = 2;
    cols = 2;
    break;
  }

  width = sysenv.width / cols;
  height = sysenv.height / rows;

#if DEBUG_CANVAS_RESIZE
  printf("Splitting (%d, %d) : %d x %d\n", rows, cols, width, height);
#endif

  list = sysenv.canvas_list;
  for (i = rows; i--;)
  {
    for (j = 0; j < cols; j++)
    {
      if (list)
      {
        canvas = list->data;
        canvas->x = j * width;
        canvas->y = i * height;
        canvas->width = width;
        canvas->height = height;
        canvas->resize = TRUE;
#if DEBUG_CANVAS_RESIZE
        printf(" - canvas (%d, %d) : [%d, %d]\n", i, j, canvas->x, canvas->y);
#endif
        list = g_slist_next(list);
      }
    }
  }
  canvas_shuffle();
}

/* Stub: canvas layout handled by Qt */
void canvas_single(void) {}
void canvas_double(void) {}
void canvas_quad(void) {}

/* Qt bridge: create a canvas entry in sysenv.canvas_list */
void canvas_create_entry(void)
{
  if (sysenv.canvas_list)
    return; /* already created */
  canvas_new(0, 0, sysenv.width, sysenv.height);
}

/* Bridge: check if canvas list exists */
gint qt_has_canvas_list(void) { return sysenv.canvas_list ? 1 : 0; }

gboolean sysenv_ignore_tree_select(void) { return sysenv.ignore_tree_select; }

void gui_text_show(gint type, const gchar *message)
{

  /* checks */
  if (!message)
    return;

  extern void qt_append_output(const gchar *text);
  extern void qt_append_output_type(const gchar *text, gint type);
  qt_append_output_type(message, type);
  return;
}

/* Used internally by calc_valid_shifts */
gint zsort(gdouble *z1, gdouble *z2)
{
  if (*z1 > *z2)
    return (-1);
  if (*z1 < *z2)
    return (1);
  return (0);
}

/* Used internally by calc_valid_shifts */
gint simplify(gint *vec, gint size) { return (vec[0]); }

GSList *trim_zlist(GSList *zlist)
{
  gdouble dz, *z1, *z2;
  GSList *list, *rlist;

  /* get 1st item */
  list = zlist;
  if (list)
  {
    z1 = (gdouble *) list->data;
    list = g_slist_next(list);
  } else
    return (NULL);

  /* compare and eliminate repeats */
  rlist = zlist;
  while (list)
  {
    z2 = (gdouble *) list->data;
    list = g_slist_next(list);

    dz = fabs(*z1 - *z2);
    if (dz < FRACTION_TOLERANCE)
    {
      rlist = g_slist_remove(rlist, z2);
      g_free(z2);
    } else
      z1 = z2;
  }
  return (rlist);
}
#define DEBUG_CALC_SHIFTS 0
gint calc_valid_shifts(struct model_pak *data, struct plane_pak *pdata)
{
  gint build, num_cuts, dummy[3];
#if DEBUG_CALC_SHIFTS
  gint num_shifts;
#endif
  gdouble gcd, zlim, *z1, *z2, vec[3];
  GSList *slist, *list, *zlist;
  struct model_pak *surf;
  struct shift_pak *sdata;
  struct core_pak *core;
  struct mol_pak *mol;

  /* allocate & template */
  surf = g_malloc(sizeof(struct model_pak));
  model_init(surf);

  surf->mode = FREE;
  surf->id = GULP;
  surf->periodic = 2;

  /* actual surface we want constructed */
  ARR3SET(surf->surface.miller, pdata->index);
  surf->surface.shift = 0.0;
  surf->surface.region[0] = 1.0;
  surf->surface.region[1] = 0.0;

  gcd = GCD(pdata->index[0], GCD(pdata->index[1], pdata->index[2]));

#if DEBUG_CALC_SHIFTS
  printf("*** calc shifts ***\n");
  printf("miller: %f %f %f\n", surf->surface.miller[0], surf->surface.miller[1], surf->surface.miller[2]);
  printf("region: %d %d\n", (gint) surf->surface.region[0], (gint) surf->surface.region[1]);
  printf("   gcd: %f\n", gcd);
  printf(" Shift: %f\n", surf->surface.shift);
#endif

  generate_surface(data, surf);
  if (surf->surface.dspacing == 0.0)
  {
    printf("calc_valid_shifts() error, invalid dspacing.\n");
    goto calc_valid_shifts_cleanup;
  }

#if DEBUG_CALC_SHIFTS
  printf("  Dhkl: %f\n", surf->surface.dspacing);
#endif

  /* transfer appropriate setup data to new model */
  gulp_data_copy(data, surf);
  surf->gulp.run = E_SINGLE;
  surf->gulp.method = CONV;

  /* NEW */
  pdata->area = surf->area;

  /* only sample one dhkl's worth of cuts */
  zlim = G_MINDOUBLE - 1.0 / gcd;

  /* get all possible shifts from z coordinates */
  zlist = NULL;
  /*
  z1 = g_malloc(sizeof(gdouble));
  *z1 = 0.0;
  zlist = g_slist_prepend(zlist, z1);
  */
  if (data->surface.ignore_bonding)
  {
#if DEBUG_CALC_SHIFTS
    printf("Ignoring bonding...\n");
#endif
    /* core z coord */
    for (list = surf->cores; list; list = g_slist_next(list))
    {
      core = list->data;
      ARR3SET(vec, core->x);

      /* helps get around some precision problems */
      vec[2] = decimal_round(vec[2], 6);

      /* grab one slice [0, -1.0) -> [0, -Dhkl) */
      if (vec[2] < zlim)
        continue;
      if (vec[2] > 0.0)
        continue;

      /* NB: cartesian z direction is opposite to shift value sign */
      z1 = g_malloc(sizeof(gdouble));
      *z1 = -vec[2];
      dummy[0] = 0;
      dummy[1] = 0;
      dummy[2] = 0; // see the fractional_clamp _BUG_
      fractional_clamp(z1, dummy, 1);
      zlist = g_slist_prepend(zlist, z1);
    }
  } else
  {
#if DEBUG_CALC_SHIFTS
    printf("Using molecule centroids... [%d]\n", g_slist_length(surf->moles));
#endif
    /* molecule centroid */
    for (list = surf->moles; list; list = g_slist_next(list))
    {
      mol = list->data;
      ARR3SET(vec, mol->centroid);

      /* helps get around some precision problems */
      vec[2] = decimal_round(vec[2], 6);

      /* grab one slice [0, -1.0) -> [0, -Dhkl) */
      if (vec[2] < zlim)
        continue;
      if (vec[2] > 0.0)
        continue;

      /* NB: cartesian z direction is opposite to shift value sign */
      z1 = g_malloc(sizeof(gdouble));
      *z1 = -vec[2];
      dummy[0] = 0;
      dummy[1] = 0;
      dummy[2] = 0;
      fractional_clamp(z1, dummy, 1); // see the fractional_clamp _BUG_
      zlist = g_slist_prepend(zlist, z1);
    }
  }
  zlist = g_slist_sort(zlist, (gpointer) zsort);
  num_cuts = g_slist_length(zlist);

  /* NEW - some pathalogical cases have shift values just above 0.0 */
  /* resulting in no cuts - so enforce at least one */
  if (!num_cuts)
  {
    z1 = g_malloc(sizeof(gdouble));
    *z1 = 0.0;
    zlist = g_slist_prepend(zlist, z1);
#if DEBUG_CALC_SHIFTS
    num_cuts = 1; /*FIX b1b848*/
#endif
  }

#if DEBUG_CALC_SHIFTS
  printf("Found %d shifts.\n", num_cuts);
  for (list = zlist; list; list = g_slist_next(list))
  {
    z1 = (gdouble *) list->data;
    printf(" - %.20f\n", *z1);
  }
  printf("\n");
#endif

  zlist = trim_zlist(zlist);
#if DEBUG_CALC_SHIFTS
  num_cuts = g_slist_length(zlist); /*FIX 95d70c*/

  printf("Found %d unique shifts.\n", num_cuts);
  for (list = zlist; list; list = g_slist_next(list))
  {
    z1 = (gdouble *) list->data;
    printf("%.4f ", *z1);
  }
  printf("\n");
#endif

  /* failsafe */
  if (!zlist)
    goto calc_valid_shifts_cleanup;

  /* use midpoints to avoid cutoff problems (also neater shift values) */
  /* also, the unchanged first cut will be forced to 0.0 */
  zlist = g_slist_reverse(zlist);
  z1 = zlist->data;
  list = g_slist_next(zlist);
  while (list)
  {
    z2 = list->data;
    *z1 = 0.5 * (*z1 + *z2);
    z1 = z2;
    list = g_slist_next(list);
  }
  *z1 = 0.0;

#if DEBUG_CALC_SHIFTS
  printf("Midpoint equivalent cuts:\n");
  for (list = zlist; list; list = g_slist_next(list))
  {
    z1 = (gdouble *) list->data;
    printf(" %.20f", *z1);
  }
  printf("\n");
#endif

  /* re-rank */
  zlist = g_slist_sort(zlist, (gpointer) zsort);

#if DEBUG_CALC_SHIFTS
  printf("Ranked shifts: %d\n", num_cuts);
  for (list = zlist; list; list = g_slist_next(list))
  {
    z1 = (gdouble *) list->data;
    printf(" %f", *z1);
  }
  printf("\n");
#endif

  zlist = trim_zlist(zlist);

#if DEBUG_CALC_SHIFTS
  num_shifts = g_slist_length(zlist);
  printf("Unique shifts: %d\n", num_shifts);
  for (list = zlist; list; list = g_slist_next(list))
  {
    z1 = (gdouble *) list->data;
    printf(" %f", *z1);
  }
  printf("\n");
#endif

  /* turn bonding off */
  build = data->build_molecules;
  if (data->surface.ignore_bonding && build)
  {
    data->build_molecules = FALSE;
    connect_bonds(data);
    connect_molecules(data);
  }

  /* generate & check each one */
  /* NB: this'll overwrite our 1st model, saving having to delete it */
  slist = NULL;
  for (list = zlist; list; list = g_slist_next(list))
  {
    z1 = (gdouble *) list->data;

    /* destroy surfs core/shell lists */
    free_core_list(surf);

    /* make the surface (also computes dipole) */
    surf->surface.shift = *z1;
    generate_surface(data, surf);

#if DEBUG_CALC_SHIFTS
    printf("Shift = %f, dipole = %f, bonds broken = %d / %f\n", *z1, surf->gulp.sdipole, surf->surface.bonds_cut,
           surf->area);
#endif

    /* create new model if valid cut (unless user wants all cuts) */
    if (fabs(surf->gulp.sdipole) < data->surface.dipole_tolerance || data->surface.include_polar)
    {
      sdata = shift_new(*z1);
      sdata->dipole = surf->gulp.sdipole;
      sdata->dipole_computed = TRUE;

      /* NEW - broken bonds per area calc */
      sdata->bbpa = (gdouble) surf->surface.bonds_cut;
      sdata->bbpa /= surf->area;

      slist = g_slist_prepend(slist, sdata);
    }
  }

  /* new if shift list is non-empty, overwrite only if we found something */
  if (slist)
  {
    slist = g_slist_reverse(slist);
    if (pdata->shifts)
    {
      shift_data_free(pdata->shifts);
      g_slist_free(pdata->shifts);
    }
    pdata->shifts = slist;
  }

  /* restore source model's connectivity */
  if (data->surface.ignore_bonding && build)
  {
    data->build_molecules = build;
    connect_bonds(data);
    connect_molecules(data);
  }

  /* this has data only if the above was successful */
  free_slist(zlist);

calc_valid_shifts_cleanup:

  /* cleanup */
  model_free(surf);
  g_free(surf);

  return (0);
}
/* Bridge: set spatial selected from Qt */
/* TODO: implement in QT */
/* Qt bridge: spatial colour apply */
#define DEBUG_SCROLL 0
gint gui_scroll_event(void)
{
  /* change zoom -- based on "zoom section" of gui_press_event() */
  const gdouble scroll_factor = 10.0;
  gdouble scroll, v[3];
  struct model_pak *data;
  struct camera_pak *camera;

  /* get model */
  data = sysenv.active_model;
  if (!data)
    return (FALSE);

  camera = data->camera;

  sysenv.moving = FALSE;

  /* Qt mode: read scroll delta from sysenv */
  gint direction = 0; /* 0 = none */
  direction = (sysenv.scroll_delta > 0) ? 1 : 0;

  switch (direction)
  {
  case 1:
    scroll = -scroll_factor;
    break;
  case 0:
    scroll = scroll_factor;
    break;
  default: /* left and right are not used yet */
    return FALSE;
  }
  scroll *= PIX2SCALE;

#if DEBUG_SCROLL
  printf("Scroll %f\n", scroll);
#endif
  if (camera->perspective)
  {
    ARR3SET(v, camera->v);
    VEC3MUL(v, -scroll * 10.0);
    ARR3ADD(camera->x, v);
  } else
    camera->zoom += scroll;

  data->zoom = data->rmax;

  sysenv.moving = TRUE;
  redraw_canvas(SINGLE);
  return FALSE;
}
void gui_key_release_event(gpointer w, gpointer event) { /* No-op for now */ }
void set_key_event_data(int key, int state)
{
  sysenv.key_code = key;
  sysenv.key_state = state;
}
void set_scroll_event_data(int delta) { sysenv.scroll_delta = delta; }
void set_mouse_event_data(int x, int y, int button, int state)
{
  /* Store in sysenv for handlers to read */
  sysenv.mouse_x = x;
  sysenv.mouse_y = y;
  sysenv.mouse_button = button;
  sysenv.mouse_state = state;
}
#define DEBUG_BUTTON_PRESS_EVENT 0

/* Add an atom at the clicked screen position. */
static void add_atom(gint x, gint y, struct model_pak *data)
{
  gchar *elem;
  gdouble r[3];
  struct core_pak *core;
  struct canvas_pak *canvas;

  g_assert(data != NULL);

  /* Get the atom element from the Qt edit text bridge (Element field). */
  extern const gchar *qt_edit_text[];
  const gchar *elem_str = qt_edit_text[ELEMENT];
  elem = (elem_str && *elem_str) ? g_strdup(elem_str) : g_strdup("C");

  /* Create the new core. */
  core = new_core(elem, data);

  /* Project screen coords to 3D space using z=0.5 (midplane). */
  canvas = canvas_find(data);
  if (canvas)
    gl_project(r, x, y, canvas);
  else
    VEC3SET(r, 0.0, 0.0, 0.0);

  /* Set atom position in the plane running through the origin. */
  ARR3SET(core->rx, r);
  ARR3SET(core->x, r);
  vecmat(data->ilatmat, core->x);
  ARR3ADD(core->x, data->centroid);

  /* Add to model's core list. */
  data->cores = g_slist_append(data->cores, core);

  /* Select the new atom so it stays in the edit panel for further additions. */
  select_clear(data);
  select_add_core(core, data);

  /* Update derived structures. */
  zone_init(data);
  connect_bonds(data);
  connect_molecules(data);

  g_slist_free(data->unique_atom_list);
  data->unique_atom_list = find_unique(ELEMENT, data);
  init_atom_colour(core, data);
  init_atom_charge(core, data);
  init_atom_mass(core, data);
  calc_emp(data);

  g_free(elem);
}

/*****************/
/* info on bonds */
/*****************/
static void info_bond(gint x, gint y, struct model_pak *model)
{
  gpointer measure;
  struct core_pak *core[2];
  struct bond_pak *bond;

  /* seek bond at click position */
  bond = gl_seek_bond(x, y, model);
  if (!bond)
    return;

  /* ignore hydrogen bonds to stop dodgy bond measurements being drawn */
  if (bond->type == BOND_HBOND)
    return;

  core[0] = bond->atom1;
  core[1] = bond->atom2;

  measure = measure_bond_test(core, 0.0, 0.0, model);
  if (measure)
  {
    measure_update_single(measure, model);
    /* Refresh the measurements dialog tree view */
    extern void qt_measure_grafted_callback(void);
    qt_measure_grafted_callback();
  }
}

/*****************/
/* info on dists */
/*****************/
static void info_dist(gint x, gint y, struct model_pak *model)
{
  gpointer measure;
  static struct core_pak *core[2];

  /* attempt to find core */
  if (!(core[model->state] = gl_seek_core(x, y, model)))
    return;

  /* are we looking at the 1st or the 2nd call? */
  switch (model->state)
  {
  case 0:
    /* select first atom */
    select_add_core(core[0], model);
    model->state++;
    break;

  case 1:
    /* remove from selection */
    select_del_core(core[0], model);
    select_del_core(core[1], model);

    /* create the measurement (if it doesn't exist) */
    measure = measure_distance_test(MEASURE_DISTANCE, core, 0.0, 0.0, model);
    if (measure)
    {
      measure_update_single(measure, model);
      extern void qt_measure_grafted_callback(void);
      qt_measure_grafted_callback();
    }

    model->state--;
    break;

  default:
    model->state = 0;
    return;
  }
}

/******************/
/* info on angles */
/******************/
static void info_angle(gint x, gint y, struct model_pak *model)
{
  gpointer measure;
  static struct core_pak *core[3];

  /* attempt to find core */
  if (!(core[model->state] = gl_seek_core(x, y, model)))
    return;

  /* are we looking at the 1st or the 2nd call? */
  switch (model->state)
  {
  case 0:
  case 1:
    select_add_core(core[model->state], model);
    model->state++;
    break;

  case 2:
    /* remove highlighting */
    select_del_core(core[0], model);
    select_del_core(core[1], model);
    select_del_core(core[2], model);

    /* create the measurement (if it doesn't exist) */
    measure = measure_angle_test(core, 0.0, 180.0, model);
    if (measure)
    {
      measure_update_single(measure, model);
      extern void qt_measure_grafted_callback(void);
      qt_measure_grafted_callback();
    }

    model->state = 0;
    break;

  default:
    model->state = 0;
    return;
  }
}

/****************************/
/* info on torsional angles */
/****************************/
static void info_torsion(gint x, gint y, struct model_pak *model)
{
  gpointer measure;
  static struct core_pak *core[4];

  /* attempt to find core */
  if (!(core[model->state] = gl_seek_core(x, y, model)))
    return;

  /* what stage are we at? */
  switch (model->state)
  {
  case 0:
  case 1:
  case 2:
    select_add_core(core[model->state], model);
    model->state++;
    break;

  case 3:
    /* remove highlighting */
    select_del_core(core[0], model);
    select_del_core(core[1], model);
    select_del_core(core[2], model);
    select_del_core(core[3], model);

    /* create the measurement (if it doesn't exist) */
    measure = measure_torsion_test(core, 0.0, 180.0, model);
    if (measure)
    {
      measure_update_single(measure, model);
      extern void qt_measure_grafted_callback(void);
      qt_measure_grafted_callback();
    }

    model->state = 0;
    break;

  default:
    model->state = 0;
  }
}

gint gui_press_event(void)
{
  gint refresh = 0, x, y;
  gint shift = FALSE;
  gint state;
  struct model_pak *data;
  struct bond_pak *bond;

  /* Qt mode: read event data from sysenv fields */
  gint button = 0;
  x = sysenv.mouse_x;
  y = sysenv.mouse_y;
  state = sysenv.mouse_state;
  button = sysenv.mouse_button;

  struct core_pak *core;

  /* HACK TODO: remove... */
  if (sysenv.stereo)
    return (FALSE);

  /* get model */
  data = sysenv.active_model;
  if (!data)
    return (FALSE);

  sysenv.moving = FALSE;

  canvas_select(x, y);

  /* analyse the current state */
  if ((state & 1))
    shift = TRUE;

  /* only want button 1 (for now) */
  if (button != 1)
    return (FALSE);

  /* NEW - diffraction peak search */
  if (data->graph_active)
  {
    struct graph_pak *graph = (struct graph_pak *) data->graph_active;
    switch (graph->type)
    {
    case GRAPH_IY_TYPE:
    case GRAPH_XY_TYPE:
    case GRAPH_YX_TYPE:
    case GRAPH_IX_TYPE:
    case GRAPH_XX_TYPE:
      dat_graph_select(x, y, data);
      break;
    case GRAPH_REGULAR:
    default:
      break;
    }
    return (FALSE);
  }

  /* can we assoc. with a single atom? */
  core = gl_seek_core(x, y, data);

  /* allow shift+click to add/remove single atoms in the selection */
  /* TODO - depending on mode add/remove objects in selection eg atom/mols */
  if (shift)
  {
    switch (data->mode)
    {
    default:
      if (core)
      {
        select_core(core, TRUE, data);
        refresh++;
      } else
      {
        /* otherwise start a box selection */
        update_box(x, y, data, START);
      }
      break;
    }
  } else
  {
    /* determine the type of action required */
    switch (data->mode)
    {
    case BOND_INFO:
      info_bond(x, y, data);
      refresh++;
      break;

    case DIST_INFO:
      info_dist(x, y, data);
      refresh++;
      break;

    case ANGLE_INFO:
      info_angle(x, y, data);
      refresh++;
      break;

    case DIHEDRAL_INFO:
      info_torsion(x, y, data);
      refresh++;
      break;

    case BOND_DELETE:
      /* or by bond midpoints */
      bond = gl_seek_bond(x, y, data);
      if (bond)
      {
        connect_user_bond(bond->atom1, bond->atom2, BOND_DELETE, data);
        refresh++;
      }
      break;

    case BOND_SINGLE:
    case BOND_DOUBLE:
    case BOND_TRIPLE:
      if (core)
      {
        connect_make_bond(core, data->mode, data);
        refresh++;
      }
      break;

    case ATOM_ADD:
      /* Add atom at clicked position (only left button). */
      if (button == 1)
        add_atom(x, y, data);
      refresh++;
      break;

    case DEFINE_RIBBON:
      if (core)
        construct_backbone(core, data);
      break;

    case DEFINE_VECTOR:
    case DEFINE_PLANE:
      if (core)
        spatial_point_add(core, data);
      break;

      /* selection stuff */
    default:
      select_clear(data);
      /* don't select if a core is under the mouse */
      /* instead we allow the selection box to be drawn */
      update_box(x, y, data, START);
      refresh++;
      break;
    }
  }

  /* CURRENT */
  if (refresh)
    gui_active_refresh();

  return (FALSE);
}
#define DEBUG_MOTION 0
gint gui_motion_event(void)
{
  gint x, y, dx, dy, fx, fy, refresh;
  gint shift = FALSE, ctrl = FALSE;
  gdouble da, dv, zoom, v[3], mat[9];
  gint state;
  static gint ox = 0, oy = 0;
  struct model_pak *data;
  struct camera_pak *camera;

  /* get model */
  data = sysenv.active_model;
  if (!data)
    return (FALSE);

  camera = data->camera;

  /* Qt mode: read event data from sysenv fields */
  x = sysenv.mouse_x;
  y = sysenv.mouse_y;
  state = sysenv.mouse_state;

  /* discard discontinuous jumps (ie ox,oy are invalid on 1st call) */
  if (!sysenv.moving)
  {
    ox = x;
    oy = y;
    sysenv.moving = TRUE;
    /* First frame after button press — force redraw so camera state is visible. */
    data->redraw = TRUE;
    return TRUE;
  }

  /* convert relative mouse motion to an increment */
  dx = x - ox;
  dy = oy - y; /* inverted y */

  /* single update */
  refresh = 0;

  /* analyse the current state */
  if ((state & 1))
    shift = TRUE;
  if ((state & 4))
    ctrl = TRUE;

  /* first mouse button - mostly selection stuff */
  if (state & 256)
  {
    switch (data->mode)
    {
    case FREE:
      update_box(x, y, data, UPDATE);
      refresh++;
      break;

    default:
      break;
    }
    /* don't switch to low quality drawing */
    sysenv.moving = FALSE;
    if (refresh)
      redraw_canvas(SINGLE);
    ox = x;
    oy = y;
    return (TRUE);
  }

  /* NEW - disallow rotations (redraws) when graph is displayed */
  if (data->graph_active)
    return (TRUE);

  /* second mouse button */
  if (state & 512)
  {
    if (shift)
    {
      /* zoom */
      zoom = PIX2SCALE * (dx + dy);
      if (camera->perspective)
      {
        ARR3SET(v, camera->v);
        VEC3MUL(v, zoom * 10.0);
        ARR3ADD(camera->x, v);
      } else
      {
        camera->zoom -= zoom;
        data->zoom = data->rmax;
      }
      sysenv.moving = TRUE;
      refresh++;
    } else
    {
      if (ctrl)
      {
        /* selection only translation */
        select_translate(x - ox, y - oy, data);
        sysenv.moving = TRUE;
        refresh++;
      } else
      {
        /* horizontal camera translation */
        dv = ox - x;
        ARR3SET(v, camera->e);
        VEC3MUL(v, dv * PIX2ANG);
        ARR3ADD(camera->x, v);
        /* vertical  camera translation */
        dv = y - oy;
        ARR3SET(v, camera->o);
        VEC3MUL(v, dv * PIX2ANG);
        ARR3ADD(camera->x, v);

        sysenv.moving = TRUE;
        refresh++;
      }
    }
  }

  /* third mouse button clicked? */
  if (state & 1024)
  {
    /* shift clicked? */
    if (shift)
    {
      /* yaw */
      if (dx || dy)
      {
        /* rotation amount */
        da = abs(dx) + abs(dy);
        /* vector from center to mouse pointer (different for OpenGL window) */
        /* FIXME - if ctrl is pressed, the center should be the selection */
        /* centroid & not the middle of the window */
        fx = x - data->offset[0] - (sysenv.x + sysenv.width / 2);
        fy = data->offset[1] + (sysenv.y + sysenv.height / 2 - y);

        /* rotation direction via z component of cross product (+=clock, -=anti) */
        if ((fx * dy - dx * fy) < 0)
          da *= -1.0;

#if DEBUG_MOTION
        printf("(%d,%d) x (%d,%d) : %f\n", dx, dy, fx, fy, da);
#endif

        /* assign and calculate */
        da *= D2R * ROTATE_SCALE;
        if (ctrl)
        {
          matrix_relative_rotation(mat, da, ROLL, data);
          rotate_select(data, mat);
        } else
        {
          if (camera->mode == LOCKED)
            quat_concat_euler(camera->q, ROLL, da);
          else
          {
            matrix_v_rotation(mat, camera->v, -da);
            vecmat(mat, camera->o);
            vecmat(mat, camera->e);
          }
        }
      }
      sysenv.moving = TRUE;
      refresh++;
    } else
    {
#if DEBUG_MOTION
      printf("(%d,%d)\n", dx, dy);
#endif
      /* pitch and roll */
      if (dy)
      {
        da = D2R * ROTATE_SCALE * dy;
        if (ctrl)
        {
          matrix_relative_rotation(mat, -da, PITCH, data);
          rotate_select(data, mat);
        } else
        {
          if (camera->mode == LOCKED)
            quat_concat_euler(camera->q, PITCH, da);
          else
          {
            matrix_v_rotation(mat, camera->e, -da);
            vecmat(mat, camera->v);
            vecmat(mat, camera->o);
          }
        }
        sysenv.moving = TRUE;
        refresh++;
      }
      if (dx)
      {
        da = D2R * ROTATE_SCALE * dx;
        if (ctrl)
        {
          matrix_relative_rotation(mat, -da, YAW, data);
          rotate_select(data, mat);
        } else
        {
          if (camera->mode == LOCKED)
            quat_concat_euler(camera->q, YAW, -da);
          else
          {
            matrix_v_rotation(mat, camera->o, da);
            vecmat(mat, camera->v);
            vecmat(mat, camera->e);
          }
        }
        sysenv.moving = TRUE;
        refresh++;
      }
    }
  }

  /* save old values */
  ox = x;
  oy = y;

  /* Force redraw whenever motion was processed — ensures camera changes are visible.
   * During drag, depth buffer overwrites old pixels so no clear needed (need_clear
   * is only set when actual geometry changes: atoms/bonds added/removed). */
  data->redraw = TRUE;
  if (refresh)
    redraw_canvas(SINGLE);

  return (TRUE);
}
gint gui_release_event(void)
{
  gint x, y;
  struct model_pak *data;

  /* get model */
  data = sysenv.active_model;
  if (!data)
    return (FALSE);

  /* Qt mode: read event data from sysenv fields */
  gint button = 0;
  x = sysenv.mouse_x;
  y = sysenv.mouse_y;
  button = sysenv.mouse_button;

  /* NEW - motion flag */
  sysenv.moving = FALSE;
  sysenv.just_released = TRUE; /* next paintGL will clear stale framebuffer content */

  /* first mouse button */
  switch (button)
  {
  case 1:

    /* HACK */
    if (sysenv.stereo)
      return (FALSE);

    /* clean up after move operations */
    switch (data->mode)
    {
    default:
      /* perform box selection if a drag occurred */
      extern void gl_select_box(void);
      if (data->box_on)
        gl_select_box();
      data->box_on = FALSE;
      break;
    }

    /* Refresh selection display — editing panel updated on tree selection change only. */
    gui_refresh(GUI_CANVAS);
    break;

  case 2:
    /* hack to only update zoom factor when button released */
    /* ie continuous update causes significant slowdown */
    break;
  }

  redraw_canvas(SINGLE);
  return (FALSE);
}

/* FIXME: this one will be always true! */
void set_qt_mode(int val) { sysenv.qt_mode = val; }
void dialog_destroy_single(gint type, struct model_pak *model)
{
  GSList *list;
  struct dialog_pak *dialog;

  list = sysenv.dialog_list;
  while (list)
  {
    dialog = list->data;
    list = g_slist_next(list);
  }
}
gint dialog_exists(gint type, struct model_pak *model)
{
  GSList *list;
  struct dialog_pak *dialog;

  for (list = sysenv.dialog_list; list; list = g_slist_next(list))
  {
    dialog = list->data;

    if (model)
    {
      if (dialog->model == model && dialog->type == type)
        return (TRUE);
    } else
    {
      if (dialog->type == type)
        return (TRUE);
    }
  }
  return (FALSE);
}
/* Qt bridge: model frame accessors */
/* Qt bridge: stereo window open/close
 * Windowed mode creates a second canvas entry for the right eye.
 * Fullscreen mode uses dual-head with frustum-based separation.
 * Anaglyph rendering works without extra windows (handled in stereo_redraw). */
extern void stereo_open_window(void);
extern void stereo_init_window(struct canvas_pak *canvas);
extern void qt_stereo_close_window(void);
void qt_stereo_open_window(void)
{
  struct canvas_pak *main_canvas = NULL;

  if (!sysenv.canvas_list)
    return;

  /* Get the main (left eye) canvas */
  main_canvas = (struct canvas_pak *) sysenv.canvas_list->data;
  if (!main_canvas)
    return;

  /* Initialize stereo window parameters from the main canvas */
  if (!sysenv.stereo_fullscreen)
  {
    /* Set stereo geometry globals used by stereo_redraw() */
    stereo_init_window(main_canvas);

    /* Windowed mode: create a second canvas for the right eye, positioned next to the left */
    struct canvas_pak *right_canvas = g_malloc(sizeof(struct canvas_pak));
    right_canvas->x = main_canvas->x + main_canvas->width;
    right_canvas->y = main_canvas->y;
    right_canvas->width = main_canvas->width;
    right_canvas->height = main_canvas->height;
    right_canvas->size = main_canvas->size;
    right_canvas->active = TRUE;
    right_canvas->resize = TRUE;
    /* Link to the same model */
    right_canvas->model = main_canvas->model;

    /* Add to canvas list after the main canvas */
    sysenv.canvas_list = g_slist_append(sysenv.canvas_list, right_canvas);
  } else
  {
    /* Fullscreen mode: use stereo_open_window for dual-head setup */
    stereo_open_window();
  }
}
void qt_stereo_close_window(void)
{
  if (!sysenv.canvas_list || g_slist_length(sysenv.canvas_list) < 2)
    return;

  /* Remove the right-eye canvas (last entry in list) */
  GSList *last = g_slist_last(sysenv.canvas_list);
  if (last)
  {
    struct canvas_pak *rc = (struct canvas_pak *) last->data;
    sysenv.canvas_list = g_slist_remove_link(sysenv.canvas_list, last);
    g_free(rc);
    g_slist_free(last);
  }
}
/* End of Qt bridge: stereo window open/close */
void qt_init_camera(struct model_pak *model)
{
  /* Always ensure camera is initialized - check both camera and camera_default */
  if (!model->camera || !model->camera_default)
  {
    printf("DEBUG: qt_init_camera called for model %p, camera=%p, camera_default=%p\n", model, model->camera,
           model->camera_default);
    camera_init(model);
    printf("DEBUG: after camera_init, camera=%p, camera_default=%p\n", model->camera, model->camera_default);
  }
}
gint read_transform(gint n, struct model_pak *model)
{
  gpointer camera;

  g_assert(model != NULL);
  g_assert(model->camera != NULL);

  camera = g_slist_nth_data(model->transform_list, n);

  g_assert(camera != NULL);

  model->camera = camera;
  /* Invalidate GL cache — new camera */
  model->gl_cache.dirty = TRUE;

  return (0);
}
#define DEBUG_READ_FRAME 0
gint read_frame(FILE *fp, gint n, struct model_pak *model)
{
  gint status;
  gdouble rmax, v1[3], v2[3];
  gpointer camera = NULL;
  GString *err_text;

  g_assert(model != NULL);

  rmax = model->rmax;

  /* conventional or transformation style animation */
  if (model->transform_list)
  {
    /* NEW - process transformation list as an animation */
    /*status =*/read_transform(n, model); /*FIX 8f7718*/
  } else
  {
    g_assert(fp != NULL);

    ARR3SET(v1, model->centroid);
    ARR3SET(v2, model->offset);

    status = read_raw_frame(fp, n, model);
    if (status)
    {
      err_text = g_string_new("");
      g_string_printf(err_text, "Error reading frame: %d (status=%d)\n", n, status);
      gui_text_show(ERROR, err_text->str);
      g_string_free(err_text, TRUE);
      return (1);
    }

    /* setup (have to save camera) */
    camera = camera_dup(model->camera);
    model_prep(model);
    model->camera = camera;
    /* Invalidate GL cache — new camera */
    model->gl_cache.dirty = TRUE;
    g_free(model->camera_default);
    model->camera_default = camera;

    ARR3SET(model->centroid, v1);
    ARR3SET(model->offset, v2);
  }

  /* apply desired constraint */
  if (model->periodic && !model->anim_fix)
  {
    switch (model->anim_confine)
    {
    case PBC_CONFINE_ATOMS:
      coords_confine_cores(model->cores, model);
      break;

    case PBC_CONFINE_MOLS:
      coords_compute(model);
      connect_bonds(model);
      connect_molecules(model);
      break;
    }
  }

  if (model->anim_noscale)
    model->rmax = rmax;

  return (0);
}
void meas_graft_model(struct model_pak *model)
{
  g_assert(model != NULL);

  /* update underlying measurements */
  measure_update_global(model);
}
/* Qt bridge: select frame and refresh */
void qt_select_frame_model(struct model_pak *model)
{
  FILE *fp = NULL;

  if (!model)
    return;

  /* Open file for non-transform animations */
  if (!model->transform_list)
    fp = fopen(model->filename, "r");

  read_frame(fp, model->cur_frame, model);

  if (!model->transform_list && fp)
    fclose(fp);

  meas_graft_model(model);
  model->need_clear = TRUE; /* new animation frame — clear old geometry */
  gui_active_refresh();
  redraw_canvas(SINGLE);
}
void qt_refresh_content(void) { model_content_refresh(sysenv.active_model); }
/* Qt bridge: get render filename */
/* Qt bridge: get convert path */
const char *qt_get_convert_path(void) { return sysenv.convert_path; }
/* Bridge: set edit basis strings */
void qt_set_edit_basis(int idx, const char *text)
{
  if (idx < 0 || idx > 1)
    return;
  if (sysenv.cedit.edit_basis[idx])
    g_free(sysenv.cedit.edit_basis[idx]);
  sysenv.cedit.edit_basis[idx] = g_strdup(text);
}
void qt_set_edit_length(double val) { sysenv.cedit.edit_length = val; }

void qt_set_edit_chirality(int idx, gdouble val)
{
  if (idx < 0 || idx > 1)
    return;
  sysenv.cedit.edit_chirality[idx] = val;
}

#define EDIT_NANOTUBE_NEW 0
void edit_nanotube_new(void)
{
  gint i, j, k, n, m, d, dr, np, mp;
  gint imin, imax, jmin, jmax, dummy[3];
  gdouble a1[3], a2[3], a3[3];
  gdouble a, r, x[3], y[3], ch[3], t[3], v[3], p[2];
  gchar *text;
  struct core_pak *core;
  struct model_pak *model;

  /* checks */
  model = sysenv.active_model;
  if (!model)
  {
    edit_model_create();
    model = sysenv.active_model;
    g_assert(model != NULL);
  }

  /* default to brenner if potential set is absent */
  if (!model->gulp.potentials && !model->gulp.libfile)
    model->gulp.potentials = g_strdup("brenner\n");

  /* Ensure basis strings are valid (safety net for Qt mode) */
  if (!CEDIT.edit_basis[0])
  {
    g_free(CEDIT.edit_basis[0]);
    CEDIT.edit_basis[0] = g_strdup("C");
  }
  if (!CEDIT.edit_basis[1])
  {
    g_free(CEDIT.edit_basis[1]);
    CEDIT.edit_basis[1] = g_strdup("C");
  }

#define ROOT3 sqrt(3)

/* TODO - test basis atom strings for validity */
#if EDIT_NANOTUBE_NEW
  printf("Basis: %s, %s (%f)\n", CEDIT.edit_basis[0], CEDIT.edit_basis[1], CEDIT.edit_length);
#endif

  /* compute lattice basis vectors */
  VEC3SET(a1, 1.5 * CEDIT.edit_length, 0.5 * ROOT3 * CEDIT.edit_length, 0.0);
  VEC3SET(a2, 1.5 * CEDIT.edit_length, -0.5 * ROOT3 * CEDIT.edit_length, 0.0);
  VEC3SET(a3, CEDIT.edit_length, 0.0, 0.0);

  /* compute indices */
  n = (gint) CEDIT.edit_chirality[0];
  m = (gint) CEDIT.edit_chirality[1];

  d = gcd(n, m);
  if ((n - m) % (3 * d))
    dr = d;
  else
    dr = 3 * d;

  np = 2 * m + n;
  np /= dr;
  mp = 2 * n + m;
  mp /= dr;

  /* chirality vector */
  ARR3SET(x, a1);
  ARR3SET(y, a2);
  VEC3MUL(x, n);
  VEC3MUL(y, m);
  ARR3SET(ch, x);
  ARR3ADD(ch, y);

  /* tube radius */
  r = 0.5 * VEC3MAG(ch) / G_PI;

  /* translation vector */
  ARR3SET(x, a1);
  ARR3SET(y, a2);
  VEC3MUL(x, np);
  VEC3MUL(y, mp);
  ARR3SET(t, x);
  ARR3SUB(t, y);

  /* loop limits */
  imin = MIN(MIN(np, 0), n);
  imax = MAX(MAX(n + np, n), np);
  jmin = MIN(MIN(-mp, 0), m);
  jmax = MAX(MAX(m - np, m), -mp);

#if EDIT_NANOTUBE_NEW
  printf("chirality vector index: (%d, %d)\n", n, m);
  printf("chirality xlat index: (%d, %d)\n", np, -mp);
  P3VEC("chiral vector: ", ch);
  P3VEC("xlat vector: ", t);
#endif

  /* loop over graphite lattice */
  for (i = imin; i <= imax; i++)
  {
    for (j = jmin; j <= jmax; j++)
    {
      /* compute lattice vector */
      ARR3SET(x, a1);
      ARR3SET(y, a2);
      VEC3MUL(x, i);
      VEC3MUL(y, j);
      ARR3SET(v, x);
      ARR3ADD(v, y);

      /* compute basis atom coord */
      for (k = 0; k < 2; k++)
      {
        if (k)
        {
          ARR3ADD(v, a3);
        }
        ARR3SET(x, v);
        ARR3MUL(x, ch);
        p[0] = x[0] + x[1] + x[2];
        p[0] /= VEC3MAGSQ(ch);

        ARR3SET(y, v);
        ARR3MUL(y, t);
        p[1] = y[0] + y[1] + y[2];
        p[1] /= VEC3MAGSQ(t);

        /* clamp to one unit's worth of atoms */
        dummy[0] = 0;
        dummy[1] = 0; // see fractional_clamp _BUG_
        fractional_clamp(p, dummy, 2);

        /* choose basis atom type */
        if (k)
          core = core_new(CEDIT.edit_basis[0], NULL, model);
        else
          core = core_new(CEDIT.edit_basis[1], NULL, model);
        model->cores = g_slist_prepend(model->cores, core);

        /* compute 1D fractional coords */
        a = 2.0 * G_PI * p[0];
        /* xlat in x */
        /*
              core->x[0] = -p[1];
        */
        core->x[0] = p[1];
        core->x[1] = r * sin(a);
        core->x[2] = r * cos(a);
      }
    }
  }

  /* setup */
  /* TODO - what if periodicity in z is desired??? */
  model->periodic = 1;
  model->fractional = TRUE;
  model->pbc[0] = VEC3MAG(t);
  model_prep(model);

  /* model info */
  text = g_strdup_printf("(%d, %d)", n, m);
  property_add_ranked(2, "Chirality", text, model);
  g_free(text);
  text = g_strdup_printf("%.4f Angs", r);
  property_add_ranked(3, "Radius", text, model);
  g_free(text);

  gui_refresh(GUI_MODEL_PROPERTIES);
  gui_refresh(GUI_CANVAS);

  /* Rebuild the Qt tree so the model icon reflects its new type. */
  extern void tree_model_add(struct model_pak * model);
  tree_model_add(model);

  gui_relation_update(model);
}

void edit_make_p1(void)
{
  struct model_pak *model;

  model = sysenv.active_model;
  if (!model)
    return;
  if (model->periodic != 3)
    return;

  space_make_p1(model);

  /* REFRESH */
  gui_refresh(GUI_MODEL_PROPERTIES);
  gui_refresh(GUI_CANVAS);
}
void edit_make_supercell(void)
{
  struct model_pak *model;

  model = sysenv.active_model;
  if (model)
  {
    space_make_supercell(model);
    model_prep(model);
  }

  /* REFRESH */
  gui_refresh(GUI_MODEL_PROPERTIES);
  gui_refresh(GUI_CANVAS);
}
void edit_confine(gint mode)
{
  struct model_pak *model;
  model = sysenv.active_model;
  if (!model)
    return;

  switch (mode)
  {
  case CORE:
    coords_confine_cores(model->cores, model);
    coords_compute(model);
    connect_bonds(model);
    break;
  case MOL:
    connect_molecules(model);
    break;
  }
  redraw_canvas(SINGLE);
}
void edit_atom_add(void)
{
  struct model_pak *model;

  model = sysenv.active_model;

  if (model)
  {
    if (model->num_frames > 1)
    {
      gui_text_show(WARNING, "Atoms cannot be added to multiframe model.\n");
      return;
    }
    gui_mode_switch(ATOM_ADD);
  } else
    edit_model_create();
}
void edit_shells_add(void)
{
  GSList *list;
  struct core_pak *core;
  struct shel_pak *shell;
  struct model_pak *model;

  model = sysenv.active_model;
  if (model)
  {
    if (model->num_frames > 1)
    {
      gui_text_show(WARNING, "Shells cannot be added to multiframe model.\n");
      return;
    }

    for (list = model->selection; list; list = g_slist_next(list))
    {
      core = list->data;

      /* create new shell if none present */
      if (!core->shell)
      {
        shell = shell_new(core->atom_label, NULL, model);
        model->shels = g_slist_prepend(model->shels, shell);

        /* start shell at core coords */
        ARR3SET(shell->x, core->x);
        ARR3SET(shell->rx, core->rx);

        /* transfer appropriate core characteristics */
        shell->primary = core->primary;
        shell->orig = core->orig;
        shell->region = core->region;

        /* do core-shell link */
        shell->core = core;
        core->shell = shell;
      }
    }
  }
  model->need_clear = TRUE;
  redraw_canvas(SINGLE);
  model_content_refresh(model);
}
void edit_shells_delete(void)
{
  GSList *list;
  struct core_pak *core;
  struct model_pak *model;
  gdouble charge, mass;

  model = sysenv.active_model;
  if (model)
  {
    if (model->num_frames > 1)
    {
      gui_text_show(INFO, "Shells cannot be deleted from multiframe model.\n");
      return;
    }

    for (list = model->selection; list; list = g_slist_next(list))
    {
      core = list->data;

      /* delete shell if present */
      if (core->shell)
      {
        charge = atom_charge(core);
        mass = atom_mass(core);

        core->charge = charge;
        core->mass = mass;
        delete_shell(core->shell);
        core->shell = NULL;
      }
    }
  }
  delete_commit(model);
  model->need_clear = TRUE; /* atom added — new geometry in possibly empty space */
  redraw_canvas(SINGLE);
  model_content_refresh(model);
}

void gui_refresh(gint type)
{
  switch (type)
  {
  case GUI_CANVAS:
    redraw_canvas(SINGLE);
    break;
  case GUI_MODEL_TREE:
    sysenv.refresh_tree = TRUE;
    break;
  case GUI_MODEL_PROPERTIES:
    sysenv.refresh_properties = TRUE;
    break;
  case GUI_TEXT_BUFFER:
    sysenv.refresh_text = TRUE;
    break;
  }
}

#define DEBUG_PICK_MODEL 0
void gui_model_select(struct model_pak *model)
{
  /* checks */
  if (!model)
    return;

  /* make the model active */
  sysenv.active_model = model;
  model->graph_active = NULL;

  /* Update canvas model pointer */
  {
    GSList *list;
    struct canvas_pak *canvas;
    for (list = sysenv.canvas_list; list; list = g_slist_next(list))
    {
      canvas = list->data;
      canvas->model = model;
    }
  }

  /* TODO - combine all these into a dialog refresh callback */
  gui_relation_update(model);

  /* this update could replace the auto version of the relation shortcuts */
  dialog_refresh_all();

  canvas_shuffle();

  model->need_clear = TRUE; /* camera changed — clear old geometry */
  gui_refresh(GUI_MODEL_PROPERTIES);
  gui_refresh(GUI_CANVAS);
}
/* NB: this is a bit dangerous as you change everything in the */
/* selection to the specified value - even (eg) incompatible elements */
void selection_properties_change(gint type)
{
  gint n, growth, region, translate;
  gdouble charge, sof;
  gdouble mass, coord, centre = 0;
  const gchar *text;
  GSList *list;
  struct elem_pak edata;
  struct model_pak *model;
  struct core_pak *core;

  model = sysenv.active_model;
  if (!model)
    return;
  if (!model->selection)
    return;

  /* In Qt mode, read from qt_edit_text buffer */
  extern const gchar *qt_edit_text[];
  gint qt_mode = 0;
  if (type >= 0 && type <= 11 && qt_edit_text && qt_edit_text[type])
    qt_mode = 1;
  else
    return;

  switch (type)
  {
  case ELEMENT:
    text = qt_edit_text[type];
    n = elem_symbol_test(text);
    if (n)
    {
      get_elem_data(n, &edata, model);
      for (list = model->selection; list; list = g_slist_next(list))
      {
        core = list->data;
        /* update attached shell */
        if (core->shell)
        {
          struct shel_pak *shell = core->shell;
          shell->atom_code = n;
        }
        /* Update element specific data if the element type changed */
        if (n != core->atom_code)
        {
          core->atom_code = n;
          core->bond_cutoff = edata.cova;
          if (core->shell)
          {
            struct shel_pak *shell = core->shell;
            shell->atom_code = n;
          }
        }
        init_atom_colour(core, model);
        init_atom_charge(core, model);
        init_atom_mass(core, model);
      }
      /* model updates */
      g_slist_free(model->unique_atom_list);
      model->unique_atom_list = find_unique(ELEMENT, model);
      calc_emp(model);
    }
    break;

  case NAME:
    text = qt_edit_text[type];
    n = elem_symbol_test(text);
    if (n)
    {
      get_elem_data(n, &edata, model);
      /* make sure we allow enough space for the string and the \0 */
      //    n = (LABEL_SIZE-1 > strlen(text)) ? strlen(text) : LABEL_SIZE-1; /* FIX e1c506 */
      for (list = model->selection; list; list = g_slist_next(list))
      {
        core = list->data;
        g_free(core->atom_label);
        core->atom_label = g_strdup(text);
        /* update atttached shell */
        if (core->shell)
        {
          struct shel_pak *shell = core->shell;
          g_free(shell->shell_label);
          shell->shell_label = g_strdup(text);
        }
      }
      /* model updates */
      g_slist_free(model->unique_atom_list);
      model->unique_atom_list = find_unique(ELEMENT, model);
      calc_emp(model);
    }
    break;

  case CHARGE:
    text = qt_edit_text[type];
    charge = str_to_float(text);
    for (list = model->selection; list; list = g_slist_next(list))
    {
      core = list->data;
      /* core updates */
      core->charge = charge;
      core->lookup_charge = FALSE;
      if (core->shell)
      {
        (core->shell)->charge = 0.0;
        (core->shell)->lookup_charge = TRUE;
      }
    }
    calc_emp(model);
    break;

  case WEIGHT:
    text = qt_edit_text[type];
    mass = str_to_float(text);
    for (list = model->selection; list; list = g_slist_next(list))
    {
      core = list->data;
      /* core updates */
      core->mass = mass;
      core->lookup_mass = FALSE;
      if (core->shell)
      {
        (core->shell)->mass = 0.0;
        (core->shell)->lookup_mass = TRUE;
      }
    }
    break;

  case COORD_X:
    text = qt_edit_text[type];
    coord = str_to_float(text);
    for (list = model->selection; list; list = g_slist_next(list))
    {
      core = list->data;
      centre += core->x[0];
    }
    centre /= g_slist_length(model->selection);
    for (list = model->selection; list; list = g_slist_next(list))
    {
      core = list->data;
      core->x[0] += (coord - centre);
      /* update attached shell */
      if (core->shell)
      {
        struct shel_pak *shell = core->shell;
        shell->x[0] += (coord - centre);
      }
    }
    coords_compute(model);
    connect_refresh(model);
    break;

  case COORD_Y:
    text = qt_edit_text[type];
    coord = str_to_float(text);
    for (list = model->selection; list; list = g_slist_next(list))
    {
      core = list->data;
      centre += core->x[1];
    }
    centre /= g_slist_length(model->selection);
    for (list = model->selection; list; list = g_slist_next(list))
    {
      core = list->data;
      core->x[1] += (coord - centre);
      /* update attached shell */
      if (core->shell)
      {
        struct shel_pak *shell = core->shell;
        shell->x[1] += (coord - centre);
      }
    }
    coords_compute(model);
    connect_refresh(model);
    break;

  case COORD_Z:
    text = qt_edit_text[type];
    coord = str_to_float(text);
    for (list = model->selection; list; list = g_slist_next(list))
    {
      core = list->data;
      centre += core->x[2];
    }
    centre /= g_slist_length(model->selection);
    for (list = model->selection; list; list = g_slist_next(list))
    {
      core = list->data;
      core->x[2] += (coord - centre);
      /* update attached shell */
      if (core->shell)
      {
        struct shel_pak *shell = core->shell;
        shell->x[2] += (coord - centre);
      }
    }
    coords_compute(model);
    connect_refresh(model);
    break;

  case SOF:
    text = qt_edit_text[type];
    sof = str_to_float(text);
    for (list = model->selection; list; list = g_slist_next(list))
    {
      core = list->data;
      /* core updates */
      if (sof <= 11 && sof > 0)
      {
        core->sof = sof;
        core->has_sof = TRUE;
        if (core->shell)
        {
          (core->shell)->sof = sof;
          core->has_sof = TRUE;
        }
      }
    }
    break;

  case CORE_GROWTH_SLICE:
    growth = 0;
    text = qt_edit_text[type];
    if (g_ascii_tolower(text[0]) == 't' || text[0] == '1' || text[0] == 'y')
      growth = 1;
    for (list = model->selection; list; list = g_slist_next(list))
    {
      core = list->data;
      core->growth = growth;
      if (model->colour_scheme == GROWTH_SLICE)
        atom_colour_scheme(GROWTH_SLICE, core, model);
    }
    break;

  case CORE_REGION:
    text = qt_edit_text[type];
    region = str_to_float(text) - 1;
    if (region > model->region_max)
      model->region_max = region;
    if (region < 1)
      region = 0;
    for (list = model->selection; list; list = g_slist_next(list))
    {
      core = list->data;
      core->region = region;
      if (core->shell)
        (core->shell)->region = region;

      if (model->colour_scheme == REGION)
        atom_colour_scheme(REGION, core, model);
    }
    break;

  case CORE_TRANSLATE:
    translate = 0;
    text = qt_edit_text[type];
    if (g_ascii_tolower(text[0]) == 't' || text[0] == '1' || text[0] == 'y')
      translate = 1;
    for (list = model->selection; list; list = g_slist_next(list))
    {
      core = list->data;
      core->translate = translate;
      if (core->shell)
        (core->shell)->translate = translate;

      if (model->colour_scheme == TRANSLATE)
        atom_colour_scheme(TRANSLATE, core, model);
    }
    break;

  case CORE_FF: /* reachable? */
    text = qt_edit_text[type];
    for (list = model->selection; list; list = g_slist_next(list))
    {
      core = list->data;
      if (core->atom_type)
        g_free(core->atom_type);
      core->atom_type = g_strdup(text);
    }
    break;
  }

  gui_refresh(GUI_MODEL_PROPERTIES);
  gui_refresh(GUI_CANVAS);
}
void unhide_atoms(void)
{
  GSList *list;
  struct model_pak *data;
  struct core_pak *core;
  struct shel_pak *shell;

  /* deletion for the active model only */
  data = sysenv.active_model;

  /* unhide */
  for (list = data->cores; list; list = g_slist_next(list))
  {
    core = list->data;
    core->status &= ~HIDDEN;
    if (core->shell)
    {
      shell = core->shell;
      shell->status &= ~HIDDEN;
    }
  }

  /* update */
  data->need_clear = TRUE; /* atoms unhidden — new geometry visible */
  redraw_canvas(SINGLE);
  model_content_refresh(data);
}
void atom_properties_change(gint type)
{
  gint n, growth, region, translate;
  gdouble temp;
  const gchar *text;
  struct elem_pak edata;
  struct model_pak *model;

  /* In Qt mode, w is NULL — read text from qt_edit_text buffer */
  extern const gchar *qt_edit_text[];
  if (!qt_edit_text || !qt_edit_text[type])
    return;

  model = sysenv.active_model;
  if (!model)
    return;

  if (g_slist_length(model->selection) == 0)
    return;

  /* act on multiple atoms? */
  if (g_slist_length(model->selection) > 1)
  {
    selection_properties_change(type);
    return;
  }

  /* In Qt mode, use the current selection directly (CEDIT.apd_core may be stale) */
  struct core_pak *core = NULL;
  core = (struct core_pak *) model->selection->data;

  /* Get text from appropriate source */
  text = qt_edit_text[type];

  switch (type)
  {
  case ELEMENT:
    n = elem_symbol_test(text);

    /* if recognized -> update */
    if (n)
    {
      get_elem_data(n, &edata, model);

      /* Update element specific data if the element type changed */
      if (n != core->atom_code)
      {
        core->atom_code = n;
        core->bond_cutoff = edata.cova;
      }
      init_atom_colour(core, model);
      init_atom_charge(core, model);
      init_atom_mass(core, model);

      g_slist_free(model->unique_atom_list);
      model->unique_atom_list = find_unique(ELEMENT, model);
      calc_emp(model);

      /* REFRESH */
      gui_refresh(GUI_MODEL_PROPERTIES);
    }
    break;

  case NAME:
    g_free(core->atom_label);
    core->atom_label = g_strdup(text);
    n = elem_symbol_test(text);

    /* update attached shell */
    if (core->shell)
    {
      struct shel_pak *shell = core->shell;
      g_free(shell->shell_label);
      shell->shell_label = g_strdup(text);
    }

    /* if recognized -> update */
    if (n)
    {
      get_elem_data(n, &edata, model);

      /* Update element specific data if the element type changed */
      if (n != core->atom_code)
      {
        core->atom_code = n;
        core->bond_cutoff = edata.cova;
      }
      init_atom_colour(core, model);
      init_atom_charge(core, model);
      init_atom_mass(core, model);

      g_slist_free(model->unique_atom_list);
      model->unique_atom_list = find_unique(ELEMENT, model);
      calc_emp(model);

      /* REFRESH */
      gui_refresh(GUI_MODEL_PROPERTIES);
    }
    break;

  case CORE_FF:
    if (core->atom_type)
      g_free(core->atom_type);
    core->atom_type = g_strdup(text);
    break;

  case CHARGE:
    core->charge = str_to_float(text);
    core->lookup_charge = FALSE;
    calc_emp(model);
    break;

  case WEIGHT:
    core->mass = str_to_float(text);
    core->lookup_mass = FALSE;
    break;

  case COORD_X:
    core->x[0] = str_to_float(text);
    coords_compute(model);
    break;

  case COORD_Y:
    core->x[1] = str_to_float(text);
    coords_compute(model);
    break;

  case COORD_Z:
    core->x[2] = str_to_float(text);
    coords_compute(model);
    break;

  case SOF:
    temp = str_to_float(text);
    if (temp <= 1.0 && temp > 0)
    {
      core->sof = temp;
      core->has_sof = TRUE;
      if (core->shell)
      {
        (core->shell)->sof = temp;
        core->has_sof = TRUE;
      }
    }
    break;

  case CORE_GROWTH_SLICE:
    growth = 0;
    if (g_ascii_tolower(text[0]) == 't' || text[0] == '1' || text[0] == 'y')
      growth = 1;
    core->growth = growth;
    if (model->colour_scheme == GROWTH_SLICE)
      atom_colour_scheme(GROWTH_SLICE, core, model);
    break;

  case CORE_REGION:
    region = str_to_float(text) - 1;
    if (region > model->region_max)
      model->region_max = region;
    if (region < 1)
      region = 0;
    core->region = region;
    if (core->shell)
      (core->shell)->region = region;
    if (model->colour_scheme == REGION)
      atom_colour_scheme(REGION, core, model);
    break;

  case CORE_TRANSLATE:
    translate = 0;
    if (g_ascii_tolower(text[0]) == 't' || text[0] == '1' || text[0] == 'y')
      translate = 1;
    core->translate = translate;
    if (core->shell)
      (core->shell)->translate = translate;
    if (model->colour_scheme == TRANSLATE)
      atom_colour_scheme(TRANSLATE, core, model);
    break;

  default:
    printf("Not yet modifiable...\n");
  }
  gui_refresh(GUI_CANVAS);
}

/* Bridge: get active model */
struct model_pak *qt_get_active_model(void) { return (struct model_pak *) sysenv.active_model; }

void space_image_widget_reset(void)
{
  struct model_pak *model;

  /* update model (if exists) */
  model = sysenv.active_model;
  if (model)
  {
    space_make_images(INITIAL, model);
    coords_init(CENT_COORDS, model);
    /* Force framebuffer clear — geometry changed (atom positions reset) */
    model->need_clear = TRUE;
  }

  /* update widget and canvas */
  gui_refresh(GUI_MODEL_PROPERTIES);
  gui_refresh(GUI_CANVAS);
}

void gui_mode_switch(gint new_mode)
{
  GSList *list;
  struct model_pak *data;
  struct core_pak *core1;

  /* get selected model */
  data = sysenv.active_model;
  if (!data)
    return;

  /* special case for morphology */
  if (data->id == MORPH)
  {
    switch (new_mode)
    {
    case FREE:
    case RECORD:
      break;

    default:
      gui_text_show(WARNING, "Disallowed mode.\n");
      return;
    }
  }

  /* clean up (if necessary) from previous mode */
  switch (data->mode)
  {
  case DIST_INFO:
  case BOND_INFO:
    for (list = data->cores; list; list = g_slist_next(list))
    {
      core1 = list->data;
      core1->status &= ~SELECT;
    }
    break;

  case RECORD:
    if (data->num_frames > 2)
    {
      /* CURRENT - I don't know why, but we seem to get a duplicate of the 1st frame */
      /* revert to original orientation */
      camera_init(data);
      data->num_frames = g_slist_length(data->transform_list);
    } else
    {
      /* if no extra frames - it's not an animation any more */
      data->num_frames = 1;
      data->animation = FALSE;
    }
    break;
  }

  /* special initialization */
  switch (new_mode)
  {
  case RECORD:
    /* NEW - disallow transformation record mode if file is */
    /* a standard animation as this will confuse read_frame() */
    if (data->frame_list || data->gulp.trj_file)
    {
      gui_text_show(ERROR, "Disallowed mode change.\n");
      return;
    }
    data->animation = TRUE;
    break;
  }

  /* general initialization */
  select_clear(data);
  data->mode = new_mode;
  data->state = 0;
  redraw_canvas(SINGLE);
}

void edit_model_create(void)
{
  struct model_pak *data;

  /* make a slot for the new model */
  data = model_new();
  sysenv.active_model = data;

  /* set up model parameters */
  data->id = CREATOR;
  strcpy(data->filename, "new_model");
  g_free(data->basename);
  data->basename = g_strdup("new_model");

  /* initialize */
  model_prep(data);
  data->mode = FREE;

  /* rmax has to be after coords_init() as INIT_COORDS will set it to 1.0 */
  data->rmax = 5.0 * RMAX_FUDGE;

  /* update canvas model pointer */
  {
    GSList *list;
    struct canvas_pak *canvas;
    for (list = sysenv.canvas_list; list; list = g_slist_next(list))
    {
      canvas = list->data;
      canvas->model = data;
    }
  }

  /* update/redraw */
  tree_model_add(data);
  tree_select_model(data);
  redraw_canvas(SINGLE);

  /* Refresh Qt tree model so new model appears */
  extern void qt_refresh_all(void);
  qt_refresh_all();
}
const char *qt_tree_model_name(int index)
{
  if (index < 0 || index >= qt_tree_count)
    return "";
  return qt_tree_nodes[index].name;
}
/* Return 0 for model nodes, 1 for graph nodes */
int qt_tree_node_depth(int index)
{
  if (index < 0 || index >= qt_tree_count)
    return 0;
  struct qt_tree_node *node = &qt_tree_nodes[index];
  /* Graph nodes have graph != NULL */
  return (node->graph != NULL) ? 1 : 0;
}
/* Get the icon name for a tree node */
const char *qt_tree_node_icon(int index)
{
  if (index < 0 || index >= qt_tree_count)
    return NULL;
  return qt_tree_nodes[index].icon_name;
}

/* C bridge: get total property count */
int qt_get_property_count(void)
{
  struct model_pak *model = sysenv.active_model;
  if (!model)
    return 0;
  return g_slist_length(model->property_list);
}
/* C bridge: get property label by index */
const char *qt_get_property_label(int index)
{
  struct model_pak *model = sysenv.active_model;
  if (!model)
    return "";
  GSList *item = g_slist_nth(model->property_list, index);
  if (!item)
    return "";
  struct property_pak *p = item->data;
  return p->label;
}
/* C bridge: get property value by index */
const char *qt_get_property_value(int index)
{
  struct model_pak *model = sysenv.active_model;
  if (!model)
    return "";
  GSList *item = g_slist_nth(model->property_list, index);
  if (!item)
    return "";
  struct property_pak *p = item->data;
  return p->value;
}
void qt_update_content_table(void)
{
  if (!g_content_table)
    return;

  struct model_pak *model = sysenv.active_model;
  if (!model)
    return;

  /* Call the C++ slot to populate the table */
  extern void qt_content_table_populate(gpointer table);
  qt_content_table_populate(g_content_table);
}

/* Bridge: set select mode from Qt without including pak.h */
void qt_set_select_mode(int mode)
{
  sysenv.select_mode = mode;
  /* Also set the active model's mode so the canvas click handler routes clicks correctly */
  struct model_pak *model = sysenv.active_model;
  if (model)
  {
    model->mode = mode;
    model->redraw = TRUE; /* Force overlay text regeneration on next paint */
  }
}

#define DEBUG_ACTIVE_MODEL_SET 0
void tree_select_model(struct model_pak *model)
{
  if (!model)
    return;
  /* Ignore during batch loading — Qt focus events can overwrite active_model */
  if (sysenv.ignore_tree_select)
    return;
  sysenv.active_model = model;
}

void tree_select_active(void)
{
  if (!sysenv.tree_store)
    return;
  /* TODO - clear if no models */
  tree_select_model(sysenv.active_model);
}

int qt_tree_get_count(void) { return qt_tree_count; }

struct model_pak *qt_tree_get_model(int index)
{
  if (index < 0 || index >= qt_tree_count)
    return NULL;
  return qt_tree_nodes[index].model;
}

gpointer qt_tree_get_graph(int index)
{
  if (index < 0 || index >= qt_tree_count)
    return NULL;
  return qt_tree_nodes[index].graph;
}

void qt_tree_refresh(void)
{
  /* Clear and rebuild from sysenv.mal */
  for (int i = 0; i < qt_tree_count; i++)
  {
    g_free(qt_tree_nodes[i].name);
  }
  qt_tree_count = 0;
  memset(qt_tree_nodes, 0, sizeof(qt_tree_nodes));

  GSList *list;
  for (list = sysenv.mal; list; list = g_slist_next(list))
  {
    struct model_pak *m = (struct model_pak *) list->data;
    if (qt_tree_count >= 4096)
      break;

    /* Add model node (depth 0) */
    qt_tree_nodes[qt_tree_count].model = m;
    qt_tree_nodes[qt_tree_count].graph = NULL;
    qt_tree_nodes[qt_tree_count].name = g_strdup(m->basename);
    qt_tree_nodes[qt_tree_count].type = m->periodic;
    /* Set icon based on model type/periodicity */
    if (m->id == MORPH)
      qt_tree_nodes[qt_tree_count].icon_name = "DIAMOND2";
    else if (m->track_me)
      qt_tree_nodes[qt_tree_count].icon_name = "TRACK";
    else
      switch (m->periodic)
      {
      case 3:
        qt_tree_nodes[qt_tree_count].icon_name = "BOX";
        break;
      case 2:
        qt_tree_nodes[qt_tree_count].icon_name = "SURFACE";
        break;
      case 1:
        qt_tree_nodes[qt_tree_count].icon_name = "POLYMER";
        break;
      default:
        qt_tree_nodes[qt_tree_count].icon_name = "METHANE";
        break;
      }
    qt_tree_count++;

    /* Add graph child nodes (depth 1) */
    GSList *glist;
    for (glist = m->graph_list; glist; glist = g_slist_next(glist))
    {
      if (qt_tree_count >= 4096)
        break;
      struct graph_pak *graph = (struct graph_pak *) glist->data;
      qt_tree_nodes[qt_tree_count].model = m;
      qt_tree_nodes[qt_tree_count].graph = graph;
      qt_tree_nodes[qt_tree_count].name = g_strdup(graph->treename);
      qt_tree_nodes[qt_tree_count].type = graph->type;
      qt_tree_nodes[qt_tree_count].icon_name = "GRAPH";
      qt_tree_count++;
    }
  }
}

#define DEBUG_IGNORE_FLAG 0
void qt_tree_model_select(int index)
{
  if (index < 0 || index >= qt_tree_count)
    return;
  struct qt_tree_node *node = &qt_tree_nodes[index];

  /* Ignore during batch loading — Qt focus events can overwrite active_model */
  if (sysenv.ignore_tree_select && !node->graph)
    return;

  /* Set active model first */
  if (!node->graph)
  {
    sysenv.active_model = node->model;
    node->model->graph_active = NULL;
  }

  if (node->graph)
  {
    /* Graph node (depth 1): select model and activate graph */
    gui_model_select(node->model);
    node->model->graph_active = node->graph;
    extern void redraw_canvas(gint);
    extern void gl_canvas_refresh(void);
    node->model->need_clear = TRUE; /* new model — clear old geometry */
    redraw_canvas(1);
  } else
  {
    /* Model node (depth 0): shuffle canvas */
    canvas_shuffle();
    extern void redraw_canvas(gint);
    if (sysenv.active_model)
      ((struct model_pak *) sysenv.active_model)->need_clear = TRUE; /* new model — clear old geometry */
    redraw_canvas(1);
  }

  /* Refresh content properties for the active model */
  extern void model_content_refresh(struct model_pak *);
  extern void qt_update_content_table(void);
  if (sysenv.active_model)
  {
    model_content_refresh(sysenv.active_model);
    qt_update_content_table();
  }
}

void qt_refresh_all(void)
{
  qt_tree_refresh();
  extern void qt_refresh_qt_tree(void);
  qt_refresh_qt_tree();
}

void qt_tree_model_add(const char *name, int model_type)
{
  /* Called from tree_model_add - model is already in sysenv.mal.
   * Rebuild the Qt tree so the new model appears in the list. */
  qt_tree_refresh();
  /* Also refresh the Qt tree widget on the main window */
  extern void qt_refresh_qt_tree(void);
  qt_refresh_qt_tree();
}
void tree_model_add(struct model_pak *model)
{
  /* Qt tree bridge: always add to the model list */
  qt_tree_model_add(model->basename, model->periodic);
}

void redraw_canvas(gint action)
{
  GSList *list;
  struct model_pak *model;

  switch (action)
  {
  case SINGLE: {
    struct model_pak *data = sysenv.active_model;
    if (data)
    {
      data->redraw = TRUE;
      /* Note: need_clear is NOT set here. It's only set when geometry
       * actually changes (atoms/bonds added or removed). Camera moves
       * during drag don't need a clear — depth buffer overwrites old pixels. */
    }
    break;
  }
  case ALL:
    for (list = sysenv.mal; list; list = g_slist_next(list))
    {
      model = list->data;
      model->redraw = TRUE;
    }
    break;
  }
  sysenv.refresh_canvas = TRUE;
  /* Notify Qt GUI to repaint the canvas */
  extern void qt_force_canvas_refresh(void);
  qt_force_canvas_refresh();
}

gpointer canvas_find(struct model_pak *model)
{
  GSList *list;
  struct canvas_pak *canvas;

  for (list = sysenv.canvas_list; list; list = g_slist_next(list))
  {
    canvas = list->data;
    if (canvas->model == model)
      return (canvas);
  }
  return (NULL);
}

void geom_label_toggle(void)
{
  struct model_pak *model = sysenv.active_model;

  if (model)
  {
    model->show_geom_labels ^= 1;
    redraw_canvas(SINGLE);
  }
}
void render_mode_set(gpointer data)
{
  gint mode;
  struct model_pak *model;

  model = sysenv.active_model;
  if (!model)
    return;

  mode = GPOINTER_TO_INT(data);

  spatial_destroy_by_label("polyhedra", model);
  spatial_destroy_by_label("zones", model);

  /* new mode becomes default for the model */
  model->default_render_mode = mode;

  /* selection based rendering */
  if (model->selection)
    core_render_mode_set(mode, model->selection);
  else
    core_render_mode_set(mode, model->cores);

  redraw_canvas(SINGLE);
}
void render_wire_atoms(void)
{
  struct model_pak *model;

  model = sysenv.active_model;
  if (!model)
    return;

  if (model->selection)
    core_render_wire_set(TRUE, model->selection);
  else
    core_render_wire_set(TRUE, model->cores);

  redraw_canvas(SINGLE);
}
void render_solid_atoms(void)
{
  struct model_pak *model;

  model = sysenv.active_model;
  if (!model)
    return;

  if (model->selection)
    core_render_wire_set(FALSE, model->selection);
  else
    core_render_wire_set(FALSE, model->cores);

  redraw_canvas(SINGLE);
}
void render_mode_polyhedral(void)
{
  struct model_pak *model;

  model = sysenv.active_model;
  if (!model)
    return;

  create_polyhedra(model);
  coords_init(REDO_COORDS, model);
  redraw_canvas(SINGLE);
}
void render_mode_zone(void)
{
  gpointer za;
  struct model_pak *model;

  model = sysenv.active_model;
  if (!model)
    return;

  core_render_mode_set(ZONE, model->cores);

  /* CURRENT */
  za = zone_make(sysenv.render.zone_size, model);
  zone_display_init(za, model);
  zone_free(za);

  coords_init(REDO_COORDS, model);
  redraw_canvas(SINGLE);
}

/* Qt bridge: set content table pointer */
void qt_set_content_table(gpointer table) { g_content_table = table; }

void qt_gui_symmetry_refresh(gpointer dummy)
{
  /* Populate symmetry table */
  extern void qt_populate_symmetry_table(void);
  qt_populate_symmetry_table();
}

void analysis_export(gchar *name)
{
  struct model_pak *model;

  model = sysenv.active_model;
  g_assert(model != NULL);

  if (model->graph_active)
  {
    graph_write(name, model->graph_active);
    gui_text_show(STANDARD, "Successfully exported graph.\n");
  } else
    gui_text_show(WARNING, "Current model is not a graph.\n");
}

void import_pcf(gchar *filename)
{
  struct model_pak *model;

  model = sysenv.active_model;
  if (!model)
    return;

  project_read(filename, model);

  redraw_canvas(SINGLE);
}

void import_off(gchar *filename)
{
  struct model_pak *model;

  model = sysenv.active_model;
  if (!model)
    return;

  read_off(filename, model);

  redraw_canvas(SINGLE);
}

void refresh_model_from_table(void)
{
  struct model_pak *model;

  model = sysenv.active_model;
  if (model)
  {
    /* TODO - refresh bonding/connectivity & other data? */
    /* refresh colour */
    model_colour_scheme(model->colour_scheme, model);
    redraw_canvas(SINGLE);
  }
}

void write_input_file(gchar *input_file, gpointer data)
{
  FILE *fp;
  struct model_pak *model = data;
  gchar *last_dirsep, *yes, *no;
  gint i;

  yes = g_strdup("yes");
  no = g_strdup("no");

  g_unlink(input_file);

  fp = fopen(input_file, "wt");
  if (!fp)
  {
    gui_text_show(ERROR, "Unable to open Monty input file for writing");
    return;
  }

  fprintf(fp, "#Monty input file generated by GDIS %f\n", VERSION);
  fprintf(fp, "&INPUT\n");

  /* remove any whitespace, this also includes trailing \n characters */
  g_strstrip(model->monty.hkls);

  /* split the string by endline characters */
  gchar **hkls = g_strsplit(model->monty.hkls, "\n", INT_MAX);

  /* print all hkl values to the file */
  for (i = 0; *(hkls + i); ++i)
  {
    fprintf(fp, " -hkl %s\n", *(hkls + i));
  }

  /* remove any whitespace, this also includes trailing \n characters */
  g_strstrip(model->monty.supersaturations);
  gchar **supersats = g_strsplit(model->monty.supersaturations, "\n", INT_MAX);
  for (i = 0; *(supersats + i); ++i)
  {
    fprintf(fp, " -supersat %s\n", *(supersats + i));
  }
  g_strfreev(supersats);

  last_dirsep = g_strrstr(model->monty.input_surface, DIR_SEP);

  if (last_dirsep)
    fprintf(fp, " -surface %s\n", (last_dirsep + 1));
  else
  {
    /* TODO: make this work when no complete path is selected  */
    fprintf(fp, "# -surface\n");
  }

  last_dirsep = g_strrstr(model->monty.input_cgf, DIR_SEP);

  fprintf(fp, " -cgf %s\n", (last_dirsep + 1));
  fprintf(fp, " -directory %s\n", g_strndup(model->monty.input_cgf, (last_dirsep - model->monty.input_cgf)));
  fprintf(fp, " -energy_unit %s\n", model->monty.energy_unit);
  fprintf(fp, " -esolv %s\n", model->monty.esolv);

  fprintf(fp, "&END\n\n&OUTPUT\n");

  /* remove any whitespace, this also includes trailing \n characters */
  g_strstrip(model->monty.output_dirs);
  gchar **out_dirs = g_strsplit(model->monty.output_dirs, "\n", INT_MAX);
  for (i = 0; *(out_dirs + i); ++i)
  {
    fprintf(fp, " -directory %s %s\n", *(hkls + i), *(out_dirs + i));
  }

  g_strfreev(hkls);
  g_strfreev(out_dirs);

  fprintf(fp, " -extension %s\n", model->monty.output_extension);
  fprintf(fp, " -surface %s\n", (model->monty.write_surface ? yes : no));
  fprintf(fp, " -xmm %s\n", (model->monty.write_xyz ? yes : no));
  fprintf(fp, " -xyz %s\n", (model->monty.write_matlab ? yes : no));
  fprintf(fp, " -msi %s\n", (model->monty.write_msi ? yes : no));

  fprintf(fp, "&END\n\n&MODEL\n");

  fprintf(fp, " -spiral %s\n", (model->monty.write_matlab ? yes : no));
  fprintf(fp, " -xsteps %d\n", nearest_int(model->monty.xsteps));
  fprintf(fp, " -ysteps %d\n", nearest_int(model->monty.ysteps));
  fprintf(fp, " -kinetics %2.2f\n", model->monty.kinetics);
  fprintf(fp, " -temperature %10.2f\n", model->monty.temperature);

  fprintf(fp, "&END\n\n&MONITOR\n");

  fprintf(fp, " -height %s\n", (model->monty.monitor_height ? yes : no));
  fprintf(fp, " -energy %s\n", (model->monty.monitor_energy ? yes : no));
  fprintf(fp, " -hhcorr %s\n", (model->monty.monitor_hhcorr ? yes : no));
  fprintf(fp, " -gdis %s\n", (model->monty.multi_frame_xyz ? yes : no));

  fprintf(fp, "&END\n\n&RUN\n");

  fprintf(fp, " -randomseed %s\n", model->monty.random_seed);
  fprintf(fp, " -rows %d\n", nearest_int(model->monty.rows));
  fprintf(fp, " -cols %d\n", nearest_int(model->monty.cols));
  fprintf(fp, " -layers %d\n", nearest_int(model->monty.layers));
  fprintf(fp, " -increment %d\n", nearest_int(model->monty.increment));
  fprintf(fp, " -relaxation %d\n", nearest_int(model->monty.relax));
  fprintf(fp, " -cycles %d\n", nearest_int(model->monty.cycles));
  fprintf(fp, " -moves %d\n", nearest_int(model->monty.moves));

  fprintf(fp, "&END\n");
  g_free(yes);
  g_free(no);
  fclose(fp);
}

gint exec_monty(const gchar *input, struct task_pak *task)
{
  gint status = 0;
  gchar *cmd;

  /* checks */
  if (!sysenv.monty_path)
    return (-1);

  /* delete the old file to be sure output is only data from current run */
  cmd = g_strdup_printf("%s  %s", sysenv.monty_path, input);

#if DEBUG_MONTY_GUI
  printf("executing: [%s]\n", cmd);
#endif

  task->is_async = TRUE;
  // task->status_file = g_build_filename(sysenv.cwd,???,NULL);/*what should be the status file?*/
  status = task_async(cmd, &(task->pid));

  /* done */
  g_free(cmd);
  return (status);
}

void exec_monty_task(gpointer ptr, gpointer data)
{
  gchar *inpfile;
  struct model_pak *model = ptr;
  struct task_pak *task = data;

  /* checks */
  g_assert(model != NULL);
  g_assert(task != NULL);

  /* construct fullpath input filename - required for writing */
  inpfile = g_build_filename(sysenv.cwd, "input.monty2", NULL);
  /* no status file for the moment, as there are multiple output files for a
    monty simulation, so unable to follow a single one *
  task->status_file = g_build_filename(sysenv.cwd, model->gulp.out_file, NULL);
   */

#if DEBUG_MONTY_GUI
  printf(" input file: %s\n", inpfile);

#endif

  write_input_file(inpfile, model);
  g_free(inpfile);

  exec_monty("input.monty2", task);
}

void gui_monty_task(gpointer data)
{
  if (!sysenv.monty_path)
  {
    gui_text_show(ERROR, "Monty executable was not found.\n");
    return;
  }
  /* put a task that runs monty. for now, no postprocessing */
  task_new("Monty", &exec_monty_task, data, NULL, NULL, data);
}

void exec_siesta_task(gpointer ptr, gpointer data)
{
  struct model_pak *model = ptr;
  struct task_pak *task = data;
  gchar *fdfPath, *outPath;

  g_assert(model != NULL);
  g_assert(task != NULL);

  /* construct fullpath input filename */
  fdfPath = g_build_filename(sysenv.cwd, model->siesta.modelfilename, NULL);

  /* generate output filename */
  outPath = parse_extension_set(model->siesta.modelfilename, "out");
  outPath = g_build_filename(sysenv.cwd, outPath, NULL);

  task->status_file = g_strdup(outPath);

  /* write the FDF file */
  write_fdf(fdfPath, model);
  g_free(fdfPath);

  /* execute: siesta < input.fdf > output.out */
  if (!sysenv.siesta_path)
    return;

  gchar *cmd = g_strdup_printf("%s/%s < %s > %s 2>&1", sysenv.siesta_path, sysenv.siesta_exe,
                               fdfPath ? fdfPath : model->siesta.modelfilename, outPath);
  g_free(fdfPath);

  task->is_async = TRUE;
  task_async(cmd, &(task->pid));
  g_free(cmd);
}

void proc_siesta_task(gpointer ptr)
{
  struct model_pak *data = ptr;
  /* TODO - process siesta output */
}

void gui_siesta_task(struct model_pak *data)
{
  g_return_if_fail(data != NULL);
  if (!sysenv.siesta_path)
  {
    gui_text_show(ERROR, "SIESTA executable was not found.\n");
    return;
  }
  task_new("Siesta", &exec_siesta_task, data, &proc_siesta_task, data, data);
}

void gui_create_waypoint(void)
{
  struct model_pak *model;
  gpointer camera;

  model = sysenv.active_model;
  if (model)
  {
    /* allocate */
    camera = camera_dup(model->camera);
    model->waypoint_list = g_slist_append(model->waypoint_list, camera);

    model->camera = camera;
    /* Invalidate GL cache — new camera */
    model->gl_cache.dirty = TRUE;
  }
}

void povray_exec_task(gpointer ptr, struct task_pak *task)
{
  GString *cmd;
  gchar *file;

  g_return_if_fail(ptr != NULL);
  cmd = g_string_new(NULL);

  file = g_shell_quote(ptr);

  /* build the command line */
  g_string_printf(cmd, "%s +I%s -Ga -P +W%d +H%d +FT", sysenv.povray_path, file, (gint) sysenv.render.width,
                  (gint) sysenv.render.height);
  if (sysenv.render.antialias)
    g_string_append_printf(cmd, " +A +AM2");

  g_free(file);

  /* don't do continuous display updates */
  g_string_append_printf(cmd, " -D");

  task->is_async = TRUE;
  // task->status_file = g_build_filename(sysenv.cwd,???,NULL);/*what should be the status file?*/
  task_async(cmd->str, &(task->pid));

  g_string_free(cmd, TRUE);
}

void povray_exec(gchar *name)
{
  gchar *filename;
  GString *cmd;

  g_return_if_fail(name != NULL);

  cmd = g_string_new(NULL);

  filename = g_shell_quote(name);

  /* build the command line */
  g_string_printf(cmd, "%s +I%s -GA -P +W%d +H%d +FT -D", sysenv.povray_path, filename, (gint) sysenv.render.width,
                  (gint) sysenv.render.height);
  if (sysenv.render.antialias)
    g_string_append_printf(cmd, " +A +AM2");

  /* after rendering delete input file, */
  if (sysenv.render.no_keep_tempfiles)
    g_string_append_printf(cmd, "; rm -f %s", filename);

  /* execute */
  printf("system: %s\n", cmd->str);

  IGNORE_RETURN(system(cmd->str));
  printf("\n");

  /* cleanup */
  g_string_free(cmd, TRUE);
  g_free(filename);
}
void exec_img_task(gpointer ptr)
{
  gchar *tmp, *basename, *fullpath, *file_img;

  /* post povray command */
  g_return_if_fail(ptr != NULL);

  /* construct basename (no extension) */
  basename = parse_strip((gchar *) ptr);
  fullpath = g_build_filename(sysenv.cwd, basename, NULL);
  g_free(basename);

  /* remove .pov input file */
  if (sysenv.render.no_keep_tempfiles)
  {
    tmp = g_strdup_printf("%s.pov", fullpath);
    g_remove(tmp);
    g_free(tmp);
  }

  /* construct image filename */
  tmp = g_strdup_printf("%s.tga", fullpath);
  file_img = g_shell_quote(tmp);
  g_free(tmp);

  /* construct viewing task command */
  tmp = g_strdup_printf("%s %s", sysenv.viewer_path, file_img);
  g_spawn_command_line_async(tmp, NULL);
  g_free(tmp);

  /* cleanup */
  g_free(file_img);
  g_free(fullpath);

  /* this was the temporary filename which now must be cleaned up */
  g_free(ptr);
}
void povray_task(void)
{
  gchar *basename, *fullname;
  struct model_pak *model;

  model = sysenv.active_model;
  if (!model)
    return;

  /* make an input file */
  basename = gun("pov");
  if (!basename)
  {
    printf("povray_task() error: failed to get an unused filename.\n");
    return;
  }

  /* create the input file */
  fullname = g_build_filename(sysenv.cwd, basename, NULL);
  write_povray(fullname, model);
  g_free(fullname);

  if (sysenv.render.no_povray_exec)
  {
    /* no further tasks, can free basename */
    g_free(basename);
  } else
  {
    /* basename should be freed by second (cleanup) routine */
    task_new("POVRay", &povray_exec_task, basename, &exec_img_task, basename, NULL);
  }
}

/* Qt bridge: execute povray */
void qt_povray_exec(const char *filename) { povray_exec((char *) filename); }

void gui_defect_default(void)
{
  defect.cleave = FALSE;
  defect.neutral = FALSE;
  defect.cluster = FALSE;
  VEC2SET(defect.region, 10, 10);
  VEC3SET(defect.orient, 1, 0, 0);
  VEC3SET(defect.burgers, 1, 0, 0);
  VEC2SET(defect.origin, 0, 0);
  VEC2SET(defect.center, 0, 0);
}

void plot_initialize(struct model_pak *data)
{
  struct plot_pak *plot;
  int i, j;
  gdouble y;
  /* init the plot pak */
  if (!data)
    return;
  plot = (struct plot_pak *) g_malloc(sizeof(struct plot_pak));
  if (plot == NULL)
    return;
  plot->xtics = 1.0;
  plot->ytics = 1.0;
  plot->graph = data->graph_active;
  plot->data_changed = TRUE; /*forces a reload of data*/
  plot->energy.data = NULL;
  plot->force.data = NULL;
  plot->volume.data = NULL;
  plot->pressure.data = NULL;
  plot->band.data = NULL;
  plot->dos.data = NULL;
  plot->frequency.data = NULL;
  plot->ndos = 0;
  plot->nbands = 0;
  plot->nfreq = 0;
  plot->nraman = 0;
  plot->plot_mask = PLOT_NONE;
  plot->plot_sel = 0;
  plot->task = NULL;
  /*dynamics*/
  if (data->num_frames > 1)
  {
    if (property_lookup("Energy", data))
      plot->plot_mask += PLOT_ENERGY;
    if (property_lookup("Force", data))
      plot->plot_mask += PLOT_FORCE;
    if (property_lookup("Pressure", data) || property_lookup("Stress", data))
      plot->plot_mask += PLOT_PRESSURE;
    if (data->periodic == 3)
      plot->plot_mask += PLOT_VOLUME;
    if (plot->plot_mask == PLOT_NONE)
      plot->type = PLOT_NONE;
    else if ((plot->plot_mask & PLOT_ENERGY) != 0)
      plot->type = PLOT_ENERGY;
    else if ((plot->plot_mask & PLOT_FORCE) != 0)
      plot->type = PLOT_FORCE;
    else if ((plot->plot_mask & PLOT_VOLUME) != 0)
      plot->type = PLOT_VOLUME;
    else if ((plot->plot_mask & PLOT_PRESSURE) != 0)
      plot->type = PLOT_PRESSURE;
  }
  /*electronics*/
  if (data->ndos > 1)
  {
    /*we have some dos... preload (will be scaled later on a thread)*/
    plot->plot_mask += PLOT_DOS;
    plot->ndos = data->ndos;
    plot->dos.size = plot->ndos * 2;
    if (plot->dos.data != NULL)
    {
      g_free(plot->dos.data);
      plot->dos.data = NULL;
    }
    plot->dos.data = (gdouble *) g_malloc(plot->dos.size * sizeof(gdouble));
  }
  if ((data->nbands > 1) && (data->band_up != NULL))
  {
    plot->plot_mask += PLOT_BAND;
    plot->nbands = data->nbands;
    /*the sizes!*/
    plot->band.size = data->nkpoints;
    if (data->spin_polarized)
      plot->band.data = (gdouble *) g_malloc(((1 + plot->nbands) * plot->band.size + 4) * sizeof(gdouble));
    else
      plot->band.data = (gdouble *) g_malloc(((1 + 2 * plot->nbands) * plot->band.size + 4) * sizeof(gdouble));
  }
  if (!((plot->plot_mask & PLOT_BANDOS) ^ PLOT_BANDOS))
  {
    /*bandos type is possible*/
  }
  /*frequency*/
  if (data->have_frequency == TRUE)
  {
    /*this don't need to be loaded from outside.. it won't change*/
    plot->plot_mask += PLOT_FREQUENCY;
    plot->nfreq = data->nfreq;
    plot->frequency.size = plot->nfreq * 2;
    if (plot->frequency.data != NULL)
    {
      g_free(plot->frequency.data);
      plot->frequency.data = NULL;
    }
    plot->frequency.data = (gdouble *) g_malloc(plot->frequency.size * sizeof(gdouble));
    plot->frequency.xmin = 0.0; /*the default is to start plotting frequency in [0,ymax+100]*/
    plot->frequency.xmax = data->freq[0];
    plot->frequency.ymin = 0.; /*default is relative intensity ie [0.0,1.0]*/
    plot->frequency.ymax = 1.;
    j = 0;
    for (i = 0; i < data->nfreq; i++)
    {
      y = data->freq[i];
      if (y > plot->frequency.xmax)
        plot->frequency.xmax = y;
      plot->frequency.data[j] = y; /*this data is actually x*/
      if (data->freq_intens == NULL)
        plot->frequency.data[j + 1] = 1.0;
      else
      {
        plot->frequency.data[j + 1] = data->freq_intens[i];
        if (data->freq_intens[i] > plot->frequency.ymax)
          plot->frequency.ymax = data->freq_intens[i];
      }
      j = j + 2;
    }
    plot->frequency.xmax = ((gint) (plot->frequency.xmax / 1000) + 1) * 1000.0;
  }
  if (data->plot != NULL)
    g_free(data->plot);
  data->plot = (gpointer) plot;
}

void gui_vasp_init()
{
  vasp_gui.cur_page = VASP_PAGE_SIMPLIFIED;
  vasp_gui.have_potcar_folder = FALSE;
  vasp_gui.poscar_dirty = TRUE;
  if (!vasp_gui.have_xml)
  {
    vasp_gui.rions = TRUE;
    vasp_gui.rshape = FALSE;
    vasp_gui.rvolume = FALSE;
    vasp_gui.have_paw = FALSE;
  }
  /*default simplified interface*/
  vasp_gui.simple_rgeom = FALSE;
  vasp_gui.dimension = 3.;
  vasp_gui.calc.poscar_free = VPF_FREE;
}

void run_vasp_exec(vasp_exec_struct *vasp_exec, struct task_pak *task)
{
  /* Execute a vasp task TODO distant job */
  gchar *cmd;
  gchar *cwd; /*for push/pull*/

#if __WIN32
  /*    At present running vasp in __WIN32 environment is impossible.
   *    However,  it should be possible to launch a (remote) job on a
   *    distant server. See TODO */
  fprintf(stderr, "VASP calculation can't be done in this environment.\n");
  return;
#else
  /*direct launch*/
  if ((*vasp_exec).job_nproc < 2)
    cmd = g_strdup_printf("%s > vasp.log", (*vasp_exec).job_vasp_exe);
  else
    cmd = g_strdup_printf("%s -np %i %s > vasp.log", (*vasp_exec).job_mpirun, (gint) (*vasp_exec).job_nproc,
                          (*vasp_exec).job_vasp_exe);
#endif
  cwd = sysenv.cwd; /*push*/
  sysenv.cwd = g_strdup_printf("%s", (*vasp_exec).job_path);
  task->is_async = TRUE;
  task->status_file = g_build_filename((*vasp_exec).job_path, "vasp.log", NULL);
  task_async(cmd, &(task->pid));
  g_free(sysenv.cwd);
  sysenv.cwd = cwd; /*pull*/
  g_free(cmd);
}
void cleanup_vasp_exec(vasp_exec_struct *vasp_exec)
{
  /*VASP process has exit, try to load the result*/
  gchar *line;
  /*sync_ wait for result?*/
  line = g_strdup_printf("VASP job finished!\n");
  gui_text_show(ITALIC, line);
  g_free(line);
}
gint save_uspex_calc()
{
#define DEBUG_USPEX_SAVE 0
  gchar *filename;
  /*1-output*/
  // SAVE calculation parameters to INPUT.txt
  filename = g_strdup_printf("%s/INPUT.txt", uspex_gui.calc.job_path);
  if (dump_uspex_parameters(filename, &(uspex_gui.calc)) < 0)
  {
    fprintf(stderr, "#ERR: while saving INPUT.txt!\n");
    return -1;
  }
  g_free(filename);
  /*debug print*/
#if DEBUG_USPEX_SAVE
  dump_uspex_parameters(stdout, &(uspex_gui.calc));
#endif
  return 0;
}

/* Dead stub — called from qt_api.c but does nothing */
#define DEBUG_CALC_MOLSURF 0
void ms_calculate(void)
{
  gchar *text;
  gdouble value;
  const gchar *tmp;
  struct model_pak *model;

  model = sysenv.active_model;
  g_assert(model != NULL);

  /* get colouring type from ms_colour variable (set by Qt) */
  /* ms_colour is set externally by the UI before calling ms_calculate */

  /* epot values are set by the UI before calling ms_calculate */

  /* main call */
  value = ms_blur;
  switch (ms_method)
  {
  case MS_EDEN:
  case MS_SSATOMS:
    value = ms_eden;
    break;
  }
  ms_cube(value, ms_method, ms_colour, model);

  /* update widget not needed for QT */

  coords_init(CENT_COORDS, model);

  sysenv.refresh_dialog = TRUE;

  redraw_canvas(SINGLE);
}
void ms_delete(void)
{
  struct model_pak *model;

  model = sysenv.active_model;
  g_assert(model != NULL);

  /* remove any previous surfaces */
  spatial_destroy_by_label("molsurf", model);
  model->ms_colour_scale = FALSE;
  coords_init(CENT_COORDS, model);
  redraw_canvas(SINGLE);
}

#define DEBUG_PROC_GULP 0
void proc_gulp_task(gpointer ptr)
{
  gchar *inp, *res, *out;
  GString *line;
  struct model_pak *dest, *data;

  /* TODO - model locking (moldel_ptr RO/RW etc) to prevent screw ups */
  g_return_if_fail(ptr != NULL);
  data = ptr;

  /* don't attempt to process if gulp execution was turned off */
  if (data->gulp.no_exec)
    return;

  /* FIXME - if the user has moved directories since submitting */
  /* the gulp job then cwd will have changed and this will fail */
  inp = g_build_filename(sysenv.cwd, data->gulp.temp_file, NULL);
  res = g_build_filename(sysenv.cwd, data->gulp.dump_file, NULL);

  out = g_build_filename(sysenv.cwd, data->gulp.out_file, NULL);

  switch (data->gulp.run)
  {
  case E_SINGLE:
    /* same model (ie with current energetics dialog) so update */
    /* Prevent read_gulp_output from overwriting model identity */
    data->grafted = TRUE;
    read_gulp_output(out, data);
    data->grafted = FALSE;
    /* update energy (TODO - only if successful) */
    line = g_string_new(NULL);
    if (data->gulp.free)
      g_string_printf(line, "%f (free energy)", data->gulp.energy); // g_string_sprintf deprecated
    else
      g_string_printf(line, "%f", data->gulp.energy); // g_string_sprintf deprecated

    property_add_ranked(2, "Energy", line->str, data);

    gui_active_refresh();

    /* Qt: update Gulp dialog energy display */
    extern void qt_update_gulp_energy(struct model_pak * model);
    qt_update_gulp_energy(data);

    g_string_free(line, TRUE);
    break;

  case E_OPTIMIZE:
    /* TODO - make it possile to get dialog data by request */
    /* so that we can check if a dialog exsits to be updated */
    /* get new coords */
    /* create new model for the minimized result */
    dest = model_new();
    g_return_if_fail(dest != NULL);

    /* read main data from the res file (correct charges etc.) */
    read_gulp(res, dest);
    /* graft to the model tree, so subsequent GULP read doesn't replace coords */
    tree_model_add(dest);
    /* get the output energy/phonons etc. */
    read_gulp_output(out, dest);

    /* FIXME - if the GULP job fails - model_prep() isnt called, and the */
    /* model camera isnt initialized => error trap when gdis tries to visualize */
    if (!dest->camera)
    {
      printf("WARNING: GULP calculation has possibly failed.\n");
      model_prep(dest);
    }

    break;

    /* MD */
  default:
    break;
  }

  g_free(inp);
  g_free(res);
  g_free(out);

  redraw_canvas(ALL);
  return;
}
#define DEBUG_EXEC_GULP_TASK 0
void exec_gulp_task(gpointer ptr, gpointer data)
{
  gchar *inpfile;
  struct model_pak *model = ptr;
  struct task_pak *task = data;

  /* checks */
  g_assert(model != NULL);
  g_assert(task != NULL);

  /* construct fullpath input filename - required for writing */
  inpfile = g_build_filename(sysenv.cwd, model->gulp.temp_file, NULL);
  task->status_file = g_build_filename(sysenv.cwd, model->gulp.out_file, NULL);

#if DEBUG_EXEC_GULP_TASK
  printf(" input file: %s\n", inpfile);
  printf("output file: %s\n", model->gulp.out_file);
#endif
  write_gulp(inpfile, model);
  g_free(inpfile);
  /* are we supposed to execute GULP? */
  if (model->gulp.no_exec)
  {
#if DEBUG_EXEC_GULP_TASK
    printf("Skipping GULP execution on user request.\n");
#endif
    return;
  }
  bg_exec_gulp(model->gulp.temp_file, model->gulp.out_file, task);
}
#define DEBUG_RUN_GULP 0
void gui_gulp_task(struct model_pak *data)
{
  /* checks */
  g_return_if_fail(data != NULL);
  if (!sysenv.gulp_path)
  {
    gui_text_show(ERROR, "GULP executable was not found.\n");
    return;
  }

#if DEBUG_RUN_GULP
  printf("output file: %s\n", data->gulp.out_file);
#endif

  task_new("gulp", &exec_gulp_task, data, &proc_gulp_task, data, data);
}

/* PIDs that must NEVER be killed: 0 (swapper), 1 (init/launchd), 2 (kthreadd/mach_init) */
static gboolean pid_is_protected(gint pid) { return (pid <= 2); }
/* Recursively send a signal to a process tree.
 * Uses pre-collected process table for safe ancestry verification.
 * depth > 0 means recurse into children first (to avoid reparenting issues).
 */
static void kill_descendants_signal(gint target_pid, gint parent_pid, const gint *all_pids, const int *all_parents,
                                    gint total, int sig, gint depth)
{
  /* Find direct children of parent_pid from the table */
  gint children[4096];
  gint child_count = 0;

  for (gint i = 0; i < total; i++)
  {
    if (all_parents[i] == parent_pid && !pid_is_protected(all_pids[i]))
    {
      if (all_parents[i] == target_pid || all_parents[i] == parent_pid)
      {
        children[child_count++] = all_pids[i];
      }
    }
  }

  /* Recurse into children first (depth-first), then signal parent */
  for (gint i = 0; i < child_count; i++)
  {
    kill_descendants_signal(target_pid, children[i], all_pids, all_parents, total, sig, depth + 1);
    kill(children[i], sig);
  }
}
/* Cross-platform helper: get all processes and their PPIDs.
 * Populates pids[] and pparents[] arrays, returns count.
 * Uses "ps -o pid,ppid" which works on both Linux and macOS.
 */
static gint get_all_processes(gint *pids, gint *pparents, gint max_count)
{
  gint count = 0;
#ifdef __APPLE__
  /* macOS: use "ps -o pid,ppid" (no --ppid flag support) */
  FILE *fp = popen("ps -o pid,ppid -x 2>/dev/null", "r");
#else
  /* Linux: use "ps -o pid,ppid -e" */
  FILE *fp = popen("ps -o pid,ppid -e 2>/dev/null", "r");
#endif
  if (!fp)
    return 0;

  gchar line[256];
  /* Skip header line */
  if (!fgets(line, sizeof(line), fp))
  {
    pclose(fp);
    return 0;
  }

  while (fgets(line, sizeof(line), fp) && count < max_count)
  {
    gint pid, ppid;
    if (sscanf(line, "%d %d", &pid, &ppid) == 2)
    {
      if (pid > 0)
      {
        pids[count] = pid;
        pparents[count] = ppid;
        count++;
      }
    }
  }

  pclose(fp);
  return count;
}
/* Recursively SIGKILL a process tree. */
static void kill_descendants_sigkill(gint target_pid, gint parent_pid, const gint *all_pids, const int *all_parents,
                                     gint total)
{
  kill_descendants_signal(target_pid, parent_pid, all_pids, all_parents, total, SIGKILL, 0);
}
/* SIGTERM descendants, wait, then SIGKILL everything.
 * Collects process table once and reuses it for all operations.
 */
static void kill_wait_and_force(gint target_pid)
{
  /* Collect process table once */
  gint all_pids[4096];
  int all_parents[4096];
  gint total = get_all_processes(all_pids, all_parents, 4096);

  /* Give processes a brief moment to terminate gracefully */
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 200000000; /* 200ms */
  nanosleep(&ts, NULL);

  /* Recursively SIGKILL all descendants FIRST */
  kill_descendants_sigkill(target_pid, target_pid, all_pids, all_parents, total);

  /* Then SIGKILL the target itself */
  kill(target_pid, SIGKILL);
}
/* Find all descendants of target_pid by walking the process table.
 * Populates out_pids[] with descendant PIDs, returns count.
 * Uses a two-pass approach: first collect all processes, then filter.
 */
static gint find_descendants(gint target_pid, gint *out_pids, gint max_pids)
{
  /* Collect all processes and their PPIDs */
  gint all_pids[4096];
  int all_parents[4096];
  gint total = get_all_processes(all_pids, all_parents, 4096);

  /* Build a lookup: for each PID, find its index in the array */
  /* We'll use a simple O(n^2) scan since process counts are small */

  /* First pass: find direct children of target_pid */
  gint count = 0;
  for (gint i = 0; i < total && count < max_pids; i++)
  {
    if (all_parents[i] == target_pid && !pid_is_protected(all_pids[i]))
    {
      out_pids[count++] = all_pids[i];
    }
  }

  /* Recursive helper: find children of a given PID from the pre-collected table */
  /* We inline this via a stack-based approach to avoid recursion with shared data */
  gint stack[4096];
  gint stack_top = 0;

  /* Seed stack with direct children */
  for (gint i = 0; i < count; i++)
    stack[stack_top++] = out_pids[i];

  /* Process each node in the stack */
  while (stack_top > 0 && count < max_pids)
  {
    gint parent = stack[--stack_top];

    /* Find children of this parent */
    for (gint i = 0; i < total && count < max_pids; i++)
    {
      if (all_parents[i] == parent && !pid_is_protected(all_pids[i]))
      {
        out_pids[count++] = all_pids[i];
        stack[stack_top++] = all_pids[i];
      }
    }
  }

  return count;
}
void task_kill_running(struct task_pak *task)
{
  if (!task)
    return;

  /* Only attempt to kill if we have a valid async task PID */
  if (task->pid > 0 && task->is_async)
  {
    gint target = task->pid;

    /* Step 1: Verify the target process still exists and is a descendant
     * of our process (not orphaned to init). Collect all processes once
     * for reuse in descendant finding and killing. This prevents killing a
     * recycled PID that happens to match our old task PID. */
    gint all_pids[4096];
    int all_parents[4096];
    gint total = get_all_processes(all_pids, all_parents, 4096);

    if (total > 0)
    {
      /* Find target's PPID from the collected table */
      gint real_ppid = -1;
      for (gint i = 0; i < total; i++)
      {
        if (all_pids[i] == target)
        {
          real_ppid = all_parents[i];
          break;
        }
      }

      if (real_ppid < 0)
      {
        /* Process doesn't exist anymore */
        task->status = KILLED;
        return;
      }

      /* If real_ppid == 1 (init/launchd), the process has been orphaned.
       * It's no longer our descendant - only kill if PID is high
       * enough to be a user process (heuristic). */
      if (real_ppid == 1 && target <= 100)
      {
        /* Skip system processes */
        task->status = KILLED;
        return;
      }

      /* Collect all PIDs that would be killed */
      gint kill_list[4096];
      kill_list[0] = target;
      gint count = 1 + find_descendants(target, kill_list + 1, 4095);

      /* Ask user for confirmation */
      if (task_confirm_callback && !task_confirm_callback(kill_list, count))
      {
        /* User cancelled */
        task->status = RUNNING;
        return;
      }

      /* Send SIGTERM to all descendants first */
      kill_descendants_signal(target, target, all_pids, all_parents, total, SIGTERM, 0);
      /* Wait briefly, then SIGKILL survivors */
      kill_wait_and_force(target);
    }
  }

  task->status = KILLED;
}
gint find_pid_index(gint pid)
{
  gint i;

  for (i = psarray->len - 1; i >= 0 && g_array_index(psarray, struct task_pak, i).pid != pid; i--)
    ;
  return (i);
}
#define DEBUG_ADD_FROM_TREE 0
void add_from_tree(struct task_pak *tdata, gint idx)
{
  struct task_pak *process_ptr;
  gint child;
  div_t h_sec, sec, min;
  gchar *h_sec_pos, *sec_pos, *min_pos, *hour_pos;

  /* add in current process */
  process_ptr = &g_array_index(psarray, struct task_pak, idx);
  tdata->pcpu += process_ptr->pcpu;
  tdata->pmem += process_ptr->pmem;
/* parse the cpu time */
#if defined(__APPLE__) && defined(__MACH__)
  h_sec_pos = g_strrstr(process_ptr->time, ".") + 1;
  sec_pos = h_sec_pos - 3;
  min_pos = process_ptr->time;
  hour_pos = NULL;
#else
  hour_pos = process_ptr->time;
  min_pos = hour_pos + 3;
  sec_pos = min_pos + 3;
  h_sec_pos = NULL;
#endif
  tdata->h_sec += (gint) str_to_float(h_sec_pos);
  h_sec = div(tdata->h_sec, 100);
  tdata->h_sec = h_sec.rem;
  tdata->sec = tdata->sec + h_sec.quot + (gint) str_to_float(sec_pos);
  sec = div(tdata->sec, 60);
  tdata->sec = sec.rem;
  tdata->min = tdata->min + sec.quot + (gint) str_to_float(min_pos);
  min = div(tdata->min, 60);
  tdata->min = min.rem;
  tdata->hour = tdata->hour + min.quot + (gint) str_to_float(hour_pos);
#if DEBUG_ADD_FROM_TREE
  printf("%s hour=%d min=%d, sec=%d, hsec=%d\n", process_ptr->time, tdata->hour, tdata->min, tdata->sec, tdata->h_sec);
#endif

  /* process children */
  for (child = process_ptr->child; EXIST(child); child = g_array_index(psarray, struct task_pak, child).sister)
    add_from_tree(tdata, child);
}
#define DEBUG_CALC_TASK_INFO 0
void calc_task_info(struct task_pak *tdata)
{
  gint num_tokens;
  gint me, parent, sister;
  gint found;
  gchar line[LINELEN], *line_no_time, **buff, *cmd;
  struct task_pak curr_process, *process_ptr;
  struct tm start_tm;
#if DEBUG_CALC_TASK_INFO
  time_t oldest_time;
#endif
  FILE *fp;

#ifdef _WIN32
  /* it's just not worth it ... */
  return;
#endif

  /* dispose of task list if it exists */
  // if (psarray != NULL)
  //   g_array_free(psarray, TRUE);_BUG_ here (SIGSEGV)
  /* global psarray has a _BUG_ in which psarray->data
   * is sometimes filled with invalid values (0x1) ...
   * it happen on rare occasions so it is difficult to
   * FIX. below is a proposal (still testing). --OVHPA */
  if (psarray != NULL)
    psarray = g_array_set_size(psarray, 0);
  else
    psarray = g_array_new(FALSE, TRUE, sizeof(struct task_pak));

  /* initialise process record */
  curr_process.parent = curr_process.child = curr_process.sister = -1;

  /* setup ps command */

  /*
   * SG's - 'ps -Ao "pid ppid pcpu vsz etime comm"
   * 			(unfortunately, no %mem - and vsz is absolute :-(
   */

#ifdef __sgi
  cmd = g_strdup_printf("ps -Ao \"pid ppid pcpu vsz etime comm\"");
#else
  cmd = g_strdup_printf("ps axo \"lstart pid ppid %%cpu %%mem time ucomm\"");
#endif

  /* run ps command */
  fp = popen(cmd, "r");
  g_free(cmd);
  if (!fp)
  {
    gui_text_show(ERROR, "unable to launch ps command\n");
    return;
  }

  /* skip title line */
  if (fgetline(fp, line))
  {
    gui_text_show(ERROR, "unable to read first line of output from ps command\n");
    return;
  }

  /* load data into array */
  // psarray = g_array_new(FALSE, FALSE, sizeof(struct task_pak));
  /* psarray line removed as part of the above _BUG_ */
  while (!fgetline(fp, line))
  {
#if DEBUG_CALC_TASK_INFO
    printf("%s", line);
#endif
/* extract what we want */
#ifdef __WIN32
    /* TODO - win32 replacement? */
    line_no_time = NULL;
#else
    /* first get start time */
    line_no_time = strptime(line, "%c", &start_tm);
#endif
    if (line_no_time == NULL)
    {
      gui_text_show(ERROR, "file produced by ps not understood\n");
      return;
    }
    curr_process.start_time = mktime(&start_tm);
    buff = tokenize(line_no_time, &num_tokens);
    if (num_tokens >= 5)
    {
      curr_process.pid = (int) str_to_float(*(buff + 0));
      curr_process.ppid = (int) str_to_float(*(buff + 1));
      curr_process.pcpu = str_to_float(*(buff + 2));
      curr_process.pmem = str_to_float(*(buff + 3));
      curr_process.time = g_strdup(*(buff + 4));
      g_array_append_val(psarray, curr_process);
    }
    g_strfreev(buff);
  }
  pclose(fp);

  /* Build the process hierarchy. Every process marks itself as first child */
  /* of it's parent or as sister of first child of its parent */
  /* algorithm taken from pstree.c by Fred Hucht */

  for (me = 0; me < psarray->len; me++)
  {
    parent = find_pid_index(g_array_index(psarray, struct task_pak, me).ppid);
    if (parent != me && parent != -1)
    { /* valid process, not me */
      g_array_index(psarray, struct task_pak, me).parent = parent;
      if (g_array_index(psarray, struct task_pak, parent).child == -1) /* first child */
        g_array_index(psarray, struct task_pak, parent).child = me;
      else
      {
        for (sister = g_array_index(psarray, struct task_pak, parent).child;
             EXIST(g_array_index(psarray, struct task_pak, sister).sister);
             sister = g_array_index(psarray, struct task_pak, sister).sister)
          ;
        g_array_index(psarray, struct task_pak, sister).sister = me;
      }
    }
  }

#if DEBUG_CALC_TASK_INFO
  printf("process list\n");
  for (me = 0; me < psarray->len; me++)
  {
    process_ptr = &g_array_index(psarray, struct task_pak, me);
    printf("pid: %d ppid: %d pcpu: %.1f pmem: %.1f date: %s parent: %d child: %d sister: %d time: %s", process_ptr->pid,
           process_ptr->ppid, process_ptr->pcpu, process_ptr->pmem, process_ptr->time, process_ptr->parent,
           process_ptr->child, process_ptr->sister, ctime(&process_ptr->start_time));
  }
#endif

  /* set tdata's pid to 0 so if this routine fails, it won't kill gdis! */
  tdata->pcpu = tdata->pmem = 0;
  tdata->h_sec = tdata->sec = tdata->min = tdata->hour = 0;

/* scan through the running processes and find the task thread */
#if DEBUG_CALC_TASK_INFO
  oldest_time = time(NULL);
#endif
  found = FALSE;
  for (me = 0; me < psarray->len; me++)
  {
    process_ptr = &g_array_index(psarray, struct task_pak, me);
    if (process_ptr->pid == tdata->pid)
    {
      found = TRUE;

#if DEBUG_CALC_TASK_INFO
      printf("pid: %d time:%s\n", tdata->pid, ctime(&oldest_time));
#endif
      break;
    }
  }
  if (found)
  {
    /* find child */
    /*    process_ptr = &g_array_index(psarray, struct task_pak, child_idx);
        child_idx = process_ptr->child;
        process_ptr = &g_array_index(psarray, struct task_pak, child_idx);
        child_pid = tdata->pid = process_ptr->pid;
        #if DEBUG_CALC_TASK_INFO
        printf("pid: %d time:%s\n", child_pid, ctime(&oldest_time));
        #endif*/

    add_from_tree(tdata, me);
  }

  /* FIXME - core dumps here in analysis (esp. if >1 jobs queued) */
  if (tdata->time)
    g_free(tdata->time);

  /*
  sprintf(line, "%02d:%02d:%02d", tdata->hour, tdata->min, tdata->sec);
  tdata->time = g_strdup(line);
  */
  tdata->time = g_strdup_printf("%02d:%02d:%02d", tdata->hour, tdata->min, tdata->sec);

  /* search through any children of the the oldest child */
  /*
  pid = child_pid;
  while ((list = g_slist_find_custom(palist, GINT_TO_POINTER(pid), (gpointer) find_pid)) != NULL)
    {
    curr_process = (struct task_pak *) list->data;
    pid = tdata->pid = curr_process->pid;
    tdata->pcpu += curr_process->pcpu;
    tdata->pmem += curr_process->pmem;
    if (tdata->time)
        g_free(tdata->time);
    tdata->time = g_strdup(curr_process->time);
    }
  #if DEBUG_CALC_TASK_INFO
  printf("FINAL TOTALS\n");
  printf("pid: %d pcpu: %.1f pmem: %.1f time: %s\n", tdata->pid, tdata->pcpu, tdata->pmem, tdata->time);
  #endif

  free_slist(palist);
  */

  /* prevent rounding errors giving >100% */
  if (tdata->pcpu > 100.0)
    tdata->pcpu = 100.0;
  if (tdata->pmem > 100.0)
    tdata->pmem = 100.0;
}

#define DEBUG_EXEC_GAMESS 0
gint exec_gamess(gchar *input, gchar *output, struct task_pak *task)
{
  gchar *cmd, hostname[256];

  /* checks */
  if (!sysenv.gamess_path)
    return (-1);

  /* NEW - acquire hostname for GAMESS job submission */
  /*
  if (gethostname(hostname, sizeof(hostname)))
  */
  sprintf(hostname, "localhost");

#if __WIN32
  {
    gchar *basename, *bat, *fubar, *fname, *temp, *tmp, *dir, *inp, *out;
    FILE *fp;

    /* setup variables */
    basename = parse_strip(input);
    fname = g_strdup_printf("%s.F05", basename);
    fubar = g_build_filename(sysenv.gamess_path, "scratch", fname, NULL);
    temp = g_build_filename(sysenv.gamess_path, "temp", basename, NULL);
    dir = g_build_filename(sysenv.gamess_path, NULL);
    inp = g_build_filename(sysenv.cwd, input, NULL);
    out = g_build_filename(sysenv.cwd, basename, NULL);

    /* create a batch file to run GAMESS */
    bat = gun("bat");
    fp = fopen(bat, "wt");
    fprintf(fp, "@echo off\n");
    fprintf(fp, "echo Running GAMESS using 1 CPU, job: %s\n", input);

    /* remove old crap in the temp directory */
    fprintf(fp, "del %s.*\n", temp);

    /* put the input file in the scratch directory */
    fprintf(fp, "copy \"%s\" \"%s\"\n", inp, fubar);

    /* run the execution script */
    fprintf(fp, "cd %s\n", sysenv.gamess_path);
    fprintf(fp, "csh -f runscript.csh %s 04 1 %s %s > \"%s.gmot\"\n", basename, dir, hostname, out);
    fprintf(fp, "echo GAMESS job completed\n");
    fclose(fp);

    tmp = g_build_filename(sysenv.cwd, bat, NULL);
    cmd = g_strdup_printf("\"%s\"", tmp);

    g_free(basename);
    g_free(fname);
    g_free(fubar);
    g_free(temp);
    g_free(tmp);
    g_free(inp);
    g_free(out);
    g_free(bat);
  }
#else
  cmd = g_strdup_printf("%s/%s %s 00 > %s 2>&1", sysenv.gamess_path, sysenv.gamess_exe, input, output);
#endif

#if DEBUG_EXEC_GAMESS
  printf("executing: [%s]\n", cmd);
#else

  task->is_async = TRUE;
  task_async(cmd, &(task->pid));
#endif

  g_free(cmd);

  return (0);
}
#define DEBUG_EXEC_GAMESS_TASK 0
void exec_gamess_task(struct model_pak *model, struct task_pak *task)
{
  gchar *out;

  /* construct output filename */
  g_free(model->gamess.out_file);

  out = parse_extension_set(model->gamess.temp_file, "gmot");
  model->gamess.out_file = g_build_filename(sysenv.cwd, out, NULL);
  g_free(out);

#if __WIN32
  /*
  out = g_strdup_printf("\"%s\"", model->gamess.out_file);
  */
  out = g_shell_quote(model->gamess.out_file);
  g_free(model->gamess.out_file);
  model->gamess.out_file = out;
#endif

  /* save input file and execute */
  file_save_as(model->gamess.temp_file, model);
  task->status_file = g_strdup(model->gamess.out_file);
  exec_gamess(model->gamess.temp_file, model->gamess.out_file, task);
}
#define DEBUG_PROC_GAMESS 0
void proc_gamess_task(gpointer ptr)
{
  gchar *filename;
  GString *line;
  struct model_pak *data;

  /* TODO - model locking (moldel_ptr RO/RW etc) to prevent screw ups */
  g_return_if_fail(ptr != NULL);
  data = ptr;

  /* win32 fix - which can't make up its mind if it wants to quote or not */
  filename = g_shell_unquote(data->gamess.out_file, NULL);

  if (!g_file_test(filename, G_FILE_TEST_EXISTS))
  {
    printf("Missing output file [%s]\n", filename);
    return;
  }

  if (data->gamess.run_type < GMS_OPTIMIZE && !data->animation)
  {
    /* same model (ie with current energetics dialog) so update */
    file_load(filename, data);
  } else
  {
    /* TODO - make it possile to get dialog data by request */
    /* so that we can check if a dialog exsits to be updated */
    /* get new coords */
    /* create new model for the minimized result */
    file_load(filename, NULL);
  }

  g_free(filename);

  redraw_canvas(ALL);
}
#define DEBUG_RUN_GAMESS 0
void gui_gamess_task(struct model_pak *data)
{
  /* checks */
  g_return_if_fail(data != NULL);
  if (!sysenv.gamess_path)
  {
    gui_text_show(ERROR, "GAMESS executable was not found.\n");
    return;
  }

#if DEBUG_RUN_GAMESS
  printf("output file: %s\n", data->gamess.out_file);
#endif

  task_new("Gamess", &exec_gamess_task, data, &proc_gamess_task, data, data);
}

gint fn_add_valid_shifts(void)
{
  struct model_pak *model;
  struct surface_pak *surfdat = &surfdata.surface;
  struct plane_pak *plane;

  /* checks */
  model = surfdat->model;
  g_return_val_if_fail(model != NULL, 1);
  if (model->periodic != 3)
  {
    gui_text_show(ERROR, "Base model is not 3D periodic.\n");
    return (1);
  }
  if (!surfdata.surface.miller[0] && !surfdata.surface.miller[1] && !surfdata.surface.miller[2])
  {
    gui_text_show(ERROR, "Don't be silly.\n");
    return (2);
  }

  /* does the plane already exist? */
  plane = plane_find(surfdata.surface.miller, model);
  if (!plane)
  {
    plane = plane_new(surfdata.surface.miller, model);
    /* add to list */
    if (plane)
      model->planes = g_slist_append(model->planes, plane);
    else
      return (3);
  }

  /* find valid shifts for the plane */
  /* NB: this REPLACES existing shifts */
  calc_valid_shifts(model, plane);

  return (FALSE);
}
gint fn_add_plane_with_shift(void)
{
  struct model_pak *model, surf;
  struct plane_pak *plane;
  struct shift_pak *shift;

  /* checks */
  model = surfdata.surface.model;
  if (!model)
    return (FALSE);
  if (model->id == MORPH)
    return (FALSE);

  /*
  P3VEC("Adding: ", surfdata.surface.miller);
  */

  /* create new shift with current region values */
  shift = shift_new(surfdata.surface.shift);
  shift->region[0] = surfdata.surface.region[0];
  shift->region[1] = surfdata.surface.region[1];

  /* init for dipole calculation */
  model_init(&surf);
  surf.surface.shift = shift->shift;
  surf.surface.region[0] = 1;
  surf.surface.region[1] = 0;

  /* does the plane already exist? */
  plane = plane_find(surfdata.surface.miller, model);
  if (plane)
  {
    ARR3SET(surf.surface.miller, plane->index);
    generate_surface(model, &surf);

    shift->dipole = surf.gulp.sdipole;
    shift->dipole_computed = TRUE;

    plane->shifts = g_slist_append(plane->shifts, shift);

  } else
  {
    /* create new plane with current hkl */
    plane = plane_new(surfdata.surface.miller, model);
    if (plane)
    {
      ARR3SET(surf.surface.miller, plane->index);
      generate_surface(model, &surf);

      shift->dipole = surf.gulp.sdipole;
      shift->dipole_computed = TRUE;

      plane->shifts = g_slist_append(plane->shifts, shift);

      /* append new plane to model */
      model->planes = g_slist_append(model->planes, plane);
    }
  }

  model_free(&surf);

  return (TRUE);
}
/* if num -> get num ranked faces, else get until Dhkl < min */
#define DEBUG_GET_RANKED_FACES 0
GSList *get_ranked_faces(gint num, gdouble min, struct model_pak *data)
{
  gint n, h, k, l, limit;
  gint f1[3], f2[3];
  gdouble m[3];
  GSList *list = NULL, *plist = NULL, *list1 = NULL, *list2 = NULL;
  struct plane_pak *pdata;

  /* loop over a large range */
  /* FIXME - should loop until num/max is satisfied */
  limit = 4;
  for (h = limit; h >= -limit; h--)
  {
    for (k = limit; k >= -limit; k--)
    {
      for (l = limit; l >= -limit; l--)
      {
        /* skip (000) */
        if (!h && !k && !l)
          continue;
        VEC3SET(f1, h, k, l);
        /* skip things with a common multiple > 1, since */
        /* fn_make_plane() takes care of systematic absences */
        /* NB: this is ok for morph pred. but for XRD we may want such planes */
        if (num)
        {
          n = simplify(f1, 3);
          if (n != 1)
            continue;
        }

        /* add the plane */
        ARR3SET(m, f1);
        pdata = plane_new(m, data);
        if (pdata)
          plist = g_slist_prepend(plist, pdata);
      }
    }
  }
  plist = g_slist_reverse(plist);

  /* sort via Dhkl */
  plist = g_slist_sort(plist, (gpointer) dhkl_compare);

  /*
  VEC3SET(f1, 1, 0, -4);
  printf("f1 : [%d %d %d]\n", f1[0], f1[1], f1[2]);
  n = simplify(f1, 3);
  printf("n = %d\n", n);
  */

  /* mark the planes that are symmetry related */
  list1 = plist;
  while (list1 != NULL)
  {
    /* get a reference (primary) plane */
    pdata = list1->data;
    if (pdata->primary)
    {
      ARR3SET(f1, pdata->index);
    } else
    {
      list1 = g_slist_next(list1);
      continue;
    }
    /* scan for symmetry related planes */
    list2 = g_slist_next(list1);
    while (list2 != NULL)
    {
      pdata = list2->data;
      ARR3SET(f2, pdata->index);
/* get the hkl's and test */
#if DEBUG_GET_RANKED_FACES
      printf("testing: %d %d %d  &  %d %d %d\n", f1[0], f1[1], f1[2], f2[0], f2[1], f2[2]);
#endif
      if (facet_equiv(data, f1, f2))
        pdata->primary = FALSE;

      list2 = g_slist_next(list2);
    }
    list1 = g_slist_next(list1);
  }

  /* write the planes list to the model */
  /* write only up to the requested number */
  list1 = plist;
  n = 0;
  while (list1 != NULL)
  {
    /* get a reference (primary) plane */
    pdata = list1->data;
    /*
      if (pdata->primary)
    */
    {
      /* automatically add valid shifts */
      calc_valid_shifts(data, pdata);

      /* default single shift */
      /*
          sdata = create_shift(0.0);
          pdata->shifts = g_slist_append(pdata->shifts, sdata);
      */

      /* add plane */
      /* TODO - don't prepend? (may be others already there) */
      /*
          list = g_slist_prepend(list, pdata);
      */

      /* number of unique planes */
      if (pdata->primary)
        n++;

      if (num)
      {
        /* got enough unique planes? */
        /*
              if (n == num)
        */

        if (n > num)
          break;
        else
          list = g_slist_prepend(list, pdata);

      } else
      {
        /*
              if (pdata->dhkl < min)
                break;
        */
        if (pdata->dhkl > min)
          list = g_slist_prepend(list, pdata);
      }
    }
    list1 = g_slist_next(list1);
  }
  list = g_slist_reverse(list);
  return (list);
}
void cb_surf_create(struct model_pak *model)
{
  struct plane_pak *plane;
  struct shift_pak *shift;

  /* set up plane */
  plane = plane_new(surfdata.surface.miller, model);
  g_return_if_fail(plane != NULL);

  /* set up shift */
  shift = shift_new(surfdata.surface.shift);
  shift->region[0] = surfdata.surface.region[0];
  shift->region[1] = surfdata.surface.region[1];

  /* create */
  make_surface(model, plane, shift);

  /* avoid BSOD */
  coords_init(INIT_COORDS, model);

  redraw_canvas(SINGLE);

  g_free(plane);
  g_free(shift);
}
#define DEBUG_EXEC_ECALC_TASK 0
void exec_ecalc_task(struct model_pak *model, gpointer data)
{
  gchar *inp;
  struct task_pak *task = data;

  g_assert(model != NULL);
  g_assert(task != NULL);

  /* build the full path filename */
  inp = g_build_filename(sysenv.cwd, model->gulp.temp_file, NULL);
  if (write_gulp(inp, model))
  {
    fprintf(stderr, "[exec_ecalc] write_gulp FAILED, setting KILLED\n");
    task->status = KILLED;
  } else
  {
    if (!model->gulp.no_exec)
    {
      /* NEW */
      task->status_file = g_build_filename(sysenv.cwd, model->gulp.out_file, NULL);

      /* Use bg_exec_gulp which properly waits for GULP's process tree */
      if (bg_exec_gulp(model->gulp.temp_file, model->gulp.out_file, task))
      {
        fprintf(stderr, "[exec_ecalc] bg_exec_gulp FAILED, setting KILLED\n");
        task->status = KILLED;
      } else
      {
        fprintf(stderr, "[exec_ecalc] bg_exec_gulp OK\n");
      }
    }
  }
  g_free(inp);
}
#define DEBUG_PROC_ECALC_TASK 0
void proc_ecalc_task(struct model_pak *model)
{
  gchar *out;
  struct shift_pak *sdata;

  g_assert(model != NULL);
  fprintf(stderr, "[proc_ecalc] ENTERING cleanup task\n");

  /* don't process if gulp execution was turned off */
  if (model->gulp.no_exec)
  {
    fprintf(stderr, "[proc_ecalc] no_exec is set, skipping\n");
    return;
  }

  /* flag that gulp read routine shouldn't prep the model */
  model->grafted = TRUE;

  /* read gulp file — with retry loop to handle race where the output file
   * is still being written when proc_ecalc_task is first called. */
  out = g_build_filename(sysenv.cwd, model->gulp.out_file, NULL);
  FILE *check = fopen(out, "r");
  if (check)
  {
    fseek(check, 0, SEEK_END);
    long fsize = ftell(check);
    fclose(check);
    fprintf(stderr, "[proc_ecalc] initial file size: %ld bytes\n", fsize);
  } else
  {
    fprintf(stderr, "[proc_ecalc] FILE NOT FOUND: %s\n", out);
  }

  /* Retry loop: wait up to 5 seconds for the file to have content */
  int retries = 0;
  const int max_retries = 50;
  while (retries < max_retries)
  {
    check = fopen(out, "r");
    if (check)
    {
      fseek(check, 0, SEEK_END);
      long fsize = ftell(check);
      fclose(check);
      if (fsize > 0)
      {
        fprintf(stderr, "[proc_ecalc] file ready after %d retries: %ld bytes\n", retries, fsize);
        break;
      }
    }
    g_usleep(100 * 1000); /* 100ms */
    retries++;
  }
  if (retries >= max_retries)
  {
    fprintf(stderr, "[proc_ecalc] WARNING: file never had content after %d retries\n", max_retries);
  }

  int read_ret = read_gulp_output(out, model);
  fprintf(stderr, "[proc_ecalc] read_gulp_output(%s) returned %d, esurf[0]=%.4f\n", out, read_ret,
          model->gulp.esurf[0]);
  g_free(out);

#if DEBUG_PROC_ECALC_TASK
  gulp_dump(model);
#else
/*
unlink(model->gulp.temp_file);
unlink(model->gulp.out_file);
printf("inp: %s\n", model->gulp.temp_file);
printf("out: %s\n", model->gulp.out_file);
*/
#endif

  /* feed energy into shift */
  sdata = (model->planes)->data;
  if (sdata)
  {
    sdata->esurf[0] = model->gulp.esurf[0];
    sdata->esurf[1] = model->gulp.esurf[1];
    sdata->eatt[0] = model->gulp.eatt[0];
    sdata->eatt[1] = model->gulp.eatt[1];
    sdata->gnorm = model->gulp.gnorm;
  }

  /* gui update */
  fprintf(stderr, "[proc_ecalc] calling qt_update_shift_energy, esurf[0]=%.4f\n", sdata->esurf[0]);
  /* Qt: update shift energy in surface dialog tree */
  extern void qt_update_shift_energy(struct shift_pak * shift);
  qt_update_shift_energy(sdata);

  /* free model */
  /* NB: this was a borrowed pointer, so we don't want it freed */
  model->planes = NULL;
  model_free(model);
  g_free(model);
}
void new_ecalc_task(struct model_pak *model, struct plane_pak *pdata, struct shift_pak *sdata)
{
  gint r1size;
  GSList *list;
  struct model_pak *surf;
  struct core_pak *core;

  g_assert(model != NULL);
  g_assert(pdata != NULL);
  g_assert(sdata != NULL);

  /* allocate & init new model for the surface */
  surf = g_malloc(sizeof(struct model_pak));
  model_init(surf);
  gulp_data_copy(model, surf);
  surf->id = GULP;
  /* Set basename for gulp file naming */
  surf->basename = g_strdup_printf("%s_%d%d%d_%.4f", model->basename, pdata->index[0], pdata->index[1], pdata->index[2],
                                   sdata->shift);
  fprintf(stderr, "[new_ecalc] surf->basename=%s\n", surf->basename);
  surf->gulp.method = CONV;
  /* NEW - no dependance on this */
  surf->surface.model = NULL;
  surf->surface.shift = sdata->shift;
  surf->surface.region[0] = sdata->region[0];
  surf->surface.region[1] = sdata->region[1];
  ARR3SET(surf->surface.miller, pdata->index);

  /* add the cores */
  generate_surface(model, surf);
  gulp_files_init(surf);

  /* compute surface bulk energy */
  surf->gulp.sbulkenergy = model->gulp.energy;
  surf->gulp.sbulkenergy /= (gdouble) model->num_atoms;
  r1size = 0;
  for (list = surf->cores; list; list = g_slist_next(list))
  {
    core = list->data;
    if (core->region == REGION1A)
      r1size++;
  }
  if (r1size)
    surf->gulp.sbulkenergy *= (gdouble) r1size;
  else
    printf("Warning: empty region 1.\n");

  /* FIXME - cheat, by tacking the shift to update on the *planes* slist */
  g_assert(surf->planes == NULL);
  surf->planes = g_slist_append(surf->planes, sdata);

  task_new("Energy", &exec_ecalc_task, surf, &proc_ecalc_task, surf, NULL);
}

#define DEBUG_SURF_CONV 1
gint surf_conv(FILE *fp, struct model_pak *model, gint type)
{
  gint i, n;
#ifdef UNUSED_BUT_SET
  gint flag;
#endif // UNUSED_BUT_SET
  gint relax, region, r1size, status;
  gdouble sbe, de, old_energy;
  gchar *inp, *out, *full_inp, *full_out;
  GSList *list;
  struct model_pak *surf;
  struct core_pak *core;
#ifdef UNUSED_BUT_SET
  struct plane_pak *plane;
  struct shift_pak *shift;
#endif

  g_assert(model != NULL);

#ifdef UNUSED_BUT_SET
  /* retrieve plane & shift */
  plane = (model->planes)->data;
  shift = (plane->shifts)->data;
#endif

  switch (type)
  {
  case REGION1A:
    region = 0;
    break;
  case REGION2A:
    region = 1;
    break;
  default:
    printf("surf_conv() error: bad region type.\n");
    return (1);
  }

  /* init surface */
  surf = g_malloc(sizeof(struct model_pak));
  model_init(surf);
  gulp_data_copy(model, surf);
  ARR3SET(surf->surface.miller, model->surface.miller);
  surf->surface.region[0] = model->surface.region[0];
  surf->surface.region[1] = model->surface.region[1];
  surf->surface.shift = model->surface.shift;
  surf->surface.converge_eatt = model->surface.converge_eatt;
  surf->surface.converge_r1 = model->surface.converge_r1;
  surf->surface.converge_r2 = model->surface.converge_r2;

  surf->sginfo.lookup = FALSE;

  /* which energy to converge */
  if (model->gulp.run == E_OPTIMIZE)
    relax = 1;
  else
    relax = 0;

  /* surface bulk energy per atom */
  sbe = model->gulp.energy;
  sbe /= (gdouble) model->num_atoms;

/* converge region size */
#ifdef UNUSED_BUT_SET
  n = flag = 0;
#else  // UNUSED_BUT_SET
  n = 0;
#endif // UNUSED_BUT_SET
  old_energy = 0.0;
  // de = 99999999.9;/*FIX e176b9*/
  status = 0;
  for (;;)
  {
    /* FIXME - any other stuff to free? */
    free_core_list(surf);
    generate_surface(model, surf);

    /* compute surface bulk energy */
    r1size = 0;
    for (list = surf->cores; list; list = g_slist_next(list))
    {
      core = list->data;
      if (core->region == REGION1A)
        r1size++;
    }
    if (!r1size)
      printf("Warning: empty region 1.\n");

    surf->gulp.sbulkenergy = sbe * (gdouble) r1size;

    /* create appropriate name */
    inp =
        g_strdup_printf("%s_%d_%d.gin", surf->basename, (gint) surf->surface.region[0], (gint) surf->surface.region[1]);

    out =
        g_strdup_printf("%s_%d_%d.got", surf->basename, (gint) surf->surface.region[0], (gint) surf->surface.region[1]);

    full_inp = g_build_filename(sysenv.cwd, inp, NULL);
    full_out = g_build_filename(sysenv.cwd, out, NULL);

    /* command cycle: write input, execute, read output */
    /*
    if (0)
    */
    for (i = 0; i < 3; i++)
    {
      if (!status)
      {
        switch (i)
        {
        case 0:
          status = write_gulp(full_inp, surf);
          break;

        case 1:
          status = exec_gulp(inp, out);
          if (!status)
            g_unlink(full_inp);
          /*
          printf("input: %s\n", full_inp);
          */
          break;

        case 2:
          /* read_gulp_out() can call gui_text_show() which modifies */
          /* the message widget, hence the thread lock is required */
          ensure_threads_mutex();
          g_mutex_lock(gdis_threads_mutex);
          /* Retry: wait for output file to have content */
          {
            int r = 0;
            for (; r < 50; r++)
            {
              FILE *chk = fopen(full_out, "r");
              if (chk)
              {
                fseek(chk, 0, SEEK_END);
                long sz = ftell(chk);
                fclose(chk);
                if (sz > 0)
                  break;
              }
              g_usleep(100 * 1000);
            }
          }
          status = read_gulp_output(full_out, surf);
          if (!status)
            g_unlink(full_out);
          /*
          printf("output: %s\n", full_out);
          */
          g_mutex_unlock(gdis_threads_mutex);
          break;
        }
      }
    }

    /* remove old files for next cycle */
    g_free(full_inp);
    g_free(full_out);
    g_free(inp);
    g_free(out);

    /* get difference */
    if (surf->surface.converge_eatt)
    {
      de = surf->gulp.eatt[relax] - old_energy;
      old_energy = surf->gulp.eatt[relax];
    } else
    {
      de = surf->gulp.esurf[relax] - old_energy;
      old_energy = surf->gulp.esurf[relax];
    }

    /* bad convergence checks */
    if (n > 10 && de > 0.1)
    {
      fprintf(fp, "ERROR: failed convergence cycle.\n");
      status = 1;
      break;
    }

    if (surf->surface.converge_eatt)
    {
      fprintf(fp, "[%d:%d]  Eatt = %f (%f)\n", (gint) surf->surface.region[0], (gint) surf->surface.region[1],
              surf->gulp.eatt[relax], de);
    } else
    {
      fprintf(fp, "[%d:%d] Esurf = %f (%f)\n", (gint) surf->surface.region[0], (gint) surf->surface.region[1],
              surf->gulp.esurf[relax], de);
    }
    fflush(fp);

    /* convergence check */
    if (surf->surface.converge_eatt)
    {
      if (fabs(de) < MAX_DEEATT)
        break;
    } else
    {
      if (fabs(de) < MAX_DESURF)
        break;
    }

    /* next size */
    surf->surface.region[region]++;
    n++;
  }
  if (!status)
    fprintf(fp, "Region %d converged.\n", region + 1);
  fflush(fp);

  /* we can go back one size, due to the way convergence is checked */
  surf->surface.region[region]--;

  /* transfer data to source model */
  model->surface.region[region] = surf->surface.region[region];
  model->gulp.eatt[region] = surf->gulp.eatt[region];
  model->gulp.esurf[region] = surf->gulp.esurf[region];
  model->gulp.gnorm = surf->gulp.gnorm;

  /* cleanup */
  model_free(surf);
  g_free(surf);

  return (status);
}
#define DEBUG_EXEC_REGCON_TASK 0
void exec_regcon_task(struct model_pak *model, struct task_pak *task)
{
  gint m, r, run;
  gchar *name, *tmp;
  struct plane_pak *plane;
  FILE *fp;

  /* checks */
  g_assert(model != NULL);
  g_assert(model->planes != NULL);
  g_assert(model->periodic == 3);

  /* NEW - status file */
  tmp = gun("txt");
  name = g_build_filename(sysenv.cwd, tmp, NULL);
  g_free(tmp);
  fp = fopen(name, "wt");
  if (fp)
    task->status_file = name;
  else
  {
    fp = stdout;
    g_free(name);
  }

  /* retrieve plane & shift */
  plane = (model->planes)->data;

  /* estimate starting region sizes from the dspacing for the plane */
  g_return_if_fail(plane != NULL);

  /* mult dhkl by the gcd */
  m = GCD(plane->index[0], GCD(plane->index[1], plane->index[2]));

  if (plane->dhkl < MIN_THICKNESS)
  {
    /* FIXME - what to do if dhkl is very small or even zero */
    r = 1 + (gint) (MIN_THICKNESS / (m * plane->dhkl));

    if (r > model->surface.region[0])
      model->surface.region[0] = r;
    if (r > model->surface.region[1])
      model->surface.region[1] = r;
  }

  fprintf(fp, "----------------------------------------------\n");
  fprintf(fp, "         Miller: %d %d %d \n", plane->index[0], plane->index[1], plane->index[2]);
  fprintf(fp, "           Dhkl: %f\n", plane->dhkl);
  fprintf(fp, "          Shift: %f\n", model->surface.shift);
  fprintf(fp, "Initial regions: %d , %d\n", (gint) model->surface.region[0], (gint) model->surface.region[1]);
  fprintf(fp, "    Convergence: ");
  if (model->surface.converge_eatt)
    fprintf(fp, "Attachment Energy based\n");
  else
    fprintf(fp, "Surface Energy based\n");
  fprintf(fp, "----------------------------------------------\n");

  /* save run type */
  run = model->gulp.run;

#if DEBUG_EXEC_REGCON_TASK
  printf("run type %i\n", run);
  printf("converge r1 %i\n", model->surface.converge_r1);
  printf("converge r2 %i\n", model->surface.converge_r2);
#endif

  fprintf(fp, "Beginning unrelaxed convergence...\n");

  /* converge region 2 size */
  model->gulp.run = E_SINGLE;
  if (model->surface.converge_r2)
  {
    if (surf_conv(fp, model, REGION2A))
    {
      task->message = g_strdup("Failed unrelaxed convergence of region 2.\n");
      return;
    }
  } else
    fprintf(fp, "Skipping region 2 convergence...\n");

  /* converge region 1 size */
  if (model->surface.converge_r1)
  {
    if (surf_conv(fp, model, REGION1A))
    {
      task->message = g_strdup("Failed unrelaxed convergence of region 1.\n");
      return;
    }
  } else
    fprintf(fp, "Skipping region 1 convergence...\n");

  /* if opti is specified - repeat with unrelaxed regions as starting point */
  if (run == E_OPTIMIZE)
  {
    model->gulp.run = E_OPTIMIZE;

    fprintf(fp, "Beginning relaxed convergence...\n");
    fflush(fp);

    /* converge region 2 size */
    if (model->surface.converge_r2)
    {
      if (surf_conv(fp, model, REGION2A))
      {
        if (task)
          task->message = g_strdup("Failed relaxed convergence of region 2.\n");
        return;
      }
      fflush(fp);
    } else
      fprintf(fp, "Skipping region 2 convergence...\n");

    /* converge region 1 size */
    if (model->surface.converge_r1)
    {
      if (surf_conv(fp, model, REGION1A))
      {
        if (task)
          task->message = g_strdup("Failed relaxed convergence of region 1.\n");
        return;
      }
      fflush(fp);
    } else
      printf("Skipping region 1 convergence...\n");
  }
  fclose(fp);
}
#define DEBUG_PROC_REGON_TASK 0
void proc_regcon_task(struct model_pak *model)
{
  struct plane_pak *plane;
  struct shift_pak *shift;

  /* checks */
  g_assert(model != NULL);
  plane = (model->planes)->data;
  g_assert(plane != NULL);
  shift = (plane->shifts)->data;
  g_assert(shift != NULL);

  /* the shift pointer belongs to the main model, so */
  /* we don't want it free'd when model is destroyed */
  plane->shifts = NULL;

#if DEBUG_PROC_REGON_TASK
  printf("Final regions: %d , %d\n", (gint) model->surface.region[0], (gint) model->surface.region[1]);
  printf("Final Esurf: %f , %f\n", model->gulp.esurf[0], model->gulp.esurf[1]);
  printf("Final Eatt: %f , %f\n", model->gulp.eatt[0], model->gulp.eatt[1]);
  printf("Final gnorm: %f\n", model->gulp.gnorm);
#endif

  shift->region[0] = model->surface.region[0];
  shift->region[1] = model->surface.region[1];
  shift->esurf[0] = model->gulp.esurf[0];
  shift->esurf[1] = model->gulp.esurf[1];
  shift->eatt[0] = model->gulp.eatt[0];
  shift->eatt[1] = model->gulp.eatt[1];
  shift->gnorm = model->gulp.gnorm;

  /* update gui */
  /* Qt: update shift values in surface dialog tree */
  extern void qt_update_shift_energy(struct shift_pak * shift);
  qt_update_shift_energy(shift);

  shift->locked = FALSE;

  /* cleanup */
  model_free(model);
  g_free(model);
}
#define DEBUG_NEW_REGCON_TASK 0
void new_regcon_task(struct model_pak *model, struct plane_pak *plane, struct shift_pak *shift)
{
  GSList *list;
  struct core_pak *core;
  struct plane_pak *plane2;
  struct model_pak *temp;

  /* checks */
  g_assert(model != NULL);
  g_assert(plane != NULL);
  g_assert(shift != NULL);

#if DEBUG_NEW_REGCON_TASK
  printf("new_regcon_task\n");
  printf("model->surface.converge_r1 %i\n", model->surface.converge_r1);
  printf("model->surface.converge_r2 %i\n", model->surface.converge_r2);
#endif

  /* NEW - prevent deletion */
  shift->locked = TRUE;

  /* duplicate the data passed, since it may be changed */
  /* or destroyed by the user while the task is still queued */
  temp = g_malloc(sizeof(struct model_pak));
  model_init(temp);

  /* duplicate source model data */
  temp->gulp.energy = model->gulp.energy;
  gulp_data_copy(model, temp);
  temp->cores = dup_core_list(model->cores);
  /* FIXME - this is ugly, but the only way (currently) to do it, */
  /* as dup_core_list dups shells - but doesn't add them to a list */
  for (list = temp->cores; list; list = g_slist_next(list))
  {
    core = list->data;
    if (core->shell)
      temp->shels = g_slist_prepend(temp->shels, core->shell);
  }

  /* TODO - implement a copy_lattice_info() primitive */
  temp->periodic = model->periodic;
  temp->fractional = model->fractional;
  memcpy(temp->pbc, model->pbc, 6 * sizeof(gdouble));
  memcpy(temp->latmat, model->latmat, 9 * sizeof(gdouble));
  memcpy(temp->ilatmat, model->ilatmat, 9 * sizeof(gdouble));
  temp->sginfo.spacename = g_strdup(model->sginfo.spacename);
  temp->sginfo.cellchoice = model->sginfo.cellchoice;

  /* always use name for lookup */
  temp->sginfo.spacenum = -1;

  if (model->surface.ignore_bonding)
    temp->build_molecules = FALSE;

  /* TODO - enforce unfragment() ??? */
  model_prep(temp);

  /*
  temp->fractional = TRUE;
  zone_init(temp);
  connect_bonds(temp);
  connect_molecules(temp);
  */

  /* NEW - no dependence once we've spawned the task */
  temp->surface.converge_eatt = model->surface.converge_eatt;
  temp->surface.converge_r1 = model->surface.converge_r1;
  temp->surface.converge_r2 = model->surface.converge_r2;
  temp->surface.model = NULL;
  temp->surface.shift = shift->shift;
  temp->surface.region[0] = shift->region[0];
  temp->surface.region[1] = shift->region[1];
  ARR3SET(temp->surface.miller, plane->index);

  /* append the required plane and shift to the temporary model */
  plane2 = plane_new(temp->surface.miller, model);
  plane2->shifts = g_slist_append(plane2->shifts, shift);
  g_assert(temp->planes == NULL);
  temp->planes = g_slist_append(temp->planes, plane2);

  task_new("Regcon", &exec_regcon_task, temp, &proc_regcon_task, temp, NULL);
}
gint sculpt_image_test(gint *t, gdouble scale, struct model_pak *model)
{
  gdouble r[3], n[3];
  GSList *list;
  struct plane_pak *plane;

  /* convert periodic image to cartesian point */
  /* NB: ensure we get the closest lattice point to the origin */
  if (t[0] < 0)
    r[0] = t[0] + 1;
  else
    r[0] = t[0];

  if (t[1] < 0)
    r[1] = t[1] + 1;
  else
    r[1] = t[1];

  if (t[2] < 0)
    r[2] = t[2] + 1;
  else
    r[2] = t[2];

  vecmat(model->latmat, r);

  /* compare against planes to see if this image is excluded */
  for (list = model->planes; list; list = g_slist_next(list))
  {
    plane = list->data;

    /* get cartesian normal */
    ARR3SET(n, plane->index);
    vecmat(model->rlatmat, n);
    normalize(n, 3);

    /* test dot product against required distance to plane */
    ARR3MUL(n, r);
    if ((n[0] + n[1] + n[2]) > scale * plane->f[0])
    {
      return (FALSE);
    }
  }

  return (TRUE);
}
#define DEBUG_SCULPT_CREATE 0
void sculpt_model_create(struct model_pak *model)
{
  gint i, t[3], limit[3];
  gdouble scale, r = 1.0, s, rmin, x[3], n[3];
  GSList *list, *clist, *slist, *plist;
  struct model_pak *dest;
  struct core_pak *core;
  struct shel_pak *shell;
  struct mol_pak *mol;
  struct plane_pak *plane;
  struct spatial_pak *spatial;

  /* checks */
  g_assert(model != NULL);
  if (model->periodic != 3)
  {
    gui_text_show(ERROR, "Source model is not 3D periodic.\n");
    return;
  }
  if (!model->planes)
  {
    gui_text_show(ERROR, "No cleavage planes supplied.\n");
    return;
  }
#if DEBUG_SCULPT_CREATE
  else
    printf("Planes: %d\n", g_slist_length(model->planes));
#endif

  /* compute required periodic images (assumed symmetric) */
  /* FIXME - most of the nuclei shape problems come from an insufficient number of repeats */
  VEC3SET(limit, 0, 0, 0);
  for (i = 0; i < (gint) model->periodic; i++)
    limit[i] = 1 + sculpt_length / model->pbc[i];

  /* init destination model for sculpture */
  dest = model_new();
  if (!dest)
  {
    gui_text_show(ERROR, "Failed to allocate for new model.\n");
    return;
  }
  model_init(dest);
  gulp_data_copy(model, dest);

#if DEBUG_SCULPT_CREATE
  printf("maximum length: %f\n", sculpt_length);
  printf("periodic images: %d %d %d\n", limit[0], limit[1], limit[2]);
#endif

  /* TODO - going to have to put this ABOVE the sculpt_image_test() call, so we get rmin */
  /* morphology computation */
  morph_build(model);
  /* compute the facet closest to the center */
  rmin = G_MAXDOUBLE;
  for (plist = model->planes; plist; plist = g_slist_next(plist))
  {
    plane = plist->data;

    /* ensure we've got the best shift value data sitting in the plane structure */
    update_plane_energy(plane, model);

    /* compute distance to plane facet */
    /* NEW - a little naughty, but using the f[] (structure factor) to store the length/shift */
    /* value of the plane in the nuclei sculpting so we dont keep recalculating */
    switch (model->morph_type)
    {
    case EQUIL_UN:
      plane->f[0] = plane->esurf[0];
      plane->f[1] = plane->esurf_shift;
      break;
    case GROWTH_UN:
      plane->f[0] = fabs(plane->eatt[0]);
      plane->f[1] = plane->eatt_shift;
      break;
    case EQUIL_RE:
      plane->f[0] = plane->esurf[1];
      plane->f[1] = plane->esurf_shift;
      break;
    case GROWTH_RE:
      plane->f[0] = fabs(plane->eatt[1]);
      plane->f[1] = plane->eatt_shift;
      break;
    case DHKL:
    default:
      plane->f[0] = 1.0 / plane->dhkl;
      plane->f[1] = 0.0;
      break;
    }
    /* NEW - stop zero result (failed/bad calc) from messing things up */
    if (plane->f[0] != 0.0 && plane->f[0] < rmin)
      rmin = plane->f[0];
  }

#if DEBUG_SCULPT_CREATE
  printf("rmin = %f\n", rmin);
#endif

  scale = 0.5 * sculpt_length / rmin;

  /* periodic image creation */
  for (t[0] = -limit[0]; t[0] <= limit[0]; t[0]++)
  {
    for (t[1] = -limit[1]; t[1] <= limit[1]; t[1]++)
    {
      for (t[2] = -limit[2]; t[2] <= limit[2]; t[2]++)
      {
        /* test vertices of the t[] image against planes */
        if (!sculpt_image_test(t, scale, model))
          continue;

        /* duplicate cores, add periodic image offset & convert to cartesian */
        clist = dup_core_list(model->cores);
        slist = dup_shell_list(model->shels);
        dest->cores = g_slist_concat(dest->cores, clist);
        dest->shels = g_slist_concat(dest->shels, slist);
        for (list = clist; list; list = g_slist_next(list))
        {
          core = list->data;
          core->primary = TRUE;
          ARR3ADD(core->x, t);
          vecmat(model->latmat, core->x);
        }
        for (list = slist; list; list = g_slist_next(list))
        {
          shell = list->data;
          shell->primary = TRUE;
          ARR3ADD(shell->x, t);
          vecmat(model->latmat, shell->x);
        }
      }
    }
  }

  /* initialize connectivity and core-shell links for the new model */
  zone_init(dest);
  connect_bonds(dest);
  connect_molecules(dest);
  shell_make_links(dest);

  /* plane cutoff tests */
  for (plist = model->planes; plist; plist = g_slist_next(plist))
  {
    plane = plist->data;

    /* get cartesian distance to plane facet */
    r = plane->f[0] * scale;
    s = plane->f[1];

    /* attempt to create the correct (ie surface shift) termination */
    if (model->sculpt_shift_use)
    {
      /*
        g = GCD(GCD(plane->index[0], plane->index[1]), GCD(plane->index[1], plane->index[2]));
      */
      r /= plane->dhkl;

      /* TODO - if nearest_int(r) == 0 -> set to 1 */
      s += nearest_int(r);
      r = s * plane->dhkl;
    }

    /* get cartesian normal */
    ARR3SET(n, plane->index);
    vecmat(model->rlatmat, n);
    normalize(n, 3);

    /* project coords onto plane normal & remove if greater than distance to facet */
    /* TODO - do cutoff by molecule centroid when combining with periodic image loop */
    for (list = dest->moles; list; list = g_slist_next(list))
    {
      mol = list->data;

      ARR3SET(x, mol->centroid);
      ARR3MUL(x, n);
      if ((x[0] + x[1] + x[2]) > r)
      {
        for (clist = mol->cores; clist; clist = g_slist_next(clist))
        {
          core = clist->data;
          core->status |= DELETED;
          if (core->shell)
          {
            shell = core->shell;
            shell->status |= DELETED;
          }
        }
      }
    }
  }

  /* transfer the spatials from the source model morphology to the nuclei */
  list = model->spatial;
  while (list)
  {
    spatial = list->data;
    list = g_slist_next(list);

    /* search for all morphology related spatials */
    /* a bit crude... */
    if (g_strrstr(spatial->label, "(") && g_strrstr(spatial->label, ")"))
    {
      dest->spatial = g_slist_prepend(dest->spatial, spatial);
      model->spatial = g_slist_remove(model->spatial, spatial);
      spatial->method = GL_LINE_LOOP;

      /* mult vertices by len */
      for (plist = spatial->list; plist; plist = g_slist_next(plist))
      {
        struct vec_pak *vec = plist->data;

        ARR3SET(vec->colour, sysenv.render.fg_colour);
        /* convert fractional vertices to cartesian */
        vecmat(model->latmat, vec->x);
        /* scale the morphology to match the constructed nuclei */
        VEC3MUL(vec->x, scale);
      }
    }
  }

  /* init for display */
  delete_commit(dest);
  model_prep(dest);
  /* model_new() already added dest to sysenv.mal */
  fprintf(stderr, "[sculpt_model_create] creating nuclei: basename=%s mal_count=%d\n", dest->basename,
          g_slist_length(sysenv.mal));
  sysenv.active_model = dest;
  tree_model_add(dest);
  redraw_canvas(ALL);
}

void make_morph(void)
{
  gint status;
  gchar *filename;
  /*GSList *plist;*/
  struct model_pak *data;
  /*struct plane_pak *pdata;*/

  /* get the current model */
  data = surfdata.surface.model;
  if (!data)
    return;

  /* create a unique name per morphology type */
  const char *morph_suffix = "Dhkl";
  switch (data->morph_type)
  {
  case EQUIL_UN:
    morph_suffix = "Esurf_unrelaxed";
    break;
  case EQUIL_RE:
    morph_suffix = "Esurf_relaxed";
    break;
  case GROWTH_UN:
    morph_suffix = "Eatt_unrelaxed";
    break;
  case GROWTH_RE:
    morph_suffix = "Eatt_relaxed";
    break;
  case MORPH_BBPA:
    morph_suffix = "BrokenBonds";
    break;
  default:
    morph_suffix = "Dhkl";
    break;
  }
  filename = g_strdup_printf("%s_morph_%s.gmf", data->basename, morph_suffix);

  /* store the planes */
  write_gmf(filename, data);

  /* read morphology file in & compute hull */
  data = model_new();
  /* default morphology computation type */
  data->morph_type = DHKL;

  /* only successful if properly loaded */
  status = 2;
  if (data)
  {
    status--;
    if (!read_gmf(filename, data))
      status--;
  }

  if (!status)
  {
    /* model_new() already added data to sysenv.mal, just set active and display */
    fprintf(stderr, "[make_morph] creating morphology: basename=%s mal_count=%d\n", data->basename,
            g_slist_length(sysenv.mal));
    sysenv.active_model = data;
    tree_model_add(data);
    coords_init(CENT_COORDS, data);
    redraw_canvas(ALL);
  }

  g_free(filename);
}
