/* Internal declarations for gui_surface.c functions used by Qt GUI */
#ifndef GUI_SURFACE_INTERNAL_H
#define GUI_SURFACE_INTERNAL_H

#include "surface.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Nuclei sculpting */
extern gdouble sculpt_length;
extern void sculpt_model_create(struct model_pak *model);

/* Tree grafting */
void fn_graft_plane(struct plane_pak *plane, struct model_pak *model);
void fn_graft_plane_list(GSList *plist, struct model_pak *model);
void fn_graft_shift_list(struct plane_pak *plane, struct model_pak *model);

/* Surface operations */
gint fn_add_plane_with_shift(void);
gint fn_add_valid_shifts(void);
void make_morph(void);
void cb_surf_create(struct model_pak *model);
void surf_task_selected(gint type);
void export_planes(gchar *name);
void surf_load_planes(void);
void surf_save_planes(void);
void new_ecalc_task(struct model_pak *model, struct plane_pak *pdata, struct shift_pak *sdata);
void new_regcon_task(struct model_pak *model, struct plane_pak *plane, struct shift_pak *shift);

/* Tree operations */
void surf_shift_refresh(struct shift_pak *shift);
void surf_prune_shifts(struct plane_pak *plane);
void surf_shift_delete(struct shift_pak *shift, struct plane_pak *plane);
void surf_plane_delete(struct plane_pak *plane, struct model_pak *data);
void surf_prune_all(void);
void surf_prune_invalid(void);
void surf_prune_selected(void);
void surf_select_all(void);
void surf_collapse_all(void);
void surf_expand_all(void);

#ifdef __cplusplus
}
#endif

#endif /* GUI_SURFACE_INTERNAL_H */
