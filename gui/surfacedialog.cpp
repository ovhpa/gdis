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

#include "surfacedialog.h"
#include "gulpdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QComboBox>
#include <QPushButton>
#include <QTimer>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QScrollArea>
#include <QSplitter>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QHeaderView>
#include <QScrollBar>

#include "gdis.h"
#include "coords.h"
// #include "edit.h"
#include "file.h"
#include "parse.h"
#include "task.h"
#include "model.h"
// #include "morph.h"
/* Ensure C linkage for morph functions */
extern "C" {
void change_morph_type(struct model_pak *, gint);
}
#include "numeric.h"
#include "sginfo.h"
#include "matrix.h"
#include "space.h"
#include "surface.h"
#include "interface.h"
// #include "dialog.h"
#include "opengl.h"
#include "gulpdialog.h"

extern struct sysenv_pak sysenv;
extern struct elem_pak elements[];

gdouble rank_value = 10.0;
extern struct model_pak surfdata;

/* Column indices — mirrors SURF_* enum in gui_surface.c */
enum {
  SURF_TITLE = 0,
  SURF_REGIONS,
  SURF_ESURF_UNRE,
  SURF_EATT_UNRE,
  SURF_ESURF_RE,
  SURF_EATT_RE,
  SURF_DIPOLE,
  SURF_GNORM,
  SURF_NCOLS
};

/* Tree item data roles */
enum { UserRole_Model = Qt::UserRole, UserRole_Plane, UserRole_Shift };

/* Task types — mirrors the C enum (avoid conflict with surface.h) */
enum { TASK_CALC_SHIFTS = 0, TASK_CALC_ENERGY, TASK_CONV_REGIONS, TASK_MAKE_FACES };

/* Include internal header for gui_surface.c functions not in public headers */
#include "gui_surface_internal.h"
extern void coords_init(gint action, struct model_pak *model);
extern void tree_model_add(struct model_pak *model);
extern void surf_shift_explore(struct model_pak *model, struct surface_pak *surf);
#ifdef __cplusplus
extern "C" {
#endif
#include "edit.h"
#include "morph.h"
#include "dialog.h"

#ifdef __cplusplus
}
#endif
void change_morph_type(struct model_pak *data, gint type)
{
  gpointer camera;

  g_return_if_fail(data != NULL);

  data->morph_type = type;

  /* save camera */
  camera = camera_dup(data->camera);

  morph_build(data);
  model_prep(data);
  coords_init(REDO_COORDS, data);

  /* rescale & restore camera */
  camera_rescale(data->rmax, camera);
  camera_copy(data->camera, camera);
  g_free(camera);

  redraw_canvas(SINGLE);
}
extern void gui_text_show(gint type, const gchar *msg);
extern void update_plane_energy(struct plane_pak *plane, struct model_pak *model);
void surf_shift_delete(struct shift_pak *shift, struct plane_pak *plane)
{
  g_assert(shift != NULL);
  g_assert(plane != NULL);

  plane->shifts = g_slist_remove(plane->shifts, shift);
  shift_free(shift);
}
void surf_plane_delete(struct plane_pak *plane, struct model_pak *data)
{
  gint n = 1;
  GSList *list;
  struct plane_pak *pcomp;

  g_assert(plane != NULL);
  g_assert(data != NULL);

  /* remove equivalents */
  list = data->planes;
  while (list)
  {
    pcomp = (struct plane_pak *) list->data;
    list = g_slist_next(list);

    /* NB: don't delete the reference plane until the end */
    if (pcomp == plane)
      continue;

    if (facet_equiv(data, plane->index, pcomp->index))
    {
      data->planes = g_slist_remove(data->planes, pcomp);
      g_free(pcomp);
      n++;
    }
  }
  /* remove main plane */
  data->planes = g_slist_remove(data->planes, plane);
  plane_free(plane);
}
void surf_prune_all(void)
{
  struct model_pak *model = surfdata.surface.model;

  g_assert(model != NULL);

  /* free the underlying data */
  plane_data_free(model->planes);
  g_slist_free(model->planes);
  model->planes = NULL;
  model->num_planes = 0;
}
extern void surf_prune_invalid(void);
extern void surf_prune_selected(void);
extern void surf_select_all(void);
extern void surf_collapse_all(void);
extern void surf_expand_all(void);
extern void make_morph(void);
extern void cb_surf_create(struct model_pak *model);
extern void surf_task_selected(gint type);
extern gint fn_add_plane_with_shift(void);
extern gint fn_add_valid_shifts(void);
#define DEBUG_TEST_VALID_SHIFTS 0
void test_valid_shifts(struct plane_pak *plane, struct model_pak *data)
{
  GSList *list;
  struct shift_pak *shift;
  struct model_pak surf;

#if DEBUG_TEST_VALID_SHIFTS
  printf("Testing for valid shifts [%d %d %d]\n", plane->index[0], plane->index[1], plane->index[2]);
#endif

  /* init the model to be used for generating shifts */
  model_init(&surf);
  gulp_data_copy(data, &surf);

  /* test the shifts */
  for (list = plane->shifts; list; list = g_slist_next(list))
  {
    shift = (struct shift_pak *) list->data;

    /* init the surface we want */
    ARR3SET(surf.surface.miller, plane->index);
    surf.surface.region[0] = 1;
    surf.surface.region[1] = 0;
    surf.surface.shift = shift->shift;

    /* destroy old cores & then create the surface */
    free_core_list(&surf);
    generate_surface(data, &surf);

#if DEBUG_TEST_VALID_SHIFTS
    printf("[%f:%f]\n", shift->shift, surf.gulp.sdipole);
#endif

    shift->dipole = surf.gulp.sdipole;
    shift->dipole_computed = TRUE;
  }
}
void export_planes(gchar *name)
{
  struct model_pak *data;
  gchar *filename, *a, *b;

  data = surfdata.surface.model;
  if (!data)
    return;

  /* process filename to force .gmf extension */
  a = parse_strip(name);
  b = g_strconcat(a, ".gmf", NULL);

  filename = g_build_filename(sysenv.cwd, b, NULL);

  write_gmf(filename, data);

  g_free(filename);
  g_free(b);
  g_free(a);
}
void import_planes(gchar *name)
{
  gchar *filename;
  GSList *plist;
  struct model_pak *data;
  struct plane_pak *pdata;

  data = surfdata.surface.model;
  if (!data)
    return;
  if (data->periodic != 3)
  {
    gui_text_show(ERROR, "Source structure is not 3D periodic.");
    return;
  }

  filename = g_build_filename(sysenv.cwd, name, NULL);

  /* get the planes */
  if (load_planes(filename, data))
  {
    gui_text_show(ERROR, "Bad planes file.");
    return;
  }

  plist = data->planes;
  while (plist != NULL)
  {
    pdata = (struct plane_pak *) plist->data;

    /* if shift list is empty (primary plane only!) */
    /* then add the result of a calc valid shifts */
    /* otherwise verify the supplied shift */
    if (pdata->primary)
    {
      if (pdata->shifts)
        test_valid_shifts(pdata, data);
      else
        calc_valid_shifts(data, pdata);
    }

    plist = g_slist_next(plist);
  }

  /* clean up — dialog already closed by Qt */
  g_free(filename);
}
extern void surf_load_planes(void);
extern void surf_save_planes(void);
extern void new_ecalc_task(struct model_pak *model, struct plane_pak *pdata, struct shift_pak *sdata);
extern void new_regcon_task(struct model_pak *model, struct plane_pak *plane, struct shift_pak *shift);

/* Globals for morph energy entries — needed by shift_commit */
static QLineEdit *g_morph_energy[4] = {nullptr, nullptr, nullptr, nullptr};

/* Qt tree refresh — declared in mainwindow.cpp with extern "C" */
extern "C" void qt_refresh_qt_tree(void);

/* Tree store pointer — used by selection callback */
static QTreeWidget *g_surf_tree_view = nullptr;

/* Forward: refresh shift energy values in Qt tree (thread-safe via queued connection) */
extern "C" void qt_update_shift_energy(struct shift_pak *shift);

/* Helper: update morph energy entries from shift */
static void updateMorphEnergyEntries(struct shift_pak *shift, QLineEdit *entries[4]);

/* Helper: convert miller indices to string */
static QString millerToString(gint *m) { return QString("(%1%2%3)").arg(m[0]).arg(m[1]).arg(m[2]); }

/* Helper: format a double with fixed precision */
static QString fmtDouble(gdouble val, int digits = 4) { return QString::number(val, 'f', digits); }

/* Helper: get model from tree item */
static struct model_pak *getModelFromItem(QTreeWidgetItem *item)
{
  return reinterpret_cast<struct model_pak *>(item->data(0, UserRole_Model).toLongLong());
}

static struct plane_pak *getPlaneFromItem(QTreeWidgetItem *item)
{
  return reinterpret_cast<struct plane_pak *>(item->data(0, UserRole_Plane).toLongLong());
}

static struct shift_pak *getShiftFromItem(QTreeWidgetItem *item)
{
  return reinterpret_cast<struct shift_pak *>(item->data(0, UserRole_Shift).toLongLong());
}

/* Helper: find plane by pointer in tree */
static QTreeWidgetItem *findPlaneInTree(QTreeWidgetItem *root, struct plane_pak *plane)
{
  if (!root || !plane)
    return nullptr;
  struct plane_pak *p = getPlaneFromItem(root);
  if (p == plane)
    return root;
  for (int i = 0; i < root->childCount(); i++)
  {
    QTreeWidgetItem *child = root->child(i);
    QTreeWidgetItem *found = findPlaneInTree(child, plane);
    if (found)
      return found;
  }
  return nullptr;
}

/* Helper: find shift by pointer in tree. If parent is nullptr, search all top-level items. */
static QTreeWidgetItem *findShiftInTree(QTreeWidgetItem *parent, struct shift_pak *shift)
{
  if (!shift)
    return nullptr;

  /* If no parent specified, search from top level */
  if (!parent)
  {
    for (int i = 0; i < g_surf_tree_view->topLevelItemCount(); i++)
    {
      QTreeWidgetItem *child = g_surf_tree_view->topLevelItem(i);
      struct shift_pak *s = getShiftFromItem(child);
      if (s == shift)
        return child;
      /* Also search deeper in case shift is nested */
      QTreeWidgetItem *found = findShiftInTree(child, shift);
      if (found)
        return found;
    }
    return nullptr;
  }

  for (int i = 0; i < parent->childCount(); i++)
  {
    QTreeWidgetItem *child = parent->child(i);
    struct shift_pak *s = getShiftFromItem(child);
    if (s == shift)
      return child;
  }
  return nullptr;
}

/* Helper: get all top-level items */
static QList<QTreeWidgetItem *> getAllPlanes(QTreeWidget *tree)
{
  QList<QTreeWidgetItem *> planes;
  for (int i = 0; i < tree->topLevelItemCount(); i++)
  {
    planes.append(tree->topLevelItem(i));
  }
  return planes;
}

/* Helper: get all selected plane items (parent items only) */
static QList<QTreeWidgetItem *> getSelectedPlanes(QTreeWidget *tree)
{
  QList<QTreeWidgetItem *> result;
  QList<QTreeWidgetItem *> all = tree->selectedItems();
  for (auto *item : all)
  {
    if (item->parent() == nullptr && getPlaneFromItem(item))
    {
      result.append(item);
    }
  }
  return result;
}

/* Helper: get all selected shift items */
static QList<QTreeWidgetItem *> getSelectedShifts(QTreeWidget *tree)
{
  QList<QTreeWidgetItem *> result;
  QList<QTreeWidgetItem *> all = tree->selectedItems();
  for (auto *item : all)
  {
    if (getShiftFromItem(item))
    {
      result.append(item);
    }
  }
  return result;
}

/* Helper: expand all planes in tree */
static void expandAllPlanes(QTreeWidget *tree) { tree->expandAll(); }

/* Helper: collapse all planes in tree */
static void collapseAllPlanes(QTreeWidget *tree) { tree->collapseAll(); }

/* Helper: select all items */
static void selectAllItems(QTreeWidget *tree) { tree->selectAll(); }

/* Helper: prune invalid shifts (dipole > tolerance) */
static void pruneInvalidShifts(struct model_pak *model, QTreeWidget *tree)
{
  if (!model || !tree)
    return;

  QList<QTreeWidgetItem *> planes = getAllPlanes(tree);
  for (int p = planes.size() - 1; p >= 0; p--)
  {
    QTreeWidgetItem *planeItem = planes[p];
    struct plane_pak *plane = getPlaneFromItem(planeItem);
    if (!plane)
      continue;

    for (int s = planeItem->childCount() - 1; s >= 0; s--)
    {
      QTreeWidgetItem *shiftItem = planeItem->child(s);
      struct shift_pak *shift = getShiftFromItem(shiftItem);
      if (!shift)
        continue;

      if (fabs(shift->dipole) >= model->surface.dipole_tolerance)
      {
        planeItem->removeChild(shiftItem);
        surf_shift_delete(shift, plane);
      }
    }
  }
}

/* Helper: delete selected items */
static void deleteSelected(QTreeWidget *tree, struct model_pak *model)
{
  if (!tree || !model)
    return;

  QList<QTreeWidgetItem *> all = tree->selectedItems();
  /* Process in reverse order to avoid index shifting */
  for (int i = all.size() - 1; i >= 0; i--)
  {
    QTreeWidgetItem *item = all[i];
    struct plane_pak *plane = getPlaneFromItem(item);
    struct shift_pak *shift = getShiftFromItem(item);

    if (plane && shift)
    {
      /* Delete shift */
      if (item->parent())
      {
        item->parent()->removeChild(item);
        surf_shift_delete(shift, plane);
      }
    } else if (plane && !shift)
    {
      /* Delete plane */
      surf_plane_delete(plane, model);
      delete item;
    }
  }
}

/* Helper: prune all */
static void pruneAll(QTreeWidget *tree, struct model_pak *model)
{
  if (!tree || !model)
    return;

  surf_prune_all();
  tree->clear();
}

/* Helper: get morphology type from radio button */
static gint getMorphType(QRadioButton *active, QRadioButton *rbBfdh, QRadioButton *rbEqun, QRadioButton *rbGrun,
                         QRadioButton *rbEqre, QRadioButton *rbGrre)
{
  if (rbBfdh && rbBfdh->isChecked())
    return DHKL;
  if (rbEqun && rbEqun->isChecked())
    return EQUIL_UN;
  if (rbGrun && rbGrun->isChecked())
    return GROWTH_UN;
  if (rbEqre && rbEqre->isChecked())
    return EQUIL_RE;
  if (rbGrre && rbGrre->isChecked())
    return GROWTH_RE;
  return DHKL; /* default */
}

/* Helper: set morphology type on radio buttons */
static void setMorphType(QRadioButton *rbBfdh, QRadioButton *rbEqun, QRadioButton *rbGrun, QRadioButton *rbEqre,
                         QRadioButton *rbGrre, gint type)
{
  switch (type)
  {
  case DHKL:
    if (rbBfdh)
      rbBfdh->setChecked(true);
    break;
  case EQUIL_UN:
    if (rbEqun)
      rbEqun->setChecked(true);
    break;
  case GROWTH_UN:
    if (rbGrun)
      rbGrun->setChecked(true);
    break;
  case EQUIL_RE:
    if (rbEqre)
      rbEqre->setChecked(true);
    break;
  case GROWTH_RE:
    if (rbGrre)
      rbGrre->setChecked(true);
    break;
  default:
    if (rbBfdh)
      rbBfdh->setChecked(true);
    break;
  }
}

/* Helper: get energy values from morph entries */
static void getMorphEnergyValues(gdouble *values, QLineEdit *entries[4])
{
  for (int i = 0; i < 4; i++)
  {
    if (entries[i] && !entries[i]->text().isEmpty())
    {
      values[i] = entries[i]->text().toDouble();
    } else
    {
      values[i] = 0.0;
    }
  }
}

/* Helper: update morph energy entries from shift */
static void updateMorphEnergyEntries(struct shift_pak *shift, QLineEdit *entries[4])
{
  if (!shift || !entries[0])
    return;
  entries[0]->setText(fmtDouble(shift->esurf[0]));
  entries[1]->setText(fmtDouble(shift->eatt[0]));
  entries[2]->setText(fmtDouble(shift->esurf[1]));
  entries[3]->setText(fmtDouble(shift->eatt[1]));
}

/* Forward declarations for helper functions */
static void populateTree(struct model_pak *model, QTreeWidget *tree);
static QTreeWidgetItem *graftPlaneIntoTree(QTreeWidgetItem *parent, struct plane_pak *plane, struct model_pak *model);
static void graftShiftsIntoTree(QTreeWidgetItem *parent, struct plane_pak *plane);
static void graftShiftIntoTree(QTreeWidgetItem *parent, struct shift_pak *shift, struct plane_pak *plane,
                               struct model_pak *model);
static void updateShiftValuesInTree(struct shift_pak *shift, QTreeWidgetItem *item);

/* Helper: commit morph energy values to selected shifts */
static void commitMorphEnergy(gdouble values[4], QTreeWidget *tree, struct model_pak *model)
{
  if (!tree || !model)
    return;

  QList<QTreeWidgetItem *> shifts = getSelectedShifts(tree);
  for (auto *item : shifts)
  {
    struct shift_pak *shift = getShiftFromItem(item);
    struct plane_pak *plane = getPlaneFromItem(item->parent());
    if (!shift || !plane)
      continue;

    if (values[0] != 0.0 || !tree->selectedItems().isEmpty())
    {
      /* Only update if entry was non-empty (checked above) */
      QLineEdit *e0 = g_morph_energy[0];
      if (e0 && !e0->text().isEmpty())
        shift->esurf[0] = values[0];
      if (g_morph_energy[1] && !g_morph_energy[1]->text().isEmpty())
        shift->eatt[0] = values[1];
      if (g_morph_energy[2] && !g_morph_energy[2]->text().isEmpty())
        shift->esurf[1] = values[2];
      if (g_morph_energy[3] && !g_morph_energy[3]->text().isEmpty())
        shift->eatt[1] = values[3];
    }
    updateShiftValuesInTree(shift, item);
    update_plane_energy(plane, model);
  }

  /* Update morphology if morph model */
  if (model->id == MORPH)
  {
    change_morph_type(model, model->morph_type);
  }
}

/* Helper: load planes from file */
static void loadPlanesFromFile(struct model_pak *data, QTreeWidget *tree)
{
  if (!data || !tree)
    return;
  if (data->periodic != 3)
  {
    gui_text_show(GDIS_ERROR, "Source structure is not 3D periodic.");
    return;
  }

  QString filename = QFileDialog::getOpenFileName(nullptr, QObject::tr("Load planes"), "",
                                                  QObject::tr("GDIS planes files (*.gmf);;All files (*)"));
  if (filename.isEmpty())
    return;

  gchar *gfilename = g_strdup(filename.toUtf8().constData());
  import_planes(gfilename);
  g_free(gfilename);

  /* Repopulate our Qt tree from the model's plane list. */
  tree->clear();
  populateTree(data, tree);
}

/* Helper: save planes to file */
static void savePlanesToFile(struct model_pak *data)
{
  if (!data)
    return;

  QString filename = QFileDialog::getSaveFileName(nullptr, QObject::tr("Save planes"), data->basename,
                                                  QObject::tr("GDIS planes files (*.gmf);;All files (*)"));
  if (filename.isEmpty())
    return;

  gchar *gfilename = g_strdup(filename.toUtf8().constData());
  export_planes(gfilename);
  g_free(gfilename);
}

/* Helper: populate the tree from model planes */
static void populateTree(struct model_pak *model, QTreeWidget *tree)
{
  if (!model || !tree)
    return;

  tree->clear();

  if (model->planes)
  {
    GSList *list = model->planes;
    while (list)
    {
      struct plane_pak *plane = (struct plane_pak *) list->data;
      if (plane->primary)
      {
        QTreeWidgetItem *item = graftPlaneIntoTree(nullptr, plane, model);
        if (item)
          tree->addTopLevelItem(item);
      }
      list = g_slist_next(list);
    }
  }

  /* Expand all */
  expandAllPlanes(tree);
}

/* Helper: add ranked faces */
static void addRankedFaces(struct model_pak *data, QTreeWidget *tree, gint numFaces, gdouble rankVal)
{
  if (!data || !tree)
    return;
  if (data->periodic != 3)
    return;

  GSList *list = get_ranked_faces(numFaces, 0.0, data);
  if (list)
  {
    data->planes = g_slist_concat(data->planes, list);
    /* Repopulate tree from model's plane list */
    tree->clear();
    populateTree(data, tree);
  }
}

/* Helper: show GULP dialog */
static void showGulpDialogWrapper(struct model_pak *model)
{
  if (!model)
    return;
  qt_show_gulp_dialog();
}

/* Helper: create surface callback */
static void createSurfaceCallback(struct model_pak *model, struct surf_dialog_data *qd)
{
  if (!model)
    return;
  /* Sync Qt dialog values to global surfdata before calling C function */
  // extern struct model_pak surfdata;
  for (int i = 0; i < 3; i++)
    surfdata.surface.miller[i] = static_cast<gdouble>(qd->miller[i]);
  surfdata.surface.shift = qd->shift;
  surfdata.surface.region[0] = qd->region[0];
  surfdata.surface.region[1] = qd->region[1];
  fprintf(stderr, "[Create] miller=[%d,%d,%d] shift=%.4f region=[%d,%d]\n", qd->miller[0], qd->miller[1], qd->miller[2],
          qd->shift, (gint) qd->region[0], (gint) qd->region[1]);
  fprintf(stderr, "[Create] model->periodic=%d model->basename=%s\n", model->periodic,
          model->basename ? model->basename : "(null)");
  cb_surf_create(model);
  coords_init(INIT_COORDS, model);
  redraw_canvas(SINGLE);
  /* Refresh the Qt model tree so the new surface model appears */
  qt_tree_refresh();
  qt_refresh_qt_tree();
}

/* Helper: make surface from selected plane/shift */
static void makeSurfaceFromSelection(QTreeWidget *tree, struct model_pak *model)
{
  if (!tree || !model)
    return;

  QList<QTreeWidgetItem *> shifts = getSelectedShifts(tree);
  for (auto *item : shifts)
  {
    struct plane_pak *plane = getPlaneFromItem(item->parent());
    struct shift_pak *shift = getShiftFromItem(item);
    if (plane && shift)
    {
      make_surface(model, plane, shift);
    }
  }
}

/* Helper: calculate valid shifts for selected planes */
static void calcShiftsForPlanes(QTreeWidget *tree, struct model_pak *model)
{
  if (!tree || !model)
    return;

  QList<QTreeWidgetItem *> planes = getSelectedPlanes(tree);
  for (auto *planeItem : planes)
  {
    struct plane_pak *plane = getPlaneFromItem(planeItem);
    if (!plane)
      continue;

    /* Check if plane has shifts */
    if (planeItem->childCount() == 0)
    {
      calc_valid_shifts(model, plane);
      /* Repopulate this plane's shifts */
      planeItem->setExpanded(true);
      graftShiftsIntoTree(planeItem, plane);
    }
  }
}

/* Helper: submit energy calculation for selected shifts */
static void submitEnergyCalc(QTreeWidget *tree, struct model_pak *model)
{
  if (!tree || !model)
    return;

  QList<QTreeWidgetItem *> shifts = getSelectedShifts(tree);
  for (auto *item : shifts)
  {
    struct plane_pak *plane = getPlaneFromItem(item->parent());
    struct shift_pak *shift = getShiftFromItem(item);
    if (shift)
    {
      new_ecalc_task(model, plane, shift);
    }
  }
}

/* Helper: submit region convergence for selected shifts */
static void submitRegionConvergence(QTreeWidget *tree, struct model_pak *model)
{
  if (!tree || !model)
    return;

  QList<QTreeWidgetItem *> shifts = getSelectedShifts(tree);
  for (auto *item : shifts)
  {
    struct plane_pak *plane = getPlaneFromItem(item->parent());
    struct shift_pak *shift = getShiftFromItem(item);
    if (shift)
    {
      new_regcon_task(model, plane, shift);
    }
  }
}

/* Helper: update hkl family entry */
static void updateHklFamilyEntry(QLineEdit *entry, struct model_pak *model, struct plane_pak *plane)
{
  if (!entry || !model || !plane)
    return;

  GString *family = g_string_new(NULL);
  GSList *l2 = get_facet_equiv(model, plane->index);
  GSList *l1 = l2;
  while (l1)
  {
    gint *m = (gint *) l1->data;
    g_string_append_printf(family, "(%d %d %d) ", m[0], m[1], m[2]);
    l1 = g_slist_next(l1);
  }
  entry->setText(family->str);
  g_string_free(family, TRUE);
  free_slist(l2);
}

/* Helper: sync dialog values from tree selection */
static void syncDialogFromSelection(QTreeWidget *tree, struct model_pak *model)
{
  if (!tree || !model)
    return;

  QList<QTreeWidgetItem *> all = tree->selectedItems();
  if (all.size() != 1)
    return;

  QTreeWidgetItem *item = all[0];
  struct plane_pak *plane = getPlaneFromItem(item);
  struct shift_pak *shift = getShiftFromItem(item);

  if (!plane)
    return;

  /* Update miller indices */
  struct model_pak *am = reinterpret_cast<struct model_pak *>(sysenv.active_model);
  for (int i = 0; i < 3; i++)
    am->surface.miller[i] = static_cast<gdouble>(plane->index[i]);

  if (shift)
  {
    am->surface.shift = shift->shift;
    am->surface.region[0] = shift->region[0];
    am->surface.region[1] = shift->region[1];
  }

  /* Update widgets — gui_relation_update is a no-op stub */
}

/* Helper: update plane energy display */
static void updatePlaneEnergyDisplay(struct plane_pak *plane, struct model_pak *model)
{
  if (!plane || !model)
    return;
  update_plane_energy(plane, model);
}

/* Helper: update shift values in tree */
static void updateShiftValuesInTree(struct shift_pak *shift, QTreeWidgetItem *item)
{
  if (!shift || !item)
    return;

  /* Update all column values */
  for (int i = SURF_ESURF_UNRE; i <= SURF_GNORM; i++)
  {
    switch (i)
    {
    case SURF_REGIONS:
      item->setText(SURF_REGIONS, QString("%1:%2").arg((gint) shift->region[0]).arg((gint) shift->region[1]));
      break;
    case SURF_EATT_UNRE:
      item->setText(SURF_EATT_UNRE, fmtDouble(shift->eatt[0]));
      break;
    case SURF_ESURF_UNRE:
      item->setText(SURF_ESURF_UNRE, fmtDouble(shift->esurf[0]));
      break;
    case SURF_EATT_RE:
      item->setText(SURF_EATT_RE, fmtDouble(shift->eatt[1]));
      break;
    case SURF_ESURF_RE:
      item->setText(SURF_ESURF_RE, fmtDouble(shift->esurf[1]));
      break;
    case SURF_DIPOLE:
      if (shift->dipole_computed)
        item->setText(SURF_DIPOLE, fmtDouble(shift->dipole));
      else
        item->setText(SURF_DIPOLE, " ? ");
      break;
    case SURF_GNORM:
      if (shift->gnorm < 0.0)
        item->setText(SURF_GNORM, " ? ");
      else
        item->setText(SURF_GNORM, fmtDouble(shift->gnorm));
      break;
    default:
      break;
    }
  }
}

/* Helper: refresh a shift in the tree */
static void refreshShiftInTree(struct shift_pak *shift)
{
  if (!shift || !g_surf_tree_view)
    return;

  QTreeWidgetItem *item = findShiftInTree(nullptr, shift);
  if (item)
  {
    fprintf(stderr, "[refreshShiftInTree] found item, updating\n");
    updateShiftValuesInTree(shift, item);
  } else
  {
    fprintf(stderr, "[refreshShiftInTree] NOT FOUND in tree\n");
  }
}

/* Helper: refresh a plane in the tree */
static void refreshPlaneInTree(struct plane_pak *plane)
{
  if (!plane || !g_surf_tree_view)
    return;

  QTreeWidgetItem *item = findPlaneInTree(nullptr, plane);
  if (item)
  {
    /* Plane items show (hkl) and dhkl in columns 0,1 */
    item->setText(SURF_TITLE, millerToString(plane->index));
    item->setText(SURF_REGIONS, fmtDouble(plane->dhkl, 6));
  }
}

/* Thread-safe: update shift energy values in Qt tree from background thread.
 * Must use QMetaObject::invokeMethod with QueuedConnection because
 * QTimer::singleShot from a background thread does not fire (no event loop). */
extern "C" void qt_update_shift_energy(struct shift_pak *shift)
{
  if (!shift)
  {
    fprintf(stderr, "[qt_update_shift] NULL shift\n");
    return;
  }
  if (!g_surf_tree_view)
  {
    fprintf(stderr, "[qt_update_shift] NULL tree view\n");
    return;
  }
  fprintf(stderr, "[qt_update_shift] shift->shift=%.4f esurf[0]=%.4f\n", shift->shift, shift->esurf[0]);
  /* Post to main thread via queued connection */
  QMetaObject::invokeMethod(
      g_surf_tree_view,
      [shift]() {
        fprintf(stderr, "[qt_update_shift] main thread: refreshing shift\n");
        refreshShiftInTree(shift);
      },
      Qt::QueuedConnection);
}

/* Helper: sync Qt dialog surfdata to global surfdata */
static void syncQtToSurfdata(struct surf_dialog_data *qd, struct model_pak *model)
{
  // extern struct model_pak surfdata;
  extern gdouble rank_value;
  if (model)
  {
    surfdata.surface.model = model;
    for (int i = 0; i < 3; i++)
      surfdata.surface.miller[i] = static_cast<gdouble>(qd->miller[i]);
    surfdata.surface.shift = qd->shift;
    surfdata.surface.region[0] = qd->region[0];
    surfdata.surface.region[1] = qd->region[1];
  }
  rank_value = static_cast<gdouble>(qd->rankValue);
}

/* Helper: add plane with current shift */
static void addPlaneWithShift(struct model_pak *model, struct surf_dialog_data *qd)
{
  if (!model)
    return;
  syncQtToSurfdata(qd, model);
  fn_add_plane_with_shift();
}

/* Helper: add all valid shifts */
static void addAllValidShifts(struct model_pak *model, QTreeWidget *tree, struct surf_dialog_data *qd)
{
  if (!model || !tree)
    return;
  syncQtToSurfdata(qd, model);
  gint result = fn_add_valid_shifts();
  if (result == 0)
  {
    /* Repopulate tree */
    tree->clear();
    populateTree(model, tree);
  }
}

/* Helper: graft a plane into the tree. Returns the created item, or nullptr on failure. */
static QTreeWidgetItem *graftPlaneIntoTree(QTreeWidgetItem *parent, struct plane_pak *plane, struct model_pak *model)
{
  if (!plane || !model)
    return nullptr;

  QTreeWidgetItem *item = new QTreeWidgetItem();
  item->setText(SURF_TITLE, millerToString(plane->index));
  item->setText(SURF_REGIONS, fmtDouble(plane->dhkl, 6));
  item->setData(0, UserRole_Model, reinterpret_cast<qlonglong>(model));
  item->setData(0, UserRole_Plane, reinterpret_cast<qlonglong>(plane));
  item->setData(0, UserRole_Shift, 0LL);

  /* Set column colors */
  item->setForeground(SURF_TITLE, Qt::black);
  item->setForeground(SURF_REGIONS, Qt::darkBlue);

  /* Expand and add shifts */
  graftShiftsIntoTree(item, plane);

  /* Add to parent or tree */
  if (parent)
    parent->addChild(item);
  return item;
}

/* Helper: graft shifts into a plane item */
static void graftShiftsIntoTree(QTreeWidgetItem *parent, struct plane_pak *plane)
{
  if (!parent || !plane)
    return;

  GSList *list = plane->shifts;
  while (list)
  {
    struct shift_pak *shift = (struct shift_pak *) list->data;
    graftShiftIntoTree(parent, shift, plane, nullptr);
    list = g_slist_next(list);
  }
}

/* Helper: graft a single shift */
static void graftShiftIntoTree(QTreeWidgetItem *parent, struct shift_pak *shift, struct plane_pak *plane,
                               struct model_pak *model)
{
  if (!parent || !shift)
    return;

  QTreeWidgetItem *item = new QTreeWidgetItem(parent);
  item->setText(SURF_TITLE, fmtDouble(shift->shift));
  item->setData(0, UserRole_Model, reinterpret_cast<qlonglong>(model ? model : nullptr));
  item->setData(0, UserRole_Plane, reinterpret_cast<qlonglong>(plane ? plane : nullptr));
  item->setData(0, UserRole_Shift, reinterpret_cast<qlonglong>(shift));

  /* Set initial values */
  updateShiftValuesInTree(shift, item);

  /* Indent shift items slightly */
  QFont font = item->font(SURF_TITLE);
  font.setItalic(true);
  for (int i = 0; i < SURF_NCOLS; i++)
  {
    item->setFont(i, font);
  }
}

/* SurfaceDialog implementation */

SurfaceDialog::SurfaceDialog(struct model_pak *model, QWidget *parent) : QDialog(parent), m_model(model)
{
  g_surf_tree_view = nullptr;

  setWindowTitle(tr("Surfaces"));
  resize(900, 650);

  m_isMorph = (model->id == MORPH);

  /* Initialize surfdata — mirror surface_dialog() behavior */
  m_surfdata.model = model;
  m_surfdata.shift = model->surface.shift;
  m_surfdata.region[0] = model->surface.region[0];
  m_surfdata.region[1] = model->surface.region[1];
  for (int i = 0; i < 3; i++)
    m_surfdata.miller[i] = static_cast<gint>(model->surface.miller[i]);
  fprintf(stderr, "[SurfaceDialog] model=%s periodic=%d miller=[%d,%d,%d] shift=%.4f region=[%d,%d]\n",
          model->basename ? model->basename : "(null)", model->periodic, m_surfdata.miller[0], m_surfdata.miller[1],
          m_surfdata.miller[2], m_surfdata.shift, (gint) m_surfdata.region[0], (gint) m_surfdata.region[1]);

  setupUI();
  populateTree(model, m_treeView);

  /* Check for energy warning */
  if (fabs(model->gulp.energy) < FRACTION_TOLERANCE && model->id != MORPH)
  {
    gui_text_show(GDIS_WARNING, "Has the total energy been calculated?\n");
  }

  /* Ensure bonding count */
  model->surface.bonds_full = g_slist_length(model->bonds);
}

SurfaceDialog::~SurfaceDialog() {}

void SurfaceDialog::setupUI()
{
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(8, 6, 8, 6);
  mainLayout->setSpacing(6);

  /* Menu bar */
  setupMenuBar();

  /* Splitter: left panel + right panel */
  auto *splitter = new QSplitter(Qt::Horizontal);

  setupLeftPanel();
  setupRightPanel();

  splitter->addWidget(m_leftPanel);
  splitter->addWidget(m_rightPanel);
  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);
  splitter->setSizes(QList<int>{300, 600});

  mainLayout->addWidget(splitter);

  setupBottomButtons();

  /* Connect tree selection */
  connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
          &SurfaceDialog::on_selection_changed);
}

void SurfaceDialog::setupMenuBar()
{
  auto *menuBar = new QMenuBar(this);
  m_fileMenu = menuBar->addMenu(tr("&File"));
  m_editMenu = menuBar->addMenu(tr("&Edit"));

  /* File menu */
  QAction *loadAction = new QAction(tr("Load planes..."), this);
  connect(loadAction, &QAction::triggered, this, &SurfaceDialog::on_import_planes);
  m_fileMenu->addAction(loadAction);

  QAction *saveAction = new QAction(tr("Save planes..."), this);
  connect(saveAction, &QAction::triggered, this, &SurfaceDialog::on_export_planes);
  m_fileMenu->addAction(saveAction);

  m_fileMenu->addSeparator();

  /* Edit menu */
  QAction *collapseAction = new QAction(tr("Collapse all"), this);
  connect(collapseAction, &QAction::triggered, this, &SurfaceDialog::on_collapse_all);
  m_editMenu->addAction(collapseAction);

  QAction *expandAction = new QAction(tr("Expand all"), this);
  connect(expandAction, &QAction::triggered, this, &SurfaceDialog::on_expand_all);
  m_editMenu->addAction(expandAction);

  m_editMenu->addSeparator();

  QAction *deleteInvalidAction = new QAction(tr("Delete invalid"), this);
  connect(deleteInvalidAction, &QAction::triggered, this, &SurfaceDialog::on_delete_invalid);
  m_editMenu->addAction(deleteInvalidAction);

  QAction *deleteSelectedAction = new QAction(tr("Delete selected"), this);
  connect(deleteSelectedAction, &QAction::triggered, this, &SurfaceDialog::on_delete_selected);
  m_editMenu->addAction(deleteSelectedAction);

  QAction *deleteAllAction = new QAction(tr("Delete all"), this);
  connect(deleteAllAction, &QAction::triggered, this, &SurfaceDialog::on_delete_all);
  m_editMenu->addAction(deleteAllAction);

  m_editMenu->addSeparator();

  QAction *selectAllAction = new QAction(tr("Select all"), this);
  connect(selectAllAction, &QAction::triggered, this, &SurfaceDialog::on_select_all);
  m_editMenu->addAction(selectAllAction);

  auto *mainLayout = static_cast<QVBoxLayout *>(layout());
  mainLayout->insertWidget(0, menuBar);
}

void SurfaceDialog::setupLeftPanel()
{
  m_leftPanel = new QWidget();
  auto *layout = new QVBoxLayout(m_leftPanel);
  layout->setContentsMargins(4, 4, 4, 4);
  layout->setSpacing(8);

  if (m_isMorph)
  {
    /* Morph mode: morphology type + energy entries */
    auto *morphGroup = new QGroupBox(tr("Morphology type"));
    auto *morphLayout = new QVBoxLayout(morphGroup);
    morphLayout->setSpacing(4);

    m_rbBfdh = new QRadioButton(tr("BFDH"));
    m_rbEqun = new QRadioButton(tr("Equilibrium unrelaxed"));
    m_rbGrun = new QRadioButton(tr("Growth unrelaxed"));
    m_rbEqre = new QRadioButton(tr("Equilibrium relaxed"));
    m_rbGrre = new QRadioButton(tr("Growth relaxed"));

    morphLayout->addWidget(m_rbBfdh);
    morphLayout->addWidget(m_rbEqun);
    morphLayout->addWidget(m_rbGrun);
    morphLayout->addWidget(m_rbEqre);
    morphLayout->addWidget(m_rbGrre);

    setMorphType(m_rbBfdh, m_rbEqun, m_rbGrun, m_rbEqre, m_rbGrre, m_model->morph_type);

    connect(m_rbBfdh, &QRadioButton::toggled, this, [this]() {
      if (m_rbBfdh->isChecked())
        m_model->morph_type = DHKL;
    });
    connect(m_rbEqun, &QRadioButton::toggled, this, [this]() {
      if (m_rbEqun->isChecked())
        m_model->morph_type = EQUIL_UN;
    });
    connect(m_rbGrun, &QRadioButton::toggled, this, [this]() {
      if (m_rbGrun->isChecked())
        m_model->morph_type = GROWTH_UN;
    });
    connect(m_rbEqre, &QRadioButton::toggled, this, [this]() {
      if (m_rbEqre->isChecked())
        m_model->morph_type = EQUIL_RE;
    });
    connect(m_rbGrre, &QRadioButton::toggled, this, [this]() {
      if (m_rbGrre->isChecked())
        m_model->morph_type = GROWTH_RE;
    });

    layout->addWidget(morphGroup);

    /* Shift values */
    auto *shiftGroup = new QGroupBox(tr("Shift values"));
    auto *shiftLayout = new QVBoxLayout(shiftGroup);
    shiftLayout->setSpacing(4);

    const char *labels[] = {"Esurf (unrelaxed)", "Eatt (unrelaxed)", "Esurf (relaxed)", "Eatt (relaxed)"};
    for (int i = 0; i < 4; i++)
    {
      auto *rowLayout = new QHBoxLayout();
      rowLayout->setSpacing(4);
      auto *label = new QLabel(labels[i]);
      rowLayout->addWidget(label);
      m_morphEnergy[i] = new QLineEdit();
      m_morphEnergy[i]->setFixedWidth(150);
      rowLayout->addWidget(m_morphEnergy[i]);
      g_morph_energy[i] = m_morphEnergy[i];
      shiftLayout->addLayout(rowLayout);
    }

    layout->addWidget(shiftGroup);
  } else
  {
    /* Non-morph mode: Miller indices, shift, regions */
    auto *surfaceGroup = new QGroupBox(tr("Surface definition"));
    auto *surfaceLayout = new QFormLayout(surfaceGroup);
    surfaceLayout->setSpacing(6);

    /* Miller indices row */
    auto *millerHBox = new QHBoxLayout();
    millerHBox->setSpacing(4);
    m_spinMiller[0] = new QSpinBox();
    m_spinMiller[0]->setRange(-99, 99);
    m_spinMiller[0]->setValue(m_surfdata.miller[0]);
    m_spinMiller[1] = new QSpinBox();
    m_spinMiller[1]->setRange(-99, 99);
    m_spinMiller[1]->setValue(m_surfdata.miller[1]);
    m_spinMiller[2] = new QSpinBox();
    m_spinMiller[2]->setRange(-99, 99);
    m_spinMiller[2]->setValue(m_surfdata.miller[2]);

    auto *millerLabel = new QLabel(tr("Miller"));
    millerHBox->addWidget(millerLabel);
    millerHBox->addWidget(m_spinMiller[0]);
    millerHBox->addWidget(m_spinMiller[1]);
    millerHBox->addWidget(m_spinMiller[2]);

    surfaceLayout->addRow(millerLabel, millerHBox);

    /* Shift */
    m_spinShift = new QDoubleSpinBox();
    m_spinShift->setRange(0.0, 1.0);
    m_spinShift->setValue(m_surfdata.shift);
    m_spinShift->setDecimals(4);
    m_spinShift->setSingleStep(0.05);
    surfaceLayout->addRow(tr("Shift"), m_spinShift);

    /* Regions */
    auto *regionHBox = new QHBoxLayout();
    regionHBox->setSpacing(4);
    m_spinRegion[0] = new QSpinBox();
    m_spinRegion[0]->setRange(0, 99);
    m_spinRegion[0]->setValue((gint) m_surfdata.region[0]);
    m_spinRegion[1] = new QSpinBox();
    m_spinRegion[1]->setRange(0, 99);
    m_spinRegion[1]->setValue((gint) m_surfdata.region[1]);
    regionHBox->addWidget(new QLabel(tr("Depths")));
    regionHBox->addWidget(m_spinRegion[0]);
    regionHBox->addWidget(m_spinRegion[1]);
    surfaceLayout->addRow(tr(""), regionHBox);

    /* Create button */
    auto *createBtn = new QPushButton(tr("Create"));
    connect(createBtn, &QPushButton::clicked, this, &SurfaceDialog::on_create_surface);
    surfaceLayout->addRow(tr(""), createBtn);

    layout->addWidget(surfaceGroup);

    /* Add faces section */
    auto *addFacesGroup = new QGroupBox(tr("Add faces"));
    auto *addFacesLayout = new QVBoxLayout(addFacesGroup);
    addFacesLayout->setSpacing(6);

    auto *addCurrentBtn = new QPushButton(tr("Add the current surface"));
    connect(addCurrentBtn, &QPushButton::clicked, this, &SurfaceDialog::on_add_plane_with_shift);
    addFacesLayout->addWidget(addCurrentBtn);

    auto *addValidBtn = new QPushButton(tr("Add all valid shifts"));
    connect(addValidBtn, &QPushButton::clicked, this, &SurfaceDialog::on_add_valid_shifts);
    addFacesLayout->addWidget(addValidBtn);

    auto *rankHBox = new QHBoxLayout();
    rankHBox->setSpacing(4);
    rankHBox->addWidget(new QLabel(tr("Add ")));
    m_spinRankValue = new QSpinBox();
    m_spinRankValue->setRange(1, 50);
    m_spinRankValue->setValue(10);
    rankHBox->addWidget(m_spinRankValue);
    rankHBox->addWidget(new QLabel(tr(" Dhkl ranked faces ")));
    auto *addRankedBtn = new QPushButton(tr("Add"));
    connect(addRankedBtn, &QPushButton::clicked, this, &SurfaceDialog::on_add_ranked_faces);
    rankHBox->addWidget(addRankedBtn);
    addFacesLayout->addLayout(rankHBox);

    layout->addWidget(addFacesGroup);

    /* Morphology construction */
    auto *morphGroup = new QGroupBox(tr("Morphology construction"));
    auto *morphLayout = new QVBoxLayout(morphGroup);
    morphLayout->setSpacing(6);

    auto *makeMorphBtn = new QPushButton(tr("Create morphology"));
    connect(makeMorphBtn, &QPushButton::clicked, this, &SurfaceDialog::on_make_morph);
    morphLayout->addWidget(makeMorphBtn);

    /* Morph type combobox */
    auto *morphTypeHBox = new QHBoxLayout();
    morphTypeHBox->setSpacing(4);
    morphTypeHBox->addWidget(new QLabel(tr("Morphology type")));
    auto *morphTypeCombo = new QComboBox();
    morphTypeCombo->addItem("Dhkl");
    morphTypeCombo->addItem("Esurf (u)");
    morphTypeCombo->addItem("Esurf (r)");
    morphTypeCombo->addItem("Eatt (u)");
    morphTypeCombo->addItem("Eatt (r)");
    morphTypeCombo->addItem("Broken bonds");
    morphTypeHBox->addWidget(morphTypeCombo);
    morphLayout->addLayout(morphTypeHBox);

    connect(morphTypeCombo, &QComboBox::currentTextChanged, this, &SurfaceDialog::on_morph_type_changed);

    layout->addWidget(morphGroup);

    /* Nuclei sculpting */
    auto *sculptGroup = new QGroupBox(tr("Nuclei sculpting"));
    auto *sculptLayout = new QVBoxLayout(sculptGroup);
    sculptLayout->setSpacing(6);

    auto *sculptHBox = new QHBoxLayout();
    sculptHBox->setSpacing(4);
    sculptHBox->addWidget(new QLabel(tr("Create nuclei with size ")));
    m_spinNucleiSize = new QSpinBox();
    m_spinNucleiSize->setRange(1, 1000);
    m_spinNucleiSize->setValue(20);
    m_spinNucleiSize->setSuffix(" ");
    sculptHBox->addWidget(m_spinNucleiSize);
    auto *sculptBtn = new QPushButton(tr("Sculpt"));
    connect(sculptBtn, &QPushButton::clicked, this, [this]() {
      sculpt_length = static_cast<gdouble>(m_spinNucleiSize->value());
      sculpt_model_create(m_model);
      /* Repopulate tree since a new model was added */
      populateTree(m_model, m_treeView);
    });
    sculptHBox->addWidget(sculptBtn);
    sculptLayout->addLayout(sculptHBox);

    m_chkSculptShiftUse = new QCheckBox(tr("Cleave to match shift (EXP)"));
    m_chkSculptShiftUse->setChecked(m_model->sculpt_shift_use);
    connect(m_chkSculptShiftUse, &QCheckBox::toggled, this,
            [this](bool checked) { m_model->sculpt_shift_use = checked; });
    sculptLayout->addWidget(m_chkSculptShiftUse);

    layout->addWidget(sculptGroup);

    /* Convergence options */
    auto *convGroup = new QGroupBox(tr("Convergence"));
    auto *convLayout = new QVBoxLayout(convGroup);
    convLayout->setSpacing(4);

    m_chkConvergeEatt = new QCheckBox(tr("Attachment energy based"));
    m_chkConvergeEatt->setChecked(m_model->surface.converge_eatt);
    convLayout->addWidget(m_chkConvergeEatt);

    m_chkConvergeR1 = new QCheckBox(tr("Converge region 1"));
    m_chkConvergeR1->setChecked(m_model->surface.converge_r1);
    convLayout->addWidget(m_chkConvergeR1);

    m_chkConvergeR2 = new QCheckBox(tr("Converge region 2"));
    m_chkConvergeR2->setChecked(m_model->surface.converge_r2);
    convLayout->addWidget(m_chkConvergeR2);

    layout->addWidget(convGroup);

    /* Construction options */
    auto *constructGroup = new QGroupBox(tr("Construction"));
    auto *constructLayout = new QVBoxLayout(constructGroup);
    constructLayout->setSpacing(4);

    m_chkAllowPolar = new QCheckBox(tr("Allow polar surfaces"));
    m_chkAllowPolar->setChecked(m_model->surface.include_polar);
    constructLayout->addWidget(m_chkAllowPolar);

    m_chkAllowBondCleaving = new QCheckBox(tr("Allow bond cleaving"));
    m_chkAllowBondCleaving->setChecked(m_model->surface.ignore_bonding);
    constructLayout->addWidget(m_chkAllowBondCleaving);

    m_chkIgnoreSymmetry = new QCheckBox(tr("Ignore model symmetry"));
    m_chkIgnoreSymmetry->setChecked(m_model->surface.ignore_symmetry);
    constructLayout->addWidget(m_chkIgnoreSymmetry);

    m_chkPreserveDepthPeriodicity = new QCheckBox(tr("Preserve depth periodicity"));
    m_chkPreserveDepthPeriodicity->setChecked(m_model->surface.true_cell);
    constructLayout->addWidget(m_chkPreserveDepthPeriodicity);

    m_chkPreserveAtomOrdering = new QCheckBox(tr("Preserve atom ordering"));
    m_chkPreserveAtomOrdering->setChecked(m_model->surface.keep_atom_order);
    constructLayout->addWidget(m_chkPreserveAtomOrdering);

    layout->addWidget(constructGroup);

    /* Dipole tolerance */
    auto *dipoleGroup = new QGroupBox(tr("Misc options"));
    auto *dipoleLayout = new QHBoxLayout(dipoleGroup);
    dipoleLayout->setSpacing(4);
    dipoleLayout->addWidget(new QLabel(tr("Surface dipole cutoff ")));
    m_spinDipoleTolerance = new QDoubleSpinBox();
    m_spinDipoleTolerance->setRange(0.001, 99.999);
    m_spinDipoleTolerance->setSingleStep(0.001);
    m_spinDipoleTolerance->setValue(m_model->surface.dipole_tolerance);
    dipoleLayout->addWidget(m_spinDipoleTolerance);
    layout->addWidget(dipoleGroup);
  }

  layout->addStretch();
}

void SurfaceDialog::setupRightPanel()
{
  m_rightPanel = new QWidget();
  auto *layout = new QVBoxLayout(m_rightPanel);
  layout->setContentsMargins(4, 4, 4, 4);
  layout->setSpacing(4);

  /* Tree view in scroll area */
  auto *scrollArea = new QScrollArea();
  scrollArea->setWidgetResizable(true);
  scrollArea->setFrameShape(QScrollArea::NoFrame);

  m_treeView = new QTreeWidget();
  setupTreeColumns();
  scrollArea->setWidget(m_treeView);

  layout->addWidget(scrollArea);

  /* HKL family entry */
  m_hklFamilyEntry = new QLineEdit();
  m_hklFamilyEntry->setReadOnly(true);
  m_hklFamilyEntry->setFixedHeight(24);
  layout->addWidget(m_hklFamilyEntry);
}

void SurfaceDialog::setupTreeColumns()
{
  m_columnHeaders << "hkl/shift" << "Dhkl/depth" << "Esurf (u)" << "Eatt (u)"
                  << "Esurf (r)" << "Eatt (r)" << "dipole  " << "gnorm  ";
  m_treeView->setColumnCount(SURF_NCOLS);
  m_treeView->setHeaderLabels(m_columnHeaders);

  /* Set column widths */
  QHeaderView *header = m_treeView->header();
  header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  for (int i = 2; i < SURF_NCOLS; i++)
  {
    header->setSectionResizeMode(i, QHeaderView::Interactive);
    header->resizeSection(i, 90);
  }

  /* Allow multi-selection */
  m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);

  g_surf_tree_view = m_treeView;
}

void SurfaceDialog::setupBottomButtons()
{
  if (m_isMorph)
  {
    /* Morph mode buttons */
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(8);

    auto *commitBtn = new QPushButton(tr("Commit"));
    connect(commitBtn, &QPushButton::clicked, this, &SurfaceDialog::on_shift_commit);
    buttonLayout->addWidget(commitBtn);

    auto *deleteBtn = new QPushButton(tr("Delete"));
    connect(deleteBtn, &QPushButton::clicked, this, &SurfaceDialog::on_delete_selected);
    buttonLayout->addWidget(deleteBtn);

    auto *mainLayout = static_cast<QVBoxLayout *>(layout());
    mainLayout->addLayout(buttonLayout);
  } else
  {
    /* Non-morph mode buttons */
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(8);

    auto *createSurfaceBtn = new QPushButton(tr("Create surface"));
    connect(createSurfaceBtn, &QPushButton::clicked, this, [this]() { on_surf_task(MAKE_FACES); });
    buttonLayout->addWidget(createSurfaceBtn);

    auto *convRegionsBtn = new QPushButton(tr("Converge regions"));
    connect(convRegionsBtn, &QPushButton::clicked, this, [this]() { on_surf_task(CONV_REGIONS); });
    buttonLayout->addWidget(convRegionsBtn);

    auto *calcEnergyBtn = new QPushButton(tr("Calculate energy"));
    connect(calcEnergyBtn, &QPushButton::clicked, this, [this]() { on_surf_task(CALC_ENERGY); });
    buttonLayout->addWidget(calcEnergyBtn);

    auto *gulpBtn = new QPushButton(tr("Calculation setup"));
    connect(gulpBtn, &QPushButton::clicked, this, [this]() { showGulpDialogWrapper(m_model); });
    buttonLayout->addWidget(gulpBtn);

    auto *mainLayout = static_cast<QVBoxLayout *>(layout());
    mainLayout->addLayout(buttonLayout);
  }
}

/* Slot implementations */

void SurfaceDialog::on_create_surface()
{
  /* Sync dialog values to surface data */
  m_surfdata.miller[0] = m_spinMiller[0]->value();
  m_surfdata.miller[1] = m_spinMiller[1]->value();
  m_surfdata.miller[2] = m_spinMiller[2]->value();
  m_surfdata.shift = m_spinShift->value();
  m_surfdata.region[0] = m_spinRegion[0]->value();
  m_surfdata.region[1] = m_spinRegion[1]->value();

  createSurfaceCallback(m_model, &m_surfdata);
}

void SurfaceDialog::on_add_plane_with_shift()
{
  /* Sync dialog values */
  m_surfdata.miller[0] = m_spinMiller[0]->value();
  m_surfdata.miller[1] = m_spinMiller[1]->value();
  m_surfdata.miller[2] = m_spinMiller[2]->value();
  m_surfdata.shift = m_spinShift->value();
  m_surfdata.region[0] = m_spinRegion[0]->value();
  m_surfdata.region[1] = m_spinRegion[1]->value();

  addPlaneWithShift(m_model, &m_surfdata);
  populateTree(m_model, m_treeView);
}

void SurfaceDialog::on_add_valid_shifts() { addAllValidShifts(m_model, m_treeView, &m_surfdata); }

void SurfaceDialog::on_add_ranked_faces()
{
  m_surfdata.rankValue = m_spinRankValue->value();
  addRankedFaces(m_model, m_treeView, m_spinRankValue->value(), m_surfdata.rankValue);
}

void SurfaceDialog::on_make_morph()
{
  /* Sync Qt dialog values to global surfdata before calling C function */
  // extern struct model_pak surfdata;
  surfdata.surface.model = m_model;
  make_morph();
}

void SurfaceDialog::on_surf_task(gint type)
{
  switch (type)
  {
  case MAKE_FACES:
    makeSurfaceFromSelection(m_treeView, m_model);
    redraw_canvas(SINGLE);
    break;
  case CONV_REGIONS:
    submitRegionConvergence(m_treeView, m_model);
    break;
  case CALC_ENERGY:
    submitEnergyCalc(m_treeView, m_model);
    break;
  case CALC_SHIFTS:
    calcShiftsForPlanes(m_treeView, m_model);
    break;
  }
}

void SurfaceDialog::on_calc_shifts() { calcShiftsForPlanes(m_treeView, m_model); }

void SurfaceDialog::on_calc_energy() { submitEnergyCalc(m_treeView, m_model); }

void SurfaceDialog::on_conv_regions() { submitRegionConvergence(m_treeView, m_model); }

void SurfaceDialog::on_make_faces()
{
  makeSurfaceFromSelection(m_treeView, m_model);
  redraw_canvas(SINGLE);
}

void SurfaceDialog::on_delete_selected() { deleteSelected(m_treeView, m_model); }

void SurfaceDialog::on_delete_invalid() { pruneInvalidShifts(m_model, m_treeView); }

void SurfaceDialog::on_delete_all() { pruneAll(m_treeView, m_model); }

void SurfaceDialog::on_collapse_all() { collapseAllPlanes(m_treeView); }

void SurfaceDialog::on_expand_all() { expandAllPlanes(m_treeView); }

void SurfaceDialog::on_select_all() { selectAllItems(m_treeView); }

void SurfaceDialog::on_selection_changed()
{
  QList<QTreeWidgetItem *> all = m_treeView->selectedItems();
  if (all.isEmpty())
    return;

  QTreeWidgetItem *item = all[0];
  struct plane_pak *plane = getPlaneFromItem(item);
  struct shift_pak *shift = getShiftFromItem(item);

  if (!plane)
    return;

  /* Update hkl family */
  updateHklFamilyEntry(m_hklFamilyEntry, m_model, plane);

  /* Update morph energy entries if morph mode and shift selected */
  if (m_isMorph && plane && shift)
  {
    updateMorphEnergyEntries(shift, g_morph_energy);
  }

  /* Update dialog values */
  if (!m_isMorph)
  {
    for (int i = 0; i < 3; i++)
      m_surfdata.miller[i] = plane->index[i];
    if (shift)
    {
      m_surfdata.shift = shift->shift;
      m_surfdata.region[0] = shift->region[0];
      m_surfdata.region[1] = shift->region[1];

      /* Update spinners */
      m_spinShift->setValue(shift->shift);
      m_spinRegion[0]->setValue((gint) shift->region[0]);
      m_spinRegion[1]->setValue((gint) shift->region[1]);
    }
  }

  /* Update model relation — gui_relation_update is a no-op stub */
}

void SurfaceDialog::on_morph_type_changed(const QString &text)
{
  if (!m_model)
    return;

  if (text.startsWith("Esurf (u"))
    m_model->morph_type = EQUIL_UN;
  else if (text.startsWith("Esurf (r"))
    m_model->morph_type = EQUIL_RE;
  else if (text.startsWith("Eatt (u"))
    m_model->morph_type = GROWTH_UN;
  else if (text.startsWith("Eatt (r"))
    m_model->morph_type = GROWTH_RE;
  else if (text.startsWith("Broken"))
    m_model->morph_type = MORPH_BBPA;
  else
    m_model->morph_type = DHKL;

  /* Only rebuild morphology on actual morphology models, not bulk models */
  if (m_model->id == MORPH)
  {
    change_morph_type(m_model, m_model->morph_type);
  }
}

void SurfaceDialog::on_shift_commit()
{
  gdouble values[4];
  getMorphEnergyValues(values, g_morph_energy);
  commitMorphEnergy(values, m_treeView, m_model);
}

void SurfaceDialog::on_export_planes() { savePlanesToFile(m_model); }

void SurfaceDialog::on_import_planes() { loadPlanesFromFile(m_model, m_treeView); }

void SurfaceDialog::on_gulp_dialog() { showGulpDialogWrapper(m_model); }

void SurfaceDialog::on_miller_changed()
{
  m_surfdata.miller[0] = m_spinMiller[0]->value();
  m_surfdata.miller[1] = m_spinMiller[1]->value();
  m_surfdata.miller[2] = m_spinMiller[2]->value();
}

void SurfaceDialog::on_shift_changed() { m_surfdata.shift = m_spinShift->value(); }

void SurfaceDialog::on_region_changed()
{
  m_surfdata.region[0] = m_spinRegion[0]->value();
  m_surfdata.region[1] = m_spinRegion[1]->value();
}

/* Bridge function — called from mainwindow */
extern "C" void qt_show_surface_dialog(void)
{
  extern struct sysenv_pak sysenv;
  extern QWidget *get_main_window_widget();
  struct model_pak *model = qt_get_active_model();
  if (!model)
    return;

  SurfaceDialog *dlg = new SurfaceDialog(model, get_main_window_widget());
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->show();
}
