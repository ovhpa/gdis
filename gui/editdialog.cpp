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
 * Model Editing Dialog for GDIS Qt6 GUI
 */

#include "editdialog.h"
#include "mainwindow.h"
#include "gdis_api.h"
#include "svg_utils.h"
#include "interface.h"
#include "gdis.h"
#include "spatial.h"
#include "coords.h"
#include "matrix.h"
#include "space.h"

/* glib for sysenv, GSList, GHashTable access
 * pak.h is included transitively via gdis.h */
#ifndef G_DISABLE_SINGLE_INCLUDES
#define G_DISABLE_SINGLE_INCLUDES
#endif
#include <glib.h>

/* Forward declaration */
extern MainWindow *get_main_window();

/* spatial_destroy_all etc. are declared in spatial.h with C linkage */

/* Local enums from gui_edit.c */
enum { AT_ANY, AT_SELECTED, AT_LATTICE };
#include <QHeaderView>
#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <functional>
#include <vector>
#include <QFile>
#include <QIcon>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QListWidget>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QCheckBox>
#include <QFrame>

/* Bridge declarations */
extern void gui_edit_refresh(void);
extern void gui_text_show(gint type, const char *msg);
extern void redraw_canvas(gint);

/* C functions from gui_still_valid.c — need extern "C" for C++ linkage */
extern struct sysenv_pak sysenv;

extern "C" {
void spatial_destroy_all(struct model_pak *);
void spatial_destroy_by_label(const gchar *, struct model_pak *);
void spatial_destroy(gpointer, struct model_pak *);
void matrix_lattice_new(gdouble *, struct model_pak *);
void matrix_identity(gdouble *);
void model_prep(struct model_pak *);
void coords_compute(struct model_pak *);
void zone_init(struct model_pak *);
void connect_bonds(struct model_pak *);
void connect_molecules(struct model_pak *);
void calc_emp(struct model_pak *);
/* connect_fragment_init declared in coords.h with extern "C" */
void model_content_refresh(struct model_pak *);
void qt_set_transmat_values(const gdouble *, const gdouble *);
void qt_apply_transmat(gpointer);
void edit_transform_latmat(void);
void construct_transmat(void);
void gui_spatial_delete_all(void);
void gui_spatial_delete_selected(void);
void gui_spatial_delete(gpointer);
extern "C" void gui_spatial_colour_all(gdouble *);
extern "C" void gui_spatial_colour_select(gdouble *);
extern "C" void gui_connect_toggle(void);
extern "C" void qt_cb_modify_periodicity(gint);
}

void gui_spatial_delete_all(void)
{
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (model)
  {
    spatial_destroy_all(model);
    /* Force full redraw — deleted geometry leaves stale pixels. */
    model->need_clear = TRUE;
    sysenv.refresh_dialog = TRUE;
    redraw_canvas(ALL); /* ALL to clear all canvases */
    extern void qt_refresh_edit_dialog_spatial_list(void);
    qt_refresh_edit_dialog_spatial_list();
  }
}

/* Qt-side helper: delete selected spatial(s) from the table. */
void EditDialog::delete_selected_spacial()
{
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (!model) return;

  /* Collect all selected spatial pointers from the table, then delete them.
   * This handles multi-selection properly instead of relying on the stale
   * sysenv.cedit.spatial_selected pointer. */
  std::vector<void *> to_delete;
  auto indexes = m_spatialTable->selectionModel()->selectedRows();
  for (const QModelIndex &idx : indexes) {
    void *spatial = idx.data(Qt::UserRole).value<void *>();
    if (spatial) to_delete.push_back(spatial);
  }

  /* Delete each spatial */
  for (void *sp : to_delete) {
    spatial_destroy(sp, model);
  }

  /* Force full redraw — deleted geometry leaves stale pixels. */
  if (!to_delete.empty()) {
    model->need_clear = TRUE;
    sysenv.refresh_dialog = TRUE;
    redraw_canvas(SINGLE);
    extern void qt_refresh_edit_dialog_spatial_list(void);
    qt_refresh_edit_dialog_spatial_list();
  }
}

/* C-side wrapper for external callers */
extern "C" void gui_spatial_delete_selected(void)
{
  /* Call the Qt slot if we have access to the edit dialog. */
  extern void qt_refresh_edit_dialog_spatial_list(void);
  extern struct model_pak *qt_get_active_model(void);

  struct model_pak *model = qt_get_active_model();
  if (!model) return;

  /* Delete the C-side spatial_selected pointer. */
  if (sysenv.cedit.spatial_selected) {
    spatial_destroy(sysenv.cedit.spatial_selected, model);
    model->need_clear = TRUE;
    sysenv.refresh_dialog = TRUE;
    redraw_canvas(SINGLE);
    qt_refresh_edit_dialog_spatial_list();
  }
}

/* Transform and region functions - moved from gui_still_valid.c */
extern void matrix_lattice_new(gdouble *, struct model_pak *);
extern void matrix_identity(gdouble *);
extern void model_prep(struct model_pak *);
extern void coords_compute(struct model_pak *);
extern void connect_bonds(struct model_pak *);
extern void connect_molecules(struct model_pak *);
extern void calc_emp(struct model_pak *);
extern "C" gint region_move_atom(struct core_pak *, gint, struct model_pak *);
extern void connect_fragment_init(void);
extern void model_content_refresh(struct model_pak *);

void qt_set_transmat_values(const gdouble *tmat, const gdouble *tvec)
{
  memcpy(sysenv.cedit.tmat, tmat, 9 * sizeof(gdouble));
  memcpy(sysenv.cedit.tvec, tvec, 3 * sizeof(gdouble));
}

void qt_apply_transmat(gpointer mode)
{
  gint m = GPOINTER_TO_INT(mode);
  GSList *item, *list = NULL;
  struct model_pak *data;
  struct core_pak *core;
  struct shel_pak *shel;

  data = (struct model_pak *) sysenv.active_model;
  if (!data)
    return;

  switch (m)
  {
  case 0: /* AT_ANY */
    list = data->cores;
    break;
  case 1: /* AT_SELECTED */
    list = data->selection;
    break;
  case 2: /* AT_LATTICE */
    matrix_lattice_new(sysenv.cedit.tmat, data);
    return;
  default:
    return;
  }

  for (item = list; item; item = g_slist_next(item))
  {
    core = (struct core_pak *) item->data;
    /* Apply transformation directly in fractional space */
    vecmat(sysenv.cedit.tmat, core->x);
    ARR3ADD(core->x, sysenv.cedit.tvec);
    if (core->shell)
    {
      shel = (struct shel_pak *) core->shell;
      vecmat(sysenv.cedit.tmat, shel->x);
      ARR3ADD(shel->x, sysenv.cedit.tvec);
    }
  }

  /* Recalculate derived structures after moving atoms */
  coords_compute(data);
  zone_init(data);
  connect_bonds(data);
  connect_molecules(data);

  data->need_clear = TRUE;
  sysenv.refresh_dialog = TRUE;
  redraw_canvas(1); /* SINGLE */
  extern void qt_force_canvas_refresh(void);
  qt_force_canvas_refresh();
}

void edit_transform_latmat(void)
{
  GSList *list;
  struct model_pak *model;
  struct core_pak *core;
  struct shel_pak *shell;

  model = (struct model_pak *) sysenv.active_model;
  if (!model)
    return;

  space_make_p1(model);

  for (list = model->cores; list; list = g_slist_next(list))
  {
    core = (struct core_pak *) list->data;
    vecmat(model->latmat, core->x);
  }
  for (list = model->shels; list; list = g_slist_next(list))
  {
    shell = (struct shel_pak *) list->data;
    vecmat(model->latmat, shell->x);
  }

  matmat(sysenv.cedit.tmat, model->latmat);
  model->fractional = FALSE;
  model->construct_pbc = TRUE;

  if (model->periodic == 0)
  {
    matrix_identity(model->latmat);
    matrix_identity(model->ilatmat);
    matrix_identity(model->rlatmat);
  }

  model_prep(model);
  model->need_clear = TRUE;
  redraw_canvas(1); /* SINGLE */
}

/**
 * Static helper: construct a transformation matrix from widget values.
 * Called by both on_construct_transmat() and the extern C construct_transmat().
 */
void EditDialog::do_construct_transmat()
{
  if (!m_constructCombo || !m_axisAngleEntry || !m_refSpatialEntry || !m_transTable)
    return;

  struct model_pak *data = static_cast<struct model_pak *>(sysenv.active_model);
  if (!data)
    return;

  /* Get the construct type from combo */
  QString constructType = m_constructCombo->currentText().trimmed().toLower();

  /* Parse construct type */
  enum ConstructType { IDENTITY, LATMAT, REFLECTION, PAXIS, ALIGNMENT };
  int type = -1;
  if (constructType.startsWith("identity"))
    type = IDENTITY;
  else if (constructType.startsWith("lattice"))
    type = LATMAT;
  else if (constructType.startsWith("reflection"))
    type = REFLECTION;
  else if (constructType.startsWith("rotation"))
    type = PAXIS;
  else if (constructType.startsWith("z alignment") || constructType.startsWith("alignment"))
    type = ALIGNMENT;

  /* Handle special cases */
  switch (type)
  {
  case IDENTITY:
  {
    /* Reset to identity matrix and zero translation */
    gdouble tmat[9];
    matrix_identity(tmat);
    gdouble tvec[3] = {0.0, 0.0, 0.0};
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++)
        m_transTable->item(i, j)->setText(QString("%1").arg(tmat[3 * i + j], 0, 'f', 4));
    for (int i = 0; i < 3; i++)
      m_transTable->item(i, 3)->setText(QString("%1").arg(tvec[i], 0, 'f', 4));
    return;
  }

  case LATMAT:
  {
    /* Use lattice matrix - copy directly */
    gdouble tmat[9];
    memcpy(tmat, data->latmat, 9 * sizeof(gdouble));
    gdouble tvec[3] = {0.0, 0.0, 0.0};
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++)
        m_transTable->item(i, j)->setText(QString("%1").arg(tmat[3 * i + j], 0, 'f', 4));
    for (int i = 0; i < 3; i++)
      m_transTable->item(i, 3)->setText(QString("%1").arg(tvec[i], 0, 'f', 4));
    return;
  }

  default:
    break;
  }

  /* Read rotation angle */
  bool ok = false;
  gdouble angle = m_axisAngleEntry->text().toDouble(&ok);
  if (!ok)
    angle = 0.0;

  /* Parse reference spatial object */
  QString refText = m_refSpatialEntry->text().trimmed();
  gdouble v1[3] = {0.0, 0.0, 0.0};
  int flag = 0;

  /* Check for special axis references (x, y, z) */
  if (refText == "x")
    flag = 1;
  else if (refText == "y")
    flag = 2;
  else if (refText == "z")
    flag = 3;
  /* Check for lattice vector references (a, b, c) */
  else if (refText == "a")
    flag = 4;
  else if (refText == "b")
    flag = 5;
  else if (refText == "c")
    flag = 6;

  /* Construct reference vector */
  switch (flag)
  {
  case 1: /* x axis */
    VEC3SET(v1, 1.0, 0.0, 0.0);
    break;
  case 2: /* y axis */
    VEC3SET(v1, 0.0, 1.0, 0.0);
    break;
  case 3: /* z axis */
    VEC3SET(v1, 0.0, 0.0, 1.0);
    break;
  case 4: /* lattice vector a */
    VEC3SET(v1, 1.0, 0.0, 0.0);
    vecmat(data->latmat, v1);
    break;
  case 5: /* lattice vector b */
    VEC3SET(v1, 0.0, 1.0, 0.0);
    vecmat(data->latmat, v1);
    break;
  case 6: /* lattice vector c */
    VEC3SET(v1, 0.0, 0.0, 1.0);
    vecmat(data->latmat, v1);
    break;
  default:
  {
    /* Try to parse as spatial object index */
    bool numOk = false;
    gdouble idx_d = refText.toDouble(&numOk);
    int idx = static_cast<int>(idx_d);
    if (!numOk || idx < 0)
    {
      /* No valid reference - show error and return */
      gui_text_show(1, "Undefined reference spatial object.\n"); /* WARNING */
      return;
    }

    /* Get the spatial object from the list */
    GSList *spatialList = data->spatial;
    if (!spatialList)
    {
      gui_text_show(1, "No spatial objects defined.\n"); /* WARNING */
      return;
    }

    int listLen = g_slist_length(spatialList);
    if (idx >= listLen)
    {
      gui_text_show(1, QString("Invalid spatial index %1 (0-%2).\n")
                            .arg(idx)
                            .arg(listLen - 1)
                            .toUtf8()
                            .constData()); /* WARNING */
      return;
    }

    struct spatial_pak *spatial = static_cast<struct spatial_pak *>(g_slist_nth_data(spatialList, idx));
    if (!spatial)
    {
      gui_text_show(1, "Undefined reference spatial object.\n"); /* WARNING */
      return;
    }

    /* Compute orientation vector from spatial object */
    struct vec_pak *p[3];
    switch (spatial->type)
    {
    case SPATIAL_VECTOR:
      p[0] = static_cast<struct vec_pak *>(g_slist_nth_data(spatial->list, 0));
      p[1] = static_cast<struct vec_pak *>(g_slist_nth_data(spatial->list, 1));
      if (!p[0] || !p[1])
      {
        gui_text_show(1, "Invalid spatial vector definition.\n"); /* WARNING */
        return;
      }
      ARR3SET(v1, p[1]->rx);
      ARR3SUB(v1, p[0]->rx);
      break;

    default: /* SPATIAL_PLANE or others - use normal */
      p[0] = static_cast<struct vec_pak *>(g_slist_nth_data(spatial->list, 0));
      p[1] = static_cast<struct vec_pak *>(g_slist_nth_data(spatial->list, 1));
      p[2] = static_cast<struct vec_pak *>(g_slist_nth_data(spatial->list, 2));
      if (!p[0] || !p[1] || !p[2])
      {
        gui_text_show(1, "Invalid spatial plane definition.\n"); /* WARNING */
        return;
      }
      calc_norm(v1, p[0]->rx, p[1]->rx, p[2]->rx);
      break;
    }
    break;
  }
  }

  /* Construct the transformation matrix */
  gdouble tmat[9];
  switch (type)
  {
  case ALIGNMENT:
    matrix_z_alignment(tmat, v1);
    break;

  case PAXIS:
    matrix_v_rotation(tmat, v1, D2R * angle); /* convert degrees to radians */
    break;

  case REFLECTION:
    matrix_v_reflection(tmat, v1);
    break;

  default:
    gui_text_show(1, "Unknown transformation type.\n"); /* WARNING */
    return;
  }

  /* Zero translation */
  gdouble tvec[3] = {0.0, 0.0, 0.0};

  /* Update the matrix table */
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      m_transTable->item(i, j)->setText(QString("%1").arg(tmat[3 * i + j], 0, 'f', 4));
  for (int i = 0; i < 3; i++)
    m_transTable->item(i, 3)->setText(QString("%1").arg(tvec[i], 0, 'f', 4));
}

/**
 * C bridge: construct a transformation matrix from widget values.
 * Calls back into the EditDialog instance via the global main window.
 */
void construct_transmat(void)
{
  extern MainWindow *get_main_window();
  MainWindow *mw = get_main_window();
  if (mw && mw->getEditDialog())
    mw->getEditDialog()->do_construct_transmat();
}

extern "C" void region_move(gpointer direction)
{
  gint dir = GPOINTER_TO_INT(direction);
  gchar txt[60];
  GSList *list;
  struct model_pak *data;
  struct core_pak *core;

  data = (struct model_pak *) sysenv.active_model;
  if (!data)
    return;
  if (data->periodic != 2)
    return;
  if (!data->selection)
  {
    gui_text_show(1, "Empty selection.\n"); /* WARNING */
    return;
  }
  if (VEC3MAG(data->surface.depth_vec) < FRACTION_TOLERANCE)
  {
    gui_text_show(3, "This is only possible for surfaces generated in the current session.\n"); /* ERROR */
    return;
  }

  for (list = data->selection; list; list = g_slist_next(list))
  {
    core = (struct core_pak *) list->data;
    region_move_atom(core, dir, data);
  }

  coords_compute(data);
  connect_bonds(data);
  connect_molecules(data);
  calc_emp(data);

  sprintf(txt, "New surface dipole: %f\n", data->gulp.sdipole);
  gui_text_show(0, txt); /* STANDARD */
  redraw_canvas(1);      /* SINGLE */
}

extern "C" void region_change(gpointer region)
{
  gint r = GPOINTER_TO_INT(region);
  GSList *list;
  struct model_pak *data;
  struct core_pak *core;

  data = (struct model_pak *) sysenv.active_model;
  if (!data)
    return;
  if (!data->selection)
  {
    gui_text_show(1, "Empty selection.\n"); /* WARNING */
    return;
  }

  for (list = data->selection; list; list = g_slist_next(list))
  {
    core = (struct core_pak *) list->data;
    switch (r)
    {
    case 0:
      core->region = 1;
      if (core->shell)
        ((struct shel_pak *) core->shell)->region = 1;
      break;
    case 1:
      core->region = 2;
      if (core->shell)
        ((struct shel_pak *) core->shell)->region = 2;
      break;
    case 2:
      core->region = 3;
      if (core->shell)
        ((struct shel_pak *) core->shell)->region = 3;
      break;
    case 3:
      core->region = 4;
      if (core->shell)
        ((struct shel_pak *) core->shell)->region = 4;
      break;
    default:
      break;
    }
  }
  /* Re-apply the current colouring scheme so region colours update immediately. */
  extern void model_colour_scheme(gint, struct model_pak *);
  model_colour_scheme(data->colour_scheme, data);

  data->need_clear = TRUE;
  redraw_canvas(1); /* SINGLE */
}

extern "C" void region_growth_slice(gpointer growth)
{
  gint g = GPOINTER_TO_INT(growth);
  GSList *list;
  struct model_pak *data;
  struct core_pak *core;

  data = (struct model_pak *) sysenv.active_model;
  if (!data)
    return;
  if (data->periodic != 2)
    return;
  if (!data->selection)
  {
    gui_text_show(1, "Empty selection.\n"); /* WARNING */
    return;
  }

  for (list = data->selection; list; list = g_slist_next(list))
  {
    core = (struct core_pak *) list->data;
    switch (g)
    {
    case 0:
      core->growth = TRUE;
      break;
    case 1:
      core->growth = FALSE;
      break;
    default:
      break;
    }
  }
  /* Re-apply the current colouring scheme so growth slice colours update immediately. */
  extern void model_colour_scheme(gint, struct model_pak *);
  model_colour_scheme(data->colour_scheme, data);

  data->need_clear = TRUE;
  redraw_canvas(1); /* SINGLE */
}

/* Helper: create a styled group box */
static QGroupBox *make_group(const QString &title, QWidget *parent)
{
  auto *g = new QGroupBox(title, parent);
  g->setStyleSheet("QGroupBox { font-weight: bold; margin-top: 0.5em; padding-top: 0.5em; } "
                   "QGroupBox::title { subcontrol-origin: margin; left: 10px; }");
  return g;
}

/* Helper: create a label + icon button row for QVBoxLayout */
static void add_action(QVBoxLayout *layout, const QString &label_text, std::function<void()> callback)
{
  auto *hbox = new QHBoxLayout();
  hbox->setContentsMargins(0, 2, 0, 2);
  auto *label = new QLabel(label_text, nullptr);
  label->setStyleSheet("QLabel { font-weight: normal; }");
  hbox->addWidget(label);
  hbox->addStretch();
  auto *btn = new QPushButton(loadGdisIcon("GO"), "", nullptr);
  btn->setFixedSize(28, 28);
  btn->setStyleSheet("QPushButton { border: none; padding: 0; } "
                     "QPushButton:hover { background-color: rgba(180,180,255,0.3); }");
  QObject::connect(btn, &QPushButton::clicked, [callback]() { callback(); });
  hbox->addWidget(btn);
  layout->addLayout(hbox);
}

/* Helper: create a label + icon button row for QFormLayout */
static void add_action(QFormLayout *layout, const QString &label_text, std::function<void()> callback)
{
  auto *hbox = new QHBoxLayout();
  hbox->setContentsMargins(0, 2, 0, 2);
  auto *label = new QLabel(label_text, nullptr);
  label->setStyleSheet("QLabel { font-weight: normal; }");
  hbox->addWidget(label);
  hbox->addStretch();
  auto *btn = new QPushButton(loadGdisIcon("GO"), "", nullptr);
  btn->setFixedSize(28, 28);
  btn->setStyleSheet("QPushButton { border: none; padding: 0; } "
                     "QPushButton:hover { background-color: rgba(180,180,255,0.3); }");
  QObject::connect(btn, &QPushButton::clicked, [callback]() { callback(); });
  hbox->addWidget(btn);
  layout->addRow("", hbox);
}

/* Helper: add a button to a layout */
static QPushButton *add_btn(const QString &text, QWidget *parent, const QString &style = QString())
{
  auto *btn = new QPushButton(text, parent);
  if (!style.isEmpty())
    btn->setStyleSheet(style);
  return btn;
}

EditDialog::EditDialog(QWidget *parent) : QDialog(parent)
{
  setWindowTitle("Model Editing");
  setMinimumSize(800, 600);

  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(4, 4, 4, 4);
  mainLayout->setSpacing(4);

  m_tabs = new QTabWidget(this);

  setupBuilderPage();
  setupSpatialsPage();
  setupTransformationsPage();
  setupRegionsPage();
  setupLabellingPage();
  setupLibraryPage();

  /* Disable tabs that are under development / not yet implemented */
  m_tabs->setTabEnabled(4, false); /* Labelling */
  m_tabs->setTabEnabled(5, false); /* Library */

  mainLayout->addWidget(m_tabs);

  /* Close button */
  auto *btnBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
  connect(btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  mainLayout->addWidget(btnBox);

  refresh();
}

EditDialog::~EditDialog() {}

void EditDialog::refresh()
{
  /* Refresh spatial list in Qt table */
  populate_spatial_list();
}

void EditDialog::populate_spatial_list()
{
  /* Clear existing rows */
  m_spatialTable->setRowCount(0);

  /* Get spatial list from C side */
  extern struct model_pak *qt_get_active_model(void);
  struct model_pak *model = qt_get_active_model();
  if (!model)
    return;

  GSList *list;
  for (list = model->spatial; list; list = g_slist_next(list))
  {
    struct spatial_pak *spatial = (struct spatial_pak *) list->data;
    int n = g_slist_length(spatial->list);
    if (spatial->size)
      n /= spatial->size;

    const char *type = "vertices";
    switch (spatial->size)
    {
    case 1:
      type = "points";
      break;
    case 2:
      type = "vectors";
      break;
    case 3:
      type = "triangles";
      break;
    case 4:
      type = "quads";
      break;
    }

    int row = m_spatialTable->rowCount();
    m_spatialTable->insertRow(row);
    auto *labelItem = new QTableWidgetItem(QString::fromUtf8(spatial->label));
    labelItem->setData(Qt::UserRole, QVariant::fromValue((void *) spatial));
    m_spatialTable->setItem(row, 0, labelItem);
    m_spatialTable->setItem(row, 1, new QTableWidgetItem(QString::number(n)));
    m_spatialTable->setItem(row, 2, new QTableWidgetItem(QString::fromUtf8(type)));
  }
}

/* ============================================================
   BUILDER PAGE
   ============================================================ */

void EditDialog::setupBuilderPage()
{
  auto *page = new QWidget();
  auto *layout = new QVBoxLayout(page);
  layout->setSpacing(6);

  /* --- Atoms group --- */
  auto *atomsGroup = make_group("Atoms", page);
  auto *atomsLayout = new QVBoxLayout(atomsGroup);

  add_action(atomsLayout, "Add atoms", [this]() { on_add_atoms(); });
  add_action(atomsLayout, "Confine atoms to cell", [this]() { on_confine_atoms(); });
  add_action(atomsLayout, "Confine molecules to cell", [this]() { on_confine_molecules(); });
  add_action(atomsLayout, "Add shells to selected cores", [this]() { on_add_shells(); });
  add_action(atomsLayout, "Remove shells from selected cores", [this]() { on_del_shells(); });

  layout->addWidget(atomsGroup);

  /* --- Bonds group --- */
  auto *bondsGroup = make_group("Bonds", page);
  auto *bondsLayout = new QVBoxLayout(bondsGroup);

  add_action(bondsLayout, "Add single bonds", [this]() { on_add_bond_single(); });
  add_action(bondsLayout, "Delete bonds", [this]() { on_del_bonds(); });
  add_action(bondsLayout, "Toggle bonding", [this]() { on_toggle_bonding(); });

  layout->addWidget(bondsGroup);

  /* --- Structural group --- */
  auto *structGroup = make_group("Structural", page);
  auto *structLayout = new QVBoxLayout(structGroup);

  add_action(structLayout, "Make new model", [this]() { on_make_model(); });
  add_action(structLayout, "Make supercell", [this]() { on_make_supercell(); });
  add_action(structLayout, "Force structure to P1", [this]() { on_make_p1(); });

  layout->addWidget(structGroup);

  /* --- Nanotube group --- */
  auto *nanoGroup = make_group("Nanotube", page);
  auto *nanoLayout = new QFormLayout(nanoGroup);

  auto *chiralityLabel = new QLabel("Chirality:", nanoGroup);
  m_chiralitySpin[0] = new QSpinBox(nanoGroup);
  m_chiralitySpin[0]->setRange(0, 99);
  m_chiralitySpin[1] = new QSpinBox(nanoGroup);
  m_chiralitySpin[1]->setRange(0, 99);

  auto *chiralityHLayout = new QHBoxLayout();
  chiralityHLayout->addWidget(m_chiralitySpin[0]);
  chiralityHLayout->addWidget(m_chiralitySpin[1]);

  nanoLayout->addRow(chiralityLabel, chiralityHLayout);

  m_basisEntry[0] = new QLineEdit("C", nanoGroup);
  m_basisEntry[0]->setFixedWidth(60);
  m_basisEntry[1] = new QLineEdit("C", nanoGroup);
  m_basisEntry[1]->setFixedWidth(60);

  /* Sync initial values to CEDIT */
  extern void qt_set_edit_basis(int idx, const char *text);
  extern void qt_set_edit_length(double val);
  extern void qt_set_edit_chirality(int idx, gdouble val);
  qt_set_edit_basis(0, "C");
  qt_set_edit_basis(1, "C");
  qt_set_edit_length(1.44);
  qt_set_edit_chirality(0, 6.0);
  qt_set_edit_chirality(1, 6.0);

  /* Combined row: Basis "C" - "C" : 1.44 */
  auto *basisHLayout = new QHBoxLayout();
  basisHLayout->addWidget(new QLabel("Basis:", nanoGroup));
  basisHLayout->addWidget(m_basisEntry[0]);
  basisHLayout->addWidget(new QLabel("-", nanoGroup));
  basisHLayout->addWidget(m_basisEntry[1]);
  basisHLayout->addWidget(new QLabel(" : ", nanoGroup));
  m_nanotubeLengthSpin = new QDoubleSpinBox(nanoGroup);
  m_nanotubeLengthSpin->setRange(0.1, 5.0);
  m_nanotubeLengthSpin->setSingleStep(0.05);
  m_nanotubeLengthSpin->setDecimals(2);
  m_nanotubeLengthSpin->setValue(1.44);
  basisHLayout->addWidget(m_nanotubeLengthSpin);

  /* Sync chirality to CEDIT on change */
  connect(m_chiralitySpin[0], QOverload<int>::of(&QSpinBox::valueChanged), [this](int val) {
    extern void qt_set_edit_chirality(int idx, gdouble val);
    qt_set_edit_chirality(0, (gdouble) val);
  });
  connect(m_chiralitySpin[1], QOverload<int>::of(&QSpinBox::valueChanged), [this](int val) {
    extern void qt_set_edit_chirality(int idx, gdouble val);
    qt_set_edit_chirality(1, (gdouble) val);
  });

/* Sync length to CEDIT on change */
  connect(m_nanotubeLengthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double val) {
    extern void qt_set_edit_length(double val);
    qt_set_edit_length(val);
  });
  nanoLayout->addRow("", basisHLayout);

  /* Sync basis entries to CEDIT on text change */
  connect(m_basisEntry[0], &QLineEdit::textChanged, [this](const QString &text) {
    extern void qt_set_edit_basis(int idx, const char *text);
    qt_set_edit_basis(0, text.toUtf8().constData());
  });
  connect(m_basisEntry[1], &QLineEdit::textChanged, [this](const QString &text) {
    extern void qt_set_edit_basis(int idx, const char *text);
    qt_set_edit_basis(1, text.toUtf8().constData());
  });

  add_action(nanoLayout, "Create nanotube", [this]() { on_create_nanotube(); });

  layout->addWidget(nanoGroup);

  layout->addStretch();
  m_tabs->addTab(page, "Builder");
}

/* ============================================================
   SPATIALS PAGE
   ============================================================ */

void EditDialog::setupSpatialsPage()
{
  auto *page = new QWidget();
  auto *layout = new QHBoxLayout(page);
  layout->setSpacing(6);

  /* Left pane: controls */
  auto *leftLayout = new QVBoxLayout();
  leftLayout->setContentsMargins(0, 0, 0, 0);

  /* Adding group */
  auto *addGroup = make_group("Adding", page);
  auto *addGroupLayout = new QVBoxLayout(addGroup);

  add_action(addGroupLayout, "Add vectors", [this]() { on_define_vector(); });
  add_action(addGroupLayout, "Add planes", [this]() { on_define_plane(); });
  add_action(addGroupLayout, "Add ribbons", [this]() { on_define_ribbon(); });

  leftLayout->addWidget(addGroup);

  /* Deleting group */
  auto *delGroup = make_group("Deleting", page);
  auto *delGroupLayout = new QVBoxLayout(delGroup);

  add_action(delGroupLayout, "Delete all vectors", [this]() { on_delete_all_vectors(); });
  add_action(delGroupLayout, "Delete all planes", [this]() { on_delete_all_planes(); });
  add_action(delGroupLayout, "Delete all", [this]() { on_delete_all_spatial(); });
  add_action(delGroupLayout, "Delete selected", [this]() { on_delete_selected_spatial(); });

  leftLayout->addWidget(delGroup);

  /* Colour groups */
  auto *spatialColorGroup = make_group("Spatial fill colour", page);
  auto *spatialColorLayout = new QVBoxLayout(spatialColorGroup);

  m_spatialColour = new QColor(128, 128, 255);
  auto *spatialColorBtn = add_btn("Pick colour", spatialColorGroup);
  connect(spatialColorBtn, &QPushButton::clicked, [this]() {
    auto color = QColorDialog::getColor(*m_spatialColour, this, "Spatial fill colour");
    if (color.isValid())
    {
      m_spatialColour->setRgb(color.red(), color.green(), color.blue());
    }
  });
  spatialColorLayout->addWidget(spatialColorBtn);

  add_action(spatialColorLayout, "Apply to all", [this]() { on_spatial_colour_all(); });
  add_action(spatialColorLayout, "Apply to selected", [this]() { on_spatial_colour_select(); });

  leftLayout->addWidget(spatialColorGroup);

  auto *labelColorGroup = make_group("Spatial label colour", page);
  auto *labelColorLayout = new QVBoxLayout(labelColorGroup);

  m_labelColour = new QColor(255, 255, 255);
  auto *labelColorBtn = add_btn("Pick colour", labelColorGroup);
  connect(labelColorBtn, &QPushButton::clicked, [this]() {
    auto color = QColorDialog::getColor(*m_labelColour, this, "Spatial label colour");
    if (color.isValid())
    {
      m_labelColour->setRgb(color.red(), color.green(), color.blue());
    }
  });
  labelColorLayout->addWidget(labelColorBtn);

  add_action(labelColorLayout, "Apply to all", [this]() { on_spatial_colour_all(); });
  add_action(labelColorLayout, "Apply to selected", [this]() { on_spatial_colour_select(); });

  leftLayout->addWidget(labelColorGroup);
  leftLayout->addStretch();

  layout->addLayout(leftLayout, 0);

  /* Right pane: spatial list */
  auto *rightGroup = make_group("Spatials", page);
  auto *rightLayout = new QVBoxLayout(rightGroup);

  m_spatialTable = new QTableWidget(0, 3, page);
  m_spatialTable->setHorizontalHeaderLabels({"Label", "Size", "Primitive"});
  m_spatialTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  m_spatialTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  m_spatialTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  m_spatialTable->horizontalHeader()->setHighlightSections(false);
  m_spatialTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_spatialTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_spatialTable->setAlternatingRowColors(true);

  connect(m_spatialTable->selectionModel(), &QItemSelectionModel::currentChanged, this,
          &EditDialog::on_spatial_selection_changed);

  rightLayout->addWidget(m_spatialTable);
  layout->addWidget(rightGroup, 1);
  m_tabs->addTab(page, "Spatials");
}

/* ============================================================
   TRANSFORMATIONS PAGE
   ============================================================ */

void EditDialog::setupTransformationsPage()
{
  auto *page = new QWidget();
  auto *layout = new QVBoxLayout(page);
  layout->setSpacing(6);

  /* Construction group */
  auto *constructGroup = make_group("Construction", page);
  auto *constructLayout = new QVBoxLayout(constructGroup);

  auto *angleLayout = new QHBoxLayout();
  angleLayout->addWidget(new QLabel("Rotation angle (degrees):", constructGroup));
  m_axisAngleEntry = new QLineEdit(constructGroup);
  m_axisAngleEntry->setFixedWidth(80);
  angleLayout->addWidget(m_axisAngleEntry);
  constructLayout->addLayout(angleLayout);

  auto *refLayout = new QHBoxLayout();
  refLayout->addWidget(new QLabel("Reference spatial object:", constructGroup));
  m_refSpatialEntry = new QLineEdit(constructGroup);
  m_refSpatialEntry->setFixedWidth(120);
  refLayout->addWidget(m_refSpatialEntry);
  constructLayout->addLayout(refLayout);

  auto *btnLayout = new QHBoxLayout();
  m_constructCombo = new QComboBox(constructGroup);
  m_constructCombo->addItems(
      {"identity matrix", "lattice matrix", "reflection matrix", "rotation matrix", "z alignment matrix"});
  btnLayout->addWidget(m_constructCombo);

  auto *constructBtn = add_btn("Construct", constructGroup);
  connect(constructBtn, &QPushButton::clicked, this, &EditDialog::on_construct_transmat);
  btnLayout->addWidget(constructBtn);
  constructLayout->addLayout(btnLayout);

  /* 3x4 matrix table: 3x3 rotation + translation column */
  m_transTable = new QTableWidget(3, 4, constructGroup);
  m_transTable->setHorizontalHeaderLabels({"a*", "b*", "c*", "t"});
  m_transTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
  m_transTable->verticalHeader()->setDefaultSectionSize(28);
  QStringList rowLabels = {"a", "b", "c"};
  m_transTable->setVerticalHeaderLabels(rowLabels);
  m_transTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  m_transTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

  /* Pre-fill with identity + zeros */
  for (int i = 0; i < 3; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      auto *item = new QTableWidgetItem();
      item->setTextAlignment(Qt::AlignCenter);
      if (i == j)
        item->setText("1.00");
      else
        item->setText("0.00");
      m_transTable->setItem(i, j, item);
    }
    /* Translation column */
    auto *transItem = new QTableWidgetItem();
    transItem->setText("0.00");
    transItem->setTextAlignment(Qt::AlignCenter);
    m_transTable->setItem(i, 3, transItem);
  }

  constructLayout->addWidget(m_transTable);
  layout->addWidget(constructGroup);

  /* Coordinate transformations group */
  auto *coordGroup = make_group("Coordinate transformations", page);
  auto *coordLayout = new QVBoxLayout(coordGroup);

  add_action(coordLayout, "Apply to all atoms", [this]() { on_apply_transmat_atoms(); });
  add_action(coordLayout, "Apply to selected atoms", [this]() { on_apply_transmat_selected(); });

  layout->addWidget(coordGroup);

  /* Lattice transformations group */
  auto *latGroup = make_group("Lattice transformations", page);
  auto *latLayout = new QVBoxLayout(latGroup);

  add_action(latLayout, "Apply to lattice matrix", [this]() { on_apply_latmat(); });

  auto *periodLayout = new QHBoxLayout();
  periodLayout->addWidget(new QLabel("Alter lattice periodicity:", latGroup));
  auto *periodSpin = new QSpinBox(latGroup);
  periodSpin->setRange(0, 3);
  periodLayout->addWidget(periodSpin);

  auto *modPeriodBtn = add_btn("Modify", latGroup);
  connect(modPeriodBtn, &QPushButton::clicked, [this, periodSpin]() {
    extern void qt_cb_modify_periodicity(gint);
    qt_cb_modify_periodicity(periodSpin->value());
  });
  periodLayout->addWidget(modPeriodBtn);

  latLayout->addLayout(periodLayout);

  add_action(latLayout, "Create new lattice model from linear combination", [this]() { on_apply_transmat_lattice(); });

  layout->addWidget(latGroup);
  layout->addStretch();
  m_tabs->addTab(page, "Transformations");
}

/* ============================================================
   REGIONS PAGE
   ============================================================ */

void EditDialog::setupRegionsPage()
{
  auto *page = new QWidget();
  auto *layout = new QVBoxLayout(page);
  layout->setSpacing(6);

  auto *group = make_group("Region Options", page);
  auto *groupLayout = new QVBoxLayout(group);

  add_action(groupLayout, "Move selection up", [this]() { on_region_move_up(); });
  add_action(groupLayout, "Move selection down", [this]() { on_region_move_down(); });

  groupLayout->addSpacing(4);

  add_action(groupLayout, "Add selected atoms to region 1", [this]() { on_region_add_1a(); });
  add_action(groupLayout, "Add selected atoms to region 2", [this]() { on_region_add_2a(); });
  add_action(groupLayout, "Add selected atoms to region 3", [this]() { on_region_add_1b(); });
  add_action(groupLayout, "Add selected atoms to region 4", [this]() { on_region_add_2b(); });

  groupLayout->addSpacing(4);

  add_action(groupLayout, "Add selected atoms to growth slice", [this]() { on_region_growth_add(); });
  add_action(groupLayout, "Remove selected atoms from growth slice", [this]() { on_region_growth_del(); });

  layout->addWidget(group);
  layout->addStretch();
  m_tabs->addTab(page, "Regions");
}

/* ============================================================
   LABELLING PAGE
   ============================================================ */

void EditDialog::setupLabellingPage()
{
  auto *page = new QWidget();
  auto *layout = new QVBoxLayout(page);
  layout->setSpacing(6);

  /* Typing group */
  auto *typingGroup = make_group("Typing", page);
  auto *typingLayout = new QVBoxLayout(typingGroup);

  auto *assignLayout = new QHBoxLayout();
  assignLayout->addWidget(new QLabel("Assign atom:", typingGroup));

  m_ffLabelCombo = new QComboBox(typingGroup);
  m_ffLabelCombo->addItems({"QEq charges", "Gasteiger charges", "Dreiding labels", "CVFF labels"});
  assignLayout->addWidget(m_ffLabelCombo);

  auto *typeBtn = new QPushButton(loadGdisIcon("GO"), "", typingGroup);
  typeBtn->setFixedSize(28, 28);
  typeBtn->setStyleSheet("QPushButton { border: none; padding: 0; } "
                         "QPushButton:hover { background-color: rgba(180,180,255,0.3); }");
  connect(typeBtn, &QPushButton::clicked, this, &EditDialog::on_type_model);
  assignLayout->addWidget(typeBtn);

  typingLayout->addLayout(assignLayout);
  layout->addWidget(typingGroup);

  /* Experimental typing group */
  auto *expGroup = make_group("Experimental typing", page);
  auto *expLayout = new QFormLayout(expGroup);

  m_ffLabelEntry = new QLineEdit(expGroup);
  expLayout->addRow("FF label:", m_ffLabelEntry);

  m_ffElementEntry = new QLineEdit(expGroup);
  expLayout->addRow("Neighbour Element:", m_ffElementEntry);

  m_ffDistanceEntry = new QLineEdit(expGroup);
  expLayout->addRow("Neighbour Distance:", m_ffDistanceEntry);

  m_ffCountEntry = new QLineEdit(expGroup);
  expLayout->addRow("Neighbour Count:", m_ffCountEntry);

  auto *makeRuleBtn = new QPushButton(loadGdisIcon("GO"), "", expGroup);
  makeRuleBtn->setFixedSize(28, 28);
  makeRuleBtn->setStyleSheet("QPushButton { border: none; padding: 0; } "
                             "QPushButton:hover { background-color: rgba(180,180,255,0.3); }");
  connect(makeRuleBtn, &QPushButton::clicked, this, &EditDialog::on_make_rule);
  expLayout->addRow("", makeRuleBtn);

  layout->addWidget(expGroup);

  /* Import forcefield group */
  auto *importGroup = make_group("Experimental - Import Forcefield", page);
  auto *importLayout = new QHBoxLayout(importGroup);

  m_ffImportEntry = new QLineEdit("ffoplsaabon.itp", importGroup);
  importLayout->addWidget(new QLabel("Import GROMACS FF:", importGroup));
  importLayout->addWidget(m_ffImportEntry);

  auto *importBtn = new QPushButton(loadGdisIcon("GO"), "", importGroup);
  importBtn->setFixedSize(28, 28);
  importBtn->setStyleSheet("QPushButton { border: none; padding: 0; } "
                           "QPushButton:hover { background-color: rgba(180,180,255,0.3); }");
  connect(importBtn, &QPushButton::clicked, this, &EditDialog::on_import_ff);
  importLayout->addWidget(importBtn);

  layout->addWidget(importGroup);
  layout->addStretch();
  m_tabs->addTab(page, "Labelling");
}

/* ============================================================
   LIBRARY PAGE
   ============================================================ */

void EditDialog::setupLibraryPage()
{
  auto *page = new QWidget();
  auto *layout = new QVBoxLayout(page);
  layout->setSpacing(6);

  auto *group = make_group("Molecular Library", page);
  auto *groupLayout = new QVBoxLayout(group);

  auto *nameLayout = new QHBoxLayout();
  nameLayout->addWidget(new QLabel("Name:", group));
  m_libraryNameEntry = new QLineEdit(group);
  nameLayout->addWidget(m_libraryNameEntry);
  groupLayout->addLayout(nameLayout);

  auto *btnLayout = new QHBoxLayout();
  m_librarySaveBtn = add_btn("Save selected to library", group);
  connect(m_librarySaveBtn, &QPushButton::clicked, this, &EditDialog::on_library_save);
  btnLayout->addWidget(m_librarySaveBtn);

  auto *loadBtn = add_btn("Load from library", group);
  connect(loadBtn, &QPushButton::clicked, this, &EditDialog::on_library_load);
  btnLayout->addWidget(loadBtn);
  groupLayout->addLayout(btnLayout);

  m_libraryList = new QListWidget(group);
  groupLayout->addWidget(m_libraryList);

  layout->addWidget(group);
  layout->addStretch();
  m_tabs->addTab(page, "Library");
}

/* ============================================================
   CALLBACKS
   ============================================================ */

/* Builder callbacks */
void EditDialog::on_add_atoms()
{
  extern void edit_atom_add(void);
  edit_atom_add();
}
void EditDialog::on_confine_atoms()
{
  extern void edit_confine(gpointer);
  edit_confine(GINT_TO_POINTER(CORE));
}
void EditDialog::on_confine_molecules()
{
  extern void edit_confine(gpointer);
  edit_confine(GINT_TO_POINTER(MOL));
}
void EditDialog::on_add_shells()
{
  extern void edit_shells_add(void);
  edit_shells_add();
}
void EditDialog::on_del_shells()
{
  extern void edit_shells_delete(void);
  edit_shells_delete();
}
void EditDialog::on_add_bond_single() { gui_mode_switch(BOND_SINGLE); }
void EditDialog::on_del_bonds() { gui_mode_switch(BOND_DELETE); }
void EditDialog::on_toggle_bonding()
{
  extern void qt_force_canvas_refresh(void);
  gui_connect_toggle();
  /* Force immediate repaint — the redraw may be queued while dialog has focus. */
  qt_force_canvas_refresh();
}
void EditDialog::on_make_model()
{
  extern void edit_model_create(void);
  edit_model_create();
}
void EditDialog::on_make_supercell()
{
  extern void edit_make_supercell(void);
  edit_make_supercell();
}
void EditDialog::on_make_p1()
{
  extern void edit_make_p1(void);
  edit_make_p1();

  /* Refresh symmetry table to show updated P1 values */
  extern void qt_gui_symmetry_refresh(gpointer);
  qt_gui_symmetry_refresh(NULL);
}
void EditDialog::on_create_nanotube()
{
  /* Ensure a model exists before creating nanotube */
  extern void edit_model_create(void);
  extern void edit_nanotube_new(void);
  extern struct model_pak *qt_get_active_model(void);
  extern void qt_set_edit_basis(int idx, const char *text);
  extern void qt_set_edit_length(double val);

  /* Sync current UI values to CEDIT before creating nanotube */
  qt_set_edit_basis(0, m_basisEntry[0]->text().toUtf8().constData());
  qt_set_edit_basis(1, m_basisEntry[1]->text().toUtf8().constData());
  qt_set_edit_length(m_nanotubeLengthSpin->value());

  /* Ensure a model exists (safety net) */
  if (!qt_get_active_model())
    edit_model_create();

  edit_nanotube_new();

  /* Refresh the canvas to show the new model */
  extern void redraw_canvas(gint);
  extern void qt_update_content_table(void);
  redraw_canvas(1); /* ALL */
  qt_update_content_table();
}

/* Spatials callbacks */
void EditDialog::on_define_vector() { gui_mode_switch(DEFINE_VECTOR); }
void EditDialog::on_define_plane() { gui_mode_switch(DEFINE_PLANE); }
void EditDialog::on_define_ribbon() { gui_mode_switch(DEFINE_RIBBON); }
void EditDialog::on_delete_all_vectors()
{
  extern void spatial_destroy_by_type(gint, struct model_pak *);
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (model) {
    spatial_destroy_by_type(SPATIAL_VECTOR, model);
    model->need_clear = TRUE;
    redraw_canvas(ALL);
    extern void qt_refresh_edit_dialog_spatial_list(void);
    qt_refresh_edit_dialog_spatial_list();
  }
}
void EditDialog::on_delete_all_planes()
{
  extern void spatial_destroy_by_type(gint, struct model_pak *);
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (model) {
    spatial_destroy_by_type(SPATIAL_GENERIC, model);
    model->need_clear = TRUE;
    redraw_canvas(ALL);
    extern void qt_refresh_edit_dialog_spatial_list(void);
    qt_refresh_edit_dialog_spatial_list();
  }
}
void EditDialog::on_delete_all_spatial()
{
  extern void gui_spatial_delete_all(void);
  gui_spatial_delete_all();
}
void EditDialog::on_delete_selected_spatial()
{
  delete_selected_spacial();
}
void EditDialog::on_spatial_colour_all()
{
  gdouble col[3];
  col[0] = m_spatialColour->redF();
  col[1] = m_spatialColour->greenF();
  col[2] = m_spatialColour->blueF();
  gui_spatial_colour_all(col);
}
void EditDialog::on_spatial_colour_select()
{
  gdouble col[3];
  col[0] = m_spatialColour->redF();
  col[1] = m_spatialColour->greenF();
  col[2] = m_spatialColour->blueF();
  gui_spatial_colour_select(col);
}

/* Transformations callbacks */
void EditDialog::on_construct_transmat()
{
  do_construct_transmat();
}
void EditDialog::sync_transmat_to_cedit()
{
  gdouble tmat[9], tvec[3];
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      tmat[3 * i + j] = m_transTable->item(i, j)->text().toDouble();
  /* Translation is in column 3 of each row */
  for (int i = 0; i < 3; i++)
    tvec[i] = m_transTable->item(i, 3)->text().toDouble();
  extern void qt_set_transmat_values(const gdouble *, const gdouble *);
  qt_set_transmat_values(tmat, tvec);
}

void EditDialog::on_apply_transmat_atoms()
{
  sync_transmat_to_cedit();
  extern void qt_apply_transmat(gpointer);
  qt_apply_transmat(GINT_TO_POINTER(0));
}
void EditDialog::on_apply_transmat_selected()
{
  sync_transmat_to_cedit();
  extern void qt_apply_transmat(gpointer);
  qt_apply_transmat(GINT_TO_POINTER(1));
}
void EditDialog::on_apply_transmat_lattice()
{
  sync_transmat_to_cedit();
  extern void qt_apply_transmat(gpointer);
  qt_apply_transmat(GINT_TO_POINTER(2)); /* AT_LATTICE - create new model */
}
void EditDialog::on_apply_latmat()
{
  sync_transmat_to_cedit();
  extern void edit_transform_latmat(void);
  edit_transform_latmat();
}
void EditDialog::on_modify_periodicity()
{
  extern void qt_cb_modify_periodicity(gint);
}

/* Periodicity is handled by the inline spin box in the transformations page */

/* Regions callbacks */
void EditDialog::on_region_move_up()
{
  extern void region_move(gpointer);
  region_move(GINT_TO_POINTER(UP));
}
void EditDialog::on_region_move_down()
{
  extern void region_move(gpointer);
  region_move(GINT_TO_POINTER(DOWN));
}
void EditDialog::on_region_add_1a()
{
  extern void region_change(gpointer);
  region_change(GINT_TO_POINTER(REGION1A));
}
void EditDialog::on_region_add_2a()
{
  extern void region_change(gpointer);
  region_change(GINT_TO_POINTER(REGION2A));
}
void EditDialog::on_region_add_1b()
{
  extern void region_change(gpointer);
  region_change(GINT_TO_POINTER(REGION1B));
}
void EditDialog::on_region_add_2b()
{
  extern void region_change(gpointer);
  region_change(GINT_TO_POINTER(REGION2B));
}
void EditDialog::on_region_growth_add()
{
  extern void region_growth_slice(gpointer);
  region_growth_slice(GINT_TO_POINTER(GROWTH_ADD));
}
void EditDialog::on_region_growth_del()
{
  extern void region_growth_slice(gpointer);
  region_growth_slice(GINT_TO_POINTER(GROWTH_DEL));
}

/* Labelling callbacks */
void EditDialog::on_type_model() { /* cb_type_model is a no-op */ }
/* on_make_rule and on_import_ff are no-ops */

/* Library callbacks */
void EditDialog::on_make_rule() { /* no-op */ }
void EditDialog::on_import_ff() { /* no-op */ }
void EditDialog::on_library_save() { gui_text_show(1, "Library save: not yet implemented in Qt.\n"); }
void EditDialog::on_library_load() { gui_text_show(1, "Library load: not yet implemented in Qt.\n"); }

/* Tree selection */
void EditDialog::on_spatial_selection_changed(const QModelIndex &current, const QModelIndex &)
{
  if (current.isValid())
  {
    void *spatial = current.data(Qt::UserRole).value<void *>();
    /* Set the C-side spatial_selected */
    sysenv.cedit.spatial_selected = spatial;
  }
}

/* Spatial colour and delete functions — moved from gui_still_valid.c */
extern "C" void gui_spatial_colour_all(gdouble *colour)
{
  GSList *list1, *list2;
  struct model_pak *model;
  struct spatial_pak *spatial;
  struct vec_pak *vertex;
  model = (struct model_pak *) sysenv.active_model;

  if (model)
  {
    for (list1 = model->spatial; list1; list1 = g_slist_next(list1))
    {
      spatial = (struct spatial_pak *) list1->data;

      for (list2 = spatial->list; list2; list2 = g_slist_next(list2))
      {
        vertex = (struct vec_pak *) list2->data;
        ARR3SET(vertex->colour, colour);
      }
    }
  }
  if (model)
    model->need_clear = TRUE;
  redraw_canvas(SINGLE);
  gui_refresh(GUI_CANVAS);
}

extern "C" void gui_spatial_colour_select(gdouble *colour)
{
  GSList *list;
  struct vec_pak *vertex;
  struct spatial_pak *spatial = (struct spatial_pak *) sysenv.cedit.spatial_selected;
  if (spatial)
  {
    for (list = spatial->list; list; list = g_slist_next(list))
    {
      vertex = (struct vec_pak *) list->data;
      ARR3SET(vertex->colour, colour);
    }
    struct model_pak *mdl = (struct model_pak *) sysenv.active_model;
    if (mdl)
      mdl->need_clear = TRUE;
    redraw_canvas(SINGLE);
    gui_refresh(GUI_CANVAS);
  }
}

extern "C" void gui_spatial_delete(gpointer data)
{
  const gchar *label = (const gchar *) data;
  if (sysenv.active_model)
    spatial_destroy_by_label(label, (struct model_pak *) sysenv.active_model);

  /* Force full redraw — deleted geometry leaves stale pixels. */
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (model) model->need_clear = TRUE;

  sysenv.refresh_dialog = TRUE;
  redraw_canvas(SINGLE);
  extern void qt_refresh_edit_dialog_spatial_list(void);
  qt_refresh_edit_dialog_spatial_list();
}

/* Edit toggle and periodicity functions — moved from gui_still_valid.c */
extern "C" void gui_connect_toggle(void)
{
  struct model_pak *model = (struct model_pak *) sysenv.active_model;

  if (model)
  {
    model->show_bonds ^= 1;
    model->surface.ignore_bonding ^= 1;
    model_content_refresh(model);

    /* When turning bonds OFF, the old bond pixels remain on screen because
     * SINGLE redraw doesn't clear the framebuffer. Force a full clear so
     * the empty space is properly repainted with background colour. */
    if (!model->show_bonds)
      model->need_clear = TRUE;

    redraw_canvas(SINGLE);
  }
}

extern "C" void qt_cb_modify_periodicity(gint val)
{
  guint i, n = (guint) val;
  gdouble d, x[3];
  struct model_pak *model;
  GSList *item;
  struct core_pak *core;

  model = (struct model_pak *) sysenv.active_model;
  if (!model)
    return;

  if (n > model->periodic)
  {
    for (i = model->periodic; i < n; i++)
    {
      VEC3SET(x, sysenv.cedit.tmat[i], sysenv.cedit.tmat[i + 3], sysenv.cedit.tmat[i + 6]);
      d = fabs(VEC3MAGSQ(x) - 1.0);
      if (d < FRACTION_TOLERANCE)
      {
        gui_text_show(WARNING, "A vector was unchanged from default.\n");
        return;
      }
    }
  }

  model->periodic = n;
  model_prep(model);
  model->need_clear = TRUE;
  sysenv.refresh_dialog = TRUE;

  /* Rebuild the Qt tree so the model icon reflects its new type. */
  extern void tree_model_add(struct model_pak *model);
  tree_model_add(model);

  redraw_canvas(SINGLE);
}
