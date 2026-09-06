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
 * Measurements dialog for GDIS Qt6 GUI
 *
 *   Manual tab: 4 buttons (Distance/Bond/Angle/Torsion) that switch selection mode
 *     so clicking atoms on the canvas creates measurements
 *   Search tab: Search type combo, atom match combos (Any/Selected/element),
 *     cutoff spinboxes, and a Search button
 */

#include "measurementdialog.h"

/* Need glib for GSList/GList used by pak.h and coords.h */
#include <glib.h>

/* Need coords.h for core_pak definition — wrap with extern "C" for C linkage */
#ifdef __cplusplus
extern "C" {
#endif
#include "coords.h"
#ifdef __cplusplus
}
#endif

/* Need pak.h for sysenv_pak definition */
#define MAX_ELEMENTS 120
#define MAX_DISPLAYED 9
#define FILELEN 512
#define LINELEN 300
#include "pak.h"

#include "gdis_api.h"
#include "glcanvas.h"
#include "measure.h"
#include "select.h"

/* matrix.h functions need C linkage */
#ifdef __cplusplus
extern "C" {
#endif
#include "matrix.h"
#ifdef __cplusplus
}
#endif

#include <QMessageBox>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QTreeView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QTabWidget>
#include <QGroupBox>
#include <QApplication>
#include <QMouseEvent>

#include "svg_utils.h"

/* sysenv is defined in main.c, declared in pak.h */
extern struct sysenv_pak sysenv;

/* Measurement item data role */
static const int MEAS_POINTER_ROLE = Qt::UserRole;

/* Global search cutoffs */
static gdouble measure_min_12 = 0.1, measure_max_12 = 2.0;
static gdouble measure_min_23 = 0.1, measure_max_23 = 2.0;
static gdouble measure_min_a = 0.1, measure_max_a = 180.0;

/* Mode constants matching interface.h enum: BOND_INFO=19, DIST_INFO=20, ANGLE_INFO=21, DIHEDRAL_INFO=22 */
enum { MODE_BOND_INFO = 19, MODE_DIST_INFO = 20, MODE_ANGLE_INFO = 21, MODE_DIHEDRAL_INFO = 22 };

/* Forward declarations */
extern void qt_repaint_canvas(void);
extern void qt_text_show(int type, const char *msg);
extern void qt_set_select_mode(int mode);
extern void coords_compute(struct model_pak *model);
extern void connect_refresh(struct model_pak *model);
extern void connect_fragment_init(struct model_pak *model);
extern GSList *connect_fragment_get(struct core_pak *c1, struct core_pak *c2, struct model_pak *model);

/* Helper: compute normal to plane defined by 3 points */
static void compute_normal(double *n, const double *p1, const double *p2, const double *p3)
{
  double a[3], b[3];
  ARR3SET(a, p1);
  ARR3SUB(a, p2);
  ARR3SET(b, p3);
  ARR3SUB(b, p2);
  crossprod(n, a, b);
  normalize(n, 3);
}

/* Helper: build a measurement type label */
static QString build_measurement_name(struct measure_pak *mp)
{
  char *type_label = measure_type_label_create(mp);
  QString name(QString::fromUtf8(type_label, strlen(type_label)));
  g_free(type_label);
  return name;
}

/* Helper: build measurement constituents string */
static QString build_constituents_string(struct measure_pak *mp)
{
  char *cons = measure_constituents_create(mp);
  QString s(QString::fromUtf8(cons, strlen(cons)));
  g_free(cons);
  return s;
}

/* Refresh the tree view from all models */
void MeasurementDialog::refresh_tree()
{
  m_model->clear();
  m_model->setHorizontalHeaderLabels({"Name", "Constituent atoms", "Value"});

  GSList *mal = sysenv.mal;
  while (mal)
  {
    struct model_pak *mp_model = static_cast<struct model_pak *>(mal->data);

    GSList *list = mp_model->measure_list;
    while (list)
    {
      struct measure_pak *mp = static_cast<struct measure_pak *>(list->data);

      int row = m_model->rowCount();
      m_model->setItem(row, 0, new QStandardItem(build_measurement_name(mp)));
      m_model->setItem(row, 1, new QStandardItem(build_constituents_string(mp)));
      m_model->setItem(row, 2, new QStandardItem(mp->value ? mp->value : "?"));

      m_model->item(row, 0)->setData(QVariant::fromValue<qlonglong>(reinterpret_cast<qlonglong>(mp)),
                                     MEAS_POINTER_ROLE);

      list = g_slist_next(list);
    }
    mal = g_slist_next(mal);
  }
}

MeasurementDialog::MeasurementDialog(QWidget *parent) : QDialog(parent)
{
  setWindowTitle("Measurements");
  setMinimumSize(500, 600);

  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setSpacing(4);
  mainLayout->setContentsMargins(8, 8, 8, 8);

  /* Tab widget: Manual | Search */
  m_tabWidget = new QTabWidget(this);

  /* ===== Manual tab ===== */
  auto *manualPage = new QWidget(this);
  setup_manual_tab(manualPage);
  m_tabWidget->addTab(manualPage, "Manual");

  /* ===== Search tab ===== */
  auto *searchPage = new QWidget(this);
  setup_search_tab(searchPage);
  m_tabWidget->addTab(searchPage, "Search");

  mainLayout->addWidget(m_tabWidget);

  /* ===== Tree view for measurements list ===== */
  auto *treeFrame = new QFrame(this);
  treeFrame->setFrameShape(QFrame::StyledPanel);
  auto *treeLayout = new QVBoxLayout(treeFrame);
  treeLayout->setSpacing(4);
  treeLayout->setContentsMargins(4, 4, 4, 4);

  auto *treeLabel = new QLabel("Label list", this);
  treeLabel->setStyleSheet("font-weight: bold;");
  treeLayout->addWidget(treeLabel);

  auto *swin = new QFrame(this);
  auto *swinLayout = new QVBoxLayout(swin);
  swinLayout->setContentsMargins(0, 0, 0, 0);

  m_model = new QStandardItemModel(this);

  m_treeView = new QTreeView(this);
  m_treeView->setModel(m_model);
  m_treeView->header()->setStretchLastSection(false);
  m_treeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  m_treeView->header()->setSectionResizeMode(1, QHeaderView::Stretch);
  m_treeView->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

  swinLayout->addWidget(m_treeView);
  treeLayout->addWidget(swin);

  /* Buttons row */
  auto *btnLayout = new QHBoxLayout();
  btnLayout->setSpacing(8);

  m_selectBtn = new QPushButton("Select", this);
  m_deleteBtn = new QPushButton("Delete", this);
  m_dumpBtn = new QPushButton("Dump", this);

  btnLayout->addStretch();
  btnLayout->addWidget(m_selectBtn);
  btnLayout->addWidget(m_deleteBtn);
  btnLayout->addWidget(m_dumpBtn);
  treeLayout->addLayout(btnLayout);

  mainLayout->addWidget(treeFrame);

  /* Connections */
  QObject::connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
                   &MeasurementDialog::on_tree_selection_changed);
  QObject::connect(m_selectBtn, &QPushButton::clicked, this, &MeasurementDialog::on_select_all);
  QObject::connect(m_deleteBtn, &QPushButton::clicked, this, &MeasurementDialog::on_delete_selected);
  QObject::connect(m_dumpBtn, &QPushButton::clicked, this, &MeasurementDialog::on_dump);
}

void MeasurementDialog::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Delete)
  {
    on_delete_selected();
    return;
  }
  QDialog::keyPressEvent(event);
}

void MeasurementDialog::setup_manual_tab(QWidget *page)
{
  auto *layout = new QVBoxLayout(page);
  layout->setSpacing(6);
  layout->setContentsMargins(4, 4, 4, 4);

  /* Frame - measurement type buttons using add_action_button */
  auto *frame = new QGroupBox("Measurement type", page);
  auto *vbox = new QVBoxLayout(frame);
  vbox->setSpacing(4);
  vbox->setContentsMargins(4, 4, 4, 4);

  add_action_button(vbox, "Measure bonds", [this]() { on_create_bond(); });
  add_action_button(vbox, "Measure distances", [this]() { on_create_distance(); });
  add_action_button(vbox, "Measure angles", [this]() { on_create_angle(); });
  add_action_button(vbox, "Measure torsions", [this]() { on_create_torsion(); });

  layout->addWidget(frame);

  /* Frame - measurement value editing */
  frame = new QGroupBox("Measurement value", page);
  auto *vbox2 = new QVBoxLayout(frame);
  vbox2->setSpacing(4);
  vbox2->setContentsMargins(4, 4, 4, 4);

  auto *hbox = new QHBoxLayout();
  auto *label = new QLabel("Value:", frame);
  m_valueEntry = new QLineEdit(frame);
  m_valueEntry->setPlaceholderText("Enter new value and press Enter");
  hbox->addWidget(label);
  hbox->addWidget(m_valueEntry);
  vbox2->addLayout(hbox);

  /* Connect value entry activation to update */
  QObject::connect(m_valueEntry, &QLineEdit::returnPressed, this, [this]() {
    auto *selection = m_treeView->selectionModel();
    QModelIndexList indexes = selection->selectedIndexes();
    if (indexes.isEmpty())
      return;

    int row = indexes.first().row();
    QStandardItem *item = m_model->item(row, 0);
    if (!item)
      return;

    QVariant var = item->data(MEAS_POINTER_ROLE);
    struct measure_pak *mp = reinterpret_cast<struct measure_pak *>(var.value<qlonglong>());
    if (!mp)
      return;

    const char *old_val = mp->value;

    /* Trim whitespace and parse as double */
    QString raw = m_valueEntry->text().trimmed();
    bool ok = false;
    double new_val_d = raw.toDouble(&ok);
    if (!ok)
      return; /* Invalid number, ignore */

    /* Parse old value */
    double old_val_d = old_val ? atof(old_val) : 0.0;
    double delta = new_val_d - old_val_d;

    /* Apply the delta to the geometry */
    struct model_pak *model = static_cast<struct model_pak *>(sysenv.active_model);
    if (!model)
      return;

    switch (mp->type)
    {
    case MEASURE_BOND:
    case MEASURE_DISTANCE:
    case MEASURE_INTRA:
    case MEASURE_INTER: {
      /* Move atom 2 along the bond vector by delta */
      double v[3], o[3];
      struct core_pak *core1 = static_cast<struct core_pak *>(mp->core[0]);
      struct core_pak *core2 = static_cast<struct core_pak *>(mp->core[1]);

      ARR3SET(v, core2->x);
      ARR3SUB(v, core1->x);
      vecmat(model->latmat, v);
      normalize(v, 3);
      VEC3MUL(v, delta);
      vecmat(model->ilatmat, v);

      connect_fragment_init(model);
      GSList *list2 = connect_fragment_get(core1, core2, model);
      GSList *list;
      for (list = list2; list; list = g_slist_next(list))
      {
        struct core_pak *core = static_cast<struct core_pak *>(list->data);
        ARR3ADD(core->x, v);
      }
      break;
    }

    case MEASURE_ANGLE: {
      /* Rotate atoms 3 (and beyond) around the axis core1-core2 by delta degrees */
      double v[3], o[3], mat[9];
      struct core_pak *core1 = static_cast<struct core_pak *>(mp->core[0]);
      struct core_pak *core2 = static_cast<struct core_pak *>(mp->core[1]);
      struct core_pak *core3 = static_cast<struct core_pak *>(mp->core[2]);

      compute_normal(v, core1->x, core2->x, core3->x);
      ARR3SET(o, core2->x);
      vecmat(model->latmat, o);
      delta *= 0.01745329252; /* D2R */
      matrix_v_rotation(mat, v, delta);

      connect_fragment_init(model);
      GSList *list2 = connect_fragment_get(core2, core3, model);
      GSList *list;
      for (list = list2; list; list = g_slist_next(list))
      {
        struct core_pak *core = static_cast<struct core_pak *>(list->data);
        vecmat(model->latmat, core->x);
        ARR3SUB(core->x, o);
        vecmat(mat, core->x);
        ARR3ADD(core->x, o);
        vecmat(model->ilatmat, core->x);

        if (core->shell)
        {
          struct shel_pak *shel = core->shell;
          vecmat(model->latmat, shel->x);
          ARR3SUB(shel->x, o);
          vecmat(mat, shel->x);
          ARR3ADD(shel->x, o);
          vecmat(model->ilatmat, shel->x);
        }
      }
      break;
    }

    case MEASURE_TORSION: {
      /* Rotate atoms 4 (and beyond) around the axis core2-core3 by delta degrees */
      double v[3], o[3], mat[9];
      struct core_pak *core2 = static_cast<struct core_pak *>(mp->core[1]);
      struct core_pak *core3 = static_cast<struct core_pak *>(mp->core[2]);
      struct core_pak *core4 = static_cast<struct core_pak *>(mp->core[3]);

      ARR3SET(v, core3->x);
      ARR3SUB(v, core2->x);
      normalize(v, 3);
      ARR3SET(o, core3->x);
      vecmat(model->latmat, o);
      delta *= -0.01745329252; /* -D2R */
      matrix_v_rotation(mat, v, delta);

      connect_fragment_init(model);
      GSList *list2 = connect_fragment_get(core3, core4, model);
      GSList *list;
      for (list = list2; list; list = g_slist_next(list))
      {
        struct core_pak *core = static_cast<struct core_pak *>(list->data);
        vecmat(model->latmat, core->x);
        ARR3SUB(core->x, o);
        vecmat(mat, core->x);
        ARR3ADD(core->x, o);
        vecmat(model->ilatmat, core->x);

        if (core->shell)
        {
          struct shel_pak *shel = core->shell;
          vecmat(model->latmat, shel->x);
          ARR3SUB(shel->x, o);
          vecmat(mat, shel->x);
          ARR3ADD(shel->x, o);
          vecmat(model->ilatmat, shel->x);
        }
      }
      break;
    }

    default:
      QMessageBox::warning(this, "Edit", "Sorry, can't adjust this measurement.");
      return;
    }

    /* Update geometry and display */
    coords_compute(model);
    connect_refresh(model);

    /* Update measurement value with clean formatted string */
    g_free(mp->value);
    mp->value = g_strdup_printf("%8.3f", new_val_d);
    measure_update_single(mp, model);

    /* Update tree view */
    m_model->setItem(row, 2, new QStandardItem(mp->value));

    /* Repaint canvas */
    qt_repaint_canvas();
  });

  layout->addWidget(frame);
  layout->addStretch();
}

void MeasurementDialog::setup_search_tab(QWidget *page)
{
  auto *layout = new QVBoxLayout(page);
  layout->setSpacing(6);
  layout->setContentsMargins(4, 4, 4, 4);

  /* Top row: search type combo + Search button */
  auto *topHbox = new QHBoxLayout();
  auto *searchTypeLabel = new QLabel("Search type:", page);
  m_searchTypeCombo = new QComboBox(page);
  m_searchTypeCombo->addItem("Bonds");
  m_searchTypeCombo->addItem("Distances");
  m_searchTypeCombo->addItem("Intermolecular");
  m_searchTypeCombo->addItem("Bond Angles");
  m_searchTypeCombo->addItem("Angles");
  m_searchTypeCombo->setCurrentText("Bonds");
  /* Make combo editable for search type */
  m_searchTypeCombo->setEditable(false);

  m_searchBtn = new QPushButton("Search", page);
  QObject::connect(m_searchBtn, &QPushButton::clicked, this, &MeasurementDialog::on_search_match);

  topHbox->addWidget(searchTypeLabel);
  topHbox->addWidget(m_searchTypeCombo);
  topHbox->addStretch();
  topHbox->addWidget(m_searchBtn);
  layout->addLayout(topHbox);

  /* Atom match table */
  auto *frame = new QGroupBox("Atom matching", page);
  auto *tableLayout = new QGridLayout(frame);
  tableLayout->setSpacing(4);
  tableLayout->setContentsMargins(4, 4, 4, 4);

  /* Column headers */
  auto *h1 = new QLabel("Atom 1", frame);
  auto *h2 = new QLabel("Atom 2", frame);
  auto *h3 = new QLabel("Atom 3", frame);
  tableLayout->addWidget(h1, 0, 0);
  tableLayout->addWidget(h2, 0, 1);
  tableLayout->addWidget(h3, 0, 2);

  /* Match combos - editable so user can type element names */
  m_match1Combo = new QComboBox(frame);
  m_match2Combo = new QComboBox(frame);
  m_match3Combo = new QComboBox(frame);
  m_match1Combo->setEditable(true);
  m_match1Combo->setInsertPolicy(QComboBox::NoInsert);
  m_match1Combo->addItem("Any");
  m_match1Combo->addItem("Selected");
  m_match2Combo->setEditable(true);
  m_match2Combo->setInsertPolicy(QComboBox::NoInsert);
  m_match2Combo->addItem("Any");
  m_match2Combo->addItem("Selected");
  m_match3Combo->setEditable(true);
  m_match3Combo->setInsertPolicy(QComboBox::NoInsert);
  m_match3Combo->addItem("Any");
  m_match3Combo->addItem("Selected");

  tableLayout->addWidget(m_match1Combo, 1, 0);
  tableLayout->addWidget(m_match2Combo, 1, 1);
  tableLayout->addWidget(m_match3Combo, 1, 2);

  layout->addWidget(frame);

  /* Cutoffs frame */
  auto *cutoffFrame = new QGroupBox("Cutoffs", page);
  auto *cutoffLayout = new QGridLayout(cutoffFrame);
  cutoffLayout->setSpacing(4);
  cutoffLayout->setContentsMargins(4, 4, 4, 4);

  /* 1-2 cutoffs */
  auto *label12 = new QLabel("1 - 2:", cutoffFrame);
  m_spinMin12 = new QDoubleSpinBox(cutoffFrame);
  m_spinMax12 = new QDoubleSpinBox(cutoffFrame);
  m_spinMin12->setRange(0.1, 100.0);
  m_spinMin12->setSingleStep(0.1);
  m_spinMin12->setValue(measure_min_12);
  m_spinMax12->setRange(0.1, 100.0);
  m_spinMax12->setSingleStep(0.1);
  m_spinMax12->setValue(measure_max_12);
  cutoffLayout->addWidget(label12, 0, 0);
  cutoffLayout->addWidget(m_spinMin12, 0, 1);
  cutoffLayout->addWidget(m_spinMax12, 0, 2);

  /* 2-3 cutoffs */
  auto *label23 = new QLabel("2 - 3:", cutoffFrame);
  m_spinMin23 = new QDoubleSpinBox(cutoffFrame);
  m_spinMax23 = new QDoubleSpinBox(cutoffFrame);
  m_spinMin23->setRange(0.1, 100.0);
  m_spinMin23->setSingleStep(0.1);
  m_spinMin23->setValue(measure_min_23);
  m_spinMax23->setRange(0.1, 100.0);
  m_spinMax23->setSingleStep(0.1);
  m_spinMax23->setValue(measure_max_23);
  cutoffLayout->addWidget(label23, 1, 0);
  cutoffLayout->addWidget(m_spinMin23, 1, 1);
  cutoffLayout->addWidget(m_spinMax23, 1, 2);

  /* Angle cutoffs */
  auto *labelAngle = new QLabel("Angle:", cutoffFrame);
  m_spinMinAngle = new QDoubleSpinBox(cutoffFrame);
  m_spinMaxAngle = new QDoubleSpinBox(cutoffFrame);
  m_spinMinAngle->setRange(0.0, 180.0);
  m_spinMinAngle->setSingleStep(1.0);
  m_spinMinAngle->setValue(measure_min_a);
  m_spinMaxAngle->setRange(0.0, 180.0);
  m_spinMaxAngle->setSingleStep(1.0);
  m_spinMaxAngle->setValue(measure_max_a);
  cutoffLayout->addWidget(labelAngle, 2, 0);
  cutoffLayout->addWidget(m_spinMinAngle, 2, 1);
  cutoffLayout->addWidget(m_spinMaxAngle, 2, 2);

  layout->addWidget(cutoffFrame);
  layout->addStretch();

  /* Connect search type change to enable/disable atom3 and cutoffs */
  QObject::connect(m_searchTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                   &MeasurementDialog::on_search_type_changed);

  /* Initial state */
  on_search_type_changed();
}

void MeasurementDialog::on_search_type_changed()
{
  const QString &type = m_searchTypeCombo->currentText();

  /* Default: hide atom3 and 2-3/angle cutoffs */
  m_match3Combo->setEnabled(false);
  m_spinMin23->setEnabled(false);
  m_spinMax23->setEnabled(false);
  m_spinMinAngle->setEnabled(false);
  m_spinMaxAngle->setEnabled(false);

  if (type == "Angles" || type == "Bond Angles")
  {
    m_match3Combo->setEnabled(true);
    m_spinMin23->setEnabled(true);
    m_spinMax23->setEnabled(true);
    m_spinMinAngle->setEnabled(true);
    m_spinMaxAngle->setEnabled(true);
  }
}

void MeasurementDialog::populate_tree() { refresh_tree(); }

void MeasurementDialog::update_selected_measurement()
{
  auto *selection = m_treeView->selectionModel();
  QModelIndexList indexes = selection->selectedIndexes();
  if (indexes.isEmpty())
    return;

  int row = indexes.first().row();
  QStandardItem *item = m_model->item(row, 0);
  if (!item)
    return;

  QVariant var = item->data(MEAS_POINTER_ROLE);
  struct measure_pak *mp = reinterpret_cast<struct measure_pak *>(var.value<qlonglong>());
  if (!mp)
    return;

  /* Update value entry */
  if (mp->value)
  {
    m_valueEntry->setText(QString::fromUtf8(mp->value, strlen(mp->value)));
  } else
  {
    m_valueEntry->clear();
  }
}

void MeasurementDialog::on_select_all()
{
  /* Select all measurements in the tree */
  auto *selection = m_treeView->selectionModel();
  QModelIndex topLeft = m_model->index(0, 0);
  QModelIndex bottomRight = m_model->index(m_model->rowCount() - 1, m_model->columnCount() - 1);
  QItemSelectionRange range(topLeft, bottomRight);
  QItemSelection sel;
  sel.append(range);
  selection->select(sel, QItemSelectionModel::Select | QItemSelectionModel::Rows);
}

void MeasurementDialog::on_delete_selected()
{
  auto *selection = m_treeView->selectionModel();
  QModelIndexList indexes = selection->selectedIndexes();
  if (indexes.isEmpty())
    return;

  /* Get unique rows */
  QSet<int> rows;
  for (const QModelIndex &idx : indexes)
    rows.insert(idx.row());

  /* Delete from back to front to preserve indices */
  QList<int> sortedRows = rows.values();
  std::sort(sortedRows.begin(), sortedRows.end(), std::greater<int>());

  for (int row : sortedRows)
  {
    QStandardItem *item = m_model->item(row, 0);
    if (!item)
      continue;

    QVariant var = item->data(MEAS_POINTER_ROLE);
    struct measure_pak *mp = reinterpret_cast<struct measure_pak *>(var.value<qlonglong>());
    if (!mp)
      continue;

    /* Find and remove from the model list */
    GSList *mal = sysenv.mal;
    while (mal)
    {
      struct model_pak *model = static_cast<struct model_pak *>(mal->data);
      GSList *list = model->measure_list;
      while (list)
      {
        struct measure_pak *mp2 = static_cast<struct measure_pak *>(list->data);
        if (mp2 == mp)
        {
          measure_free(mp, model);
          break;
        }
        list = g_slist_next(list);
      }
      mal = g_slist_next(mal);
    }

    m_model->removeRow(row);
  }

  /* Repaint canvas */
  qt_repaint_canvas();
}

void MeasurementDialog::on_dump()
{
  /* Dump all measurements to text output */
  GSList *mal = sysenv.mal;
  while (mal)
  {
    struct model_pak *model = static_cast<struct model_pak *>(mal->data);
    measure_dump_all(model);
    mal = g_slist_next(mal);
  }
}

void MeasurementDialog::on_tree_selection_changed()
{
  update_selected_measurement();

  /* Highlight selected measurement on canvas */
  auto *selection = m_treeView->selectionModel();
  QModelIndexList indexes = selection->selectedIndexes();
  if (indexes.isEmpty())
    return;

  int row = indexes.first().row();
  QStandardItem *item = m_model->item(row, 0);
  if (!item)
    return;

  QVariant var = item->data(MEAS_POINTER_ROLE);
  struct measure_pak *mp = reinterpret_cast<struct measure_pak *>(var.value<qlonglong>());
  if (!mp)
    return;

  /* Set measurement color to highlight */
  measure_colour_set(1.0, 1.0, 0.0, mp);

  /* Repaint canvas */
  qt_repaint_canvas();
}

void MeasurementDialog::on_create_bond()
{
  /* Switch to bond info mode */
  qt_set_select_mode(MODE_BOND_INFO);
  qt_repaint_canvas();
}

void MeasurementDialog::on_create_distance()
{
  /* Switch to distance info mode */
  qt_set_select_mode(MODE_DIST_INFO);
  qt_repaint_canvas();
}

void MeasurementDialog::on_create_angle()
{
  /* Switch to angle info mode */
  qt_set_select_mode(MODE_ANGLE_INFO);
  qt_repaint_canvas();
}

void MeasurementDialog::on_create_torsion()
{
  /* Switch to torsion info mode */
  qt_set_select_mode(MODE_DIHEDRAL_INFO);
  qt_repaint_canvas();
}

void MeasurementDialog::on_search_match()
{
  /* Get the active model */
  struct model_pak *model = static_cast<struct model_pak *>(sysenv.active_model);
  if (!model)
  {
    QMessageBox::warning(this, "Search", "No active model. Please load a molecule first.");
    return;
  }

  /* Get search type */
  const QString &searchType = m_searchTypeCombo->currentText();

  /* Get atom match labels — keep std::string alive to avoid dangling c_str() */
  QString label1Str = m_match1Combo->lineEdit()->text().trimmed();
  QString label2Str = m_match2Combo->lineEdit()->text().trimmed();
  QString label3Str = m_match3Combo->lineEdit()->text().trimmed();
  std::string label1Std = label1Str.toStdString();
  std::string label2Std = label2Str.toStdString();
  std::string label3Std = label3Str.toStdString();
  const char *label1 = label1Std.c_str();
  const char *label2 = label2Std.c_str();
  const char *label3 = label3Std.c_str();

  /* Debug: print what we're searching for */
  printf("[DEBUG] Search type='%s', label1='%s', label2='%s', label3='%s'\n", searchType.toStdString().c_str(), label1,
         label2, label3);

  /* Update global cutoffs from spinboxes */
  measure_min_12 = m_spinMin12->value();
  measure_max_12 = m_spinMax12->value();
  measure_min_23 = m_spinMin23->value();
  measure_max_23 = m_spinMax23->value();
  measure_min_a = m_spinMinAngle->value();
  measure_max_a = m_spinMaxAngle->value();

  /* Perform search based on type */
  if (searchType == "Bonds")
  {
    const char *labels[2] = {label1, label2};
    measure_bond_search(labels, measure_min_12, measure_max_12, model);
  } else if (searchType == "Distances")
  {
    const char *labels[2] = {label1, label2};
    measure_distance_search(labels, MEASURE_DISTANCE, measure_min_12, measure_max_12, model);
  } else if (searchType == "Intermolecular")
  {
    const char *labels[2] = {label1, label2};
    measure_distance_search(labels, MEASURE_INTER, measure_min_12, measure_max_12, model);
  } else if (searchType == "Bond Angles")
  {
    const char *labels[3] = {label1, label2, label3};
    measure_bangle_search(labels, measure_min_a, measure_max_a, model);
  } else if (searchType == "Angles")
  {
    const char *labels[3] = {label1, label2, label3};
    double range[6];
    range[0] = measure_min_12;
    range[1] = measure_max_12;
    range[2] = measure_min_23;
    range[3] = measure_max_23;
    range[4] = measure_min_a;
    range[5] = measure_max_a;
    measure_angle_search(labels, range, model);
  }

  /* Refresh the tree view */
  populate_tree();

  /* Repaint canvas */
  qt_repaint_canvas();
}

/* Global callback function pointer for measurement grafting */
static MeasurementDialog *g_measurement_dlg = nullptr;

extern "C" void qt_measure_grafted_callback(void)
{
  if (g_measurement_dlg)
  {
    g_measurement_dlg->refresh_tree();
  }
}

/* Qt bridge: show measurements dialog */
extern "C" void qt_show_measurements_dialog(void)
{
  extern QWidget *get_main_window_widget();
  MeasurementDialog *dlg = new MeasurementDialog(get_main_window_widget());
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  g_measurement_dlg = dlg;

  dlg->show();
}

/* Qt bridge: hide measurements dialog (cleanup) */
extern "C" void qt_hide_measurements_dialog(void) { g_measurement_dlg = nullptr; }
