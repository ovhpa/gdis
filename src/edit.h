
/**************/
/* prototypes */
/**************/

gint core_match(const gchar *, struct core_pak *);
gint shel_match(const gchar *, struct shel_pak *);
gint pair_match(const gchar *, const gchar *, struct core_pak *, struct core_pak *);

void elem_change_colour(gpointer *);

void gulp_files_init(struct model_pak *);
void gulp_data_free(struct model_pak *);
void gulp_files_free(struct model_pak *);
void gulp_data_copy(struct model_pak *, struct model_pak *);
void gulp_extra_copy(struct model_pak *, struct model_pak *);

gint gulp_cosmo_points(gint, gint, gint);

gint free_moldy_data(struct model_pak *);

gint search_basename(const gchar *);
gint dialog_active(gint);

void unhide_atoms(void);

void type_model(const gchar *, struct model_pak *);

void delete_commit(struct model_pak *);

void core_delete_single(struct core_pak *, struct model_pak *);
void core_delete_all(struct model_pak *);

void atom_properties_change(gint);
