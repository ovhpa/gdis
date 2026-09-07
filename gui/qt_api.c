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
 * Qt API bridge — thin C wrapper exposing GDIS data to Qt GUI
 */

#include <stdio.h>
#include "gdis.h"
#include "matrix.h"
#include "coords.h"
#include "space.h"
#include "edit.h"
#include "interface.h"
#include "render.h"
#include "gui_shorts.h"
#include "model.h"
#include "gdis_api.h"

/* sysenv is defined in main.c */
extern struct sysenv_pak sysenv;

/* Qt output append callback — set by Qt GUI initialization */
typedef void (*qt_output_cb_t)(const gchar *, gint);
static qt_output_cb_t qt_output_callback = NULL;
void qt_set_output_callback(qt_output_cb_t cb) { qt_output_callback = cb; }

/* Qt canvas refresh callback — set by Qt GUI initialization */
typedef void (*qt_refresh_cb_t)(void);
static qt_refresh_cb_t qt_refresh_callback = NULL;
void qt_set_refresh_callback(qt_refresh_cb_t cb) { qt_refresh_callback = cb; }

/* Qt edit field text buffer — updated by Qt QLineEdit widgets.
 * Indexed by the C enum type values (ELEMENT=105, NAME=13, etc.),
 * so pointers remain valid across the enum-based switch in atom_properties_change. */
static gchar *qt_edit_text_storage[256];
const gchar *qt_edit_text[256];

/* ===== Render mode functions ===== */
void qt_render_mode_set(gint mode);
void qt_render_mode_polyhedral(void);
void qt_render_mode_zone(void);
void qt_render_wire_atoms(void);
void qt_render_solid_atoms(void);

/* ===== Edit field type enum values ===== */
/* These match the enum values in gdis.h for atom_properties_change */
gint qt_edit_type_element(void) { return ELEMENT; }
gint qt_edit_type_name(void) { return NAME; }
gint qt_edit_type_core_ff(void) { return CORE_FF; }
gint qt_edit_type_coord_x(void) { return COORD_X; }
gint qt_edit_type_coord_y(void) { return COORD_Y; }
gint qt_edit_type_coord_z(void) { return COORD_Z; }
gint qt_edit_type_charge(void) { return CHARGE; }
gint qt_edit_type_weight(void) { return WEIGHT; }
gint qt_edit_type_sof(void) { return SOF; }
gint qt_edit_type_growth(void) { return CORE_GROWTH_SLICE; }
gint qt_edit_type_region(void) { return CORE_REGION; }
gint qt_edit_type_translate(void) { return CORE_TRANSLATE; }

/* ===== Operation mode enum values (mirrored from interface.h) ===== */
gint qt_mode_free(void) { return FREE; }
gint qt_mode_atom_add(void) { return ATOM_ADD; }
gint qt_mode_bond_single(void) { return BOND_SINGLE; }
gint qt_mode_bond_delete(void) { return BOND_DELETE; }

/* Render mode enum values */
gint qt_render_stick(void) { return STICK; }
gint qt_render_ball_stick(void) { return BALL_STICK; }
gint qt_render_cpk(void) { return CPK; }
gint qt_render_liquorice(void) { return LIQUORICE; }
gint qt_render_polyhedral(void) { return POLYHEDRAL; }
gint qt_render_zone(void) { return ZONE; }

/* ===== Element data ===== */
extern struct elem_pak elements[];

const char *qt_elem_symbol(gint code) { return elements[code].symbol; }

/* ===== Core data accessors ===== */
const char *qt_core_label(struct core_pak *core) { return core->atom_label; }

const char *qt_core_type(struct core_pak *core) { return core->atom_type; }

gint qt_core_code(struct core_pak *core) { return core->atom_code; }

gdouble qt_core_x(struct core_pak *core, gint i) { return core->x[i]; }

gdouble qt_core_charge(struct core_pak *core) { return core->charge; }

gdouble qt_core_mass(struct core_pak *core) { return core->mass; }

gdouble qt_core_sof(struct core_pak *core) { return core->sof; }

gint qt_core_has_sof(struct core_pak *core) { return core->has_sof; }

gint qt_core_growth(struct core_pak *core) { return core->growth; }

gint qt_core_region(struct core_pak *core) { return core->region; }

gint qt_core_translate(struct core_pak *core) { return core->translate; }

/* ===== Model data accessors ===== */
gint qt_model_periodic(struct model_pak *model) { return model->periodic; }

gint qt_model_num_atoms(struct model_pak *model) { return g_slist_length(model->cores); }

gdouble qt_model_pbc(struct model_pak *model, gint i) { return model->pbc[i]; }

gdouble qt_model_area(struct model_pak *model) { return model->area; }

gdouble qt_model_volume(struct model_pak *model) { return model->volume; }

const char *qt_model_sgname(struct model_pak *model) { return model->sginfo.spacename; }

const char *qt_model_latticename(struct model_pak *model) { return model->sginfo.latticename; }

/* ===== Selection ===== */
gpointer qt_model_selection_head(struct model_pak *model) { return model->selection ? model->selection->data : NULL; }

gint qt_model_selection_count(struct model_pak *model) { return g_slist_length(model->selection); }

gpointer qt_selection_nth(struct model_pak *model, gint n)
{
  GSList *list;
  for (list = model->selection; list; list = g_slist_next(list))
  {
    if (n == 0)
      return list->data;
    n--;
  }
  return NULL;
}

/* ===== Content refresh ===== */
void qt_model_content_refresh(struct model_pak *model) { model_content_refresh(model); }

void qt_model_prep(struct model_pak *model) { model_prep(model); }

void qt_redraw_canvas_full(gint mode) { redraw_canvas(mode); }

/* Model list access */
gpointer qt_get_mal_list(void) { return (gpointer) sysenv.mal; }

/* Check if a model is still in the active model list */
gboolean qt_model_valid(struct model_pak *model)
{
  GSList *list;
  for (list = sysenv.mal; list; list = g_slist_next(list))
  {
    if (list->data == model)
      return TRUE;
  }
  return FALSE;
}
/* Apply atom property change from Qt */
void qt_atom_properties_change(gint type)
{
  if (type < 0 || type >= 256)
    return;
  const gchar *text = qt_edit_text[type];
  if (!text || !*text)
    return;

  /* Call the C function to apply the change */
  atom_properties_change(type);
}

/* Set edit field text from Qt — stores a persistent copy.
 * type is the C enum type (ELEMENT, NAME, etc.), not a panel index. */
void qt_set_edit_field_text(gint type, const gchar *text)
{
  if (type < 0 || type >= 256)
    return;
  g_free(qt_edit_text_storage[type]);
  qt_edit_text_storage[type] = g_strdup(text ? text : "");
  qt_edit_text[type] = qt_edit_text_storage[type];
}

/* Get edit field text */
const gchar *qt_get_edit_field_text(gint type)
{
  if (type < 0 || type >= 256)
    return NULL;
  return qt_edit_text[type];
}

/* Free all edit field text storage */
void qt_free_edit_text(void)
{
  for (int i = 0; i < 256; i++)
  {
    g_free(qt_edit_text_storage[i]);
    qt_edit_text_storage[i] = NULL;
    qt_edit_text[i] = NULL;
  }
}

/* Local declarations for render functions in gui_render.c */
extern void render_mode_set(gpointer);
extern void render_mode_polyhedral(void);
extern void render_mode_zone(void);
extern void render_wire_atoms(void);
extern void render_solid_atoms(void);

/* ===== Render mode function wrappers ===== */
void qt_render_mode_set(gint mode) { render_mode_set(GINT_TO_POINTER(mode)); }
void qt_render_mode_polyhedral(void) { render_mode_polyhedral(); }
void qt_render_mode_zone(void) { render_mode_zone(); }
void qt_render_wire_atoms(void) { render_wire_atoms(); }
void qt_render_solid_atoms(void) { render_solid_atoms(); }

/* ===== Periodic image spinner values ===== */
/* image_limit[6]: [-x,+x, -y,+y, -z,+z] */
gint qt_image_spinner_values[6];

void qt_image_spinner_set(gint axis, gint direction, gint value)
{
  if (axis < 0 || axis > 2)
    return;
  if (direction < 0 || direction > 5)
    return;
  qt_image_spinner_values[direction] = value;

  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (!model)
    return;

  model->image_limit[direction] = (gdouble) value;

  /* Recreate periodic images and redraw */
  space_make_images(CREATE, model);
  coords_init(CENT_COORDS, model);
  model->need_clear = TRUE;
  redraw_canvas(SINGLE);
}

gint qt_image_spinner_get(gint direction)
{
  if (direction < 0 || direction > 5)
    return 0;
  return qt_image_spinner_values[direction];
}

/* Update Qt spinner widgets from stored values */
extern void qt_image_spinner_update_widgets(void);

/* Sync spinner values from model's image_limit and update widgets */
void qt_image_spinner_sync(void)
{
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (!model)
    return;

  for (int i = 0; i < 6; i++)
  {
    qt_image_spinner_values[i] = (gint) model->image_limit[i];
  }
  qt_image_spinner_update_widgets();
}

/* ===== Symmetry table population ===== */
extern void qt_set_symmetry_table(gpointer, const gchar **, const gchar **, gint);

void qt_populate_symmetry_table(void)
{
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (!model)
    return;

  GString *labels[12];
  GString *values[12];
  gint count = 0;

  if (!model->periodic)
  {
    /* Clear table for non-periodic models */
    extern void qt_set_symmetry_table(gpointer, const gchar **, const gchar **, gint);
    qt_set_symmetry_table(NULL, NULL, NULL, 0);
    return;
  }

  /* 3D: space group info */
  if (model->periodic == 3)
  {
    labels[count] = g_string_new("space group");
    values[count] = g_string_new(model->sginfo.spacename);
    count++;
    labels[count] = g_string_new("system");
    values[count] = g_string_new(model->sginfo.latticename);
    count++;
    labels[count] = g_string_new("a");
    values[count] = g_string_new("");
    g_string_printf(values[count], "%8.4f", model->pbc[0]);
    count++;
    labels[count] = g_string_new("b");
    values[count] = g_string_new("");
    g_string_printf(values[count], "%8.4f", model->pbc[1]);
    count++;
    labels[count] = g_string_new("c");
    values[count] = g_string_new("");
    g_string_printf(values[count], "%8.4f", model->pbc[2]);
    count++;
    labels[count] = g_string_new("alpha");
    values[count] = g_string_new("");
    g_string_printf(values[count], "%8.2f", model->pbc[3] * R2D);
    count++;
    labels[count] = g_string_new("beta");
    values[count] = g_string_new("");
    g_string_printf(values[count], "%8.2f", model->pbc[4] * R2D);
    count++;
    labels[count] = g_string_new("gamma");
    values[count] = g_string_new("");
    g_string_printf(values[count], "%8.2f", model->pbc[5] * R2D);
    count++;
    labels[count] = g_string_new("volume");
    values[count] = g_string_new("");
    g_string_printf(values[count], "%.2f", model->volume);
    count++;
  }
  /* 2D: surface area */
  else if (model->periodic == 2)
  {
    labels[count] = g_string_new("a");
    values[count] = g_string_new("");
    g_string_printf(values[count], "%8.4f", model->pbc[0]);
    count++;
    labels[count] = g_string_new("b");
    values[count] = g_string_new("");
    g_string_printf(values[count], "%8.4f", model->pbc[1]);
    count++;
    labels[count] = g_string_new("gamma");
    values[count] = g_string_new("");
    g_string_printf(values[count], "%8.2f", model->pbc[5] * R2D);
    count++;
    labels[count] = g_string_new("surface area");
    values[count] = g_string_new("");
    g_string_printf(values[count], "%.4f", model->area);
    count++;
  }

  /* Convert to arrays of strings */
  const gchar *label_arr[12];
  const gchar *value_arr[12];
  for (int i = 0; i < count; i++)
  {
    label_arr[i] = labels[i]->str;
    value_arr[i] = values[i]->str;
  }

  qt_set_symmetry_table(NULL, label_arr, value_arr, count);

  /* Free GStrings */
  for (int i = 0; i < count; i++)
  {
    g_string_free(labels[i], TRUE);
    g_string_free(values[i], TRUE);
  }
}

/* ===== Iso-surface bridge functions ===== */
extern void ms_calculate(void);
extern void ms_delete(void);
extern gdouble ms_blur;
extern gdouble ms_eden;
extern gint ms_method;
extern gint ms_colour;

void qt_ms_calculate(void) { ms_calculate(); }
void qt_ms_delete(void) { ms_delete(); }

void qt_ms_get_state(gint *method, gint *colour)
{
  if (method)
    *method = ms_method;
  if (colour)
    *colour = ms_colour;
}
void qt_ms_set_method(gint m) { ms_method = m; }
void qt_ms_set_colour(gint c) { ms_colour = c; }

gdouble qt_ms_get_blur(void) { return ms_blur; }
void qt_ms_set_blur(gdouble v) { ms_blur = v; }

gdouble qt_ms_get_eden(void) { return ms_eden; }
void qt_ms_set_eden(gdouble v) { ms_eden = v; }

gdouble qt_ms_get_grid_size(void) { return sysenv.render.ms_grid_size; }
void qt_ms_set_grid_size(gdouble v) { sysenv.render.ms_grid_size = v; }

void qt_ms_get_epot(gdouble *min, gdouble *max, gint *div)
{
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (!model)
  {
    if (min)
      *min = 0;
    if (max)
      *max = 0;
    if (div)
      *div = 0;
    return;
  }
  if (min)
    *min = model->epot_min;
  if (max)
    *max = model->epot_max;
  if (div)
    *div = model->epot_div;
}
void qt_ms_set_epot(gdouble min, gdouble max, gint div)
{
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (!model)
    return;
  model->epot_min = min;
  model->epot_max = max;
  model->epot_div = div;
}

gboolean qt_ms_get_epot_autoscale(void)
{
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (!model)
    return FALSE;
  return model->epot_autoscale;
}
void qt_ms_set_epot_autoscale(gboolean v)
{
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (!model)
    return;
  model->epot_autoscale = v;
}

/* ===== Periodic table bridge functions ===== */
extern struct elem_pak elements[];
extern gint num_elements;

void qt_refresh_model_from_table(void)
{
  extern void refresh_model_from_table(void);
  refresh_model_from_table();
}

int qt_get_num_elements(void) { return sysenv.num_elements; }

/* Element number at position in the table[] array */
static gint table_elem_numbers[] = {
    1,   /* H */
    2,   /* He */
    3,   /* Li */
    4,   /* Be */
    5,   /* B */
    6,   /* C */
    7,   /* N */
    8,   /* O */
    9,   /* F */
    10,  /* Ne */
    11,  /* Na */
    12,  /* Mg */
    13,  /* Al */
    14,  /* Si */
    15,  /* P */
    16,  /* S */
    17,  /* Cl */
    18,  /* Ar */
    19,  /* K */
    20,  /* Ca */
    21,  /* Sc */
    22,  /* Ti */
    23,  /* V */
    24,  /* Cr */
    25,  /* Mn */
    26,  /* Fe */
    27,  /* Co */
    28,  /* Ni */
    29,  /* Cu */
    30,  /* Zn */
    31,  /* Ga */
    32,  /* Ge */
    33,  /* As */
    34,  /* Se */
    35,  /* Br */
    36,  /* Kr */
    37,  /* Rb */
    38,  /* Sr */
    39,  /* Y */
    40,  /* Zr */
    41,  /* Nb */
    42,  /* Mo */
    43,  /* Tc */
    44,  /* Ru */
    45,  /* Rh */
    46,  /* Pd */
    47,  /* Ag */
    48,  /* Cd */
    49,  /* In */
    50,  /* Sn */
    51,  /* Sb */
    52,  /* Te */
    53,  /* I */
    54,  /* Xe */
    55,  /* Cs */
    56,  /* Ba */
    57,  /* La */
    58,  /* Ce */
    59,  /* Pr */
    60,  /* Nd */
    61,  /* Pm */
    62,  /* Sm */
    63,  /* Eu */
    64,  /* Gd */
    65,  /* Tb */
    66,  /* Dy */
    67,  /* Ho */
    68,  /* Er */
    69,  /* Tm */
    70,  /* Yb */
    71,  /* Lu */
    72,  /* Hf */
    73,  /* Ta */
    74,  /* W */
    75,  /* Re */
    76,  /* Os */
    77,  /* Ir */
    78,  /* Pt */
    79,  /* Au */
    80,  /* Hg */
    81,  /* Tl */
    82,  /* Pb */
    83,  /* Bi */
    84,  /* Po */
    85,  /* At */
    86,  /* Rn */
    87,  /* Fr */
    88,  /* Ra */
    89,  /* Ac */
    90,  /* Th */
    91,  /* Pa */
    92,  /* U */
    93,  /* Np */
    94,  /* Pu */
    95,  /* Am */
    96,  /* Cm */
    97,  /* Bk */
    98,  /* Cf */
    99,  /* Es */
    100, /* Fm */
};

int qt_get_elem_number_at_position(int pos)
{
  if (pos < 0 || pos >= 72)
    return 0;
  return table_elem_numbers[pos];
}

int qt_get_elem_data(int code, int *number, char *symbol, char *name, double *weight, double *cova, double *vdw,
                     double *charge, double *colour_r, double *colour_g, double *colour_b)
{
  struct elem_pak elem;
  if (get_elem_data(code, &elem, NULL) != 0)
    return -1;
  if (number)
    *number = elem.number;
  if (symbol)
  {
    strncpy(symbol, elem.symbol, 3);
    symbol[3] = '\0';
  }
  if (name)
  {
    strncpy(name, elem.name, 31);
    name[31] = '\0';
  }
  if (weight)
    *weight = elem.weight;
  if (cova)
    *cova = elem.cova;
  if (vdw)
    *vdw = elem.vdw;
  if (charge)
    *charge = elem.charge;
  if (colour_r)
    *colour_r = elem.colour[0];
  if (colour_g)
    *colour_g = elem.colour[1];
  if (colour_b)
    *colour_b = elem.colour[2];
  return 0;
}

void qt_refresh_model_after_elem_change(void)
{
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (!model)
    return;
  model_colour_scheme(model->colour_scheme, model);
  connect_refresh(model);
  redraw_canvas(SINGLE);
}

void qt_elem_apply(int number, double cova, double vdw, double cr, double cg, double cb, int to_model)
{
  struct elem_pak elem;
  if (get_elem_data(number, &elem, NULL) != 0)
    return;
  elem.cova = cova;
  elem.vdw = vdw;
  elem.colour[0] = cr;
  elem.colour[1] = cg;
  elem.colour[2] = cb;

  /* Apply globally first (removes any existing exception and sets new one) */
  put_elem_data(&elem, NULL);

  /* Refresh ALL loaded models so global changes are visible everywhere */
  GSList *mal = sysenv.mal;
  while (mal)
  {
    struct model_pak *model = (struct model_pak *) mal->data;
    model_colour_scheme(model->colour_scheme, model);
    connect_refresh(model);
    mal = g_slist_next(mal);
  }

  /* Then apply to active model if requested */
  if (to_model)
  {
    struct model_pak *model = (struct model_pak *) sysenv.active_model;
    put_elem_data(&elem, model);
  }

  /* Trigger Qt repaint */
  extern void qt_repaint_canvas(void);
  qt_repaint_canvas();
}

void qt_elem_reset(int number)
{
  /* Remove exception from global database */
  GSList *list = sysenv.elements;
  while (list)
  {
    struct elem_pak *elem = (struct elem_pak *) list->data;
    list = g_slist_next(list);
    if (elem->number == number)
      sysenv.elements = g_slist_remove(sysenv.elements, elem);
  }

  /* Remove exception from all models */
  GSList *mal = sysenv.mal;
  while (mal)
  {
    struct model_pak *model = (struct model_pak *) mal->data;
    mal = g_slist_next(mal);
    GSList *list2 = model->elements;
    while (list2)
    {
      struct elem_pak *elem = (struct elem_pak *) list2->data;
      list2 = g_slist_next(list2);
      if (elem->number == number)
        model->elements = g_slist_remove(model->elements, elem);
    }
  }

  /* Refresh active model display */
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (model)
  {
    qt_refresh_model_after_elem_change();
  }

  /* Trigger Qt repaint */
  extern void qt_repaint_canvas(void);
  qt_repaint_canvas();
}

/* Callback: called after a measurement is created via canvas click. */
void qt_measure_grafted(void)
{
  extern void qt_measure_grafted_callback(void);
  qt_measure_grafted_callback();
}

/* ===== Text overlay accessors ===== */
#ifdef __cplusplus
extern "C" gint qt_text_overlay_count_raw(void);
#else
gint qt_text_overlay_count_raw(void);
#endif
gint qt_text_overlay_count(void) { return qt_text_overlay_count_raw(); }

/* Snapshot count before refresh — used to detect if refresh ran */
gint qt_text_overlay_count_before_refresh(void) { return qt_text_overlay_count_raw(); }

#ifdef __cplusplus
extern "C" void qt_text_overlay_get_raw(gint idx, gint *out_x, gint *out_y, gint *out_w, gint *out_h);
#else
void qt_text_overlay_get_raw(gint idx, gint *out_x, gint *out_y, gint *out_w, gint *out_h);
#endif
void qt_text_overlay_get(gint idx, gint *out_x, gint *out_y, gint *out_w, gint *out_h)
{
  qt_text_overlay_get_raw(idx, out_x, out_y, out_w, out_h);
}

#ifdef __cplusplus
extern "C" unsigned char *qt_text_overlay_get_buf_raw(gint idx);
#else
unsigned char *qt_text_overlay_get_buf_raw(gint idx);
#endif
gpointer qt_text_overlay_get_buf(gint idx) { return qt_text_overlay_get_buf_raw(idx); }

/* Push overlay data to Qt text overlay widget */
#ifdef __cplusplus
extern "C" void qt_text_overlay_widget_push(void);
#else
void qt_text_overlay_widget_push(void);
#endif

void qt_push_text_overlays_to_widget(void) { qt_text_overlay_widget_push(); }

/* ===== Tree model accessors (declared in gui_tree.c) ===== */
extern struct model_pak *qt_tree_get_model(int index);
extern gpointer qt_tree_get_graph(int index);
extern void graph_free(gpointer, struct model_pak *);

int qt_tree_get_selected_index(void)
{
  extern int MainWindow_get_selected_tree_index(void);
  return MainWindow_get_selected_tree_index();
}

/* ===== Tree selection delete (close model or graph) ===== */
void qt_tree_select_delete(void)
{
  extern int qt_tree_get_count(void);
  extern void qt_tree_model_select(int);
  extern void qt_tree_refresh(void);
  extern void qt_refresh_qt_tree(void);
  extern int qt_tree_get_selected_index(void);
  extern void graph_free(gpointer, struct model_pak *);
  extern void qt_refresh_all(void);

  int count = qt_tree_get_count();
  if (count <= 0)
    return;

  int selected = qt_tree_get_selected_index();
  if (selected < 0 || selected >= count)
    return;

  int depth = qt_tree_node_depth(selected);

  if (depth == 0)
  {
    /* Delete model */
    struct model_pak *model = qt_tree_get_model(selected);
    if (model)
      model_delete(model);
  } else if (depth == 1)
  {
    /* Delete graph */
    struct model_pak *model = qt_tree_get_model(selected);
    gpointer graph = qt_tree_get_graph(selected);
    if (model && graph)
      graph_free(graph, model);
  }

  /* Refresh tree data and Qt model */
  qt_refresh_all();

  /* Force Qt canvas repaint */
  extern void qt_force_canvas_refresh(void);
  qt_force_canvas_refresh();

  /* Select the next available item */
  int newCount = qt_tree_get_count();
  if (newCount > 0)
  {
    int next = selected < newCount ? selected : newCount - 1;
    qt_tree_model_select(next);
  }
}

/* GULP dialog bridge */
extern void gui_gulp_task(struct model_pak *);
void qt_gulp_task(struct model_pak *model) { gui_gulp_task(model); }

/* Task manager bridge */
#include "task.h"
extern void calc_task_info(struct task_pak *);
extern void task_status_update(struct task_pak *);
extern void task_kill_running(struct task_pak *);
extern void task_set_confirm_callback(confirm_kill_func cb);

void qt_task_kill_running(struct task_pak *task) { task_kill_running(task); }
void qt_task_calc_info(struct task_pak *task) { calc_task_info(task); }
void qt_task_status_update(struct task_pak *task) { task_status_update(task); }

/* Executable path accessors */
const char *qt_get_babel_path(void) { return sysenv.babel_path; }
const char *qt_get_gamess_path(void) { return sysenv.gamess_path; }
const char *qt_get_vasp_path(void) { return sysenv.vasp_path; }
const char *qt_get_uspex_path(void) { return sysenv.uspex_path; }
const char *qt_get_mpirun_path(void) { return sysenv.mpirun_path; }
const char *qt_get_gulp_path(void) { return sysenv.gulp_path; }
const char *qt_get_monty_path(void) { return sysenv.monty_path; }
const char *qt_get_siesta_path(void) { return sysenv.siesta_path; }
const char *qt_get_povray_path(void) { return sysenv.povray_path; }
const char *qt_get_viewer_path(void) { return sysenv.viewer_path; }

void qt_set_babel_path(const char *path)
{
  g_free(sysenv.babel_path);
  sysenv.babel_path = g_strdup(path);
}
void qt_set_gamess_path(const char *path)
{
  g_free(sysenv.gamess_path);
  sysenv.gamess_path = g_strdup(path);
}

/* GAMESS task bridge */
extern void gui_gamess_task(struct model_pak *);
void qt_gamess_task(struct model_pak *model) { gui_gamess_task(model); }

/* Monty task bridge */
extern void gui_monty_task(struct model_pak *);
void qt_monty_task(struct model_pak *model) { gui_monty_task(model); }

/* SIESTA task bridge */
extern void gui_siesta_task(struct model_pak *);
void qt_siesta_task(struct model_pak *model) { gui_siesta_task(model); }
void qt_set_vasp_path(const char *path)
{
  g_free(sysenv.vasp_path);
  sysenv.vasp_path = g_strdup(path);
}
void qt_set_uspex_path(const char *path)
{
  g_free(sysenv.uspex_path);
  sysenv.uspex_path = g_strdup(path);
}
void qt_set_mpirun_path(const char *path)
{
  g_free(sysenv.mpirun_path);
  sysenv.mpirun_path = g_strdup(path);
}
void qt_set_gulp_path(const char *path)
{
  g_free(sysenv.gulp_path);
  sysenv.gulp_path = g_strdup(path);
}
void qt_set_monty_path(const char *path)
{
  g_free(sysenv.monty_path);
  sysenv.monty_path = g_strdup(path);
}
void qt_set_siesta_path(const char *path)
{
  g_free(sysenv.siesta_path);
  sysenv.siesta_path = g_strdup(path);
}
void qt_set_povray_path(const char *path)
{
  g_free(sysenv.povray_path);
  sysenv.povray_path = g_strdup(path);
}
void qt_set_viewer_path(const char *path)
{
  g_free(sysenv.viewer_path);
  sysenv.viewer_path = g_strdup(path);
}

/* Append text to Qt Output dock */
void qt_append_output(const gchar *text) { qt_append_output_type(text, 0); }

void qt_append_output_type(const gchar *text, gint type)
{
  if (!text)
    return;
  if (qt_output_callback)
    qt_output_callback(text, type);
}

/* Cairo surface buffer accessors for snapshot/export. */
unsigned char *qt_cairo_surface(void) { return (unsigned char *) sysenv.cairo_surface; }
gint qt_cairo_width(void) { return sysenv.cairo_width; }
gint qt_cairo_height(void) { return sysenv.cairo_height; }

/* Active model accessor — already defined in gui_main.c */
