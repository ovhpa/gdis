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
 * C++-friendly API header for GDIS
 */

#ifndef GDIS_API_H
#define GDIS_API_H

#ifdef __cplusplus
extern "C" {
#endif

/* gpointer already defined by glib */
typedef int gint;
typedef unsigned int guint;
typedef char gchar;
typedef double gdouble;
typedef int gboolean;
typedef void *gpointer;

struct model_pak;
struct canvas_pak;
struct sysenv_pak;
struct model_pak *qt_get_active_model(void);
#ifdef __cplusplus
class MainWindow;
MainWindow *get_main_window(void);
#endif
#ifdef __cplusplus
class QWidget;
#endif

/* Message types for gui_text_show — mirrors enum in gdis.h */
enum { GDIS_STANDARD, GDIS_INFO, GDIS_WARNING, GDIS_ERROR, GDIS_BOLD, GDIS_ITALIC };

/* File path length — from gdis.h */
#define FILELEN 512
#define LINELEN 300
#define MAX_DISPLAYED 9

/* File operations */
void file_load(gchar *filename, struct model_pak *model);

/* System */
void sys_init(int argc, char *argv[]);

/* Module system */
void module_setup(void);

/* Rendering */
void gui_surface_dialog(void);
void povray_task(void);
void gui_create_waypoint(void);

/* Canvas */
void redraw_canvas(gint action);
void qt_redraw_canvas(void);
void canvas_single(void);
void canvas_create(void);
void canvas_delete(void);

/* Interface */
void gui_text_show(gint type, const gchar *msg);
void gui_refresh(gint action);
void gui_active_refresh(void);

/* Qt bridge: output dock text append */
void qt_append_output(const gchar *text);
void qt_append_output_type(const gchar *text, gint type);
void qt_set_output_callback(void (*)(const gchar *, gint));

/* Qt bridge: mouse/scroll/key event data setters */
void set_mouse_event_data(int x, int y, int button, int state);
void set_scroll_event_data(int delta);
void set_key_event_data(int key, int state);

/* Qt bridge: get GUI handler function pointers */
typedef void (*gui_event_func)(void *, void *);
gui_event_func get_gui_press_handler(void);
gui_event_func get_gui_motion_handler(void);
gui_event_func get_gui_release_handler(void);
gui_event_func get_gui_scroll_handler(void);

/* Canvas resize */
void canvas_resize(void);
void canvas_create_entry(void);
void set_canvas_dimensions(int w, int h);

/* Get canvas pointer */
void *qt_get_gl_canvas(void);
void qt_repaint_canvas(void);

/* Callback: called after a measurement is created via canvas click */
void qt_measure_grafted(void);
void qt_diffraction_calc(struct model_pak *model);

/* Show/hide measurements dialog */
void qt_show_measurements_dialog(void);
void qt_hide_measurements_dialog(void);

/* Key events */
void gui_key_release_event(void *, void *);

/* File dialogs */
void file_load_dialog(void);
void file_save_dialog(void);

/* CLI mode */
void command_main_loop(int argc, char *argv[]);

/* Mode switching */
void gui_mode_default(void);
void gui_mode_record(void);
void gui_mode_switch(gint mode);

/* Space/image */
void space_image_widget_reset(void);

/* Qt mode */
void set_qt_mode(int);

/* View presets */
void gui_view_default(void);
void gui_view_x(void);
void gui_view_y(void);
void gui_view_z(void);
void gui_view_a(void);
void gui_view_b(void);
void gui_view_c(void);

/* Rotation */
void gui_rotate_x(void);
void gui_rotate_y(void);
void gui_rotate_z(void);

/* Export/Track */
void analysis_export_dialog(void);
void gui_track_output(void);

/* Task */
void task_dialog(void);

/* File operations */
void edit_model_create(void);

/* Dialogs */
void qt_show_measurements_dialog(void);
void gui_plots_dialog(void);
void gui_setup_dialog(void);
void gui_siesta_dialog(void);
void gui_vasp_dialog(void);
void gui_uspex_dialog(void);
void gui_analysis_dialog(void);
void qt_show_md_analysis_dialog(void);
void gulp_dialog(void);
void gamess_dialog(void);
void monty_dialog(void);
void gui_zmat_dialog(void);
void gui_diffract_dialog(void);
void gui_isosurf_dialog(void);
void gui_gperiodic_dialog(void);
void gui_defect_dialog(void);
void gui_dock_dialog(void);
void gui_animate_dialog(void);
void gui_import_project(void);
void gui_import_graph(void);
void gui_import_geomview(void);
void gui_edit_dialog(void);

/* Animation */
void qt_animate_dialog(void);
#ifdef __cplusplus
extern "C" void qt_show_animate_dialog(struct model_pak *model);
extern "C" void qt_show_isosurfaces_dialog(struct model_pak *model);
#endif

/* Selection */
void select_copy(void);
void select_all(void);
void select_invert(void);
void select_hide(void);
void select_delete(void);
void select_colour(void);
void select_paste(void);
void unselect_hide(void);
void unhide_atoms(void);

/* Tree */

/* Qt tree bridge */
void qt_tree_model_add(const char *name, int model_type);
void qt_tree_model_select(int index);
int qt_tree_get_count(void);
void qt_tree_refresh(void);

/* Tree model accessors (C side) */
const char *qt_tree_model_name(int index);
int qt_tree_model_type(int index);
int qt_tree_node_depth(int index);
const char *qt_tree_node_icon(int index);
struct model_pak *qt_tree_get_model(int index);
gpointer qt_tree_get_graph(int index);
int qt_tree_get_selected_index(void);
void qt_force_canvas_refresh(void);
void qt_set_refresh_callback(void (*)(void));
void qt_show_plots_dialog(void);
void qt_show_gulp_dialog(void);
void qt_gulp_task(struct model_pak *);
void qt_show_gamess_dialog(struct model_pak *model);
void qt_show_monty_dialog(struct model_pak *model);
void qt_show_siesta_dialog(struct model_pak *model);
void qt_show_vasp_dialog(void);
void qt_show_uspex_dialog(void);
void qt_show_task_manager_dialog(void);
void qt_show_exec_paths_dialog(void);
struct task_pak;
gboolean qt_confirm_kill(gint *pids, gint count);
void qt_task_calc_info(struct task_pak *);
void qt_task_status_update(struct task_pak *);

/* Model panel setup functions (Qt-compatible wrappers) */
void qt_gui_content_refresh(gpointer);
void qt_set_content_table(gpointer table);
void qt_update_content_table(void);

/* Content table property accessors (C side) */
int qt_get_property_count(void);
const char *qt_get_property_label(int index);
const char *qt_get_property_value(int index);

/* Render filename getter/setter */
const char *qt_get_render_filename(void);
void qt_set_render_filename(const char *filename);

/* Render settings getters/setters */
void qt_get_render_settings(int *animate, int *animate_type, int *mpeg_quality, int *delay);
void qt_set_render_settings(int animate, int animate_type, int mpeg_quality, int delay);

/* Model animation state accessors */
void qt_model_set_animating(int val);
int qt_model_get_animating(void);
void qt_model_set_confine(int val);
int qt_model_get_confine(void);

/* Model frame accessors */
void qt_model_set_cur_frame(int val);
int qt_model_get_cur_frame(void);
int qt_model_get_num_frames(void);

/* Model animation params */
void qt_model_set_anim_speed(double val);
double qt_model_get_anim_speed(void);
void qt_model_set_anim_step(int val);
int qt_model_get_anim_step(void);
void qt_model_set_anim_fix(int val);
int qt_model_get_anim_fix(void);
void qt_model_set_anim_noscale(int val);
int qt_model_get_anim_noscale(void);
void qt_model_set_anim_loop(int val);
int qt_model_get_anim_loop(void);

/* Select frame and refresh */
void qt_select_frame(void);
void qt_select_frame_model(struct model_pak *model);
void qt_refresh_content(void);
void qt_init_camera(struct model_pak *model);
void qt_open_animation_file(struct model_pak *model);
void qt_close_animation_file(struct model_pak *model);

/* POV rendering for movie creation */
gint qt_write_povray(const char *filename, struct model_pak *model);
void qt_povray_exec(const char *filename);
gint qt_get_render_no_povray_exec(void);

/* Convert path for movie assembly */
const char *qt_get_convert_path(void);
const char *qt_get_cwd(void);

/* Keep intermediate images toggle */
gint qt_get_render_no_keep_tempfiles(void);
void qt_set_render_no_keep_tempfiles(gint val);

/* Spatial colour apply */
void qt_spatial_colour_all(gdouble *colour);
void qt_spatial_colour_select(gdouble *colour);

/* Stereo window management */
void qt_stereo_open_window(void);
void qt_stereo_close_window(void);

/* Model editing bridges */
void qt_refresh_edit_dialog_spatial_list(void);
void qt_set_spatial_selected(void *spatial);
void qt_set_transmat_values(const gdouble *tmat, const gdouble *tvec);
void edit_atom_add(void);
void edit_confine(gpointer);
void edit_shells_add(void);
void edit_shells_delete(void);
void qt_gui_mode_switch(gpointer);
void select_flag_ghost(void);
void select_flag_normal(void);
void gui_connect_toggle(void);
void edit_make_supercell(void);
void edit_make_p1(void);
void edit_nanotube_new(void);
void qt_gui_spatial_delete(gpointer data);
void gui_spatial_delete_all(void);
void gui_spatial_delete_selected(void);
void construct_transmat(void);
void qt_apply_transmat(gpointer);
void edit_transform_latmat(void);
void qt_cb_modify_periodicity(gint val);
void region_move(gpointer);
void region_change(gpointer);
void region_growth_slice(gpointer);
void qt_cb_type_model(gpointer combo);
void qt_gui_make_rule(gpointer dialog);
void qt_gui_import_ff(gpointer dialog);
void gui_library_save(void);
void gui_library_load(void);

void qt_gui_edit_widget(gpointer);
void qt_gui_display_widget(gpointer);
void qt_space_image_widget_setup(gpointer);
void qt_gui_symmetry_refresh(gpointer);
void qt_gui_view_widget(gpointer);

/* Set selection mode from Qt (avoids including pak.h in C++) */
void qt_set_select_mode(int mode);

/* Get active model */
gint qt_has_canvas_list(void);

struct model_pak *qt_get_active_model(void);

/* Set edit basis strings */
void qt_set_edit_basis(int idx, const char *text);
void qt_set_edit_length(double val);
void qt_set_edit_chirality(int idx, gdouble val);

/* Undo */
void undo_active(void);

/* Surface */
void surface_dialog(void);

#ifdef __cplusplus
}
#endif

/* Qt API bridge — C wrapper for Qt GUI access to GDIS data */
#ifdef __cplusplus
extern "C" {
#endif
/* Edit field type enum values */
gint qt_edit_type_element(void);
gint qt_edit_type_name(void);
gint qt_edit_type_core_ff(void);
gint qt_edit_type_coord_x(void);
gint qt_edit_type_coord_y(void);
gint qt_edit_type_coord_z(void);
gint qt_edit_type_charge(void);
gint qt_edit_type_weight(void);
gint qt_edit_type_sof(void);
gint qt_edit_type_growth(void);
gint qt_edit_type_region(void);
gint qt_edit_type_translate(void);

/* Operation mode enum values */
gint qt_mode_free(void);
gint qt_mode_atom_add(void);
gint qt_mode_bond_single(void);
gint qt_mode_bond_delete(void);

/* Render mode enum values */
gint qt_render_stick(void);
gint qt_render_ball_stick(void);
gint qt_render_cpk(void);
gint qt_render_liquorice(void);
gint qt_render_polyhedral(void);
gint qt_render_zone(void);

/* Render mode functions */
void qt_render_mode_set(gint mode);
void qt_render_mode_polyhedral(void);
void qt_render_mode_zone(void);
void qt_render_wire_atoms(void);
void qt_render_solid_atoms(void);

/* Periodic image spinners */
void qt_image_spinner_set(gint axis, gint direction, gint value);
gint qt_image_spinner_get(gint direction);
void qt_image_spinner_sync(void);
void qt_image_spinner_update_widgets(void);

/* Symmetry table */
void qt_set_symmetry_table(gpointer, const gchar **, const gchar **, gint);

/* Symmetry analysis */
void gui_symmetry_analyse(void);
void gui_symmetry_analyse_periodic(void);
void gui_symmetry_toggle(void);

const char *qt_elem_symbol(gint code);
const char *qt_core_label(struct core_pak *core);
const char *qt_core_type(struct core_pak *core);
gint qt_core_code(struct core_pak *core);
gdouble qt_core_x(struct core_pak *core, gint i);
gdouble qt_core_charge(struct core_pak *core);
gdouble qt_core_mass(struct core_pak *core);
gdouble qt_core_sof(struct core_pak *core);
gint qt_core_has_sof(struct core_pak *core);
gint qt_core_growth(struct core_pak *core);
gint qt_core_region(struct core_pak *core);
gint qt_core_translate(struct core_pak *core);
gint qt_model_periodic(struct model_pak *model);
gint qt_model_num_atoms(struct model_pak *model);
gdouble qt_model_pbc(struct model_pak *model, gint i);
gdouble qt_model_area(struct model_pak *model);
gdouble qt_model_volume(struct model_pak *model);
const char *qt_model_sgname(struct model_pak *model);
const char *qt_model_latticename(struct model_pak *model);
gpointer qt_model_selection_head(struct model_pak *model);
gint qt_model_selection_count(struct model_pak *model);
gpointer qt_selection_nth(struct model_pak *model, gint n);
void qt_model_content_refresh(struct model_pak *model);
void qt_update_content_table_raw(void);
void qt_model_prep(struct model_pak *model);
void qt_redraw_canvas_full(gint);
gpointer qt_get_mal_list(void);
gboolean qt_model_valid(struct model_pak *model);
void qt_refresh_all(void);
void qt_refresh_edit_panel(void);
extern const gchar *qt_edit_text[];
void qt_atom_properties_change(gint type);
void qt_set_edit_field_text(gint type, const gchar *text);
void qt_free_edit_text(void);
/* Iso-surface bridge functions */
void qt_ms_calculate(void);
void qt_ms_delete(void);
void qt_ms_get_state(gint *method, gint *colour);
void qt_ms_set_method(gint method);
void qt_ms_set_colour(gint colour);
gdouble qt_ms_get_blur(void);
void qt_ms_set_blur(gdouble v);
gdouble qt_ms_get_eden(void);
void qt_ms_set_eden(gdouble v);
gdouble qt_ms_get_grid_size(void);
void qt_ms_set_grid_size(gdouble v);
void qt_ms_get_epot(gdouble *min, gdouble *max, gint *div);
void qt_ms_set_epot(gdouble min, gdouble max, gint div);
gboolean qt_ms_get_epot_autoscale(void);
void qt_ms_set_epot_autoscale(gboolean v);

/* Periodic table bridge functions */
void qt_show_periodic_table_dialog(void);
void qt_refresh_model_from_table(void);
int qt_get_num_elements(void);
int qt_get_elem_number_at_position(int pos);
int qt_get_elem_data(int code, int *number, char *symbol, char *name, double *weight, double *cova, double *vdw,
                     double *charge, double *colour_r, double *colour_g, double *colour_b);
void qt_show_element_dialog(int number);
#ifdef __cplusplus
void qt_show_diffraction_dialog(struct model_pak *model, QWidget *parent);
#endif
void qt_elem_apply(int number, double cova, double vdw, double cr, double cg, double cb, int to_model);
void qt_elem_reset(int number);
void qt_refresh_model_after_elem_change(void);

#ifdef __cplusplus
/* Force overlay repaint */
extern "C" void qt_force_overlay_repaint(void);
#endif

/* Text overlay accessors */
#ifdef __cplusplus
extern "C" {
#endif
gint qt_text_overlay_count(void);
gint qt_text_overlay_count_before_refresh(void);
void qt_text_overlay_get(gint idx, gint *out_x, gint *out_y, gint *out_w, gint *out_h);
gpointer qt_text_overlay_get_buf(gint idx);
void qt_text_overlay_free_all(void);
void qt_push_text_overlays_to_widget(void);
#ifdef __cplusplus
}
#endif

/* Raw overlay accessors (for C++ code) */
#ifdef __cplusplus
extern "C" gint qt_text_overlay_count_raw(void);

/* Cairo surface accessors for snapshot/export. */
extern "C" unsigned char *qt_cairo_surface(void);
extern "C" gint qt_cairo_width(void);
extern "C" gint qt_cairo_height(void);
extern "C" void qt_text_overlay_get_raw(gint idx, gint *out_x, gint *out_y, gint *out_w, gint *out_h);
extern "C" unsigned char *qt_text_overlay_get_buf_raw(gint idx);
}
#endif

/* Executable path accessors (for ExecPathsDialog) */
#ifdef __cplusplus
extern "C" {
#endif
const char *qt_get_babel_path(void);
const char *qt_get_gamess_path(void);
const char *qt_get_vasp_path(void);
const char *qt_get_uspex_path(void);
const char *qt_get_mpirun_path(void);
const char *qt_get_gulp_path(void);
const char *qt_get_monty_path(void);
const char *qt_get_povray_path(void);
const char *qt_get_viewer_path(void);
void qt_set_babel_path(const char *path);
void qt_set_gamess_path(const char *path);
void qt_set_vasp_path(const char *path);
void qt_set_uspex_path(const char *path);
void qt_set_mpirun_path(const char *path);
void qt_set_gulp_path(const char *path);
void qt_set_monty_path(const char *path);
void qt_set_povray_path(const char *path);
void qt_set_viewer_path(const char *path);
#ifdef __cplusplus
}
#endif

#endif /* GDIS_API_H */
