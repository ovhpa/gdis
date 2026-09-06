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
 * USPEX Configuration Dialog for GDIS Qt6 GUI
 * Complete rewrite using QHBoxLayout rows (like VASP dialog) instead of QTableWidget.
 * Each row has a label on the left and widget(s) on the right.
 * All widgets are visible; locked widgets are disabled.
 */

#include "uspexdialog.h"

#include "gdis.h"
#include "file.h"
#include "parse.h"
#include "task.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "file_uspex.h"
#ifdef __cplusplus
}
#endif
#include "coords.h"
#include "model.h"
#include "interface.h"
#include "gdis_api.h"
#include "svg_utils.h"
#include "uspex_tooltips.h"

extern struct sysenv_pak sysenv;
#include <cstring>

extern struct elem_pak elements[];

#include "gui_uspex_internal.h"
extern "C" struct uspex_calc_gui uspex_gui;

/* External reference to the global uspex_gui struct */
// extern struct uspex_calc_gui uspex_gui;

/* Safe pointer check: returns true if pointer is non-NULL and likely valid
 * (not a dangling pointer like 0x100000000). */
static inline bool ptr_valid(void *p) { return p != NULL && (guintptr) p > 0x1000; }

/* gui_uspex.h already included above */

/* Declare save and exec from gui_uspex.c */
extern "C" gint save_uspex_calc();

#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QVBoxLayout>
#include <QGroupBox>

/* Pimpl - all widget pointers to avoid header include order issues */
struct UspexWidgets {
  /* SYSTEM page */
  QComboBox *calculationMethod = nullptr;
  QSpinBox *calctype_dim = nullptr;
  QDoubleSpinBox *ExternalPressure = nullptr;
  QCheckBox *sel_v1030_1 = nullptr;
  QComboBox *calculationType = nullptr;
  QCheckBox *calctype_mag = nullptr;
  QCheckBox *calctype_mol = nullptr;
  QCheckBox *calctype_var = nullptr;
  QComboBox *atomType = nullptr;
  QLineEdit *atom_sym = nullptr;
  QLineEdit *atom_typ = nullptr;
  QPushButton *apply_atom = nullptr;
  QLineEdit *atom_num = nullptr;
  QLineEdit *atom_val = nullptr;
  QPushButton *remove_atom = nullptr;
  QComboBox *numSpecies = nullptr;
  QLineEdit *blockSpecies = nullptr;
  QPushButton *Species_apply_button = nullptr;
  QPushButton *Species_delete_button = nullptr;
  QComboBox *goodBonds = nullptr;
  QLineEdit *bond_d = nullptr;
  QPushButton *apply_bonds = nullptr;
  QPushButton *remove_bonds = nullptr;
  QLineEdit *ldaU = nullptr;
  QCheckBox *auto_bonds = nullptr;
  QComboBox *optType = nullptr;
  QLineEdit *new_optType = nullptr;
  QCheckBox *sel_new_opt = nullptr;
  QCheckBox *anti_opt = nullptr;
  QCheckBox *checkMolecules = nullptr;
  QCheckBox *checkConnectivity = nullptr;
  QComboBox *Latticevalues = nullptr;
  QLineEdit *latticevalue = nullptr;
  QPushButton *apply_latticevalue = nullptr;
  QComboBox *latticeformat = nullptr;
  QLineEdit *splitInto = nullptr;
  QCheckBox *auto_C_lat = nullptr;
  QComboBox *IonDistances = nullptr;
  QLineEdit *distances = nullptr;
  QPushButton *apply_distances = nullptr;
  QLineEdit *minVectorLength = nullptr;
  QLineEdit *constraint_enhancement = nullptr;
  QCheckBox *auto_C_ion = nullptr;
  QComboBox *MolCenters = nullptr;
  QLineEdit *centers = nullptr;
  QPushButton *centers_button = nullptr;

  /* STRUCTURE page */
  QLineEdit *populationSize = nullptr;
  QLineEdit *initialPopSize = nullptr;
  QLineEdit *numGenerations = nullptr;
  QLineEdit *stopCrit = nullptr;
  QLineEdit *mag_nm = nullptr;
  QLineEdit *mag_fmls = nullptr;
  QLineEdit *mag_afml = nullptr;
  QLineEdit *mag_fmlh = nullptr;
  QCheckBox *calctype_mag_2 = nullptr;
  QLineEdit *mag_fmhs = nullptr;
  QLineEdit *mag_afmh = nullptr;
  QLineEdit *mag_aflh = nullptr;
  QLineEdit *bestFrac = nullptr;
  QLineEdit *keepBestHM = nullptr;
  QCheckBox *reoptOld = nullptr;
  QLineEdit *fitLimit = nullptr;
  QLineEdit *symmetries = nullptr;
  QLineEdit *fracGene = nullptr;
  QLineEdit *fracRand = nullptr;
  QLineEdit *fracTopRand = nullptr;
  QLineEdit *fracPerm = nullptr;
  QLineEdit *fracAtomsMut = nullptr;
  QLineEdit *fracRotMut = nullptr;
  QLineEdit *fracLatMut = nullptr;
  QLineEdit *fracSpinMut = nullptr;
  QLineEdit *howManySwaps = nullptr;
  QLineEdit *specificSwaps = nullptr;
  QLineEdit *mutationDegree = nullptr;
  QLineEdit *mutationRate = nullptr;
  QLineEdit *DisplaceInLatmutation = nullptr;
  QCheckBox *AutoFrac = nullptr;
  QLineEdit *RmaxFing = nullptr;
  QLineEdit *deltaFing = nullptr;
  QLineEdit *sigmaFing = nullptr;
  QLineEdit *toleranceFing = nullptr;
  QLineEdit *antiSeedsActivation = nullptr;
  QLineEdit *antiSeedsMax = nullptr;
  QLineEdit *antiSeedsSigma = nullptr;
  QCheckBox *doSpaceGroup = nullptr;
  QLineEdit *SymTolerance = nullptr;
  QLineEdit *firstGeneMax = nullptr;
  QLineEdit *minAt = nullptr;
  QLineEdit *maxAt = nullptr;
  QLineEdit *fracTrans = nullptr;
  QLineEdit *howManyTrans = nullptr;
  QLineEdit *specificTrans = nullptr;

  /* CALCULATION page */
  QRadioButton *use_specific = nullptr;
  QRadioButton *set_specific = nullptr;
  QLineEdit *spe_folder = nullptr;
  QPushButton *spe_folder_button = nullptr;
  QSpinBox *num_opt_steps = nullptr;
  QSpinBox *curr_step = nullptr;
  QCheckBox *auto_step = nullptr;
  QCheckBox *isfixed = nullptr;
  QComboBox *abinitioCode = nullptr;
  QLineEdit *KresolStart = nullptr;
  QLineEdit *vacuumSize = nullptr;
  QLineEdit *ai_input = nullptr;
  QPushButton *ai_input_button = nullptr;
  QLineEdit *ai_opt = nullptr;
  QPushButton *ai_opt_button = nullptr;
  QLineEdit *ai_lib = nullptr;
  QPushButton *ai_lib_button = nullptr;
  QComboBox *ai_lib_flavor = nullptr;
  QLineEdit *ai_lib_sel = nullptr;
  QLineEdit *commandExecutable = nullptr;
  QPushButton *load_abinitio_exe = nullptr;
  QPushButton *apply_step = nullptr;
  QLineEdit *job_uspex_exe = nullptr;
  QPushButton *load_uspex_exe = nullptr;
  QLineEdit *whichCluster = nullptr;
  QCheckBox *PhaseDiagram = nullptr;
  QCheckBox *sel_v1030_3 = nullptr;
  QLineEdit *numProcessors = nullptr;
  QLineEdit *numParallelCalcs = nullptr;
  QCheckBox *sel_octave = nullptr;
  QLineEdit *job_path = nullptr;
  QPushButton *uspex_path_dialog = nullptr;
  QLineEdit *remoteFolder = nullptr;
  QPushButton *load_remote_folder = nullptr;
  QCheckBox *pickUpYN = nullptr;
  QLineEdit *pickUpGen = nullptr;
  QLineEdit *pickUpFolder = nullptr;
  QCheckBox *restart_cleanup = nullptr;

  /* ADVANCED page */
  QLineEdit *repeatForStatistics = nullptr;
  QLineEdit *stopFitness = nullptr;
  QLineEdit *fixRndSeed = nullptr;
  QCheckBox *collectForces = nullptr;
  QCheckBox *ordering_active = nullptr;
  QCheckBox *symmetrize = nullptr;
  QLineEdit *valenceElectr = nullptr;
  QLineEdit *percSliceShift = nullptr;
  QLineEdit *minSlice = nullptr;
  QComboBox *dynamicalBestHM = nullptr;
  QLineEdit *maxSlice = nullptr;
  QLineEdit *softMutOnly = nullptr;
  QLineEdit *maxDistHeredity = nullptr;
  QLineEdit *numberparents = nullptr;
  QComboBox *manyParents = nullptr;
  QComboBox *TE_goal = nullptr;
  QLineEdit *BoltzTraP_T_max = nullptr;
  QLineEdit *BoltzTraP_T_delta = nullptr;
  QLineEdit *BoltzTraP_T_efcut = nullptr;
  QLineEdit *cmd_BoltzTraP = nullptr;
  QPushButton *cmd_BoltzTraP_button = nullptr;
  QLineEdit *TE_T_interest = nullptr;
  QLineEdit *TE_threshold = nullptr;
  QLineEdit *numIterations = nullptr;
  QLineEdit *shiftRatio = nullptr;
  QCheckBox *orderParaType = nullptr;
  QLineEdit *speciesSymbol = nullptr;
  QLineEdit *mass = nullptr;
  QLineEdit *amplitudeShoot_AB = nullptr;
  QLineEdit *amplitudeShoot_BA = nullptr;
  QLineEdit *magnitudeShoot_success = nullptr;
  QLineEdit *magnitudeShoot_failure = nullptr;
  QLineEdit *cmdOrderParameter = nullptr;
  QPushButton *cmdOrderParameter_button = nullptr;
  QLineEdit *opCriteria_start = nullptr;
  QLineEdit *cmdEnthalpyTemperature = nullptr;
  QPushButton *cmdEnthalpyTemperature_button = nullptr;
  QLineEdit *opCriteria_end = nullptr;
  QLineEdit *orderParameterFile = nullptr;
  QPushButton *orderParameterFile_button = nullptr;
  QLineEdit *enthalpyTemperatureFile = nullptr;
  QPushButton *enthalpyTemperatureFile_button = nullptr;
  QLineEdit *trajectoryFile = nullptr;
  QPushButton *trajectoryFile_button = nullptr;
  QLineEdit *MDrestartFile = nullptr;
  QPushButton *MDrestartFile_button = nullptr;

  /* SPECIFIC page */
  QComboBox *FullRelax = nullptr;
  QLineEdit *maxVectorLength = nullptr;
  QLineEdit *GaussianWidth = nullptr;
  QLineEdit *GaussianHeight = nullptr;
  QComboBox *meta_model = nullptr;
  QPushButton *meta_model_button = nullptr;
  QLineEdit *PSO_softMut = nullptr;
  QLineEdit *PSO_BestStruc = nullptr;
  QLineEdit *PSO_BestEver = nullptr;
  QComboBox *vcnebtype_method = nullptr;
  QLineEdit *vcnebType = nullptr;
  QCheckBox *vcnebtype_img_num = nullptr;
  QCheckBox *vcnebtype_spring = nullptr;
  QComboBox *optReadImages = nullptr;
  QLineEdit *numImages = nullptr;
  QLineEdit *numSteps = nullptr;
  QCheckBox *optFreezing = nullptr;
  QComboBox *optimizerType = nullptr;
  QLineEdit *dt = nullptr;
  QLineEdit *ConvThreshold = nullptr;
  QLineEdit *VarPathLength = nullptr;
  QComboBox *optRelaxType = nullptr;
  QLineEdit *K_min = nullptr;
  QLineEdit *K_max = nullptr;
  QLineEdit *Kconstant = nullptr;
  QComboBox *optMethodCIDI = nullptr;
  QLineEdit *startCIDIStep = nullptr;
  QLineEdit *pickupImages = nullptr;
  QLineEdit *PrintStep = nullptr;
  QComboBox *FormatType = nullptr;
  QComboBox *img_model = nullptr;
  QPushButton *img_model_button = nullptr;
  QComboBox *mol_model = nullptr;
  QSpinBox *num_mol = nullptr;
  QPushButton *mol_model_button = nullptr;
  QComboBox *mol_gdis = nullptr;
  QSpinBox *curr_mol = nullptr;
  QCheckBox *mol_gulp = nullptr;
  QPushButton *mol_apply_button = nullptr;
  QComboBox *substrate_model = nullptr;
  QPushButton *substrate_model_button = nullptr;
  QLineEdit *reconstruct = nullptr;
  QLineEdit *thicknessS = nullptr;
  QLineEdit *thicknessB = nullptr;
  QLineEdit *StoichiometryStart = nullptr;
  QLineEdit *E_AB = nullptr;
  QLineEdit *Mu_A = nullptr;
  QLineEdit *Mu_B = nullptr;
  QLineEdit *name = nullptr;
  QLineEdit *file_entry = nullptr;
  QPushButton *file_entry_button = nullptr;
  QGroupBox *molFrame = nullptr;
};

/* Free helper functions */
static void setWidgetTooltip(QWidget *widget, const QString &tooltip)
{
  if (widget)
    widget->setToolTip(tooltip);
}

/* Add a row to a QVBoxLayout: label + single widget */
static void addRow(QVBoxLayout *vbox, const QString &label, QWidget *widget, const QString &tooltip = "")
{
  auto *hbox = new QHBoxLayout();
  if (!label.isEmpty())
  {
    auto *lbl = new QLabel(label);
    lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    hbox->addWidget(lbl);
  }
  hbox->addWidget(widget);
  hbox->setContentsMargins(0, 2, 0, 2);
  vbox->addLayout(hbox);
  if (!tooltip.isEmpty())
    setWidgetTooltip(widget, tooltip);
}

/* Add a row to a QVBoxLayout: label + two widgets side by side */
static void addRow(QVBoxLayout *vbox, const QString &label, QWidget *w1, QWidget *w2, const QString &tooltip = "")
{
  auto *hbox = new QHBoxLayout();
  if (!label.isEmpty())
  {
    auto *lbl = new QLabel(label);
    lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    hbox->addWidget(lbl);
  }
  hbox->addWidget(w1);
  hbox->addWidget(w2);
  hbox->setContentsMargins(0, 2, 0, 2);
  vbox->addLayout(hbox);
  if (!tooltip.isEmpty())
    setWidgetTooltip(w1, tooltip);
}

/* Add a row to a QVBoxLayout: label + widget + button */
static void addRow(QVBoxLayout *vbox, const QString &label, QWidget *widget, QPushButton *btn,
                   const QString &tooltip = "")
{
  auto *hbox = new QHBoxLayout();
  if (!label.isEmpty())
  {
    auto *lbl = new QLabel(label);
    lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    hbox->addWidget(lbl);
  }
  hbox->addWidget(widget);
  hbox->addWidget(btn);
  hbox->setContentsMargins(0, 2, 0, 2);
  vbox->addLayout(hbox);
  if (!tooltip.isEmpty())
    setWidgetTooltip(widget, tooltip);
}

/* Add a row to a QVBoxLayout: just widgets (no label) */
static void addRow(QVBoxLayout *vbox, QWidget *w1, QWidget *w2 = nullptr, QWidget *w3 = nullptr,
                   const QString &tooltip = "")
{
  auto *hbox = new QHBoxLayout();
  hbox->addWidget(w1);
  if (w2)
    hbox->addWidget(w2);
  if (w3)
    hbox->addWidget(w3);
  hbox->setContentsMargins(0, 2, 0, 2);
  vbox->addLayout(hbox);
  if (!tooltip.isEmpty())
    setWidgetTooltip(w1, tooltip);
}

/* Add a group box frame to a QVBoxLayout, returns the inner layout */
static QVBoxLayout *addFrame(QVBoxLayout *pageLayout, const QString &title)
{
  auto *frame = new QGroupBox(title);
  auto *vbox = new QVBoxLayout(frame);
  vbox->setSpacing(2);
  vbox->setContentsMargins(6, 4, 6, 4);
  frame->setLayout(vbox);
  pageLayout->addWidget(frame);
  return vbox;
}

UspexDialog::UspexDialog(QWidget *parent, struct model_pak *model)
    : QDialog(parent), m_w(new UspexWidgets()), m_model(model)
{
  setWindowTitle(tr("USPEX Setup"));
  resize(800, 650);

  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(8, 6, 8, 6);
  mainLayout->setSpacing(6);

  /* --- MODEL NAME --- */
  auto *nameLayout = new QHBoxLayout();
  nameLayout->addWidget(new QLabel(tr("MODEL NAME:")));
  m_w->name = new QLineEdit();
  nameLayout->addWidget(m_w->name);
  setWidgetTooltip(m_w->name, TOOLTIP_NAME); // The model name will be used in USPEX files\nas well as GDIS display.");
  mainLayout->addLayout(nameLayout);

  /* --- CONNECTED OUTPUT --- */
  auto *outputLayout = new QHBoxLayout();
  outputLayout->addWidget(new QLabel(tr("CONNECTED OUTPUT:")));
  m_w->file_entry = new QLineEdit();
  outputLayout->addWidget(m_w->file_entry);
  m_w->file_entry_button = new QPushButton(tr("..."));
  outputLayout->addWidget(m_w->file_entry_button);
  setWidgetTooltip(m_w->file_entry, TOOLTIP_FILE_ENTRY);
  QObject::connect(m_w->file_entry_button, &QPushButton::clicked, this, &UspexDialog::on_file_entry_button_clicked);
  mainLayout->addLayout(outputLayout);

  m_notebook = new QTabWidget(this);
  m_notebook->setTabPosition(QTabWidget::North);
  m_notebook->setDocumentMode(true);
  mainLayout->addWidget(m_notebook);

  setupSystemPage();
  setupStructurePage();
  setupCalculationPage();
  setupAdvancedPage();
  setupSpecificPage();

  auto *btnLayout = new QHBoxLayout();
  btnLayout->addStretch();
  auto *saveBtn = new QPushButton(tr("Save"), this);
  auto *runBtn = new QPushButton(tr("Run"), this);
  auto *closeBtn = new QPushButton(tr("Close"), this);
  btnLayout->addWidget(saveBtn);
  btnLayout->addWidget(runBtn);
  btnLayout->addWidget(closeBtn);
  mainLayout->addLayout(btnLayout);

  connect(saveBtn, &QPushButton::clicked, this, &UspexDialog::on_save);
  connect(runBtn, &QPushButton::clicked, this, &UspexDialog::on_run);
  connect(closeBtn, &QPushButton::clicked, this, &UspexDialog::on_close);

  refresh();

  wire_up_signals(this);

  /* Trigger format change to seed per-format lattice storage from model */
  on_latticeformat_changed(0);

  /* Initialize SPECIFIC tab visibility based on current method */
  on_method_changed(m_w->calculationMethod->currentIndex());

  /* Trigger Ver 10.4 toggle to apply its effects */
  on_v1030_toggled(true);

  /* Trigger AUTO STEP toggle to apply its effects */
  on_auto_step_toggled(true);

  /* FORCE dependent field states — must be last */
  m_w->K_min->setDisabled(true);
  m_w->K_max->setDisabled(true);
  m_w->Kconstant->setDisabled(true);
  m_w->startCIDIStep->setDisabled(true);
  m_w->pickupImages->setDisabled(true);
}

UspexDialog::~UspexDialog()
{
  g_free(m_lattice_volume);
  g_free(m_lattice_lattice);
  g_free(m_lattice_crystal);
  delete m_w;
}

/* ============================================================
 * Wire up signals — call after all widgets are created
 * ============================================================ */
void wire_up_signals(UspexDialog *dlg)
{
  /* Method combo → SPECIFIC tab visibility */
  QObject::connect(dlg->m_w->calculationMethod, QOverload<int>::of(&QComboBox::currentIndexChanged), dlg,
                   &UspexDialog::on_method_changed);

  /* DIM spinner → calculationType */
  QObject::connect(dlg->m_w->calctype_dim, QOverload<int>::of(&QSpinBox::valueChanged), dlg,
                   &UspexDialog::on_dim_changed);

  /* MAG toggle (SYSTEM) → magnetic fields + calculationType */
  QObject::connect(dlg->m_w->calctype_mag, &QCheckBox::toggled, dlg, &UspexDialog::on_mag_toggled);

  /* MAG toggle (STRUCTURE) → sync with SYSTEM MAG */
  QObject::connect(dlg->m_w->calctype_mag_2, &QCheckBox::toggled, dlg, [dlg](bool checked) {
    dlg->m_w->calctype_mag->blockSignals(true);
    dlg->m_w->calctype_mag->setChecked(checked);
    dlg->m_w->calctype_mag->blockSignals(false);
    dlg->on_mag_toggled(checked);
  });

  /* MOL toggle → molecularity fields + calculationType */
  QObject::connect(dlg->m_w->calctype_mol, &QCheckBox::toggled, dlg, &UspexDialog::on_mol_toggled);

  /* VAR toggle → variable-composition fields + calculationType */
  QObject::connect(dlg->m_w->calctype_var, &QCheckBox::toggled, dlg, &UspexDialog::on_var_toggled);

  /* Type combo → sync all flags + SPECIFIC tab visibility */
  QObject::connect(dlg->m_w->calculationType, QOverload<int>::of(&QComboBox::currentIndexChanged), dlg,
                   &UspexDialog::on_type_changed);

  /* atomType combo → fill @Sym/@Z/@Num/@Val */
  QObject::connect(dlg->m_w->atomType, QOverload<int>::of(&QComboBox::currentIndexChanged), dlg,
                   &UspexDialog::on_atomType_changed);

  /* atomType Add/Remove buttons */
  QObject::connect(dlg->m_w->apply_atom, &QPushButton::clicked, dlg, &UspexDialog::on_apply_atom);
  QObject::connect(dlg->m_w->remove_atom, &QPushButton::clicked, dlg, &UspexDialog::on_remove_atom);

  /* AUTO_BONDS */
  QObject::connect(dlg->m_w->auto_bonds, &QCheckBox::toggled, dlg, &UspexDialog::on_auto_bonds_toggled);

  /* AUTO_LAT */
  QObject::connect(dlg->m_w->auto_C_lat, &QCheckBox::toggled, dlg, &UspexDialog::on_auto_C_lat_toggled);
  /* Lattice format change → repopulate Latticevalues combo */
  QObject::connect(dlg->m_w->latticeformat, QOverload<int>::of(&QComboBox::currentIndexChanged), dlg,
                   &UspexDialog::on_latticeformat_changed);
  /* Latticevalues selection → copy to VALUES field */
  QObject::connect(dlg->m_w->Latticevalues, QOverload<int>::of(&QComboBox::currentIndexChanged), dlg,
                   &UspexDialog::on_latticevalues_selected);
  /* Apply lattice button */
  QObject::connect(dlg->m_w->apply_latticevalue, &QPushButton::clicked, dlg, &UspexDialog::on_apply_latticevalue);

  /* AUTO_ION */
  QObject::connect(dlg->m_w->auto_C_ion, &QCheckBox::toggled, dlg, &UspexDialog::on_auto_C_ion_toggled);
  /* IonDistances combo selection → copy to DIST field */
  QObject::connect(dlg->m_w->IonDistances, QOverload<int>::of(&QComboBox::currentIndexChanged), dlg,
                   &UspexDialog::on_iondistances_selected);
  /* Apply ion distances button */
  QObject::connect(dlg->m_w->apply_distances, &QPushButton::clicked, dlg, &UspexDialog::on_apply_distances);

  /* AUTO_STEP */
  QObject::connect(dlg->m_w->auto_step, &QCheckBox::toggled, dlg, &UspexDialog::on_auto_step_toggled);

  /* USE/SET Specific Folder radio → toggle widget states */
  QObject::connect(dlg->m_w->use_specific, &QRadioButton::toggled, dlg, &UspexDialog::on_use_specific_toggled);
  QObject::connect(dlg->m_w->set_specific, &QRadioButton::toggled, dlg, &UspexDialog::on_set_specific_toggled);
  /* SPE folder button → open folder dialog */
  QObject::connect(dlg->m_w->spe_folder_button, &QPushButton::clicked, dlg, &UspexDialog::on_spe_folder_button_clicked);

  /* Step spinner → load step data */
  QObject::connect(dlg->m_w->curr_step, QOverload<int>::of(&QSpinBox::valueChanged), dlg,
                   &UspexDialog::on_curr_step_changed);
  /* N_STEPS spinner → resize step arrays */
  QObject::connect(dlg->m_w->num_opt_steps, QOverload<int>::of(&QSpinBox::valueChanged), dlg,
                   &UspexDialog::on_num_opt_steps_changed);
  /* AI file buttons → open file dialogs */
  QObject::connect(dlg->m_w->ai_input_button, &QPushButton::clicked, dlg, &UspexDialog::on_ai_input_button_clicked);
  QObject::connect(dlg->m_w->ai_opt_button, &QPushButton::clicked, dlg, &UspexDialog::on_ai_opt_button_clicked);
  QObject::connect(dlg->m_w->load_abinitio_exe, &QPushButton::clicked, dlg, &UspexDialog::on_load_abinitio_exe_clicked);
  /* LIB button → apply library flavor */
  QObject::connect(dlg->m_w->ai_lib_button, &QPushButton::clicked, dlg, &UspexDialog::on_ai_lib_button_clicked);
  /* Apply step button */
  QObject::connect(dlg->m_w->apply_step, &QPushButton::clicked, dlg, &UspexDialog::on_apply_step_clicked);

  /* USPEX launch buttons */
  QObject::connect(dlg->m_w->load_uspex_exe, &QPushButton::clicked, dlg, &UspexDialog::on_load_uspex_exe_clicked);
  QObject::connect(dlg->m_w->uspex_path_dialog, &QPushButton::clicked, dlg, &UspexDialog::on_uspex_path_dialog_clicked);
  QObject::connect(dlg->m_w->load_remote_folder, &QPushButton::clicked, dlg,
                   &UspexDialog::on_load_remote_folder_clicked);

  /* Space group Active → enable/disable TOL field */
  QObject::connect(dlg->m_w->doSpaceGroup, &QCheckBox::toggled, dlg, &UspexDialog::on_spacegroup_active_toggled);

  /* Ver 10.4 (SYSTEM page) */
  QObject::connect(dlg->m_w->sel_v1030_1, &QCheckBox::toggled, dlg, &UspexDialog::on_v1030_toggled);
  /* Ver 10.4 (CALCULATION page) — same handler */
  QObject::connect(dlg->m_w->sel_v1030_3, &QCheckBox::toggled, dlg, &UspexDialog::on_v1030_toggled);
  /* NEW optType → same logic as Ver 10.4 */
  QObject::connect(dlg->m_w->sel_new_opt, &QCheckBox::toggled, dlg, &UspexDialog::on_v1030_toggled);
  /* optType change → re-evaluate BoltzTraP visibility */
  QObject::connect(dlg->m_w->optType, QOverload<int>::of(&QComboBox::currentIndexChanged), dlg,
                   &UspexDialog::on_optType_changed);
  /* new_optType change → re-evaluate BoltzTraP visibility */
  QObject::connect(dlg->m_w->new_optType, &QLineEdit::textChanged, dlg, &UspexDialog::on_new_optType_changed);
  /* meta_model combo selection → enable/disable button */
  QObject::connect(dlg->m_w->meta_model, QOverload<int>::of(&QComboBox::currentIndexChanged), dlg,
                   &UspexDialog::on_meta_model_changed);
  /* meta_model button → open file dialog */
  QObject::connect(dlg->m_w->meta_model_button, &QPushButton::clicked, dlg, &UspexDialog::on_meta_model_button_clicked);
  /* VCNEB: method/Var_Iname/Var_Spring → update vcnebType */
  QObject::connect(dlg->m_w->vcnebtype_method, QOverload<int>::of(&QComboBox::currentIndexChanged), dlg,
                   &UspexDialog::on_vcnebtype_changed);
  QObject::connect(dlg->m_w->vcnebtype_img_num, &QCheckBox::toggled, dlg, &UspexDialog::on_vcnebtype_changed);
  QObject::connect(dlg->m_w->vcnebtype_spring, &QCheckBox::toggled, dlg, &UspexDialog::on_vcnebtype_changed);
  /* optMethodCIDI change → enable/disable Start CI/DI and Pickup */
  QObject::connect(dlg->m_w->optMethodCIDI, QOverload<int>::of(&QComboBox::currentIndexChanged), dlg,
                   &UspexDialog::on_cidi_method_changed);
  /* img_model combo selection → enable/disable button */
  QObject::connect(dlg->m_w->img_model, QOverload<int>::of(&QComboBox::currentIndexChanged), dlg,
                   &UspexDialog::on_img_model_changed);
  /* img_model button → open file dialog */
  QObject::connect(dlg->m_w->img_model_button, &QPushButton::clicked, dlg, &UspexDialog::on_img_model_button_clicked);
  /* FormatType change → update file filter */
  QObject::connect(dlg->m_w->FormatType, QOverload<int>::of(&QComboBox::currentIndexChanged), dlg,
                   &UspexDialog::on_FormatType_changed);
  /* substrate_model combo selection → enable/disable button */
  QObject::connect(dlg->m_w->substrate_model, QOverload<int>::of(&QComboBox::currentIndexChanged), dlg,
                   &UspexDialog::on_substrate_model_changed);
  /* substrate_model button → open file dialog */
  QObject::connect(dlg->m_w->substrate_model_button, &QPushButton::clicked, dlg,
                   &UspexDialog::on_substrate_model_button_clicked);
  /* mol_model combo selection → enable/disable other Molecules frame widgets */
  QObject::connect(dlg->m_w->mol_model, QOverload<int>::of(&QComboBox::currentIndexChanged), dlg,
                   &UspexDialog::on_mol_model_changed);
  /* mol_model button → open folder/file dialog based on combo selection */
  QObject::connect(dlg->m_w->mol_model_button, &QPushButton::clicked, dlg, &UspexDialog::on_mol_model_button_clicked);
  /* num_mol spinner → resize _tmp_mols arrays and update curr_mol range */
  QObject::connect(dlg->m_w->num_mol, QOverload<int>::of(&QSpinBox::valueChanged), dlg,
                   &UspexDialog::on_num_mol_changed);
  /* curr_mol spinner → load molecule data */
  QObject::connect(dlg->m_w->curr_mol, QOverload<int>::of(&QSpinBox::valueChanged), dlg,
                   &UspexDialog::on_curr_mol_changed);
  /* mol_apply_button → store molecule selection */
  QObject::connect(dlg->m_w->mol_apply_button, &QPushButton::clicked, dlg, &UspexDialog::on_mol_apply_clicked);
}

/* ============================================================
 * SYSTEM page setup
 * ============================================================ */
void UspexDialog::setupSystemPage()
{
  auto *page = new QWidget();
  auto *mainLayout = new QVBoxLayout(page);
  mainLayout->setSpacing(6);
  mainLayout->setContentsMargins(4, 4, 4, 4);

  /* ============================================================
   * FRAME 1: "Type & System" — 9 rows
   * ============================================================ */
  auto *typeFrame = new QGroupBox(tr("Type & System"));
  auto *typeLayout = new QVBoxLayout(typeFrame);
  typeLayout->setSpacing(4);
  typeLayout->setContentsMargins(4, 4, 4, 4);

  /* Row 1: "Method:" + combo + "DIM" + spinner + "ExtP" + field + toggle "Ver 10.4" */
  m_w->calculationMethod = new QComboBox();
  m_w->calculationMethod->addItems(
      {tr("USPEX"), tr("META"), tr("VCNEB"), tr("PSO"), tr("TPS"), tr("MINHOP"), tr("COPEX")});
  m_w->calctype_dim = new QSpinBox();
  m_w->calctype_dim->setRange(-2, 3);
  m_w->calctype_dim->setSingleStep(1);
  m_w->ExternalPressure = new QDoubleSpinBox();
  m_w->ExternalPressure->setRange(0.0, 1000.0);
  m_w->ExternalPressure->setDecimals(4);
  m_w->sel_v1030_1 = new QCheckBox(tr("Ver 10.4"));
  m_w->sel_v1030_1->setChecked(true);

  auto *row1 = new QHBoxLayout();
  row1->addWidget(new QLabel(tr("Method: ")));
  row1->addWidget(m_w->calculationMethod);
  row1->addSpacing(12);
  row1->addWidget(new QLabel(tr("DIM:")));
  row1->addWidget(m_w->calctype_dim);
  row1->addSpacing(12);
  row1->addWidget(new QLabel(tr("ExtP:")));
  row1->addWidget(m_w->ExternalPressure);
  row1->addSpacing(12);
  row1->addWidget(m_w->sel_v1030_1);
  typeLayout->addLayout(row1);
  setWidgetTooltip(m_w->calculationMethod, TOOLTIP_CALCULATIONMETHOD);
  setWidgetTooltip(m_w->calctype_dim, TOOLTIP_CALCTYPE_DIM);
  setWidgetTooltip(m_w->ExternalPressure, TOOLTIP_EXTERNALPRESSURE);
  setWidgetTooltip(m_w->sel_v1030_1, TOOLTIP_VER_104);

  /* Row 2: "Type:" + combo + toggle "MAG" + spacer + toggle "MOL" + toggle "VAR" */
  m_w->calculationType = new QComboBox();
  m_w->calculationType->addItems({tr("300"), tr("s300"), tr("301"), tr("s301"), tr("310"), tr("311"), tr("000"),
                                  tr("s000"), tr("001"), tr("110"), tr("200"), tr("s200"), tr("201"), tr("s201"),
                                  tr("-200"), tr("-s200"), tr("-201"), tr("-s201")});
  m_w->calctype_mag = new QCheckBox(tr("MAG"));
  m_w->calctype_mol = new QCheckBox(tr("MOL"));
  m_w->calctype_var = new QCheckBox(tr("VAR"));

  auto *row2 = new QHBoxLayout();
  row2->addWidget(new QLabel(tr("Type: ")));
  row2->addWidget(m_w->calculationType);
  row2->addWidget(m_w->calctype_mag);
  row2->addStretch();
  row2->addWidget(m_w->calctype_mol);
  row2->addWidget(m_w->calctype_var);
  typeLayout->addLayout(row2);
  setWidgetTooltip(m_w->calculationType, TOOLTIP_CALCULATIONTYPE);
  setWidgetTooltip(m_w->calctype_mag, TOOLTIP_MAG);
  setWidgetTooltip(m_w->calctype_mol, TOOLTIP_MOL);
  setWidgetTooltip(m_w->calctype_var, TOOLTIP_VAR);

  /* Row 3: "atomType:" + combo + "@Sym:" + field + "@Z:" + field + "Add" button */
  m_w->atomType = new QComboBox();
  m_w->atomType->addItems({tr("ADD ATOMTYPE")});
  m_w->atom_sym = new QLineEdit();
  m_w->atom_typ = new QLineEdit();
  m_w->apply_atom = new QPushButton(tr("Add"));

  auto *row3 = new QHBoxLayout();
  row3->addWidget(new QLabel(tr("atomType:")));
  row3->addWidget(m_w->atomType);
  row3->addSpacing(12);
  row3->addWidget(new QLabel(tr("@Sym:")));
  row3->addWidget(m_w->atom_sym);
  row3->addSpacing(12);
  row3->addWidget(new QLabel(tr("@Z:")));
  row3->addWidget(m_w->atom_typ);
  row3->addSpacing(12);
  row3->addWidget(m_w->apply_atom);
  typeLayout->addLayout(row3);
  setWidgetTooltip(m_w->atomType, TOOLTIP_ATOMTYPE);
  setWidgetTooltip(m_w->atom_sym, TOOLTIP_ATOM_SYM);
  setWidgetTooltip(m_w->atom_typ, TOOLTIP_ATOM_TYP);
  setWidgetTooltip(m_w->apply_atom, TOOLTIP_APPLY_ATOM);

  /* Row 4: spacer + spacer + "@Num:" + field + "@Val:" + field + "Del" button */
  m_w->atom_num = new QLineEdit();
  m_w->atom_val = new QLineEdit();
  m_w->remove_atom = new QPushButton(tr("Del"));

  auto *row4 = new QHBoxLayout();
  row4->addStretch();
  row4->addStretch();
  row4->addWidget(new QLabel(tr("@Num:")));
  row4->addWidget(m_w->atom_num);
  row4->addSpacing(12);
  row4->addWidget(new QLabel(tr("@Val:")));
  row4->addWidget(m_w->atom_val);
  row4->addSpacing(12);
  row4->addWidget(m_w->remove_atom);
  typeLayout->addLayout(row4);
  setWidgetTooltip(m_w->atom_num, TOOLTIP_ATOM_NUM);
  setWidgetTooltip(m_w->atom_val, TOOLTIP_ATOM_VAL);
  setWidgetTooltip(m_w->remove_atom, TOOLTIP_REMOVE_ATOM);

  /* Row 5: "numSpecies:" + combo + "Species:" + field + "Add" button + "Del" button */
  m_w->numSpecies = new QComboBox();
  m_w->numSpecies->addItems({tr("ADD SPECIES BLOCK")});
  m_w->blockSpecies = new QLineEdit();
  m_w->Species_apply_button = new QPushButton(tr("Add"));
  m_w->Species_delete_button = new QPushButton(tr("Del"));

  auto *row5 = new QHBoxLayout();
  row5->addWidget(new QLabel(tr("numSpecies:")));
  row5->addWidget(m_w->numSpecies);
  row5->addSpacing(12);
  row5->addWidget(new QLabel(tr("Species:")));
  row5->addWidget(m_w->blockSpecies);
  row5->addSpacing(12);
  row5->addWidget(m_w->Species_apply_button);
  row5->addWidget(m_w->Species_delete_button);
  typeLayout->addLayout(row5);
  setWidgetTooltip(m_w->numSpecies, TOOLTIP_NUMSPECIES);
  setWidgetTooltip(m_w->blockSpecies, TOOLTIP_BLOCKSPECIES);

  /* Row 6: "goodBonds:" + combo + "Bonds:" + field + "Add" button + "Del" button */
  m_w->goodBonds = new QComboBox();
  m_w->goodBonds->addItems({tr("ADD GOODBOND")});
  m_w->bond_d = new QLineEdit();
  m_w->apply_bonds = new QPushButton(tr("Add"));
  m_w->remove_bonds = new QPushButton(tr("Del"));

  auto *row6 = new QHBoxLayout();
  row6->addWidget(new QLabel(tr("goodBonds:")));
  row6->addWidget(m_w->goodBonds);
  row6->addSpacing(12);
  row6->addWidget(new QLabel(tr("Bonds:")));
  row6->addWidget(m_w->bond_d);
  row6->addSpacing(12);
  row6->addWidget(m_w->apply_bonds);
  row6->addWidget(m_w->remove_bonds);
  typeLayout->addLayout(row6);
  setWidgetTooltip(m_w->goodBonds, TOOLTIP_GOODBONDS);
  setWidgetTooltip(m_w->bond_d, TOOLTIP_BOND_D);

  /* Row 7: "lda+U:" + field (spans) + toggle "AUTO_BONDS" */
  m_w->ldaU = new QLineEdit();
  m_w->auto_bonds = new QCheckBox(tr("AUTO_BONDS"));
  m_w->auto_bonds->setChecked(true);
  m_w->goodBonds->setDisabled(true);
  m_w->bond_d->setDisabled(true);
  m_w->apply_bonds->setDisabled(true);
  m_w->remove_bonds->setDisabled(true);

  auto *row7 = new QHBoxLayout();
  row7->addWidget(new QLabel(tr("lda+U:")));
  row7->addWidget(m_w->ldaU);
  row7->addStretch();
  row7->addWidget(m_w->auto_bonds);
  typeLayout->addLayout(row7);
  setWidgetTooltip(m_w->ldaU, TOOLTIP_LDAU);
  setWidgetTooltip(m_w->auto_bonds, TOOLTIP_AUTO_BONDS);

  /* Row 8: "optType:" + combo + "NEW optType:" + field (spans) + toggle "NEW" */
  m_w->optType = new QComboBox();
  m_w->optType->addItems({tr("1: MIN Enthalpy (stable phases)"),
                          tr("2: MIN Volume (densest structure)"),
                          tr("3: MAX Hardness (hardest phase)"),
                          tr("4: MAX Order (most order structure)"),
                          tr("5: MAX Density"),
                          tr("6: MAX Dielectric susceptibility"),
                          tr("7: MAX Band gap"),
                          tr("8: MAX electric energy storage capacity"),
                          tr("9: MAX Magnetization"),
                          tr("10: MAX Structure quasientropy"),
                          tr("11: MAX L/H eigenvalue difference of refractive index"),
                          tr("12: MAX halfmetalicity parameter"),
                          tr("14: MAX ZT thermoelectric figure of merit"),
                          tr("17: MAX free energy at finite temperature"),
                          tr("1101: MAX Bulk modulus"),
                          tr("1102: MAX Shear modulus"),
                          tr("1103: MAX Young modulus"),
                          tr("1104: MAX Poisson ratio"),
                          tr("1105: MAX Pugh modulus ratio"),
                          tr("1106: MAX Vickers hardness"),
                          tr("1107: MAX Fracture toughness"),
                          tr("1108: MAX Debye temperature"),
                          tr("1109: MAX Sound velocity"),
                          tr("1110: MAX S-wave velocity"),
                          tr("1111: MAX P-wave velocity")});
  m_w->new_optType = new QLineEdit();
  m_w->sel_new_opt = new QCheckBox(tr("NEW"));
  m_w->sel_new_opt->setChecked(true);

  auto *row8 = new QHBoxLayout();
  row8->addWidget(new QLabel(tr("optType:")));
  row8->addWidget(m_w->optType);
  row8->addSpacing(12);
  row8->addWidget(new QLabel(tr("NEW optType:")));
  row8->addWidget(m_w->new_optType);
  row8->addStretch();
  row8->addWidget(m_w->sel_new_opt);
  typeLayout->addLayout(row8);
  setWidgetTooltip(m_w->optType, TOOLTIP_OPTTYPE);
  setWidgetTooltip(m_w->new_optType, TOOLTIP_NEW_OPTTYPE);
  setWidgetTooltip(m_w->sel_new_opt, TOOLTIP_SEL_NEW_OPT);

  /* Row 9: spacer + spacer + toggle "ANTI-OPT" + toggle "ckMol" + toggle "ckCon" */
  m_w->anti_opt = new QCheckBox(tr("ANTI-OPT"));
  m_w->checkMolecules = new QCheckBox(tr("ckMol"));
  m_w->checkConnectivity = new QCheckBox(tr("ckCon"));

  auto *row9 = new QHBoxLayout();
  row9->addStretch();
  row9->addStretch();
  row9->addWidget(m_w->anti_opt);
  row9->addWidget(m_w->checkMolecules);
  row9->addWidget(m_w->checkConnectivity);
  typeLayout->addLayout(row9);
  setWidgetTooltip(m_w->anti_opt, TOOLTIP_ANTI_OPT);
  setWidgetTooltip(m_w->checkMolecules, TOOLTIP_CHECKMOLECULES);
  setWidgetTooltip(m_w->checkConnectivity, TOOLTIP_CHECKCONNECTIVITY);

  mainLayout->addWidget(typeFrame);

  /* ============================================================
   * FRAME 2: "Cell" — 2 rows
   * ============================================================ */
  auto *cellFrame = new QGroupBox(tr("Cell"));
  auto *cellLayout = new QVBoxLayout(cellFrame);
  cellLayout->setSpacing(4);
  cellLayout->setContentsMargins(4, 4, 4, 4);

  /* Row 1: "Lattice:" + combo + "VALUES:" + field (spans) + "Add" button */
  m_w->Latticevalues = new QComboBox();
  m_w->latticevalue = new QLineEdit();
  m_w->apply_latticevalue = new QPushButton(tr("Apply"));

  auto *cellRow1 = new QHBoxLayout();
  cellRow1->addWidget(new QLabel(tr("Lattice:")));
  cellRow1->addWidget(m_w->Latticevalues);
  cellRow1->addSpacing(12);
  cellRow1->addWidget(new QLabel(tr("VALUES:")));
  cellRow1->addWidget(m_w->latticevalue);
  cellRow1->addStretch();
  cellRow1->addWidget(m_w->apply_latticevalue);
  cellLayout->addLayout(cellRow1);
  setWidgetTooltip(m_w->Latticevalues, TOOLTIP_LATTICEVALUES);

  /* Row 2: "FORMAT:" + combo + "split:" + field (spans) + toggle "AUTO_LAT" */
  m_w->latticeformat = new QComboBox();
  m_w->latticeformat->addItems({tr("Volumes"), tr("Lattice"), tr("Crystal")});
  m_w->splitInto = new QLineEdit();
  m_w->auto_C_lat = new QCheckBox(tr("AUTO_LAT"));
  m_w->auto_C_lat->setChecked(true);
  m_w->Latticevalues->setDisabled(true);
  m_w->latticevalue->setDisabled(true);
  m_w->latticeformat->setDisabled(true);

  auto *cellRow2 = new QHBoxLayout();
  cellRow2->addWidget(new QLabel(tr("FORMAT:")));
  cellRow2->addWidget(m_w->latticeformat);
  cellRow2->addSpacing(12);
  cellRow2->addWidget(new QLabel(tr("split:")));
  cellRow2->addWidget(m_w->splitInto);
  cellRow2->addStretch();
  cellRow2->addWidget(m_w->auto_C_lat);
  cellLayout->addLayout(cellRow2);
  setWidgetTooltip(m_w->latticeformat, TOOLTIP_LATTICEFORMAT);
  setWidgetTooltip(m_w->splitInto, TOOLTIP_SPLITINTO);
  setWidgetTooltip(m_w->auto_C_lat, TOOLTIP_AUTO_C_LAT);

  mainLayout->addWidget(cellFrame);

  /* ============================================================
   * FRAME 3: "Constraints" — 3 rows
   * ============================================================ */
  auto *constrFrame = new QGroupBox(tr("Constraints"));
  auto *constrLayout = new QVBoxLayout(constrFrame);
  constrLayout->setSpacing(4);
  constrLayout->setContentsMargins(4, 4, 4, 4);

  /* Row 1: "Ion:" + combo + "DIST:" + field (spans) + "Add" button */
  m_w->IonDistances = new QComboBox();
  m_w->distances = new QLineEdit(" ");
  m_w->apply_distances = new QPushButton(tr("Apply"));

  auto *constrRow1 = new QHBoxLayout();
  constrRow1->addWidget(new QLabel(tr("Ion:")));
  constrRow1->addWidget(m_w->IonDistances);
  constrRow1->addSpacing(12);
  constrRow1->addWidget(new QLabel(tr("DIST:")));
  constrRow1->addWidget(m_w->distances);
  constrRow1->addStretch();
  constrRow1->addWidget(m_w->apply_distances);
  constrLayout->addLayout(constrRow1);
  setWidgetTooltip(m_w->IonDistances, TOOLTIP_IONDISTANCES);
  setWidgetTooltip(m_w->distances, TOOLTIP_DISTANCES);

  /* Row 2: spacer + spacer + "MV:" + field + "CE:" + field + toggle "AUTO_ION" */
  m_w->minVectorLength = new QLineEdit();
  m_w->constraint_enhancement = new QLineEdit();
  m_w->auto_C_ion = new QCheckBox(tr("AUTO_ION"));
  m_w->auto_C_ion->setChecked(true);
  m_w->IonDistances->setDisabled(true);
  m_w->distances->setDisabled(true);
  m_w->apply_distances->setDisabled(true);

  auto *constrRow2 = new QHBoxLayout();
  constrRow2->addStretch();
  constrRow2->addStretch();
  constrRow2->addWidget(new QLabel(tr("MV:")));
  constrRow2->addWidget(m_w->minVectorLength);
  constrRow2->addSpacing(12);
  constrRow2->addWidget(new QLabel(tr("CE:")));
  constrRow2->addWidget(m_w->constraint_enhancement);
  constrRow2->addSpacing(12);
  constrRow2->addWidget(m_w->auto_C_ion);
  constrLayout->addLayout(constrRow2);
  setWidgetTooltip(m_w->minVectorLength, TOOLTIP_MINVECTORLENGTH);
  setWidgetTooltip(m_w->constraint_enhancement, TOOLTIP_CE);
  setWidgetTooltip(m_w->auto_C_ion, TOOLTIP_AUTO_C_ION);

  /* Row 3: "Mol:" + combo + "CENTER:" + field (spans) + "Add" button */
  m_w->MolCenters = new QComboBox();
  m_w->MolCenters->addItems({tr("NONE"), tr("MANUAL")});
  m_w->centers = new QLineEdit(" ");
  m_w->centers_button = new QPushButton(tr("Add"));

  auto *constrRow3 = new QHBoxLayout();
  constrRow3->addWidget(new QLabel(tr("Mol:")));
  constrRow3->addWidget(m_w->MolCenters);
  constrRow3->addSpacing(12);
  constrRow3->addWidget(new QLabel(tr("CENTER:")));
  constrRow3->addWidget(m_w->centers);
  constrRow3->addStretch();
  constrRow3->addWidget(m_w->centers_button);
  constrLayout->addLayout(constrRow3);
  setWidgetTooltip(m_w->MolCenters, TOOLTIP_MOLCENTERS);
  setWidgetTooltip(m_w->centers, TOOLTIP_CENTERS);

  mainLayout->addWidget(constrFrame);

  m_notebook->addTab(page, tr("SYSTEM"));
}

/* ============================================================
 * STRUCTURE page setup
 * ============================================================ */
void UspexDialog::setupStructurePage()
{
  auto *page = new QWidget();
  auto *mainLayout = new QVBoxLayout(page);
  mainLayout->setSpacing(6);
  mainLayout->setContentsMargins(4, 4, 4, 4);

  /* ============================================================
   * FRAME 1: "Population & selection" — 4 rows
   * ============================================================ */
  auto *popFrame = new QGroupBox(tr("Population & selection"));
  auto *popLayout = new QVBoxLayout(popFrame);
  popLayout->setSpacing(4);
  popLayout->setContentsMargins(4, 4, 4, 4);

  /* Row 1: SIZE | INIT | NGEN | STOP */
  m_w->populationSize = new QLineEdit();
  m_w->initialPopSize = new QLineEdit();
  m_w->numGenerations = new QLineEdit();
  m_w->stopCrit = new QLineEdit();

  auto *row1 = new QHBoxLayout();
  row1->addWidget(new QLabel(tr("SIZE:")));
  row1->addWidget(m_w->populationSize);
  row1->addSpacing(12);
  row1->addWidget(new QLabel(tr("INIT:")));
  row1->addWidget(m_w->initialPopSize);
  row1->addSpacing(12);
  row1->addWidget(new QLabel(tr("NGEN:")));
  row1->addWidget(m_w->numGenerations);
  row1->addSpacing(12);
  row1->addWidget(new QLabel(tr("STOP:")));
  row1->addWidget(m_w->stopCrit);
  popLayout->addLayout(row1);
  setWidgetTooltip(m_w->populationSize, TOOLTIP_POPULATIONSIZE);
  setWidgetTooltip(m_w->initialPopSize, TOOLTIP_INITIALPOPSIZE);
  setWidgetTooltip(m_w->numGenerations, TOOLTIP_NUMGENERATIONS);
  setWidgetTooltip(m_w->stopCrit, TOOLTIP_STOPCRIT);

  /* Row 2: N.M. | FM-LS | AFM-L | FM-LH */
  m_w->mag_nm = new QLineEdit();
  m_w->mag_fmls = new QLineEdit();
  m_w->mag_afml = new QLineEdit();
  m_w->mag_fmlh = new QLineEdit();

  auto *row2 = new QHBoxLayout();
  row2->addWidget(new QLabel(tr("N.M.:")));
  row2->addWidget(m_w->mag_nm);
  row2->addSpacing(12);
  row2->addWidget(new QLabel(tr("FM-LS:")));
  row2->addWidget(m_w->mag_fmls);
  row2->addSpacing(12);
  row2->addWidget(new QLabel(tr("AFM-L:")));
  row2->addWidget(m_w->mag_afml);
  row2->addSpacing(12);
  row2->addWidget(new QLabel(tr("FM-LH:")));
  row2->addWidget(m_w->mag_fmlh);
  popLayout->addLayout(row2);
  setWidgetTooltip(m_w->mag_nm, TOOLTIP_MAG_NM);
  setWidgetTooltip(m_w->mag_fmls, TOOLTIP_MAG_FMLS);
  setWidgetTooltip(m_w->mag_afml, TOOLTIP_MAG_AFML);
  setWidgetTooltip(m_w->mag_fmlh, TOOLTIP_MAG_FMLH);

  /* Row 3: MAG | FM-HS | AFM-H | AF-LH */
  m_w->calctype_mag_2 = new QCheckBox(tr("MAG"));
  m_w->mag_fmhs = new QLineEdit();
  m_w->mag_afmh = new QLineEdit();
  m_w->mag_aflh = new QLineEdit();

  auto *row3 = new QHBoxLayout();
  row3->addWidget(m_w->calctype_mag_2);
  row3->addSpacing(12);
  row3->addWidget(new QLabel(tr("FM-HS:")));
  row3->addWidget(m_w->mag_fmhs);
  row3->addSpacing(12);
  row3->addWidget(new QLabel(tr("AFM-H:")));
  row3->addWidget(m_w->mag_afmh);
  row3->addSpacing(12);
  row3->addWidget(new QLabel(tr("AF-LH:")));
  row3->addWidget(m_w->mag_aflh);
  popLayout->addLayout(row3);
  setWidgetTooltip(m_w->calctype_mag_2, TOOLTIP_MAG);
  setWidgetTooltip(m_w->mag_fmhs, TOOLTIP_MAG_FMHS);
  setWidgetTooltip(m_w->mag_afmh, TOOLTIP_MAG_AFMH);
  setWidgetTooltip(m_w->mag_aflh, TOOLTIP_MAG_AFLH);

  /* Row 4: Best | BestHM | reopt | fitLimit */
  m_w->bestFrac = new QLineEdit();
  m_w->keepBestHM = new QLineEdit();
  m_w->reoptOld = new QCheckBox(tr("reopt"));
  m_w->fitLimit = new QLineEdit();

  auto *row4 = new QHBoxLayout();
  row4->addWidget(new QLabel(tr("Best:")));
  row4->addWidget(m_w->bestFrac);
  row4->addSpacing(12);
  row4->addWidget(new QLabel(tr("BestHM:")));
  row4->addWidget(m_w->keepBestHM);
  row4->addSpacing(12);
  row4->addWidget(m_w->reoptOld);
  row4->addSpacing(12);
  row4->addWidget(new QLabel(tr("fitLimit:")));
  row4->addWidget(m_w->fitLimit);
  popLayout->addLayout(row4);
  setWidgetTooltip(m_w->bestFrac, TOOLTIP_BESTFRAC);
  setWidgetTooltip(m_w->keepBestHM, TOOLTIP_KEEPBESTHM);
  setWidgetTooltip(m_w->reoptOld, TOOLTIP_REOPTOLD);
  setWidgetTooltip(m_w->fitLimit, TOOLTIP_FITLIMIT);

  mainLayout->addWidget(popFrame);

  /* ============================================================
   * FRAME 3: "Structure & Variation" — 5 rows
   * ============================================================ */
  auto *structFrame = new QGroupBox(tr("Structure & Variation"));
  auto *structLayout = new QVBoxLayout(structFrame);
  structLayout->setSpacing(4);
  structLayout->setContentsMargins(4, 4, 4, 4);

  /* Row 1: symmetries (full width) */
  m_w->symmetries = new QLineEdit();

  auto *row5 = new QHBoxLayout();
  row5->addWidget(new QLabel(tr("symmetries: ")));
  row5->addWidget(m_w->symmetries);
  structLayout->addLayout(row5);
  setWidgetTooltip(m_w->symmetries, TOOLTIP_SYMMETRIES);

  /* Row 2: Heredity | Random | TOPRand | Permutation */
  m_w->fracGene = new QLineEdit();
  m_w->fracRand = new QLineEdit();
  m_w->fracTopRand = new QLineEdit();
  m_w->fracPerm = new QLineEdit();

  auto *row6 = new QHBoxLayout();
  row6->addWidget(new QLabel(tr("Heredity:")));
  row6->addWidget(m_w->fracGene);
  row6->addSpacing(12);
  row6->addWidget(new QLabel(tr("Random:")));
  row6->addWidget(m_w->fracRand);
  row6->addSpacing(12);
  row6->addWidget(new QLabel(tr("TOPRand:")));
  row6->addWidget(m_w->fracTopRand);
  row6->addSpacing(12);
  row6->addWidget(new QLabel(tr("Permutation:")));
  row6->addWidget(m_w->fracPerm);
  structLayout->addLayout(row6);
  setWidgetTooltip(m_w->fracGene, TOOLTIP_FRACGENE);
  setWidgetTooltip(m_w->fracRand, TOOLTIP_FRACRAND);
  setWidgetTooltip(m_w->fracTopRand, TOOLTIP_FRACTOPRAND);
  setWidgetTooltip(m_w->fracPerm, TOOLTIP_FRACPERM);

  /* Row 3: AtmMut | RotMut | LatMut | SpinMut */
  m_w->fracAtomsMut = new QLineEdit();
  m_w->fracRotMut = new QLineEdit();
  m_w->fracLatMut = new QLineEdit();
  m_w->fracSpinMut = new QLineEdit();

  auto *row7 = new QHBoxLayout();
  row7->addWidget(new QLabel(tr("AtmMut:")));
  row7->addWidget(m_w->fracAtomsMut);
  row7->addSpacing(12);
  row7->addWidget(new QLabel(tr("RotMut:")));
  row7->addWidget(m_w->fracRotMut);
  row7->addSpacing(12);
  row7->addWidget(new QLabel(tr("LatMut:")));
  row7->addWidget(m_w->fracLatMut);
  row7->addSpacing(12);
  row7->addWidget(new QLabel(tr("SpinMut:")));
  row7->addWidget(m_w->fracSpinMut);
  structLayout->addLayout(row7);
  setWidgetTooltip(m_w->fracAtomsMut, TOOLTIP_FRACATOMSMUT);
  setWidgetTooltip(m_w->fracRotMut, TOOLTIP_FRACROTMUT);
  setWidgetTooltip(m_w->fracLatMut, TOOLTIP_FRACLATMUT);
  setWidgetTooltip(m_w->fracSpinMut, TOOLTIP_FRACSPINMUT);

  /* Row 4: NSwaps | Swaps */
  m_w->howManySwaps = new QLineEdit();
  m_w->specificSwaps = new QLineEdit();

  auto *row8 = new QHBoxLayout();
  row8->addWidget(new QLabel(tr("NSwaps:")));
  row8->addWidget(m_w->howManySwaps);
  row8->addSpacing(12);
  row8->addWidget(new QLabel(tr("Swaps:")));
  row8->addWidget(m_w->specificSwaps);
  structLayout->addLayout(row8);
  setWidgetTooltip(m_w->howManySwaps, TOOLTIP_HOWMANYSWAPS);
  setWidgetTooltip(m_w->specificSwaps, TOOLTIP_SPECIFICSWAPS);

  /* Row 5: mutationDegree | mutationRate | D_LATMUT | AutoFrac */
  m_w->mutationDegree = new QLineEdit();
  m_w->mutationRate = new QLineEdit();
  m_w->DisplaceInLatmutation = new QLineEdit();
  m_w->AutoFrac = new QCheckBox(tr("AutoFrac"));

  auto *row9 = new QHBoxLayout();
  row9->addWidget(new QLabel(tr("mutationDegree:")));
  row9->addWidget(m_w->mutationDegree);
  row9->addSpacing(12);
  row9->addWidget(new QLabel(tr("mutationRate:")));
  row9->addWidget(m_w->mutationRate);
  row9->addSpacing(12);
  row9->addWidget(new QLabel(tr("D_LATMUT:")));
  row9->addWidget(m_w->DisplaceInLatmutation);
  row9->addSpacing(12);
  row9->addWidget(m_w->AutoFrac);
  structLayout->addLayout(row9);
  setWidgetTooltip(m_w->mutationDegree, TOOLTIP_MUTATIONDEGREE);
  setWidgetTooltip(m_w->mutationRate, TOOLTIP_MUTATIONRATE);
  setWidgetTooltip(m_w->DisplaceInLatmutation, TOOLTIP_DISPLACEINLATMUTATION);
  setWidgetTooltip(m_w->AutoFrac, TOOLTIP_AUTOFRAC);

  mainLayout->addWidget(structFrame);

  /* ============================================================
   * FRAME 4: "Fingerprint, antiseed, & spacegroup" — 3 rows
   * ============================================================ */
  auto *fpFrame = new QGroupBox(tr("Fingerprint, antiseed, & spacegroup"));
  auto *fpLayout = new QVBoxLayout(fpFrame);
  fpLayout->setSpacing(4);
  fpLayout->setContentsMargins(4, 4, 4, 4);

  /* Row 1: RMax | delta | sigma | TOL */
  m_w->RmaxFing = new QLineEdit();
  m_w->deltaFing = new QLineEdit();
  m_w->sigmaFing = new QLineEdit();
  m_w->toleranceFing = new QLineEdit();

  auto *row10 = new QHBoxLayout();
  row10->addWidget(new QLabel(tr("RMax:")));
  row10->addWidget(m_w->RmaxFing);
  row10->addSpacing(12);
  row10->addWidget(new QLabel(tr("delta:")));
  row10->addWidget(m_w->deltaFing);
  row10->addSpacing(12);
  row10->addWidget(new QLabel(tr("sigma:")));
  row10->addWidget(m_w->sigmaFing);
  row10->addSpacing(12);
  row10->addWidget(new QLabel(tr("TOL:")));
  row10->addWidget(m_w->toleranceFing);
  fpLayout->addLayout(row10);
  setWidgetTooltip(m_w->RmaxFing, TOOLTIP_RMAXFING);
  setWidgetTooltip(m_w->deltaFing, TOOLTIP_DELTAFING);
  setWidgetTooltip(m_w->sigmaFing, TOOLTIP_SIGMAFING);
  setWidgetTooltip(m_w->toleranceFing, TOOLTIP_TOLERANCEFING);

  /* Row 2: (Antiseed label) Activation | Max | sigma */
  m_w->antiSeedsActivation = new QLineEdit();
  m_w->antiSeedsMax = new QLineEdit();
  m_w->antiSeedsSigma = new QLineEdit();

  auto *row11 = new QHBoxLayout();
  row11->addWidget(new QLabel(tr("Antiseed")));
  row11->addSpacing(8);
  row11->addWidget(new QLabel(tr("Activation:")));
  row11->addWidget(m_w->antiSeedsActivation);
  row11->addSpacing(12);
  row11->addWidget(new QLabel(tr("Max:")));
  row11->addWidget(m_w->antiSeedsMax);
  row11->addSpacing(12);
  row11->addWidget(new QLabel(tr("sigma:")));
  row11->addWidget(m_w->antiSeedsSigma);
  fpLayout->addLayout(row11);
  setWidgetTooltip(m_w->antiSeedsActivation, TOOLTIP_ANTISEEDSACTIVATION);
  setWidgetTooltip(m_w->antiSeedsMax, TOOLTIP_ANTISEEDSMAX);
  setWidgetTooltip(m_w->antiSeedsSigma, TOOLTIP_ANTISEEDSSIGMA);

  /* Row 3: (Space group label) Active | TOL */
  m_w->doSpaceGroup = new QCheckBox(tr("Active"));
  m_w->SymTolerance = new QLineEdit();

  auto *row12 = new QHBoxLayout();
  row12->addWidget(new QLabel(tr("Space group")));
  row12->addSpacing(8);
  row12->addWidget(m_w->doSpaceGroup);
  row12->addSpacing(12);
  row12->addWidget(new QLabel(tr("TOL:")));
  row12->addWidget(m_w->SymTolerance);
  fpLayout->addLayout(row12);
  setWidgetTooltip(m_w->doSpaceGroup, TOOLTIP_DOSPACEGROUP);
  setWidgetTooltip(m_w->SymTolerance, TOOLTIP_SYMTOLERANCE);

  mainLayout->addWidget(fpFrame);

  /* ============================================================
   * FRAME 5: "Variable-composition" — 2 rows
   * ============================================================ */
  auto *vcFrame = new QGroupBox(tr("Variable-composition"));
  auto *vcLayout = new QVBoxLayout(vcFrame);
  vcLayout->setSpacing(4);
  vcLayout->setContentsMargins(4, 4, 4, 4);

  /* Row 1: 1st_Gen | min@ | max@ */
  m_w->firstGeneMax = new QLineEdit();
  m_w->minAt = new QLineEdit();
  m_w->maxAt = new QLineEdit();

  auto *vcRow1 = new QHBoxLayout();
  vcRow1->addWidget(new QLabel(tr("1st_Gen:")));
  vcRow1->addWidget(m_w->firstGeneMax);
  vcRow1->addSpacing(12);
  vcRow1->addWidget(new QLabel(tr("min@:")));
  vcRow1->addWidget(m_w->minAt);
  vcRow1->addSpacing(12);
  vcRow1->addWidget(new QLabel(tr("max@:")));
  vcRow1->addWidget(m_w->maxAt);
  vcLayout->addLayout(vcRow1);
  setWidgetTooltip(m_w->firstGeneMax, TOOLTIP_FIRSTGENEMAX);
  setWidgetTooltip(m_w->minAt, TOOLTIP_MINAT);
  setWidgetTooltip(m_w->maxAt, TOOLTIP_MAXAT);

  /* Row 2: fTrans | rTrans | Trans */
  m_w->fracTrans = new QLineEdit();
  m_w->howManyTrans = new QLineEdit();
  m_w->specificTrans = new QLineEdit();

  auto *vcRow2 = new QHBoxLayout();
  vcRow2->addWidget(new QLabel(tr("fTrans:")));
  vcRow2->addWidget(m_w->fracTrans);
  vcRow2->addSpacing(12);
  vcRow2->addWidget(new QLabel(tr("rTrans:")));
  vcRow2->addWidget(m_w->howManyTrans);
  vcRow2->addSpacing(12);
  vcRow2->addWidget(new QLabel(tr("Trans: ")));
  vcRow2->addWidget(m_w->specificTrans);
  vcLayout->addLayout(vcRow2);
  setWidgetTooltip(m_w->fracTrans, TOOLTIP_FRACTRANS);
  setWidgetTooltip(m_w->howManyTrans, TOOLTIP_HOWMANYTRANS);
  setWidgetTooltip(m_w->specificTrans, TOOLTIP_SPECIFICTRANS);

  mainLayout->addWidget(vcFrame);

  m_notebook->addTab(page, tr("STRUCTURE"));
}

/* ============================================================
 * CALCULATION page setup
 * ============================================================ */
void UspexDialog::setupCalculationPage()
{
  auto *page = new QWidget();
  auto *mainLayout = new QVBoxLayout(page);
  mainLayout->setSpacing(6);
  mainLayout->setContentsMargins(4, 4, 4, 4);

  /* ============================================================
   * FRAME 1: "Ab initio" — 6 rows
   * ============================================================ */
  auto *aiFrame = new QGroupBox(tr("Ab initio"));
  auto *aiLayout = new QVBoxLayout(aiFrame);
  aiLayout->setSpacing(4);
  aiLayout->setContentsMargins(4, 4, 4, 4);

  /* Row 1: USE Specific Folder / SET Specific Folder radio + SPE: field + Open button */
  m_w->use_specific = new QRadioButton(tr("USE Specific Folder"));
  m_w->set_specific = new QRadioButton(tr("SET Specific Folder"));
  m_w->spe_folder = new QLineEdit();
  m_w->spe_folder_button = new QPushButton(tr("Open"));

  auto *row1 = new QHBoxLayout();
  row1->addWidget(m_w->use_specific);
  row1->addWidget(m_w->set_specific);
  row1->addSpacing(12);
  row1->addWidget(new QLabel(tr("SPE:")));
  row1->addWidget(m_w->spe_folder);
  row1->addStretch();
  row1->addWidget(m_w->spe_folder_button);
  aiLayout->addLayout(row1);
  setWidgetTooltip(m_w->use_specific, TOOLTIP_USE_SPECIFIC);
  setWidgetTooltip(m_w->set_specific, TOOLTIP_SET_SPECIFIC);
  setWidgetTooltip(m_w->spe_folder, TOOLTIP_SPE_FOLDER);

  /* Row 2: N_STEPS | STEPS # | AUTO STEP | Relax_fixed */
  m_w->num_opt_steps = new QSpinBox();
  m_w->num_opt_steps->setRange(1, 9999);
  m_w->curr_step = new QSpinBox();
  m_w->curr_step->setRange(1, 9999);
  m_w->auto_step = new QCheckBox(tr("AUTO STEP"));
  m_w->auto_step->setChecked(true);
  m_w->isfixed = new QCheckBox(tr("Relax_fixed"));

  auto *row2 = new QHBoxLayout();
  row2->addWidget(new QLabel(tr("N_STEPS:")));
  row2->addWidget(m_w->num_opt_steps);
  row2->addSpacing(12);
  row2->addWidget(new QLabel(tr("STEPS #:")));
  row2->addWidget(m_w->curr_step);
  row2->addSpacing(12);
  row2->addWidget(m_w->auto_step);
  row2->addSpacing(12);
  row2->addWidget(m_w->isfixed);
  aiLayout->addLayout(row2);
  setWidgetTooltip(m_w->num_opt_steps, TOOLTIP_NUM_OPT_STEPS);
  setWidgetTooltip(m_w->curr_step, TOOLTIP_CURR_STEP);
  setWidgetTooltip(m_w->auto_step, TOOLTIP_AUTO_STEP);
  setWidgetTooltip(m_w->isfixed, TOOLTIP_ISFIXED);

  /* Row 3: CODE: combo | Kresol: field | vacuum: field */
  m_w->abinitioCode = new QComboBox();
  m_w->abinitioCode->addItems({tr("0  - NONE"),     tr("1  - VASP"),     tr("2  - SIESTA"),    tr("3  - GULP"),
                               tr("4  - LAMMPS"),   tr("5  - ORCA"),     tr("6  - DMACRYS"),   tr("7  - CP2K"),
                               tr("8  - QE"),       tr("9  - FHI-aims"), tr("10 - ATK"),       tr("11 - CASTEP"),
                               tr("12 - Tinker"),   tr("13 - MOPAC"),    tr("14 - BoltzTraP"), tr("15 - DFTB"),
                               tr("16 - Gaussian"), tr("17 - N/A"),      tr("18 - Abinit"),    tr("19 - CRYSTAL")});
  m_w->KresolStart = new QLineEdit();
  m_w->vacuumSize = new QLineEdit();

  auto *row3 = new QHBoxLayout();
  row3->addWidget(new QLabel(tr("CODE:")));
  row3->addWidget(m_w->abinitioCode);
  row3->addSpacing(12);
  row3->addWidget(new QLabel(tr("Kresol:")));
  row3->addWidget(m_w->KresolStart);
  row3->addSpacing(12);
  row3->addWidget(new QLabel(tr("vacuum:")));
  row3->addWidget(m_w->vacuumSize);
  aiLayout->addLayout(row3);
  setWidgetTooltip(m_w->abinitioCode, TOOLTIP_ABINITIOCODE);
  setWidgetTooltip(m_w->KresolStart, TOOLTIP_KRESOLSTART);
  setWidgetTooltip(m_w->vacuumSize, TOOLTIP_VACUUMSIZE);

  /* Row 4: INP: field + Open | OPT: field + Open */
  m_w->ai_input = new QLineEdit();
  m_w->ai_input_button = new QPushButton(tr("Open"));
  m_w->ai_opt = new QLineEdit();
  m_w->ai_opt_button = new QPushButton(tr("Open"));

  auto *row4 = new QHBoxLayout();
  row4->addWidget(new QLabel(tr("INP:")));
  row4->addWidget(m_w->ai_input);
  row4->addSpacing(12);
  row4->addWidget(m_w->ai_input_button);
  row4->addSpacing(12);
  row4->addWidget(new QLabel(tr("OPT:")));
  row4->addWidget(m_w->ai_opt);
  row4->addSpacing(12);
  row4->addWidget(m_w->ai_opt_button);
  aiLayout->addLayout(row4);
  setWidgetTooltip(m_w->ai_input, TOOLTIP_AI_INPUT);
  setWidgetTooltip(m_w->ai_opt, TOOLTIP_AI_OPT);

  /* Row 5: LIB: field + Open | FLAVOR: combo | lib: field */
  m_w->ai_lib = new QLineEdit();
  m_w->ai_lib_button = new QPushButton(tr("Apply"));
  m_w->ai_lib_flavor = new QComboBox();
  m_w->ai_lib_flavor->addItems({tr("N/A"), tr("AUTO"), tr("ALL")});
  m_w->ai_lib_sel = new QLineEdit();

  auto *row5 = new QHBoxLayout();
  row5->addWidget(new QLabel(tr("LIB:")));
  row5->addWidget(m_w->ai_lib);
  row5->addSpacing(12);
  row5->addWidget(m_w->ai_lib_button);
  row5->addSpacing(12);
  row5->addWidget(new QLabel(tr("FLAVOR:")));
  row5->addWidget(m_w->ai_lib_flavor);
  row5->addSpacing(12);
  row5->addWidget(new QLabel(tr("lib:")));
  row5->addWidget(m_w->ai_lib_sel);
  aiLayout->addLayout(row5);
  setWidgetTooltip(m_w->ai_lib, TOOLTIP_AI_LIB);
  setWidgetTooltip(m_w->ai_lib_flavor, TOOLTIP_AI_LIB_FLAVOR);
  setWidgetTooltip(m_w->ai_lib_sel, TOOLTIP_AI_LIB_SEL);

  /* Row 6: EXE: field + Open | (blank) | Apply button */
  m_w->commandExecutable = new QLineEdit();
  m_w->load_abinitio_exe = new QPushButton(tr("Open"));
  m_w->apply_step = new QPushButton(tr("Apply"));

  auto *row6 = new QHBoxLayout();
  row6->addWidget(new QLabel(tr("EXE:")));
  row6->addWidget(m_w->commandExecutable);
  row6->addSpacing(12);
  row6->addWidget(m_w->load_abinitio_exe);
  row6->addStretch();
  row6->addWidget(m_w->apply_step);
  aiLayout->addLayout(row6);
  setWidgetTooltip(m_w->commandExecutable, TOOLTIP_COMMANDEXECUTABLE);

  mainLayout->addWidget(aiFrame);

  /* ============================================================
   * FRAME 2: "USPEX launch" — 3 rows
   * ============================================================ */
  auto *uspexFrame = new QGroupBox(tr("USPEX launch"));
  auto *uspexLayout = new QVBoxLayout(uspexFrame);
  uspexLayout->setSpacing(4);
  uspexLayout->setContentsMargins(4, 4, 4, 4);

  /* Row 1: USPEX: field + Open | Cluster: field | PhaseDiag toggle */
  m_w->job_uspex_exe = new QLineEdit();
  m_w->load_uspex_exe = new QPushButton(tr("Open"));
  m_w->whichCluster = new QLineEdit();
  m_w->PhaseDiagram = new QCheckBox(tr("PhaseDiag"));

  auto *row7 = new QHBoxLayout();
  row7->addWidget(new QLabel(tr("USPEX:")));
  row7->addWidget(m_w->job_uspex_exe);
  row7->addSpacing(12);
  row7->addWidget(m_w->load_uspex_exe);
  row7->addSpacing(12);
  row7->addWidget(new QLabel(tr("Cluster:")));
  row7->addWidget(m_w->whichCluster);
  row7->addSpacing(12);
  row7->addWidget(m_w->PhaseDiagram);
  uspexLayout->addLayout(row7);
  setWidgetTooltip(m_w->job_uspex_exe, TOOLTIP_JOB_USPEX_EXE);
  setWidgetTooltip(m_w->whichCluster, TOOLTIP_WHICHCLUSTER);
  setWidgetTooltip(m_w->PhaseDiagram, TOOLTIP_PHASEDIAGRAM);

  /* Row 2: Ver 10.4 toggle | CPU: field | PAR: field | OCTAVE toggle */
  m_w->sel_v1030_3 = new QCheckBox(tr("Ver 10.4"));
  m_w->numProcessors = new QLineEdit();
  m_w->numParallelCalcs = new QLineEdit();
  m_w->sel_octave = new QCheckBox(tr("OCTAVE"));

  auto *row8 = new QHBoxLayout();
  row8->addWidget(m_w->sel_v1030_3);
  row8->addSpacing(12);
  row8->addWidget(new QLabel(tr("CPU:")));
  row8->addWidget(m_w->numProcessors);
  row8->addSpacing(12);
  row8->addWidget(new QLabel(tr("PAR:")));
  row8->addWidget(m_w->numParallelCalcs);
  row8->addSpacing(12);
  row8->addWidget(m_w->sel_octave);
  uspexLayout->addLayout(row8);
  setWidgetTooltip(m_w->sel_v1030_3, TOOLTIP_VER_104);
  setWidgetTooltip(m_w->numProcessors, TOOLTIP_NUMPROCESSORS);
  setWidgetTooltip(m_w->numParallelCalcs, TOOLTIP_NUMPARALLELCALCS);
  setWidgetTooltip(m_w->sel_octave, TOOLTIP_SEL_OCTAVE);

  /* Row 3: Folder: field + Open | Remote: field + Open */
  m_w->job_path = new QLineEdit();
  m_w->uspex_path_dialog = new QPushButton(tr("Open"));
  m_w->remoteFolder = new QLineEdit();
  m_w->load_remote_folder = new QPushButton(tr("Open"));

  auto *row9 = new QHBoxLayout();
  row9->addWidget(new QLabel(tr("Folder:")));
  row9->addWidget(m_w->job_path);
  row9->addSpacing(12);
  row9->addWidget(m_w->uspex_path_dialog);
  row9->addSpacing(12);
  row9->addWidget(new QLabel(tr("Remote:")));
  row9->addWidget(m_w->remoteFolder);
  row9->addSpacing(12);
  row9->addWidget(m_w->load_remote_folder);
  uspexLayout->addLayout(row9);
  setWidgetTooltip(m_w->job_path, TOOLTIP_JOB_PATH);
  setWidgetTooltip(m_w->remoteFolder, TOOLTIP_REMOTEFOLDER);

  mainLayout->addWidget(uspexFrame);

  /* ============================================================
   * FRAME 3: "Restart" — 1 row
   * ============================================================ */
  auto *restartFrame = new QGroupBox(tr("Restart"));
  auto *restartLayout = new QVBoxLayout(restartFrame);
  restartLayout->setSpacing(4);
  restartLayout->setContentsMargins(4, 4, 4, 4);

  /* Row 1: RESTART toggle | GEN: field | Folder: field | CLEANUP toggle */
  m_w->pickUpYN = new QCheckBox(tr("RESTART"));
  m_w->pickUpGen = new QLineEdit();
  m_w->pickUpFolder = new QLineEdit();
  m_w->restart_cleanup = new QCheckBox(tr("CLEANUP"));

  auto *row10 = new QHBoxLayout();
  row10->addWidget(m_w->pickUpYN);
  row10->addSpacing(12);
  row10->addWidget(new QLabel(tr("GEN:")));
  row10->addWidget(m_w->pickUpGen);
  row10->addSpacing(12);
  row10->addWidget(new QLabel(tr("Folder:")));
  row10->addWidget(m_w->pickUpFolder);
  row10->addSpacing(12);
  row10->addWidget(m_w->restart_cleanup);
  restartLayout->addLayout(row10);
  setWidgetTooltip(m_w->pickUpYN, TOOLTIP_PICKUPYN);
  setWidgetTooltip(m_w->pickUpGen, TOOLTIP_PICKUPGEN);
  setWidgetTooltip(m_w->pickUpFolder, TOOLTIP_PICKUPFOLDER);
  setWidgetTooltip(m_w->restart_cleanup, TOOLTIP_RESTART_CLEANUP);

  mainLayout->addWidget(restartFrame);

  m_notebook->addTab(page, tr("CALCULATION"));
}

/* ============================================================
 * ADVANCED page setup
 * ============================================================ */
void UspexDialog::setupAdvancedPage()
{
  auto *page = new QWidget();
  auto *mainLayout = new QVBoxLayout(page);
  mainLayout->setSpacing(6);
  mainLayout->setContentsMargins(4, 4, 4, 4);

  /* ============================================================
   * FRAME 1: "Developers" — 1 row
   * ============================================================ */
  auto *devFrame = new QGroupBox(tr("Developers"));
  auto *devLayout = new QVBoxLayout(devFrame);
  devLayout->setSpacing(4);
  devLayout->setContentsMargins(4, 4, 4, 4);

  /* Row 1: REPEAT | STOP_FIT | RND_SEED | CollectForces */
  m_w->repeatForStatistics = new QLineEdit();
  m_w->stopFitness = new QLineEdit();
  m_w->fixRndSeed = new QLineEdit();
  m_w->collectForces = new QCheckBox(tr("CollectForces"));

  auto *row1 = new QHBoxLayout();
  row1->addWidget(new QLabel(tr("REPEAT:")));
  row1->addWidget(m_w->repeatForStatistics);
  row1->addSpacing(12);
  row1->addWidget(new QLabel(tr("STOP_FIT:")));
  row1->addWidget(m_w->stopFitness);
  row1->addSpacing(12);
  row1->addWidget(new QLabel(tr("RND_SEED:")));
  row1->addWidget(m_w->fixRndSeed);
  row1->addSpacing(12);
  row1->addWidget(m_w->collectForces);
  devLayout->addLayout(row1);
  setWidgetTooltip(m_w->repeatForStatistics, TOOLTIP_REPEATFORSTATISTICS);
  setWidgetTooltip(m_w->stopFitness, TOOLTIP_STOPFITNESS);
  setWidgetTooltip(m_w->fixRndSeed, TOOLTIP_FIXRNDSEED);
  setWidgetTooltip(m_w->collectForces, TOOLTIP_COLLECTFORCES);

  mainLayout->addWidget(devFrame);

  /* ============================================================
   * FRAME 2: "Seldom" — 4 rows
   * ============================================================ */
  auto *seldomFrame = new QGroupBox(tr("Seldom"));
  auto *seldomLayout = new QVBoxLayout(seldomFrame);
  seldomLayout->setSpacing(4);
  seldomLayout->setContentsMargins(4, 4, 4, 4);

  /* Row 1: ordering | symmetrize | VALENCE */
  m_w->ordering_active = new QCheckBox(tr("ordering"));
  m_w->symmetrize = new QCheckBox(tr("symmetrize"));
  m_w->valenceElectr = new QLineEdit();

  auto *row2 = new QHBoxLayout();
  row2->addWidget(m_w->ordering_active);
  row2->addSpacing(12);
  row2->addWidget(m_w->symmetrize);
  row2->addSpacing(12);
  row2->addWidget(new QLabel(tr("VALENCE:")));
  row2->addWidget(m_w->valenceElectr);
  seldomLayout->addLayout(row2);
  setWidgetTooltip(m_w->ordering_active, TOOLTIP_ORDERING_ACTIVE);
  setWidgetTooltip(m_w->symmetrize, TOOLTIP_SYMMETRIZE);
  setWidgetTooltip(m_w->valenceElectr, TOOLTIP_VALENCEELECTR);

  /* Row 2: SliceShift | minSlice | DYN_HM */
  m_w->percSliceShift = new QLineEdit();
  m_w->minSlice = new QLineEdit();
  m_w->dynamicalBestHM = new QComboBox();
  m_w->dynamicalBestHM->addItems({tr("0 - NONE"), tr("1 - lowest Energy"), tr("2 - promote diversity")});

  auto *row3 = new QHBoxLayout();
  row3->addWidget(new QLabel(tr("SliceShift:")));
  row3->addWidget(m_w->percSliceShift);
  row3->addSpacing(12);
  row3->addWidget(new QLabel(tr("minSlice:")));
  row3->addWidget(m_w->minSlice);
  row3->addSpacing(12);
  row3->addWidget(new QLabel(tr("DYN_HM:")));
  row3->addWidget(m_w->dynamicalBestHM);
  seldomLayout->addLayout(row3);
  setWidgetTooltip(m_w->percSliceShift, TOOLTIP_PERCSLICESHIFT);
  setWidgetTooltip(m_w->minSlice, TOOLTIP_MINSICE);
  setWidgetTooltip(m_w->dynamicalBestHM, TOOLTIP_DYNAMICALBESTHM);

  /* Row 3: maxSlice | SoftMut */
  m_w->maxSlice = new QLineEdit();
  m_w->softMutOnly = new QLineEdit();

  auto *row4 = new QHBoxLayout();
  row4->addWidget(new QLabel(tr("maxSlice:")));
  row4->addWidget(m_w->maxSlice);
  row4->addSpacing(12);
  row4->addWidget(new QLabel(tr("SoftMut:")));
  row4->addWidget(m_w->softMutOnly);
  seldomLayout->addLayout(row4);
  setWidgetTooltip(m_w->maxSlice, TOOLTIP_MAXSLICE);

  /* Row 4: DistHer | NumP | many_P */
  m_w->maxDistHeredity = new QLineEdit();
  m_w->numberparents = new QLineEdit();
  m_w->manyParents = new QComboBox();
  m_w->manyParents->addItems({tr("0 - 2 parents,    1 slice each"), tr("1 - n structures, 1 slice each"),
                              tr("2 - 2 structures, n slices, independants"),
                              tr("3 - 2 structures, n slices, fixed offset")});
  m_w->manyParents->setCurrentIndex(1); /* DEFAULT: 1 */

  auto *row5 = new QHBoxLayout();
  row5->addWidget(new QLabel(tr("DistHer:")));
  row5->addWidget(m_w->maxDistHeredity);
  row5->addSpacing(12);
  row5->addWidget(new QLabel(tr("NumP:")));
  row5->addWidget(m_w->numberparents);
  row5->addSpacing(12);
  row5->addWidget(new QLabel(tr("many_P:")));
  row5->addWidget(m_w->manyParents);
  seldomLayout->addLayout(row5);
  setWidgetTooltip(m_w->maxDistHeredity, TOOLTIP_MAXDISTHEREDITY);
  setWidgetTooltip(m_w->softMutOnly, TOOLTIP_SOFTMUTONLY);
  setWidgetTooltip(m_w->numberparents, TOOLTIP_NUMBERPARENTS);
  setWidgetTooltip(m_w->manyParents, TOOLTIP_MANYPARENTS);

  mainLayout->addWidget(seldomFrame);

  /* ============================================================
   * FRAME 3: "BoltzTraP" — 2 rows
   * ============================================================ */
  auto *boltzFrame = new QGroupBox(tr("BoltzTraP"));
  auto *boltzLayout = new QVBoxLayout(boltzFrame);
  boltzLayout->setSpacing(4);
  boltzLayout->setContentsMargins(4, 4, 4, 4);

  /* Row 1: Goal | T_Max | T_delta | T_efcut */
  m_w->TE_goal = new QComboBox();
  m_w->TE_goal->addItems(
      {tr("ZT tensor trace"), tr("ZT_xx x component"), tr("ZT_yy y component"), tr("ZT_zz z component")});
  m_w->BoltzTraP_T_max = new QLineEdit();
  m_w->BoltzTraP_T_delta = new QLineEdit();
  m_w->BoltzTraP_T_efcut = new QLineEdit();

  auto *row6 = new QHBoxLayout();
  row6->addWidget(new QLabel(tr("Goal:")));
  row6->addWidget(m_w->TE_goal);
  row6->addSpacing(12);
  row6->addWidget(new QLabel(tr("T_Max:")));
  row6->addWidget(m_w->BoltzTraP_T_max);
  row6->addSpacing(12);
  row6->addWidget(new QLabel(tr("T_delta:")));
  row6->addWidget(m_w->BoltzTraP_T_delta);
  row6->addSpacing(12);
  row6->addWidget(new QLabel(tr("T_efcut:")));
  row6->addWidget(m_w->BoltzTraP_T_efcut);
  boltzLayout->addLayout(row6);
  setWidgetTooltip(m_w->TE_goal, TOOLTIP_TE_GOAL);
  setWidgetTooltip(m_w->BoltzTraP_T_max, TOOLTIP_BOLTZTRAP_T_MAX);
  setWidgetTooltip(m_w->BoltzTraP_T_delta, TOOLTIP_BOLTZTRAP_T_DELTA);
  setWidgetTooltip(m_w->BoltzTraP_T_efcut, TOOLTIP_BOLTZTRAP_T_EFCUT);

  /* Row 2: cmd: + Open | T_target | Threshold */
  m_w->cmd_BoltzTraP = new QLineEdit();
  m_w->cmd_BoltzTraP_button = new QPushButton(tr("..."));
  m_w->TE_T_interest = new QLineEdit();
  m_w->TE_threshold = new QLineEdit();

  auto *row7 = new QHBoxLayout();
  row7->addWidget(new QLabel(tr("cmd:")));
  row7->addWidget(m_w->cmd_BoltzTraP);
  row7->addSpacing(12);
  row7->addWidget(m_w->cmd_BoltzTraP_button);
  row7->addSpacing(12);
  row7->addWidget(new QLabel(tr("T_target:")));
  row7->addWidget(m_w->TE_T_interest);
  row7->addSpacing(12);
  row7->addWidget(new QLabel(tr("Threshold:")));
  row7->addWidget(m_w->TE_threshold);
  boltzLayout->addLayout(row7);
  setWidgetTooltip(m_w->cmd_BoltzTraP, TOOLTIP_CMD_BOLTZTRAP);
  setWidgetTooltip(m_w->TE_T_interest, TOOLTIP_TE_T_INTEREST);
  setWidgetTooltip(m_w->TE_threshold, TOOLTIP_TE_THRESHOLD);
  setWidgetTooltip(m_w->cmd_BoltzTraP, TOOLTIP_CMD_BOLTZTRAP);

  mainLayout->addWidget(boltzFrame);

  /* ============================================================
   * FRAME 4: "Transition Path Sampling" — 7 rows
   * ============================================================ */
  auto *tpsFrame = new QGroupBox(tr("Transition Path Sampling"));
  auto *tpsLayout = new QVBoxLayout(tpsFrame);
  tpsLayout->setSpacing(4);
  tpsLayout->setContentsMargins(4, 4, 4, 4);

  /* Row 1: N_Iter | rShift | OP_TYPE */
  m_w->numIterations = new QLineEdit();
  m_w->shiftRatio = new QLineEdit();
  m_w->orderParaType = new QCheckBox(tr("OP_TYPE"));

  auto *row8 = new QHBoxLayout();
  row8->addWidget(new QLabel(tr("N_Iter:")));
  row8->addWidget(m_w->numIterations);
  row8->addSpacing(12);
  row8->addWidget(new QLabel(tr("rShift:")));
  row8->addWidget(m_w->shiftRatio);
  row8->addSpacing(12);
  row8->addWidget(m_w->orderParaType);
  tpsLayout->addLayout(row8);
  setWidgetTooltip(m_w->numIterations, TOOLTIP_NUMITERATIONS);
  setWidgetTooltip(m_w->shiftRatio, TOOLTIP_SHIFT_RATIO);
  setWidgetTooltip(m_w->orderParaType, TOOLTIP_ORDERPARATYPE);

  /* Row 2: SpeciesSymbols | mass */
  m_w->speciesSymbol = new QLineEdit();
  m_w->mass = new QLineEdit();

  auto *row9 = new QHBoxLayout();
  row9->addWidget(new QLabel(tr("SpeciesSymbols:")));
  row9->addWidget(m_w->speciesSymbol);
  row9->addSpacing(12);
  row9->addWidget(new QLabel(tr("mass:")));
  row9->addWidget(m_w->mass);
  tpsLayout->addLayout(row9);
  setWidgetTooltip(m_w->speciesSymbol, TOOLTIP_SPECIESSYMBOL);
  setWidgetTooltip(m_w->mass, TOOLTIP_MASS);

  /* Row 3: A(A->B) | A(B->A) | M(success) | M(failure) */
  m_w->amplitudeShoot_AB = new QLineEdit();
  m_w->amplitudeShoot_BA = new QLineEdit();
  m_w->magnitudeShoot_success = new QLineEdit();
  m_w->magnitudeShoot_failure = new QLineEdit();

  auto *row10 = new QHBoxLayout();
  row10->addWidget(new QLabel(tr("A(A->B):")));
  row10->addWidget(m_w->amplitudeShoot_AB);
  row10->addSpacing(12);
  row10->addWidget(new QLabel(tr("A(B->A):")));
  row10->addWidget(m_w->amplitudeShoot_BA);
  row10->addSpacing(12);
  row10->addWidget(new QLabel(tr("M(success):")));
  row10->addWidget(m_w->magnitudeShoot_success);
  row10->addSpacing(12);
  row10->addWidget(new QLabel(tr("M(failure):")));
  row10->addWidget(m_w->magnitudeShoot_failure);
  tpsLayout->addLayout(row10);
  setWidgetTooltip(m_w->amplitudeShoot_AB, TOOLTIP_AMPLITUDESHOOT_AB);
  setWidgetTooltip(m_w->amplitudeShoot_BA, TOOLTIP_AMPLITUDESHOOT_BA);
  setWidgetTooltip(m_w->magnitudeShoot_success, TOOLTIP_MAGNITUDESHOOT_SUCCESS);
  setWidgetTooltip(m_w->magnitudeShoot_failure, TOOLTIP_MAGNITUDESHOOT_FAILURE);

  /* Row 4: cmdOP: + Open | SIM(start) */
  m_w->cmdOrderParameter = new QLineEdit();
  m_w->cmdOrderParameter_button = new QPushButton(tr("..."));
  m_w->opCriteria_start = new QLineEdit();

  auto *row11 = new QHBoxLayout();
  row11->addWidget(new QLabel(tr("cmdOP:")));
  row11->addWidget(m_w->cmdOrderParameter);
  row11->addSpacing(12);
  row11->addWidget(m_w->cmdOrderParameter_button);
  row11->addSpacing(12);
  row11->addWidget(new QLabel(tr("SIM(start):")));
  row11->addWidget(m_w->opCriteria_start);
  tpsLayout->addLayout(row11);
  setWidgetTooltip(m_w->cmdOrderParameter, TOOLTIP_CMDORDERPARAMETER);
  setWidgetTooltip(m_w->opCriteria_start, TOOLTIP_OP_CRITERIA_START);

  /* Row 5: cmdET: + Open | SIM(end) */
  m_w->cmdEnthalpyTemperature = new QLineEdit();
  m_w->cmdEnthalpyTemperature_button = new QPushButton(tr("..."));
  m_w->opCriteria_end = new QLineEdit();

  auto *row12 = new QHBoxLayout();
  row12->addWidget(new QLabel(tr("cmdET:")));
  row12->addWidget(m_w->cmdEnthalpyTemperature);
  row12->addSpacing(12);
  row12->addWidget(m_w->cmdEnthalpyTemperature_button);
  row12->addSpacing(12);
  row12->addWidget(new QLabel(tr("SIM(end):")));
  row12->addWidget(m_w->opCriteria_end);
  tpsLayout->addLayout(row12);
  setWidgetTooltip(m_w->cmdEnthalpyTemperature, TOOLTIP_CMDENTHALPYTEMPERATURE);
  setWidgetTooltip(m_w->opCriteria_end, TOOLTIP_OP_CRITERIA_END);

  /* Row 6: OP_file: + Open | ET_file: + Open */
  m_w->orderParameterFile = new QLineEdit();
  m_w->orderParameterFile_button = new QPushButton(tr("..."));
  m_w->enthalpyTemperatureFile = new QLineEdit();
  m_w->enthalpyTemperatureFile_button = new QPushButton(tr("..."));

  auto *row13 = new QHBoxLayout();
  row13->addWidget(new QLabel(tr("OP_file:")));
  row13->addWidget(m_w->orderParameterFile);
  row13->addSpacing(12);
  row13->addWidget(m_w->orderParameterFile_button);
  row13->addSpacing(12);
  row13->addWidget(new QLabel(tr("ET_file:")));
  row13->addWidget(m_w->enthalpyTemperatureFile);
  row13->addSpacing(12);
  row13->addWidget(m_w->enthalpyTemperatureFile_button);
  tpsLayout->addLayout(row13);
  setWidgetTooltip(m_w->orderParameterFile, TOOLTIP_ORDERPARAMETERFILE);
  setWidgetTooltip(m_w->enthalpyTemperatureFile, TOOLTIP_ENTHALPYTEMPERATUREFILE);

  /* Row 7: traj_file: + Open | MD_file: + Open */
  m_w->trajectoryFile = new QLineEdit();
  m_w->trajectoryFile_button = new QPushButton(tr("..."));
  m_w->MDrestartFile = new QLineEdit();
  m_w->MDrestartFile_button = new QPushButton(tr("..."));

  auto *row14 = new QHBoxLayout();
  row14->addWidget(new QLabel(tr("traj_file:")));
  row14->addWidget(m_w->trajectoryFile);
  row14->addSpacing(12);
  row14->addWidget(m_w->trajectoryFile_button);
  row14->addSpacing(12);
  row14->addWidget(new QLabel(tr("MD_file:")));
  row14->addWidget(m_w->MDrestartFile);
  row14->addSpacing(12);
  row14->addWidget(m_w->MDrestartFile_button);
  tpsLayout->addLayout(row14);
  setWidgetTooltip(m_w->trajectoryFile, TOOLTIP_TRAJECTORYFILE);
  setWidgetTooltip(m_w->MDrestartFile, TOOLTIP_MDRSTARTFILE);

  mainLayout->addWidget(tpsFrame);

  m_notebook->addTab(page, tr("ADVANCED"));
}

/* ============================================================
 * SPECIFIC page setup
 * ============================================================ */
void UspexDialog::setupSpecificPage()
{
  auto *page = new QWidget();
  auto *mainLayout = new QVBoxLayout(page);
  mainLayout->setSpacing(6);
  mainLayout->setContentsMargins(4, 4, 4, 4);

  /* ============================================================
   * FRAME 1: "Metadynamics" — 2 rows
   * ============================================================ */
  auto *metaFrame = new QGroupBox(tr("Metadynamics"));
  auto *metaLayout = new QVBoxLayout(metaFrame);
  metaLayout->setSpacing(4);
  metaLayout->setContentsMargins(4, 4, 4, 4);

  /* Row 1: Relax: | MaxV | GaussW | GaussH */
  m_w->FullRelax = new QComboBox();
  m_w->FullRelax->addItems({tr("0 - No full relaxation (fix cells)"), tr("1 - Relax only the best structures"),
                            tr("2 - Relax all different structures")});
  m_w->maxVectorLength = new QLineEdit();
  m_w->GaussianWidth = new QLineEdit();
  m_w->GaussianHeight = new QLineEdit();

  auto *row1 = new QHBoxLayout();
  row1->addWidget(new QLabel(tr("Relax:")));
  row1->addWidget(m_w->FullRelax);
  row1->addSpacing(12);
  row1->addWidget(new QLabel(tr("MaxV:")));
  row1->addWidget(m_w->maxVectorLength);
  setWidgetTooltip(m_w->maxVectorLength, TOOLTIP_MAXVECTORLENGTH);
  row1->addSpacing(12);
  row1->addWidget(new QLabel(tr("GaussW:")));
  row1->addWidget(m_w->GaussianWidth);
  setWidgetTooltip(m_w->GaussianWidth, TOOLTIP_GAUSSIANWIDTH);
  row1->addSpacing(12);
  row1->addWidget(new QLabel(tr("GaussH:")));
  row1->addWidget(m_w->GaussianHeight);
  setWidgetTooltip(m_w->GaussianHeight, TOOLTIP_GAUSSIANHEIGHT);
  metaLayout->addLayout(row1);
  setWidgetTooltip(m_w->FullRelax, TOOLTIP_FULLRELAX);

  /* Row 2: MODEL: + Open */
  m_w->meta_model = new QComboBox();
  m_w->meta_model->addItems({tr("From POSCAR FILE"), tr("UNDER CONSTRUCTION")});
  m_w->meta_model_button = new QPushButton(tr("..."));

  auto *row2 = new QHBoxLayout();
  row2->addWidget(new QLabel(tr("MODEL:")));
  row2->addWidget(m_w->meta_model);
  row2->addStretch();
  row2->addWidget(m_w->meta_model_button);
  metaLayout->addLayout(row2);
  setWidgetTooltip(m_w->meta_model, TOOLTIP_META_MODEL);

  mainLayout->addWidget(metaFrame);

  /* ============================================================
   * FRAME 2: "PSO:" — 1 row
   * ============================================================ */
  auto *psoFrame = new QGroupBox(tr("PSO:"));
  auto *psoLayout = new QVBoxLayout(psoFrame);
  psoLayout->setSpacing(4);
  psoLayout->setContentsMargins(4, 4, 4, 4);

  /* Row 1: SoftMut | BestStruct | BestEver */
  m_w->PSO_softMut = new QLineEdit();
  m_w->PSO_BestStruc = new QLineEdit();
  m_w->PSO_BestEver = new QLineEdit();

  auto *row3 = new QHBoxLayout();
  row3->addWidget(new QLabel(tr("SoftMut:")));
  row3->addWidget(m_w->PSO_softMut);
  row3->addSpacing(12);
  row3->addWidget(new QLabel(tr("BestStruct:")));
  row3->addWidget(m_w->PSO_BestStruc);
  row3->addSpacing(12);
  row3->addWidget(new QLabel(tr("BestEver:")));
  row3->addWidget(m_w->PSO_BestEver);
  psoLayout->addLayout(row3);
  setWidgetTooltip(m_w->PSO_softMut, TOOLTIP_PSO_SOFTMUT);
  setWidgetTooltip(m_w->PSO_BestStruc, TOOLTIP_PSO_BESTSTRUC);
  setWidgetTooltip(m_w->PSO_BestEver, TOOLTIP_PSO_BESTEVER);

  mainLayout->addWidget(psoFrame);

  /* ============================================================
   * FRAME 3: "Variable-cell nudged elastic band" — 7 rows
   * ============================================================ */
  auto *vcnebFrame = new QGroupBox(tr("Variable-cell nudged elastic band"));
  auto *vcnebLayout = new QVBoxLayout(vcnebFrame);
  vcnebLayout->setSpacing(4);
  vcnebLayout->setContentsMargins(4, 4, 4, 4);

  /* Row 1: Method: | VC-NEB: | Var_Image | Var_Spring */
  m_w->vcnebtype_method = new QComboBox();
  m_w->vcnebtype_method->addItems({tr("1 - VC-NEB method"), tr("2 - simple relaxation")});
  m_w->vcnebType = new QLineEdit();
  m_w->vcnebType->setReadOnly(true); /* entry is always locked */
  m_w->vcnebtype_img_num = new QCheckBox(tr("Var_Image"));
  m_w->vcnebtype_spring = new QCheckBox(tr("Var_Spring"));

  auto *row4 = new QHBoxLayout();
  row4->addWidget(new QLabel(tr("Method:")));
  row4->addWidget(m_w->vcnebtype_method);
  row4->addSpacing(12);
  row4->addWidget(new QLabel(tr("VC-NEB:")));
  row4->addWidget(m_w->vcnebType);
  row4->addSpacing(12);
  row4->addWidget(m_w->vcnebtype_img_num);
  row4->addSpacing(12);
  row4->addWidget(m_w->vcnebtype_spring);
  vcnebLayout->addLayout(row4);
  setWidgetTooltip(m_w->vcnebtype_method, TOOLTIP_VCNEBTYPE_METHOD);
  setWidgetTooltip(m_w->vcnebType, TOOLTIP_VCNEBTYPE);
  setWidgetTooltip(m_w->vcnebtype_img_num, TOOLTIP_VCNEBTYPE_IMG_NUM);
  setWidgetTooltip(m_w->vcnebtype_spring, TOOLTIP_VCNEBTYPE_SPRING);

  /* Row 2: Img: | N_Img | N_Step | Freeze_Img */
  m_w->optReadImages = new QComboBox();
  m_w->optReadImages->addItems({tr("0 - All structures are needed"), tr("1 - Only initial and final"),
                                tr("2 - Initial and final + intermediates")});
  m_w->numImages = new QLineEdit();
  m_w->numSteps = new QLineEdit();
  m_w->optFreezing = new QCheckBox(tr("Freeze_Img"));

  auto *row5 = new QHBoxLayout();
  row5->addWidget(new QLabel(tr("Img:")));
  row5->addWidget(m_w->optReadImages);
  row5->addSpacing(12);
  row5->addWidget(new QLabel(tr("N_Img:")));
  row5->addWidget(m_w->numImages);
  row5->addSpacing(12);
  row5->addWidget(new QLabel(tr("N_Step:")));
  row5->addWidget(m_w->numSteps);
  row5->addSpacing(12);
  row5->addWidget(m_w->optFreezing);
  vcnebLayout->addLayout(row5);
  setWidgetTooltip(m_w->optReadImages, TOOLTIP_OPTREADIMAGES);

  /* Row 3: Opt: | dt | Conv | PathLength */
  m_w->optimizerType = new QComboBox();
  m_w->optimizerType->addItems({tr("1 - Steepest Descent"), tr("2 - Fast Inertial Relaxation Engine")});
  m_w->dt = new QLineEdit();
  m_w->ConvThreshold = new QLineEdit();
  m_w->VarPathLength = new QLineEdit();

  auto *row6 = new QHBoxLayout();
  row6->addWidget(new QLabel(tr("Opt:")));
  row6->addWidget(m_w->optimizerType);
  row6->addSpacing(12);
  row6->addWidget(new QLabel(tr("dt:")));
  row6->addWidget(m_w->dt);
  row6->addSpacing(12);
  row6->addWidget(new QLabel(tr("Conv:")));
  row6->addWidget(m_w->ConvThreshold);
  row6->addSpacing(12);
  row6->addWidget(new QLabel(tr("PathLength:")));
  row6->addWidget(m_w->VarPathLength);
  vcnebLayout->addLayout(row6);
  setWidgetTooltip(m_w->optimizerType, TOOLTIP_OPTIMIZERTYPE);
  setWidgetTooltip(m_w->numImages, TOOLTIP_NUMIMAGES);
  setWidgetTooltip(m_w->numSteps, TOOLTIP_NUMSTEPS);
  setWidgetTooltip(m_w->optFreezing, TOOLTIP_OPTFREEZING);
  setWidgetTooltip(m_w->dt, TOOLTIP_DT);
  setWidgetTooltip(m_w->ConvThreshold, TOOLTIP_CONVTHRESHOLD);
  setWidgetTooltip(m_w->VarPathLength, TOOLTIP_VARPATHLENGTH);

  /* Row 4: Relax: | Kmin | Kmax | Kcte */
  m_w->optRelaxType = new QComboBox();
  m_w->optRelaxType->addItems({tr("1 - fixed cell, positions relaxed (=NEB)"),
                               tr("2 - cell lattice only (only for testing)"),
                               tr("3 - full, cell and positions relaxation.")});
  m_w->K_min = new QLineEdit();
  m_w->K_max = new QLineEdit();
  m_w->Kconstant = new QLineEdit();

  auto *row7 = new QHBoxLayout();
  row7->addWidget(new QLabel(tr("Relax:")));
  row7->addWidget(m_w->optRelaxType);
  row7->addSpacing(12);
  row7->addWidget(new QLabel(tr("Kmin:")));
  row7->addWidget(m_w->K_min);
  row7->addSpacing(12);
  row7->addWidget(new QLabel(tr("Kmax:")));
  row7->addWidget(m_w->K_max);
  row7->addSpacing(12);
  row7->addWidget(new QLabel(tr("Kcte:")));
  row7->addWidget(m_w->Kconstant);
  vcnebLayout->addLayout(row7);
  setWidgetTooltip(m_w->optRelaxType, TOOLTIP_OPTRELAXTYPE);
  setWidgetTooltip(m_w->K_min, TOOLTIP_K_MIN);
  setWidgetTooltip(m_w->K_max, TOOLTIP_K_MAX);
  setWidgetTooltip(m_w->Kconstant, TOOLTIP_KCONSTANT);

  /* Row 5: CI/DI: | Start CI/DI | Pickup | PrintStep */
  m_w->optMethodCIDI = new QComboBox();
  m_w->optMethodCIDI->addItems({tr("0 - No CI/DI method will be used"), tr("1 - single CI on highest energy TS"),
                                tr("-1 - Single DI on lowest energy LM"), tr("2 - Multi-CI/DI on provided TS/LM")});
  m_w->startCIDIStep = new QLineEdit();
  m_w->pickupImages = new QLineEdit();
  m_w->PrintStep = new QLineEdit();

  auto *row8 = new QHBoxLayout();
  row8->addWidget(new QLabel(tr("CI/DI:")));
  row8->addWidget(m_w->optMethodCIDI);
  row8->addSpacing(12);
  row8->addWidget(new QLabel(tr("Start CI/DI:")));
  row8->addWidget(m_w->startCIDIStep);
  row8->addSpacing(12);
  row8->addWidget(new QLabel(tr("Pickup:")));
  row8->addWidget(m_w->pickupImages);
  row8->addSpacing(12);
  row8->addWidget(new QLabel(tr("PrintStep:")));
  row8->addWidget(m_w->PrintStep);
  vcnebLayout->addLayout(row8);
  setWidgetTooltip(m_w->optMethodCIDI, TOOLTIP_OPTMETHODCIDI);
  setWidgetTooltip(m_w->startCIDIStep, TOOLTIP_STARTCIDISTEP);
  setWidgetTooltip(m_w->pickupImages, TOOLTIP_PICKUPIMAGES);
  setWidgetTooltip(m_w->PrintStep, TOOLTIP_PRINTSTEP);

  /* Row 6: Format: | Model: + Open */
  m_w->FormatType = new QComboBox();
  m_w->FormatType->addItems(
      {tr("1 - XCRYSTDENS format (.xsf)"), tr("2 - VASP v5 (POSCAR) format."), tr("3 - XYZ format with lattice.")});
  m_w->img_model = new QComboBox();
  m_w->img_model_button = new QPushButton(tr("Open"));

  auto *row9 = new QHBoxLayout();
  row9->addWidget(new QLabel(tr("Format:")));
  row9->addWidget(m_w->FormatType);
  row9->addSpacing(12);
  row9->addWidget(new QLabel(tr("Model:")));
  row9->addWidget(m_w->img_model);
  row9->addStretch();
  row9->addWidget(m_w->img_model_button);
  vcnebLayout->addLayout(row9);
  setWidgetTooltip(m_w->FormatType, TOOLTIP_FORMATTYPE);
  setWidgetTooltip(m_w->img_model, TOOLTIP_IMG_MODEL);

  mainLayout->addWidget(vcnebFrame);

  /* ============================================================
   * FRAME 4: "Molecules" — 2 rows
   * ============================================================ */
  auto *molFrame = new QGroupBox(tr("Molecules"));
  auto *molLayout = new QVBoxLayout(molFrame);
  molLayout->setSpacing(4);
  molLayout->setContentsMargins(4, 4, 4, 4);

  /* Row 1: MODEL: | N_MOLS + Open */
  m_w->mol_model = new QComboBox();
  m_w->mol_model->addItems({tr("Already provided"), tr("From MOL_ folder"), tr("From GDIS models")});
  m_w->num_mol = new QSpinBox();
  m_w->num_mol->setRange(1, 9999);
  m_w->mol_model_button = new QPushButton(tr("Open"));

  auto *row10 = new QHBoxLayout();
  row10->addWidget(new QLabel(tr("MODEL:")));
  row10->addWidget(m_w->mol_model);
  row10->addSpacing(12);
  row10->addWidget(new QLabel(tr("N_MOLS:")));
  row10->addWidget(m_w->num_mol);
  row10->addStretch();
  row10->addWidget(m_w->mol_model_button);
  molLayout->addLayout(row10);
  setWidgetTooltip(m_w->mol_model, TOOLTIP_MOL_MODEL);

  /* Row 2: GDIS_MOL: | MOL_# | GULP_FORM + Add */
  m_w->mol_gdis = new QComboBox();
  m_w->curr_mol = new QSpinBox();
  m_w->curr_mol->setRange(1, 9999);
  m_w->mol_gulp = new QCheckBox(tr("GULP_FORM"));
  m_w->mol_apply_button = new QPushButton(tr("Apply"));

  auto *row11 = new QHBoxLayout();
  row11->addWidget(new QLabel(tr("GDIS_MOL:")));
  row11->addWidget(m_w->mol_gdis);
  row11->addSpacing(12);
  row11->addWidget(new QLabel(tr("MOL_#:")));
  row11->addWidget(m_w->curr_mol);
  row11->addSpacing(12);
  row11->addWidget(m_w->mol_gulp);
  row11->addStretch();
  row11->addWidget(m_w->mol_apply_button);
  molLayout->addLayout(row11);
  setWidgetTooltip(m_w->mol_gdis, TOOLTIP_MOL_GDIS);
  setWidgetTooltip(m_w->num_mol, TOOLTIP_NUM_MOL);
  setWidgetTooltip(m_w->curr_mol, TOOLTIP_CURR_MOL);
  setWidgetTooltip(m_w->mol_gulp, TOOLTIP_MOL_GULP);

  m_w->molFrame = molFrame;
  mainLayout->addWidget(molFrame);

  /* ============================================================
   * FRAME 5: "Surfaces" — 2 rows
   * ============================================================ */
  auto *surfFrame = new QGroupBox(tr("Surfaces"));
  auto *surfLayout = new QVBoxLayout(surfFrame);
  surfLayout->setSpacing(4);
  surfLayout->setContentsMargins(4, 4, 4, 4);

  /* Row 1: MODEL: + Open | N_Surf | S_thick | B_thick */
  m_w->substrate_model = new QComboBox();
  m_w->substrate_model_button = new QPushButton(tr("Open"));
  m_w->reconstruct = new QLineEdit();
  m_w->thicknessS = new QLineEdit();
  m_w->thicknessB = new QLineEdit();

  auto *row12 = new QHBoxLayout();
  row12->addWidget(new QLabel(tr("MODEL:")));
  row12->addWidget(m_w->substrate_model);
  row12->addSpacing(12);
  row12->addWidget(m_w->substrate_model_button);
  row12->addSpacing(12);
  row12->addWidget(new QLabel(tr("N_Surf:")));
  row12->addWidget(m_w->reconstruct);
  row12->addSpacing(12);
  row12->addWidget(new QLabel(tr("S_thick:")));
  row12->addWidget(m_w->thicknessS);
  row12->addSpacing(12);
  row12->addWidget(new QLabel(tr("B_thick:")));
  row12->addWidget(m_w->thicknessB);
  surfLayout->addLayout(row12);
  setWidgetTooltip(m_w->substrate_model, TOOLTIP_SUBSTRATE_MODEL);
  setWidgetTooltip(m_w->reconstruct, TOOLTIP_RECONSTRUCT);
  setWidgetTooltip(m_w->thicknessS, TOOLTIP_THICKNESSS);
  setWidgetTooltip(m_w->thicknessB, TOOLTIP_THICKNESSB);

  /* Row 2: Stoichio | E_AB | Mu_A | Mu_B */
  m_w->StoichiometryStart = new QLineEdit();
  m_w->E_AB = new QLineEdit();
  m_w->Mu_A = new QLineEdit();
  m_w->Mu_B = new QLineEdit();

  auto *row13 = new QHBoxLayout();
  row13->addWidget(new QLabel(tr("Stoichio:")));
  row13->addWidget(m_w->StoichiometryStart);
  row13->addSpacing(12);
  row13->addWidget(new QLabel(tr("E_AB:")));
  row13->addWidget(m_w->E_AB);
  row13->addSpacing(12);
  row13->addWidget(new QLabel(tr("Mu_A:")));
  row13->addWidget(m_w->Mu_A);
  row13->addSpacing(12);
  row13->addWidget(new QLabel(tr("Mu_B:")));
  row13->addWidget(m_w->Mu_B);
  surfLayout->addLayout(row13);
  setWidgetTooltip(m_w->StoichiometryStart, TOOLTIP_STOICHIOMETRYSTART);
  setWidgetTooltip(m_w->E_AB, TOOLTIP_E_AB);
  setWidgetTooltip(m_w->Mu_A, TOOLTIP_MU_A);
  setWidgetTooltip(m_w->Mu_B, TOOLTIP_MU_B);

  mainLayout->addWidget(surfFrame);

  m_notebook->addTab(page, tr("SPECIFIC"));
}

/* ============================================================
 * refresh() - populate widgets from uspex_gui.calc
 * ============================================================ */
void UspexDialog::refresh()
{
  /* Initialize to defaults before populating from model */
  init_uspex_parameters(&uspex_gui.calc);

  /* Set symmetries default based on dimensionality */
  if (uspex_gui.calc.symmetries == NULL)
  {
    switch (uspex_gui.calc._calctype_dim)
    {
    case 0:
      uspex_gui.calc.symmetries = g_strdup("E C2 D2 C4 C3 C6 T S2 Ch1 Cv2 S4 S6 Ch3 Th Ch2 Ch4 D3 Ch6 O D4 Cv3 D6 Td "
                                           "Cv4 Dd3 Cv6 Oh C5 S5 S10 Cv5 Ch5 D5 Dd5 Dh5 I Ih");
      break;
    case 1:
      uspex_gui.calc.symmetries = g_strdup("");
      break;
    case 2:
      uspex_gui.calc.symmetries = g_strdup("2-17");
      break;
    case 3:
    default:
      uspex_gui.calc.symmetries = g_strdup("2-230");
    }
  }

  /* MODEL NAME */
  m_w->name->blockSignals(true);
  {
    struct model_pak *data = qt_get_active_model();
    if (data && data->basename)
      m_w->name->setText(QString::fromUtf8(data->basename));
    else
      m_w->name->setText(QString());
  }
  m_w->name->blockSignals(false);

  /* CONNECTED OUTPUT */
  m_w->file_entry->blockSignals(true);
  {
    /* If user explicitly set a file, preserve it */
    if (uspex_gui.file_entry_path && uspex_gui.file_entry_path[0])
    {
      m_w->file_entry->setText(QString::fromUtf8(uspex_gui.file_entry_path));
    } else
    {
      struct model_pak *data = qt_get_active_model();
      if (data && data->id == USPEX && data->uspex)
      {
        uspex_output_struct *uo = (uspex_output_struct *) data->uspex;
        if (uo && uo->calc)
        {
          QString path = QString::fromUtf8(uo->calc->path);
          m_w->file_entry->setText(path + "Parameters.txt");
        } else
        {
          m_w->file_entry->setText(QString());
        }
      } else
      {
        m_w->file_entry->setText(QString());
      }
    }
  }
  m_w->file_entry->blockSignals(false);

  /* SYSTEM page */
  int methodIdx = 0;
  switch (uspex_gui.calc.calculationMethod)
  {
  case US_CM_META:
    methodIdx = 1;
    break;
  case US_CM_VCNEB:
    methodIdx = 2;
    break;
  case US_CM_PSO:
    methodIdx = 3;
    break;
  case US_CM_TPS:
    methodIdx = 4;
    break;
  case US_CM_MINHOP:
    methodIdx = 5;
    break;
  case US_CM_COPEX:
    methodIdx = 6;
    break;
  default:
    methodIdx = 0;
    break;
  }
  m_w->calculationMethod->blockSignals(true);
  m_w->calculationMethod->setCurrentIndex(methodIdx);
  m_w->calculationMethod->blockSignals(false);

  m_w->calctype_dim->setValue((int) uspex_gui._dim);
  m_w->ExternalPressure->setValue(uspex_gui.calc.ExternalPressure);
  /* Ver 10.4 defaults to checked, sync both toggles */
  uspex_gui.have_v1030 = TRUE;
  m_w->sel_v1030_1->setChecked(uspex_gui.have_v1030);
  m_w->sel_v1030_3->setChecked(uspex_gui.have_v1030);

  int typeIdx = 0;
  /* Default to 300 (index 0) if unset (zero-init) */
  if (uspex_gui.calc.calculationType == 0)
    uspex_gui.calc.calculationType = US_CT_300;
  switch (uspex_gui.calc.calculationType)
  {
  case US_CT_s300:
    typeIdx = 1;
    break;
  case US_CT_301:
    typeIdx = 2;
    break;
  case US_CT_s301:
    typeIdx = 3;
    break;
  case US_CT_310:
    typeIdx = 4;
    break;
  case US_CT_311:
    typeIdx = 5;
    break;
  case US_CT_000:
    typeIdx = 6;
    break;
  case US_CT_s000:
    typeIdx = 7;
    break;
  case US_CT_001:
    typeIdx = 8;
    break;
  case US_CT_110:
    typeIdx = 9;
    break;
  case US_CT_200:
    typeIdx = 10;
    break;
  case US_CT_s200:
    typeIdx = 11;
    break;
  case US_CT_201:
    typeIdx = 12;
    break;
  case US_CT_s201:
    typeIdx = 13;
    break;
  case US_CT_m200:
    typeIdx = 14;
    break;
  case US_CT_sm200:
    typeIdx = 15;
    break;
  case US_CT_m201:
    typeIdx = 16;
    break;
  case US_CT_sm201:
    typeIdx = 17;
    break;
  default:
    typeIdx = 0;
    break;
  }
  m_w->calculationType->setCurrentIndex(typeIdx);
  /* Sync DIM/MAG/MOL/VAR from Type combo */
  on_type_changed(typeIdx);

  /* Populate atomType combo from active model */
  m_w->atomType->blockSignals(true);
  m_w->atomType->clear();
  {
    struct model_pak *model = qt_get_active_model();
    GSList *list = find_unique(ELEMENT, model);
    gint nspecies = g_slist_length(list);
    if (nspecies < 1)
      nspecies = 1;

    /* Build atomType and numSpecies from model */
    gint *atomType = (gint *) g_malloc(nspecies * sizeof(gint));
    gint *numSpecies = (gint *) g_malloc0(nspecies * sizeof(gint));
    gint idx = 0;
    for (GSList *l = list; l; l = g_slist_next(l))
    {
      atomType[idx++] = GPOINTER_TO_INT(l->data);
    }
    g_slist_free(list);

    /* Count atoms per species from model->cores */
    for (GSList *l = model->cores; l; l = g_slist_next(l))
    {
      struct core_pak *core = (struct core_pak *) l->data;
      for (idx = 0; idx < nspecies; idx++)
      {
        if (core->atom_code == atomType[idx])
          numSpecies[idx]++;
      }
    }

    /* Populate combo */
    for (idx = 0; idx < nspecies; idx++)
    {
      if (atomType[idx] >= 0 && atomType[idx] < MAX_ELEMENTS && elements[atomType[idx]].symbol[0])
      {
        QString item =
            QString("%1(%2) (V=%3)").arg(QString(elements[atomType[idx]].symbol)).arg(numSpecies[idx]).arg(0);
        m_w->atomType->addItem(item);
      }
    }

    /* Save to uspex_gui for Apply/Remove */
    g_free(uspex_gui.calc.atomType);
    uspex_gui.calc.atomType = atomType;
    g_free(uspex_gui.calc.numSpecies);
    uspex_gui.calc.numSpecies = numSpecies;
    uspex_gui.calc._nspecies = nspecies;
    /* _var_nspecies = 1 for non-variable composition */
    uspex_gui.calc._var_nspecies = 1;

    m_w->atomType->addItem(tr("ADD ATOMTYPE"));
  }
  m_w->atomType->blockSignals(false);

  /* Update numSpecies combo after atomType is populated */
  update_numSpecies_combo();

  /* Sync @Sym/@Z/@Num/@Val with first atomType (or defaults) */
  if (m_w->atomType->count() > 1)
  {
    /* First item is a real atomType (index 0, "ADD ATOMTYPE" is at end) */
    QString text = m_w->atomType->itemText(0);
    QRegularExpression re(QStringLiteral("^([A-Za-z]+)\\((\\d+)\\) \\(V=(\\d+)\\)$"));
    QRegularExpressionMatch match = re.match(text);
    if (match.hasMatch())
    {
      QString sym = match.captured(1);
      int num = match.captured(2).toInt();
      int val = match.captured(3).toInt();
      int typ = elem_symbol_test(sym.toUtf8().constData());
      g_strlcpy(uspex_gui._tmp_atom_sym, sym.toUtf8().constData(), 3);
      uspex_gui._tmp_atom_sym[2] = '\0';
      uspex_gui._tmp_atom_typ = typ;
      uspex_gui._tmp_atom_num = num;
      uspex_gui._tmp_atom_val = val;
      m_w->atom_sym->setText(sym);
      m_w->atom_typ->setText(QString::number(typ));
      m_w->atom_num->setText(QString::number(num));
      m_w->atom_val->setText(QString::number(val));
    }
  } else
  {
    /* No atomTypes — reset to defaults */
    g_strlcpy(uspex_gui._tmp_atom_sym, "XX", 3);
    uspex_gui._tmp_atom_sym[2] = '\0';
    uspex_gui._tmp_atom_typ = 0;
    uspex_gui._tmp_atom_num = 0;
    uspex_gui._tmp_atom_val = 0;
    m_w->atom_sym->setText("XX");
    m_w->atom_typ->setText("0");
    m_w->atom_num->setText("0");
    m_w->atom_val->setText("0");
  }

  m_w->blockSpecies->setText(QString::fromUtf8(uspex_gui._tmp_blockSpecies ? uspex_gui._tmp_blockSpecies : ""));
  m_w->bond_d->setText(QString::fromUtf8(uspex_gui._tmp_bond_d ? uspex_gui._tmp_bond_d : ""));
  m_w->ldaU->setText(QString::fromUtf8(uspex_gui._tmp_ldaU ? uspex_gui._tmp_ldaU : ""));

  int optIdx = 0;
  if (uspex_gui.calc.optType > 0 && uspex_gui.calc.optType <= 25)
    optIdx = (int) uspex_gui.calc.optType - 1;
  m_w->optType->setCurrentIndex(optIdx);

  m_w->new_optType->setText(QString::fromUtf8(uspex_gui._tmp_new_optType ? uspex_gui._tmp_new_optType : ""));
  m_w->sel_new_opt->setChecked(uspex_gui.have_new_optType);
  m_w->anti_opt->setChecked(uspex_gui.calc.anti_opt);
  m_w->checkMolecules->setChecked(uspex_gui.calc.checkMolecules);
  m_w->checkConnectivity->setChecked(uspex_gui.calc.checkConnectivity);

  /* Populate Latticevalues combo based on format selection */
  m_w->Latticevalues->blockSignals(true);
  int formatIdx = 0;
  /* Only populate Latticevalues if AUTO_LAT is unchecked */
  if (!uspex_gui.auto_C_lat)
  {
    while (m_w->Latticevalues->count() > 0)
      m_w->Latticevalues->removeItem(0);

    /* Ensure Latticevalues array exists */
    if (uspex_gui.calc.Latticevalues == NULL || !ptr_valid(uspex_gui.calc.Latticevalues))
    {
      uspex_gui.calc._nlattice_line = 1;
      uspex_gui.calc._nlattice_vals = 1;
      uspex_gui.calc.Latticevalues = (double *) g_malloc(sizeof(gdouble));
      uspex_gui.calc.Latticevalues[0] = 0.0;
    }

    /* Determine format from _nlattice_vals and _nlattice_line */
    if (uspex_gui.calc._nlattice_vals == 6)
      formatIdx = 2; /* crystal — 6 values: a b c alpha beta gamma */
    else if (uspex_gui.calc._nlattice_line > 1 || uspex_gui.calc._nlattice_vals == 3)
      formatIdx = 1; /* lattice — 3 values per row: ux uy uz / vx vy vz / wx wy wz */
    else
      formatIdx = 0; /* volume — 1 value */

    /* Populate combo based on format */
    switch (formatIdx)
    {
    case 0: /* Volume — 1 line, 1 value */
      uspex_gui.calc._nlattice_line = 1;
      uspex_gui.calc._nlattice_vals = 1;
      if (uspex_gui.calc.Latticevalues == NULL || !ptr_valid(uspex_gui.calc.Latticevalues))
      {
        uspex_gui.calc.Latticevalues = (double *) g_malloc(sizeof(gdouble));
        uspex_gui.calc.Latticevalues[0] = 0.0;
      }
      m_w->Latticevalues->addItem(QString::number(uspex_gui.calc.Latticevalues[0], 'f', 4));
      break;
    case 1: /* Lattice — _nlattice_line lines, 3 values each */
      if (uspex_gui._dim == 3)
      {
        uspex_gui.calc._nlattice_line = 3;
        uspex_gui.calc._nlattice_vals = 3;
      } else
      {
        uspex_gui.calc._nlattice_line = 2;
        uspex_gui.calc._nlattice_vals = 2;
      }
      if (uspex_gui.calc.Latticevalues == NULL || !ptr_valid(uspex_gui.calc.Latticevalues))
      {
        uspex_gui.calc.Latticevalues =
            (double *) g_malloc0(uspex_gui.calc._nlattice_line * uspex_gui.calc._nlattice_vals * sizeof(gdouble));
      }
      for (int i = 0; i < uspex_gui.calc._nlattice_line; i++)
      {
        QString row;
        for (int j = 0; j < uspex_gui.calc._nlattice_vals; j++)
        {
          if (j > 0)
            row += " ";
          row += QString::number(uspex_gui.calc.Latticevalues[i * uspex_gui.calc._nlattice_vals + j], 'f', 4);
        }
        m_w->Latticevalues->addItem(row);
      }
      break;
    case 2: /* Crystal — 1 line, 6 values (a b c alpha beta gamma) */
      uspex_gui.calc._nlattice_line = 1;
      uspex_gui.calc._nlattice_vals = 6;
      if (uspex_gui.calc.Latticevalues == NULL || !ptr_valid(uspex_gui.calc.Latticevalues))
      {
        uspex_gui.calc.Latticevalues = (double *) g_malloc0(6 * sizeof(gdouble));
      }
      {
        QString row;
        for (int j = 0; j < 6; j++)
        {
          if (j > 0)
            row += " ";
          row += QString::number(uspex_gui.calc.Latticevalues[j], 'f', 4);
        }
        m_w->Latticevalues->addItem(row);
      }
      break;
    }
    m_w->Latticevalues->setCurrentIndex(0);
    m_w->Latticevalues->blockSignals(false);
  } else
  {
    /* AUTO_LAT checked — clear combo, free Latticevalues to prevent output */
    g_free(uspex_gui.calc.Latticevalues);
    uspex_gui.calc.Latticevalues = NULL;
    uspex_gui.calc._nlattice_line = 0;
    uspex_gui.calc._nlattice_vals = 0;
    while (m_w->Latticevalues->count() > 0)
      m_w->Latticevalues->removeItem(0);
    m_w->Latticevalues->blockSignals(false);
  }

  m_w->latticevalue->setText(QString::fromUtf8(uspex_gui._tmp_latticevalue ? uspex_gui._tmp_latticevalue : ""));
  /* Set format combo to match data */
  m_w->latticeformat->setCurrentIndex(formatIdx);

  /* Copy first combo line to VALUES field */
  if (m_w->Latticevalues->count() > 0)
    m_w->latticevalue->setText(m_w->Latticevalues->itemText(0));

  /* SplitInto default */
  if (uspex_gui.calc.splitInto == NULL || !ptr_valid(uspex_gui.calc.splitInto))
  {
    g_free(uspex_gui.calc.splitInto);
    uspex_gui.calc.splitInto = (gint *) g_malloc0(sizeof(gint));
    uspex_gui.calc.splitInto[0] = 1;
  }
  {
    QString splitText;
    for (int i = 0; i < uspex_gui.calc._nsplits; i++)
    {
      if (i > 0)
        splitText += " ";
      splitText += QString::number(uspex_gui.calc.splitInto[i]);
    }
    m_w->splitInto->setText(splitText);
  }

  /* Auto flags always default to TRUE */
  uspex_gui.auto_bonds = TRUE;
  uspex_gui.auto_C_lat = TRUE;
  uspex_gui.auto_C_ion = TRUE;
  uspex_gui.auto_step = TRUE;

  m_w->auto_bonds->setChecked(uspex_gui.auto_bonds);
  m_w->auto_C_lat->setChecked(uspex_gui.auto_C_lat);

  /* Populate IonDistances combo from calc.IonDistances (triangular matrix) */
  m_w->IonDistances->blockSignals(true);
  {
    while (m_w->IonDistances->count() > 0)
      m_w->IonDistances->removeItem(0);

    if (ptr_valid(uspex_gui.calc.IonDistances) && uspex_gui.calc._nspecies > 0)
    {
      for (int i = 0; i < uspex_gui.calc._nspecies; i++)
      {
        QString row;
        for (int j = 0; j < uspex_gui.calc._nspecies; j++)
        {
          if (j > 0)
            row += " ";
          row += QString::number(uspex_gui.calc.IonDistances[i * uspex_gui.calc._nspecies + j], 'f', 4);
        }
        m_w->IonDistances->addItem(row);
      }
    } else
    {
      /* No data — create empty matrix with correct size */
      if (uspex_gui.calc._nspecies <= 0)
        uspex_gui.calc._nspecies = 1;
      if (uspex_gui.calc.IonDistances == NULL || !ptr_valid(uspex_gui.calc.IonDistances))
      {
        g_free(uspex_gui.calc.IonDistances);
        uspex_gui.calc.IonDistances =
            (double *) g_malloc0(uspex_gui.calc._nspecies * uspex_gui.calc._nspecies * sizeof(double));
      }
      for (int i = 0; i < uspex_gui.calc._nspecies; i++)
      {
        QString row;
        for (int j = 0; j < uspex_gui.calc._nspecies; j++)
        {
          if (j > 0)
            row += " ";
          row += "0.0000";
        }
        m_w->IonDistances->addItem(row);
      }
    }
    m_w->IonDistances->setCurrentIndex(0);
  }
  m_w->IonDistances->blockSignals(false);

  /* Sync DIST field with first combo line */
  on_iondistances_selected(0);
  m_w->minVectorLength->setText(QString::number(uspex_gui.calc.minVectorLength, 'f', 4));
  m_w->constraint_enhancement->setText(QString::number(uspex_gui.calc.constraint_enhancement));
  m_w->auto_C_ion->setChecked(uspex_gui.auto_C_ion);

  /* Populate MolCenters combo */
  m_w->MolCenters->blockSignals(true);
  {
    while (m_w->MolCenters->count() > 0)
      m_w->MolCenters->removeItem(0);

    if (ptr_valid(uspex_gui.calc.MolCenters) && uspex_gui.calc._nmolecules > 0)
    {
      for (int i = 0; i < uspex_gui.calc._nmolecules; i++)
      {
        QString row;
        for (int j = 0; j < uspex_gui.calc._nmolecules; j++)
        {
          if (j > 0)
            row += " ";
          row += QString::number(uspex_gui.calc.MolCenters[i * uspex_gui.calc._nmolecules + j], 'f', 4);
        }
        m_w->MolCenters->addItem(row);
      }
    } else
    {
      /* MolCenters NULL or _nmolecules==0 — leave as NULL (don't allocate) */
      m_w->MolCenters->addItem(tr("NONE"));
    }
    m_w->MolCenters->setCurrentIndex(0);
  }
  m_w->MolCenters->blockSignals(false);

  m_w->centers->setText(" ");

  /* Apply defaults for STRUCTURE page */
  {
    struct model_pak *model = qt_get_active_model();
    if (uspex_gui.calc.populationSize == 0 && model)
    {
      int n = (uspex_gui.calc._calctype_var && uspex_gui.calc.maxAt > 0) ? uspex_gui.calc.maxAt : model->num_atoms;
      if (n > 0)
      {
        uspex_gui.calc.populationSize = qMin(60, (2 * n % 10) * 10);
        uspex_gui.calc.initialPopSize = uspex_gui.calc.populationSize;
      }
    }
    if (uspex_gui.calc.keepBestHM == 0 && uspex_gui.calc.populationSize > 0)
    {
      uspex_gui.calc.keepBestHM = (int) (0.15 * uspex_gui.calc.populationSize);
    }
    if (uspex_gui.calc.stopCrit == 0 && model)
    {
      if (uspex_gui.calc._calctype_var)
        uspex_gui.calc.stopCrit = uspex_gui.calc.maxAt;
      else
        uspex_gui.calc.stopCrit = model->num_atoms;
    }
    /* Default numGenerations */
    if (uspex_gui.calc.numGenerations == 0)
      uspex_gui.calc.numGenerations = 100;
    /* Default bestFrac */
    if (uspex_gui.calc.bestFrac == 0.0)
      uspex_gui.calc.bestFrac = 0.25;
    /* Default fracGene/fracRand/fracTopRand/fracPerm/fracAtomsMut/fracRotMut/fracLatMut/fracSpinMut */
    if (uspex_gui.calc.fracGene == 0.0)
      uspex_gui.calc.fracGene = 0.4;
    if (uspex_gui.calc.fracRand == 0.0)
      uspex_gui.calc.fracRand = 0.4;
    if (uspex_gui.calc.fracTopRand == 0.0)
      uspex_gui.calc.fracTopRand = 0.2;
    if (uspex_gui.calc.fracPerm == 0.0)
      uspex_gui.calc.fracPerm = 0.1;
    if (uspex_gui.calc.fracAtomsMut == 0.0)
      uspex_gui.calc.fracAtomsMut = 0.4;
    /* fracRotMut = 0.1 only when MOL is checked */
    if (uspex_gui.calc.fracRotMut == 0.0 && uspex_gui.calc._calctype_mol)
      uspex_gui.calc.fracRotMut = 0.1;
    /* fracLatMut = 0.1 when optRelaxType != 1 */
    if (uspex_gui.calc.fracLatMut == 0.0 && uspex_gui.calc.optRelaxType != 1)
      uspex_gui.calc.fracLatMut = 0.1;
    /* Default reoptOld */
    if (!uspex_gui.calc.reoptOld)
      uspex_gui.calc.reoptOld = false;
  }

  /* STRUCTURE page */
  m_w->populationSize->setText(QString::number(uspex_gui.calc.populationSize));
  m_w->initialPopSize->setText(QString::number(uspex_gui.calc.initialPopSize));
  m_w->numGenerations->setText(QString::number(uspex_gui.calc.numGenerations));
  m_w->stopCrit->setText(QString::number(uspex_gui.calc.stopCrit));
  m_w->mag_nm->setText(QString::number(uspex_gui.calc.magRatio[0], 'f', 4));
  m_w->mag_fmls->setText(QString::number(uspex_gui.calc.magRatio[1], 'f', 4));
  m_w->mag_afml->setText(QString::number(uspex_gui.calc.magRatio[3], 'f', 4));
  m_w->mag_fmlh->setText(QString::number(uspex_gui.calc.magRatio[5], 'f', 4));
  m_w->calctype_mag_2->setChecked(uspex_gui.calc._calctype_mag);
  m_w->mag_fmhs->setText(QString::number(uspex_gui.calc.magRatio[2], 'f', 4));
  m_w->mag_afmh->setText(QString::number(uspex_gui.calc.magRatio[4], 'f', 4));
  m_w->mag_aflh->setText(QString::number(uspex_gui.calc.magRatio[6], 'f', 4));
  m_w->bestFrac->setText(QString::number(uspex_gui.calc.bestFrac, 'f', 4));
  m_w->keepBestHM->setText(QString::number(uspex_gui.calc.keepBestHM));
  m_w->reoptOld->setChecked(uspex_gui.calc.reoptOld);
  m_w->fitLimit->setText(QString::number(uspex_gui.calc.fitLimit, 'f', 4));
  m_w->symmetries->setText(
      QString::fromUtf8((gchar *) uspex_gui.calc.symmetries ? (gchar *) uspex_gui.calc.symmetries : ""));
  m_w->fracGene->setText(QString::number(uspex_gui.calc.fracGene, 'f', 4));
  m_w->fracRand->setText(QString::number(uspex_gui.calc.fracRand, 'f', 4));
  m_w->fracTopRand->setText(QString::number(uspex_gui.calc.fracTopRand, 'f', 4));
  m_w->fracPerm->setText(QString::number(uspex_gui.calc.fracPerm, 'f', 4));
  m_w->fracAtomsMut->setText(QString::number(uspex_gui.calc.fracAtomsMut, 'f', 4));
  m_w->fracRotMut->setText(QString::number(uspex_gui.calc.fracRotMut, 'f', 4));
  m_w->fracLatMut->setText(QString::number(uspex_gui.calc.fracLatMut, 'f', 4));
  m_w->fracSpinMut->setText(QString::number(uspex_gui.calc.fracSpinMut, 'f', 4));
  m_w->howManySwaps->setText(QString::number(uspex_gui.calc.howManySwaps));
  m_w->specificSwaps->setText(
      QString::fromUtf8((gchar *) uspex_gui.calc.specificSwaps ? (gchar *) uspex_gui.calc.specificSwaps : ""));
  m_w->mutationDegree->setText(QString::number(uspex_gui.calc.mutationDegree, 'f', 4));
  m_w->mutationRate->setText(QString::number(uspex_gui.calc.mutationRate, 'f', 4));
  m_w->DisplaceInLatmutation->setText(QString::number(uspex_gui.calc.DisplaceInLatmutation, 'f', 4));
  m_w->AutoFrac->setChecked(uspex_gui.calc.AutoFrac);
  /* Fingerprint defaults */
  if (uspex_gui.calc.RmaxFing == 0.0)
    uspex_gui.calc.RmaxFing = 10.0;
  if (uspex_gui.calc.deltaFing == 0.0)
    uspex_gui.calc.deltaFing = 0.08;
  if (uspex_gui.calc.sigmaFing == 0.0)
    uspex_gui.calc.sigmaFing = 0.03;
  if (uspex_gui.calc.toleranceFing == 0.0)
    uspex_gui.calc.toleranceFing = 0.25;
  m_w->RmaxFing->setText(QString::number(uspex_gui.calc.RmaxFing, 'f', 4));
  m_w->deltaFing->setText(QString::number(uspex_gui.calc.deltaFing, 'f', 4));
  m_w->sigmaFing->setText(QString::number(uspex_gui.calc.sigmaFing, 'f', 4));
  m_w->toleranceFing->setText(QString::number(uspex_gui.calc.toleranceFing, 'f', 4));
  /* Antiseed defaults */
  if (uspex_gui.calc.antiSeedsActivation == 0)
    uspex_gui.calc.antiSeedsActivation = 5000;
  if (uspex_gui.calc.antiSeedsMax == 0.0)
    uspex_gui.calc.antiSeedsMax = 0.0;
  if (uspex_gui.calc.antiSeedsSigma == 0.0)
    uspex_gui.calc.antiSeedsSigma = 0.001;
  m_w->antiSeedsActivation->setText(QString::number(uspex_gui.calc.antiSeedsActivation));
  m_w->antiSeedsMax->setText(QString::number(uspex_gui.calc.antiSeedsMax, 'f', 4));
  m_w->antiSeedsSigma->setText(QString::number(uspex_gui.calc.antiSeedsSigma, 'f', 4));
  m_w->doSpaceGroup->setChecked(uspex_gui.calc.doSpaceGroup);
  if (uspex_gui.calc.SymTolerance == 0.0)
    uspex_gui.calc.SymTolerance = 0.1;
  m_w->SymTolerance->setDisabled(!uspex_gui.calc.doSpaceGroup);
  m_w->SymTolerance->setText(QString::number(uspex_gui.calc.SymTolerance, 'f', 6));
  m_w->firstGeneMax->setText(QString::number(uspex_gui.calc.firstGeneMax));
  m_w->minAt->setText(QString::number(uspex_gui.calc.minAt));
  m_w->maxAt->setText(QString::number(uspex_gui.calc.maxAt));
  m_w->fracTrans->setText(QString::number(uspex_gui.calc.fracTrans, 'f', 4));
  m_w->howManyTrans->setText(QString::number(uspex_gui.calc.howManyTrans, 'f', 4));

  /* CALCULATION page */
  m_w->use_specific->setChecked(!uspex_gui.have_specific);
  m_w->set_specific->setChecked(uspex_gui.have_specific);
  /* Specific folder: if file_entry_path is set, USE Specific; otherwise SET Specific */
  if (uspex_gui.file_entry_path && uspex_gui.file_entry_path[0])
  {
    uspex_gui.have_specific = TRUE;
    m_w->use_specific->setChecked(true);
    m_w->set_specific->setChecked(false);
  } else
  {
    uspex_gui.have_specific = FALSE;
    m_w->set_specific->setChecked(true);
    m_w->use_specific->setChecked(false);
  }
  on_set_specific_toggled(true);

  /* _tmp_spe_folder default */
  if (uspex_gui._tmp_spe_folder == NULL)
  {
    if (uspex_gui.calc.path && uspex_gui.calc.path[0])
      uspex_gui._tmp_spe_folder = g_build_filename(uspex_gui.calc.path, "../Specific", NULL);
    else
      uspex_gui._tmp_spe_folder = g_strdup("../Specific");
  }
  m_w->spe_folder->setText(QString::fromUtf8(uspex_gui._tmp_spe_folder));

  m_w->num_opt_steps->setValue((int) uspex_gui._tmp_num_opt_steps);
  m_w->curr_step->setValue((int) uspex_gui._tmp_curr_step);
  m_w->auto_step->setChecked(uspex_gui.auto_step);
  m_w->isfixed->setChecked(uspex_gui._tmp_isfixed);

  /* abinitioCode default: 1 */
  if (uspex_gui.calc.abinitioCode == NULL || !ptr_valid(uspex_gui.calc.abinitioCode))
  {
    g_free(uspex_gui.calc.abinitioCode);
    uspex_gui.calc.abinitioCode = (gint *) g_malloc(sizeof(gint));
    uspex_gui.calc.abinitioCode[0] = 1;
  }
  int aiIdx = 0;
  if (uspex_gui.calc.abinitioCode[0] >= 0)
    aiIdx = uspex_gui.calc.abinitioCode[0];
  m_w->abinitioCode->setCurrentIndex(aiIdx);

  /* KresolStart default: 0.2 */
  if (uspex_gui.calc.KresolStart == NULL || !ptr_valid(uspex_gui.calc.KresolStart))
  {
    g_free(uspex_gui.calc.KresolStart);
    uspex_gui.calc.KresolStart = (double *) g_malloc(sizeof(double));
    uspex_gui.calc.KresolStart[0] = 0.2;
  }
  m_w->KresolStart->setText(QString::number(uspex_gui.calc.KresolStart[0], 'f', 4));

  /* vacuumSize default: 10.0 */
  if (uspex_gui.calc.vacuumSize == NULL || !ptr_valid(uspex_gui.calc.vacuumSize))
  {
    g_free(uspex_gui.calc.vacuumSize);
    uspex_gui.calc.vacuumSize = (double *) g_malloc(sizeof(double));
    uspex_gui.calc.vacuumSize[0] = 10.0;
  }
  m_w->vacuumSize->setText(QString::number(uspex_gui.calc.vacuumSize[0], 'f', 4));

  /* AI fields default to "N/A" */
  if (!uspex_gui._tmp_ai_input || !ptr_valid(uspex_gui._tmp_ai_input))
  {
    uspex_gui._tmp_ai_input = (gchar **) g_malloc(sizeof(gchar *));
    uspex_gui._tmp_ai_input[0] = g_strdup("N/A");
  } else if (!uspex_gui._tmp_ai_input[0])
  {
    uspex_gui._tmp_ai_input[0] = g_strdup("N/A");
  }

  if (!uspex_gui._tmp_ai_opt || !ptr_valid(uspex_gui._tmp_ai_opt))
  {
    uspex_gui._tmp_ai_opt = (gchar **) g_malloc(sizeof(gchar *));
    uspex_gui._tmp_ai_opt[0] = g_strdup("N/A");
  } else if (!uspex_gui._tmp_ai_opt[0])
  {
    uspex_gui._tmp_ai_opt[0] = g_strdup("N/A");
  }

  if (!uspex_gui._tmp_ai_lib_folder || !ptr_valid(uspex_gui._tmp_ai_lib_folder))
  {
    uspex_gui._tmp_ai_lib_folder = (gchar **) g_malloc(sizeof(gchar *));
    uspex_gui._tmp_ai_lib_folder[0] = g_strdup("N/A");
  } else if (!uspex_gui._tmp_ai_lib_folder[0])
  {
    uspex_gui._tmp_ai_lib_folder[0] = g_strdup("N/A");
  }

  if (!uspex_gui._tmp_ai_lib_sel || !ptr_valid(uspex_gui._tmp_ai_lib_sel))
  {
    uspex_gui._tmp_ai_lib_sel = (gchar **) g_malloc(sizeof(gchar *));
    uspex_gui._tmp_ai_lib_sel[0] = g_strdup("N/A");
  } else if (!uspex_gui._tmp_ai_lib_sel[0])
  {
    uspex_gui._tmp_ai_lib_sel[0] = g_strdup("N/A");
  }

  if (!uspex_gui._tmp_commandExecutable || !ptr_valid(uspex_gui._tmp_commandExecutable))
  {
    uspex_gui._tmp_commandExecutable = (gchar **) g_malloc(sizeof(gchar *));
    uspex_gui._tmp_commandExecutable[0] = g_strdup("N/A");
  } else if (!uspex_gui._tmp_commandExecutable[0])
  {
    uspex_gui._tmp_commandExecutable[0] = g_strdup("N/A");
  }

  m_w->ai_input->setText(QString::fromUtf8(uspex_gui._tmp_ai_input[0]));
  m_w->ai_opt->setText(QString::fromUtf8(uspex_gui._tmp_ai_opt[0]));
  m_w->ai_lib->setText(QString::fromUtf8(uspex_gui._tmp_ai_lib_folder[0]));
  m_w->ai_lib_sel->setText(QString::fromUtf8(uspex_gui._tmp_ai_lib_sel[0]));
  m_w->commandExecutable->setText(QString::fromUtf8(uspex_gui._tmp_commandExecutable[0]));

  m_w->job_uspex_exe->setText(QString::fromUtf8(uspex_gui.calc.job_uspex_exe ? uspex_gui.calc.job_uspex_exe : ""));
  m_w->whichCluster->setText(QString::number(uspex_gui.calc.whichCluster));
  m_w->PhaseDiagram->setChecked(uspex_gui.calc.PhaseDiagram);
  m_w->sel_v1030_3->setChecked(uspex_gui.have_v1030);

  if (uspex_gui.calc.numProcessors)
    m_w->numProcessors->setText(QString::fromUtf8(reinterpret_cast<const char *>(uspex_gui.calc.numProcessors)));
  if (uspex_gui.calc.numParallelCalcs == 0)
    uspex_gui.calc.numParallelCalcs = 1;
  m_w->numParallelCalcs->setText(QString::number(uspex_gui.calc.numParallelCalcs));
  if (uspex_gui.have_octave == 0)
    uspex_gui.have_octave = TRUE;
  m_w->sel_octave->setChecked(uspex_gui.have_octave);
  /* job_path default: current working directory */
  if (!uspex_gui.calc.job_path || uspex_gui.calc.job_path[0] == '\0')
  {
    g_free(uspex_gui.calc.job_path);
    uspex_gui.calc.job_path = g_strdup(sysenv.cwd);
  }
  m_w->job_path->setText(QString::fromUtf8(uspex_gui.calc.job_path));
  m_w->remoteFolder->setText(QString::fromUtf8(uspex_gui.calc.remoteFolder ? uspex_gui.calc.remoteFolder : ""));
  m_w->pickUpYN->setChecked(uspex_gui.calc.pickUpYN);
  m_w->pickUpGen->setText(QString::number(uspex_gui.calc.pickUpGen));
  m_w->pickUpFolder->setText(QString::number(uspex_gui.calc.pickUpFolder));
  m_w->restart_cleanup->setChecked(uspex_gui.restart_cleanup);

  /* ADVANCED page defaults */
  if (uspex_gui.calc.repeatForStatistics == 0)
    uspex_gui.calc.repeatForStatistics = 1;
  if (uspex_gui.calc.percSliceShift == 0.0)
    uspex_gui.calc.percSliceShift = 1.0;
  if (uspex_gui.calc.maxDistHeredity == 0.0)
    uspex_gui.calc.maxDistHeredity = 0.5;
  m_w->repeatForStatistics->setText(QString::number(uspex_gui.calc.repeatForStatistics));
  m_w->stopFitness->setText(QString::number(uspex_gui.calc.stopFitness, 'f', 4));
  m_w->fixRndSeed->setText(QString::number(uspex_gui.calc.fixRndSeed));
  m_w->collectForces->setChecked(uspex_gui.calc.collectForces);
  m_w->ordering_active->setChecked(uspex_gui.calc.ordering_active);
  m_w->symmetrize->setChecked(uspex_gui.calc.symmetrize);

  if (uspex_gui.calc.valenceElectr)
    m_w->valenceElectr->setText(QString::fromUtf8(reinterpret_cast<const char *>(uspex_gui.calc.valenceElectr)));
  m_w->percSliceShift->setText(QString::number(uspex_gui.calc.percSliceShift, 'f', 4));
  m_w->minSlice->setText(QString::number(uspex_gui.calc.minSlice, 'f', 4));
  m_w->dynamicalBestHM->setCurrentIndex(2); /* DEFAULT: 2 */
  m_w->maxSlice->setText(QString::number(uspex_gui.calc.maxSlice, 'f', 4));

  /* SoftMut, NumP, many_P defaults */
  if (uspex_gui.calc.numberparents == 0)
    uspex_gui.calc.numberparents = 2;
  /* softMutOnly default is NULL — don't allocate unless user sets it */
  if (uspex_gui.calc.softMutOnly)
    m_w->softMutOnly->setText(QString::fromUtf8(uspex_gui.calc.softMutOnly));
  m_w->maxDistHeredity->setText(QString::number(uspex_gui.calc.maxDistHeredity, 'f', 4));
  m_w->numberparents->setText(QString::number(uspex_gui.calc.numberparents));
  /* manyParents: map calc value to combo index (0,1,2,3) */
  if (uspex_gui.calc.manyParents >= 0 && uspex_gui.calc.manyParents <= 3)
    m_w->manyParents->setCurrentIndex((int) uspex_gui.calc.manyParents);
  else
    m_w->manyParents->setCurrentIndex(1); /* DEFAULT: 1 */
  m_w->TE_goal->setCurrentIndex(0);
  /* BoltzTraP defaults */
  if (uspex_gui.calc.BoltzTraP_T_max == 0.0)
    uspex_gui.calc.BoltzTraP_T_max = 800.0;
  if (uspex_gui.calc.BoltzTraP_T_delta == 0.0)
    uspex_gui.calc.BoltzTraP_T_delta = 50.0;
  if (uspex_gui.calc.BoltzTraP_T_efcut == 0.0)
    uspex_gui.calc.BoltzTraP_T_efcut = 0.15;
  m_w->BoltzTraP_T_max->setText(QString::number(uspex_gui.calc.BoltzTraP_T_max, 'f', 4));
  m_w->BoltzTraP_T_delta->setText(QString::number(uspex_gui.calc.BoltzTraP_T_delta, 'f', 4));
  m_w->BoltzTraP_T_efcut->setText(QString::number(uspex_gui.calc.BoltzTraP_T_efcut, 'f', 4));

  if (uspex_gui._tmp_cmd_BoltzTraP)
    m_w->cmd_BoltzTraP->setText(QString::fromUtf8(uspex_gui._tmp_cmd_BoltzTraP));
  m_w->TE_T_interest->setText(QString::number(uspex_gui.calc.TE_T_interest, 'f', 4));
  m_w->TE_threshold->setText(QString::number(uspex_gui.calc.TE_threshold, 'f', 4));
  /* numIterations default: 1000 */
  if (uspex_gui.calc.numIterations == 0)
    uspex_gui.calc.numIterations = 1000;
  m_w->numIterations->setText(QString::number(uspex_gui.calc.numIterations));
  /* TPS defaults */
  if (uspex_gui.calc.shiftRatio == 0.0)
    uspex_gui.calc.shiftRatio = 0.1;
  if (uspex_gui.calc.amplitudeShoot[0] == 0.0 && uspex_gui.calc.amplitudeShoot[1] == 0.0)
  {
    uspex_gui.calc.amplitudeShoot[0] = 0.1;
    uspex_gui.calc.amplitudeShoot[1] = 0.1;
  }
  if (uspex_gui.calc.magnitudeShoot[0] == 0.0 && uspex_gui.calc.magnitudeShoot[1] == 0.0)
  {
    uspex_gui.calc.magnitudeShoot[0] = 1.05;
    uspex_gui.calc.magnitudeShoot[1] = 1.05;
  }
  m_w->shiftRatio->setText(QString::number(uspex_gui.calc.shiftRatio, 'f', 4));
  m_w->orderParaType->setChecked(uspex_gui.calc.orderParaType);

  if (uspex_gui.calc.speciesSymbol)
    m_w->speciesSymbol->setText(QString::fromUtf8(uspex_gui.calc.speciesSymbol));

  if (uspex_gui.calc.mass)
    m_w->mass->setText(QString::fromUtf8(reinterpret_cast<const char *>(uspex_gui.calc.mass)));
  m_w->amplitudeShoot_AB->setText(QString::number(uspex_gui.calc.amplitudeShoot[0], 'f', 4));
  m_w->amplitudeShoot_BA->setText(QString::number(uspex_gui.calc.amplitudeShoot[1], 'f', 4));
  m_w->magnitudeShoot_success->setText(QString::number(uspex_gui.calc.magnitudeShoot[0], 'f', 4));
  m_w->magnitudeShoot_failure->setText(QString::number(uspex_gui.calc.magnitudeShoot[1], 'f', 4));
  if (uspex_gui.calc.cmdOrderParameter)
    m_w->cmdOrderParameter->setText(QString::fromUtf8(uspex_gui.calc.cmdOrderParameter));
  if (uspex_gui.calc.opCriteria)
  {
    m_w->opCriteria_start->setText(QString::number(uspex_gui.calc.opCriteria[0], 'f', 4));
    m_w->opCriteria_end->setText(QString::number(uspex_gui.calc.opCriteria[1], 'f', 4));
  }
  /* TPS file path defaults */
  if (uspex_gui.calc.orderParameterFile == NULL || !ptr_valid(uspex_gui.calc.orderParameterFile))
  {
    g_free(uspex_gui.calc.orderParameterFile);
    uspex_gui.calc.orderParameterFile = g_strdup("fp.dat");
  }
  if (uspex_gui.calc.enthalpyTemperatureFile == NULL || !ptr_valid(uspex_gui.calc.enthalpyTemperatureFile))
  {
    g_free(uspex_gui.calc.enthalpyTemperatureFile);
    uspex_gui.calc.enthalpyTemperatureFile = g_strdup("HT.dat");
  }
  if (uspex_gui.calc.trajectoryFile == NULL || !ptr_valid(uspex_gui.calc.trajectoryFile))
  {
    g_free(uspex_gui.calc.trajectoryFile);
    uspex_gui.calc.trajectoryFile = g_strdup("traj.dat");
  }
  if (uspex_gui.calc.MDrestartFile == NULL || !ptr_valid(uspex_gui.calc.MDrestartFile))
  {
    g_free(uspex_gui.calc.MDrestartFile);
    uspex_gui.calc.MDrestartFile = g_strdup("traj.restart");
  }

  if (uspex_gui.calc.cmdEnthalpyTemperature)
    m_w->cmdEnthalpyTemperature->setText(QString::fromUtf8(uspex_gui.calc.cmdEnthalpyTemperature));
  if (uspex_gui.calc.orderParameterFile)
    m_w->orderParameterFile->setText(QString::fromUtf8(uspex_gui.calc.orderParameterFile));
  if (uspex_gui.calc.enthalpyTemperatureFile)
    m_w->enthalpyTemperatureFile->setText(QString::fromUtf8(uspex_gui.calc.enthalpyTemperatureFile));
  if (uspex_gui.calc.trajectoryFile)
    m_w->trajectoryFile->setText(QString::fromUtf8(uspex_gui.calc.trajectoryFile));
  if (uspex_gui.calc.MDrestartFile)
    m_w->MDrestartFile->setText(QString::fromUtf8(uspex_gui.calc.MDrestartFile));

  /* SPECIFIC page */
  m_w->FullRelax->setCurrentIndex(qMin(uspex_gui.calc.FullRelax, 2));
  m_w->maxVectorLength->setText(QString::number(uspex_gui.calc.maxVectorLength, 'f', 4));
  m_w->GaussianWidth->setText(QString::number(uspex_gui.calc.GaussianWidth, 'f', 4));
  m_w->GaussianHeight->setText(QString::number(uspex_gui.calc.GaussianHeight, 'f', 4));
  /* Populate meta_model combo (same as on_method_changed META case) */
  {
    m_w->meta_model->blockSignals(true);
    {
      while (m_w->meta_model->count() > 0)
        m_w->meta_model->removeItem(0);
      m_w->meta_model->addItem(tr("From VASP5 POSCAR file"));
      int idx = 1;
      for (GSList *l = sysenv.mal; l; l = g_slist_next(l))
      {
        struct model_pak *data = (struct model_pak *) l->data;
        if (g_slist_length(data->cores) > 0)
        {
          QString name = data->basename ? QString::fromUtf8(data->basename) : QString::number(idx);
          m_w->meta_model->addItem(QString("%1: %2").arg(idx).arg(name));
          idx++;
        }
      }
    }
    m_w->meta_model->blockSignals(false);
  }
  /* PSO defaults */
  if (uspex_gui.calc.PSO_softMut == 0.0)
    uspex_gui.calc.PSO_softMut = 1.0;
  if (uspex_gui.calc.PSO_BestStruc == 0.0)
    uspex_gui.calc.PSO_BestStruc = 1.0;
  if (uspex_gui.calc.PSO_BestEver == 0.0)
    uspex_gui.calc.PSO_BestEver = 1.0;

  m_w->meta_model->setCurrentIndex(0);
  on_meta_model_changed(0);
  m_w->PSO_softMut->setText(QString::number(uspex_gui.calc.PSO_softMut, 'f', 4));
  m_w->PSO_BestStruc->setText(QString::number(uspex_gui.calc.PSO_BestStruc, 'f', 4));
  m_w->PSO_BestEver->setText(QString::number(uspex_gui.calc.PSO_BestEver, 'f', 4));
  /* VCNEB defaults */
  if (uspex_gui.calc.vcnebType == 0)
    uspex_gui.calc.vcnebType = 110;
  if (uspex_gui.calc._vcnebtype_img_num == 0)
    uspex_gui.calc._vcnebtype_img_num = TRUE;
  if (uspex_gui.calc._vcnebtype_spring == 0)
    uspex_gui.calc._vcnebtype_spring = FALSE;
  m_w->vcnebtype_method->blockSignals(true);
  m_w->vcnebtype_method->setCurrentIndex(0);
  m_w->vcnebtype_method->blockSignals(false);
  m_w->vcnebType->setText(QString::number(uspex_gui.calc.vcnebType));
  m_w->vcnebtype_img_num->blockSignals(true);
  m_w->vcnebtype_img_num->setChecked(uspex_gui.calc._vcnebtype_img_num);
  m_w->vcnebtype_img_num->blockSignals(false);
  m_w->vcnebtype_spring->blockSignals(true);
  m_w->vcnebtype_spring->setChecked(uspex_gui.calc._vcnebtype_spring);
  m_w->vcnebtype_spring->blockSignals(false);
  /* VCNEB/ADVANCED defaults */
  if (uspex_gui.calc.optReadImages == 0)
    uspex_gui.calc.optReadImages = 2;
  if (uspex_gui.calc.numImages == 0)
    uspex_gui.calc.numImages = 9;
  if (uspex_gui.calc.numSteps == 0)
    uspex_gui.calc.numSteps = 600;
  if (uspex_gui.calc.optimizerType == 0)
    uspex_gui.calc.optimizerType = 1;
  if (uspex_gui.calc.dt == 0.0)
    uspex_gui.calc.dt = 0.05;
  if (uspex_gui.calc.ConvThreshold == 0.0)
    uspex_gui.calc.ConvThreshold = 0.003;
  if (uspex_gui.calc.optRelaxType == 0)
    uspex_gui.calc.optRelaxType = 3;
  if (uspex_gui.calc.K_min == 0.0)
    uspex_gui.calc.K_min = 5.0;
  if (uspex_gui.calc.K_max == 0.0)
    uspex_gui.calc.K_max = 5.0;
  if (uspex_gui.calc.Kconstant == 0.0)
    uspex_gui.calc.Kconstant = 5.0;
  if (uspex_gui.calc.optMethodCIDI == 0)
    uspex_gui.calc.optMethodCIDI = 0;
  if (uspex_gui.calc.startCIDIStep == 0)
    uspex_gui.calc.startCIDIStep = 100;

  m_w->optReadImages->setCurrentIndex(qMin(uspex_gui.calc.optReadImages, 2));
  m_w->numImages->setText(QString::number(uspex_gui.calc.numImages));
  m_w->numSteps->setText(QString::number(uspex_gui.calc.numSteps));
  m_w->optFreezing->setChecked(uspex_gui.calc.optFreezing);
  m_w->optimizerType->setCurrentIndex(qMin(uspex_gui.calc.optimizerType - 1, 1));
  m_w->dt->setText(QString::number(uspex_gui.calc.dt, 'f', 4));
  m_w->ConvThreshold->setText(QString::number(uspex_gui.calc.ConvThreshold, 'f', 4));
  m_w->VarPathLength->setText(QString::number(uspex_gui.calc.VarPathLength, 'f', 4));
  m_w->optRelaxType->setCurrentIndex(qMin(uspex_gui.calc.optRelaxType - 1, 2));
  m_w->K_min->setText(QString::number(uspex_gui.calc.K_min, 'f', 4));
  m_w->K_max->setText(QString::number(uspex_gui.calc.K_max, 'f', 4));
  m_w->Kconstant->setText(QString::number(uspex_gui.calc.Kconstant, 'f', 4));
  /* CI/DI defaults */
  if (uspex_gui.calc.optMethodCIDI == 0)
    uspex_gui.calc.optMethodCIDI = 0;
  if (uspex_gui.calc.startCIDIStep == 0)
    uspex_gui.calc.startCIDIStep = 100;
  if (uspex_gui.calc.PrintStep == 0)
    uspex_gui.calc.PrintStep = 1;

  /* optMethodCIDI: combo items are [0, 1, -1, 2], calc values are [0, 1, -1, 2] */
  {
    int vals[] = {0, 1, -1, 2};
    int idx = 0;
    for (int i = 0; i < 4; i++)
    {
      if (uspex_gui.calc.optMethodCIDI == vals[i])
      {
        idx = i;
        break;
      }
    }
    m_w->optMethodCIDI->setCurrentIndex(idx);
  }
  m_w->startCIDIStep->setText(QString::number(uspex_gui.calc.startCIDIStep));

  if (uspex_gui.calc.pickupImages)
    m_w->pickupImages->setText(QString::number(uspex_gui.calc.pickupImages ? uspex_gui.calc.pickupImages[0] : 0));
  m_w->PrintStep->setText(QString::number(uspex_gui.calc.PrintStep));

  /* Disable Start CI/DI and Pickup when optMethodCIDI is 0 */
  int cidiIdx = qMin(uspex_gui.calc.optMethodCIDI + 1, 3);
  m_w->startCIDIStep->setDisabled(cidiIdx == 0);
  m_w->pickupImages->setDisabled(cidiIdx == 0);
  /* FormatType default: 2 (VASP v5) */
  if (uspex_gui.calc.FormatType == 0)
    uspex_gui.calc.FormatType = 2;
  m_w->FormatType->setCurrentIndex(qMin(uspex_gui.calc.FormatType - 1, 2));

  /* Populate img_model combo (same as meta_model but only periodic==3 models) */
  {
    m_w->img_model->blockSignals(true);
    {
      while (m_w->img_model->count() > 0)
        m_w->img_model->removeItem(0);
      m_w->img_model->addItem(tr("From file"));
      int idx = 1;
      for (GSList *l = sysenv.mal; l; l = g_slist_next(l))
      {
        struct model_pak *data = (struct model_pak *) l->data;
        if (data->periodic == 3)
        {
          QString name = data->basename ? QString::fromUtf8(data->basename) : QString::number(idx);
          m_w->img_model->addItem(QString("%1: %2").arg(idx).arg(name));
          idx++;
        }
      }
    }
    m_w->img_model->blockSignals(false);
  }
  m_w->img_model->setCurrentIndex(0);
  m_w->mol_model->setCurrentIndex(0);
  m_w->num_mol->setValue((int) uspex_gui._tmp_num_mol);

  /* Populate GDIS_MOL combo with models that have molecules */
  {
    m_w->mol_gdis->blockSignals(true);
    {
      while (m_w->mol_gdis->count() > 0)
        m_w->mol_gdis->removeItem(0);
      int idx = 0;
      for (GSList *l = sysenv.mal; l; l = g_slist_next(l))
      {
        struct model_pak *data = (struct model_pak *) l->data;
        if (g_slist_length(data->moles) > 0)
        {
          QString name = data->basename ? QString::fromUtf8(data->basename) : QString::number(idx);
          m_w->mol_gdis->addItem(QString("%1: %2").arg(idx).arg(name));
          idx++;
        }
      }
    }
    m_w->mol_gdis->blockSignals(false);
  }

  /* Initialize _tmp_mols arrays if needed */
  if (uspex_gui.calc._nmolecules <= 0)
    uspex_gui.calc._nmolecules = 1;
  if (!uspex_gui._tmp_mols_gdis)
  {
    uspex_gui._tmp_mols_gdis = g_new(gint, uspex_gui.calc._nmolecules);
    for (int i = 0; i < uspex_gui.calc._nmolecules; i++)
      uspex_gui._tmp_mols_gdis[i] = 0;
  }
  if (!uspex_gui._tmp_mols_gulp)
  {
    uspex_gui._tmp_mols_gulp = g_new(gboolean, uspex_gui.calc._nmolecules);
    for (int i = 0; i < uspex_gui.calc._nmolecules; i++)
      uspex_gui._tmp_mols_gulp[i] = FALSE;
  }

  on_num_mol_changed((int) uspex_gui._tmp_num_mol);
  m_w->mol_gdis->setCurrentIndex(0);
  m_w->curr_mol->setValue((int) uspex_gui._tmp_curr_mol);
  m_w->mol_gulp->setChecked(uspex_gui.mol_as_gulp);
  /* Surfaces defaults */
  if (uspex_gui.calc.reconstruct == 0)
    uspex_gui.calc.reconstruct = 1;
  if (uspex_gui.calc.thicknessS == 0.0)
    uspex_gui.calc.thicknessS = 2.0;
  if (uspex_gui.calc.thicknessB == 0.0)
    uspex_gui.calc.thicknessB = 3.0;

  /* Populate substrate_model combo */
  {
    m_w->substrate_model->blockSignals(true);
    {
      while (m_w->substrate_model->count() > 0)
        m_w->substrate_model->removeItem(0);
      m_w->substrate_model->addItem(tr("From VASP5 POSCAR file"));
      int idx = 1;
      for (GSList *l = sysenv.mal; l; l = g_slist_next(l))
      {
        struct model_pak *data = (struct model_pak *) l->data;
        if (data->periodic == 3)
        {
          QString name = data->basename ? QString::fromUtf8(data->basename) : QString::number(idx);
          m_w->substrate_model->addItem(QString("%1: %2").arg(idx).arg(name));
          idx++;
        }
      }
    }
    m_w->substrate_model->blockSignals(false);
  }
  m_w->substrate_model->setCurrentIndex(0);
  on_substrate_model_changed(0);

  m_w->reconstruct->setText(QString::number(uspex_gui.calc.reconstruct));
  m_w->thicknessS->setText(QString::number(uspex_gui.calc.thicknessS, 'f', 4));
  m_w->thicknessB->setText(QString::number(uspex_gui.calc.thicknessB, 'f', 4));

  if (uspex_gui.calc.StoichiometryStart)
    m_w->StoichiometryStart->setText(
        QString::number(uspex_gui.calc.StoichiometryStart ? uspex_gui.calc.StoichiometryStart[0] : 0));
  m_w->E_AB->setText(QString::number(uspex_gui.calc.E_AB, 'f', 4));
  m_w->Mu_A->setText(QString::number(uspex_gui.calc.Mu_A, 'f', 4));
  m_w->Mu_B->setText(QString::number(uspex_gui.calc.Mu_B, 'f', 4));

  /* Apply Molecules frame enable/disable logic after all widgets are set */
  on_mol_model_changed(m_w->mol_model->currentIndex());
}

/* ============================================================
 * sync() - write widget values back to uspex_gui.calc
 * ============================================================ */
void UspexDialog::sync()
{
  /* MODEL NAME — sync to model basename */
  {
    struct model_pak *data = qt_get_active_model();
    if (data)
    {
      g_free(data->basename);
      data->basename = g_strdup(m_w->name->text().toUtf8().constData());
      /* Also set the name used in INPUT.txt */
      g_free(uspex_gui.calc.name);
      uspex_gui.calc.name = g_strdup(m_w->name->text().toUtf8().constData());
    }
  }
  /* CONNECTED OUTPUT */
  g_free(uspex_gui.file_entry_path);
  uspex_gui.file_entry_path = g_strdup(m_w->file_entry->text().toUtf8().constData());

  /* Invalidate widget pointers — Qt dialog has no such widgets */
  uspex_gui.name = NULL;
  uspex_gui.file_entry = NULL;

  int methodIdx = m_w->calculationMethod->currentIndex();
  switch (methodIdx)
  {
  case 1:
    uspex_gui.calc.calculationMethod = US_CM_META;
    break;
  case 2:
    uspex_gui.calc.calculationMethod = US_CM_VCNEB;
    break;
  case 3:
    uspex_gui.calc.calculationMethod = US_CM_PSO;
    break;
  case 4:
    uspex_gui.calc.calculationMethod = US_CM_TPS;
    break;
  case 5:
    uspex_gui.calc.calculationMethod = US_CM_MINHOP;
    break;
  case 6:
    uspex_gui.calc.calculationMethod = US_CM_COPEX;
    break;
  default:
    uspex_gui.calc.calculationMethod = US_CM_USPEX;
    break;
  }

  int typeIdx = m_w->calculationType->currentIndex();
  switch (typeIdx)
  {
  case 1:
    uspex_gui.calc.calculationType = US_CT_s300;
    break;
  case 2:
    uspex_gui.calc.calculationType = US_CT_301;
    break;
  case 3:
    uspex_gui.calc.calculationType = US_CT_s301;
    break;
  case 4:
    uspex_gui.calc.calculationType = US_CT_310;
    break;
  case 5:
    uspex_gui.calc.calculationType = US_CT_311;
    break;
  case 6:
    uspex_gui.calc.calculationType = US_CT_000;
    break;
  case 7:
    uspex_gui.calc.calculationType = US_CT_s000;
    break;
  case 8:
    uspex_gui.calc.calculationType = US_CT_001;
    break;
  case 9:
    uspex_gui.calc.calculationType = US_CT_110;
    break;
  case 10:
    uspex_gui.calc.calculationType = US_CT_200;
    break;
  case 11:
    uspex_gui.calc.calculationType = US_CT_s200;
    break;
  case 12:
    uspex_gui.calc.calculationType = US_CT_201;
    break;
  case 13:
    uspex_gui.calc.calculationType = US_CT_s201;
    break;
  case 14:
    uspex_gui.calc.calculationType = US_CT_m200;
    break;
  case 15:
    uspex_gui.calc.calculationType = US_CT_sm200;
    break;
  case 16:
    uspex_gui.calc.calculationType = US_CT_m201;
    break;
  case 17:
    uspex_gui.calc.calculationType = US_CT_sm201;
    break;
  default:
    uspex_gui.calc.calculationType = US_CT_300;
    break;
  }

  uspex_gui._dim = (gdouble) m_w->calctype_dim->value();
  uspex_gui.calc.ExternalPressure = m_w->ExternalPressure->value();
  uspex_gui.have_v1030 = m_w->sel_v1030_1->isChecked();
  uspex_gui.calc._calctype_mag = m_w->calctype_mag->isChecked();
  uspex_gui.calc._calctype_mol = m_w->calctype_mol->isChecked();
  uspex_gui.calc._calctype_var = m_w->calctype_var->isChecked();

  g_strlcpy(uspex_gui._tmp_atom_sym, m_w->atom_sym->text().toUtf8().constData(), 3);
  uspex_gui._tmp_atom_typ = m_w->atom_typ->text().toInt();
  uspex_gui._tmp_atom_num = m_w->atom_num->text().toInt();
  uspex_gui._tmp_atom_val = m_w->atom_val->text().toInt();

  g_free(uspex_gui._tmp_blockSpecies);
  uspex_gui._tmp_blockSpecies = g_strdup(m_w->blockSpecies->text().toUtf8().constData());
  g_free(uspex_gui._tmp_bond_d);
  uspex_gui._tmp_bond_d = g_strdup(m_w->bond_d->text().toUtf8().constData());
  g_free(uspex_gui._tmp_ldaU);
  uspex_gui._tmp_ldaU = g_strdup(m_w->ldaU->text().toUtf8().constData());

  int optIdx = m_w->optType->currentIndex();
  uspex_gui.calc.optType = (uspex_opt) (optIdx + 1);
  g_free(uspex_gui._tmp_new_optType);
  uspex_gui._tmp_new_optType = g_strdup(m_w->new_optType->text().toUtf8().constData());
  uspex_gui.have_new_optType = m_w->sel_new_opt->isChecked();
  uspex_gui.calc.anti_opt = m_w->anti_opt->isChecked();
  uspex_gui.calc.checkMolecules = m_w->checkMolecules->isChecked();
  uspex_gui.calc.checkConnectivity = m_w->checkConnectivity->isChecked();

  uspex_gui.auto_C_lat = m_w->auto_C_lat->isChecked();
  uspex_gui.calc.minVectorLength = m_w->minVectorLength->text().toDouble();
  uspex_gui.calc.constraint_enhancement = m_w->constraint_enhancement->text().toInt();

  /* Sync IonDistances from combo */
  g_free(uspex_gui.calc.IonDistances);
  uspex_gui.calc.IonDistances = nullptr;
  if (m_w->IonDistances->count() > 0 && uspex_gui.calc._nspecies > 0)
  {
    int total = uspex_gui.calc._nspecies * uspex_gui.calc._nspecies;
    uspex_gui.calc.IonDistances = (double *) g_malloc0(total * sizeof(double));
    for (int i = 0; i < m_w->IonDistances->count() && i < uspex_gui.calc._nspecies; i++)
    {
      QString line = m_w->IonDistances->itemText(i);
      QStringList parts = line.trimmed().split(' ', Qt::SkipEmptyParts);
      for (int j = 0; j < parts.size() && j < uspex_gui.calc._nspecies; j++)
      {
        bool ok;
        double val = parts[j].toDouble(&ok);
        if (ok)
          uspex_gui.calc.IonDistances[i * uspex_gui.calc._nspecies + j] = val;
      }
    }
  }

  uspex_gui.auto_C_ion = m_w->auto_C_ion->isChecked();

  /* STRUCTURE */
  uspex_gui.calc.populationSize = m_w->populationSize->text().toInt();
  uspex_gui.calc.initialPopSize = m_w->initialPopSize->text().toInt();
  uspex_gui.calc.numGenerations = m_w->numGenerations->text().toInt();
  uspex_gui.calc.stopCrit = m_w->stopCrit->text().toInt();
  uspex_gui.calc.magRatio[0] = m_w->mag_nm->text().toDouble();
  uspex_gui.calc.magRatio[1] = m_w->mag_fmls->text().toDouble();
  uspex_gui.calc.magRatio[3] = m_w->mag_afml->text().toDouble();
  uspex_gui.calc.magRatio[5] = m_w->mag_fmlh->text().toDouble();
  uspex_gui.calc._calctype_mag = m_w->calctype_mag_2->isChecked();
  uspex_gui.calc.magRatio[2] = m_w->mag_fmhs->text().toDouble();
  uspex_gui.calc.magRatio[4] = m_w->mag_afmh->text().toDouble();
  uspex_gui.calc.magRatio[6] = m_w->mag_aflh->text().toDouble();
  uspex_gui.calc.bestFrac = m_w->bestFrac->text().toDouble();
  uspex_gui.calc.keepBestHM = m_w->keepBestHM->text().toInt();
  uspex_gui.calc.reoptOld = m_w->reoptOld->isChecked();
  uspex_gui.calc.fitLimit = m_w->fitLimit->text().toDouble();

  g_free(uspex_gui.calc.symmetries);
  QString sym_text = m_w->symmetries->text().trimmed();
  uspex_gui.calc.symmetries = sym_text.isEmpty() ? NULL : g_strdup(sym_text.toUtf8().constData());
  uspex_gui.calc.fracGene = m_w->fracGene->text().toDouble();
  uspex_gui.calc.fracRand = m_w->fracRand->text().toDouble();
  uspex_gui.calc.fracTopRand = m_w->fracTopRand->text().toDouble();
  uspex_gui.calc.fracPerm = m_w->fracPerm->text().toDouble();
  uspex_gui.calc.fracAtomsMut = m_w->fracAtomsMut->text().toDouble();
  uspex_gui.calc.fracRotMut = m_w->fracRotMut->text().toDouble();
  uspex_gui.calc.fracLatMut = m_w->fracLatMut->text().toDouble();
  uspex_gui.calc.fracSpinMut = m_w->fracSpinMut->text().toDouble();
  uspex_gui.calc.howManySwaps = m_w->howManySwaps->text().toInt();
  g_free(uspex_gui.calc.specificSwaps);
  QString ss_text = m_w->specificSwaps->text().trimmed();
  uspex_gui.calc.specificSwaps = ss_text.isEmpty() ? NULL : g_strdup(ss_text.toUtf8().constData());
  uspex_gui.calc.mutationDegree = m_w->mutationDegree->text().toDouble();
  uspex_gui.calc.mutationRate = m_w->mutationRate->text().toDouble();
  uspex_gui.calc.DisplaceInLatmutation = m_w->DisplaceInLatmutation->text().toDouble();
  uspex_gui.calc.AutoFrac = m_w->AutoFrac->isChecked();
  uspex_gui.calc.RmaxFing = m_w->RmaxFing->text().toDouble();
  uspex_gui.calc.deltaFing = m_w->deltaFing->text().toDouble();
  uspex_gui.calc.sigmaFing = m_w->sigmaFing->text().toDouble();
  uspex_gui.calc.toleranceFing = m_w->toleranceFing->text().toDouble();
  uspex_gui.calc.antiSeedsActivation = m_w->antiSeedsActivation->text().toInt();
  uspex_gui.calc.antiSeedsMax = m_w->antiSeedsMax->text().toDouble();
  uspex_gui.calc.antiSeedsSigma = m_w->antiSeedsSigma->text().toDouble();
  uspex_gui.calc.doSpaceGroup = m_w->doSpaceGroup->isChecked();
  uspex_gui.calc.SymTolerance = m_w->SymTolerance->text().toDouble();
  uspex_gui.calc.firstGeneMax = m_w->firstGeneMax->text().toInt();
  uspex_gui.calc.minAt = m_w->minAt->text().toInt();
  uspex_gui.calc.maxAt = m_w->maxAt->text().toInt();
  uspex_gui.calc.fracTrans = m_w->fracTrans->text().toDouble();
  uspex_gui.calc.howManyTrans = m_w->howManyTrans->text().toDouble();

  /* CALCULATION */
  /* set_specific is a gpointer* in the real struct — store state in have_specific instead */
  uspex_gui.have_specific = m_w->set_specific->isChecked();
  g_free(uspex_gui._tmp_spe_folder);
  uspex_gui._tmp_spe_folder = g_strdup(m_w->spe_folder->text().toUtf8().constData());
  uspex_gui._tmp_num_opt_steps = (gdouble) m_w->num_opt_steps->value();
  uspex_gui._tmp_curr_step = (gdouble) m_w->curr_step->value();
  uspex_gui.auto_step = m_w->auto_step->isChecked();
  uspex_gui._tmp_isfixed = m_w->isfixed->isChecked();

  /* Ensure _isfixed and _num_opt_steps are consistent */
  gint n_steps = (gint) uspex_gui._tmp_num_opt_steps;
  if (n_steps <= 0)
    n_steps = 1;
  if (uspex_gui.calc._num_opt_steps != n_steps || uspex_gui.calc._isfixed == NULL)
  {
    g_free(uspex_gui.calc._isfixed);
    uspex_gui.calc._isfixed = g_new(gboolean, n_steps);
    for (gint i = 0; i < n_steps; i++)
      uspex_gui.calc._isfixed[i] = FALSE;
    uspex_gui.calc._num_opt_steps = n_steps;
  }

  int aiIdx = m_w->abinitioCode->currentIndex();
  if (uspex_gui.calc.abinitioCode == NULL || uspex_gui.calc._num_opt_steps <= 0)
  {
    g_free(uspex_gui.calc.abinitioCode);
    gint n = (uspex_gui.calc._num_opt_steps > 0) ? uspex_gui.calc._num_opt_steps : 1;
    uspex_gui.calc.abinitioCode = (gint *) g_malloc(n * sizeof(gint));
    for (gint i = 0; i < n; i++)
      uspex_gui.calc.abinitioCode[i] = aiIdx;
  } else if (uspex_gui.calc._num_opt_steps > 0)
  {
    for (gint i = 0; i < uspex_gui.calc._num_opt_steps; i++)
    {
      uspex_gui.calc.abinitioCode[i] = aiIdx;
    }
  }
  if (uspex_gui.calc.KresolStart == NULL)
  {
    uspex_gui.calc.KresolStart = (double *) g_malloc(sizeof(gdouble));
  }
  uspex_gui.calc.KresolStart[0] = m_w->KresolStart->text().toDouble();
  if (uspex_gui.calc.vacuumSize == NULL)
  {
    uspex_gui.calc.vacuumSize = (double *) g_malloc(sizeof(gdouble));
  }
  uspex_gui.calc.vacuumSize[0] = m_w->vacuumSize->text().toDouble();

  if (uspex_gui._tmp_ai_input && ptr_valid(uspex_gui._tmp_ai_input))
  {
    g_free(uspex_gui._tmp_ai_input[0]);
    uspex_gui._tmp_ai_input[0] = g_strdup(m_w->ai_input->text().toUtf8().constData());
  }
  if (uspex_gui._tmp_ai_opt && ptr_valid(uspex_gui._tmp_ai_opt))
  {
    g_free(uspex_gui._tmp_ai_opt[0]);
    uspex_gui._tmp_ai_opt[0] = g_strdup(m_w->ai_opt->text().toUtf8().constData());
  }
  if (uspex_gui._tmp_ai_lib_folder && ptr_valid(uspex_gui._tmp_ai_lib_folder))
  {
    g_free(uspex_gui._tmp_ai_lib_folder[0]);
    uspex_gui._tmp_ai_lib_folder[0] = g_strdup(m_w->ai_lib->text().toUtf8().constData());
  }
  if (uspex_gui._tmp_ai_lib_sel && ptr_valid(uspex_gui._tmp_ai_lib_sel))
  {
    g_free(uspex_gui._tmp_ai_lib_sel[0]);
    uspex_gui._tmp_ai_lib_sel[0] = g_strdup(m_w->ai_lib_sel->text().toUtf8().constData());
  }
  if (uspex_gui._tmp_commandExecutable && ptr_valid(uspex_gui._tmp_commandExecutable))
  {
    g_free(uspex_gui._tmp_commandExecutable[0]);
    uspex_gui._tmp_commandExecutable[0] = g_strdup(m_w->commandExecutable->text().toUtf8().constData());
  }

  g_free(uspex_gui.calc.job_uspex_exe);
  uspex_gui.calc.job_uspex_exe = g_strdup(m_w->job_uspex_exe->text().toUtf8().constData());
  uspex_gui.calc.whichCluster = m_w->whichCluster->text().toInt();
  uspex_gui.calc.PhaseDiagram = m_w->PhaseDiagram->isChecked();
  uspex_gui.have_v1030 = m_w->sel_v1030_3->isChecked();
  /* numProcessors is gint* in struct — skip sync for deprecated field */
  (void) m_w->numProcessors;
  uspex_gui.calc.numParallelCalcs = m_w->numParallelCalcs->text().toInt();
  uspex_gui.have_octave = m_w->sel_octave->isChecked();
  g_free(uspex_gui.calc.job_path);
  uspex_gui.calc.job_path = g_strdup(m_w->job_path->text().toUtf8().constData());
  g_free(uspex_gui.calc.remoteFolder);
  uspex_gui.calc.remoteFolder = g_strdup(m_w->remoteFolder->text().toUtf8().constData());
  uspex_gui.calc.pickUpYN = m_w->pickUpYN->isChecked();
  uspex_gui.calc.pickUpGen = m_w->pickUpGen->text().toInt();
  uspex_gui.calc.pickUpFolder = m_w->pickUpFolder->text().toInt();
  uspex_gui.restart_cleanup = m_w->restart_cleanup->isChecked();

  /* ADVANCED */
  uspex_gui.calc.repeatForStatistics = m_w->repeatForStatistics->text().toInt();
  uspex_gui.calc.stopFitness = m_w->stopFitness->text().toDouble();
  uspex_gui.calc.fixRndSeed = m_w->fixRndSeed->text().toInt();
  uspex_gui.calc.collectForces = m_w->collectForces->isChecked();
  uspex_gui.calc.ordering_active = m_w->ordering_active->isChecked();
  uspex_gui.calc.symmetrize = m_w->symmetrize->isChecked();
  /* valenceElectr is gint* in struct — skip sync for deprecated field */
  (void) m_w->valenceElectr;
  uspex_gui.calc.percSliceShift = m_w->percSliceShift->text().toDouble();
  uspex_gui.calc.minSlice = m_w->minSlice->text().toDouble();
  uspex_gui.calc.maxSlice = m_w->maxSlice->text().toDouble();

  if (uspex_gui.calc.softMutOnly)
    g_free(uspex_gui.calc.softMutOnly);
  QString smo_text = m_w->softMutOnly->text().trimmed();
  uspex_gui.calc.softMutOnly = smo_text.isEmpty() ? NULL : g_strdup(smo_text.toUtf8().constData());

  uspex_gui.calc.maxDistHeredity = m_w->maxDistHeredity->text().toDouble();
  uspex_gui.calc.numberparents = m_w->numberparents->text().toInt();

  /* SPECIFIC */
  uspex_gui.calc.FullRelax = m_w->FullRelax->currentIndex();
  uspex_gui.calc.maxVectorLength = m_w->maxVectorLength->text().toDouble();
  uspex_gui.calc.GaussianWidth = m_w->GaussianWidth->text().toDouble();
  uspex_gui.calc.GaussianHeight = m_w->GaussianHeight->text().toDouble();
  uspex_gui.calc.PSO_softMut = m_w->PSO_softMut->text().toDouble();
  uspex_gui.calc.PSO_BestStruc = m_w->PSO_BestStruc->text().toDouble();
  uspex_gui.calc.PSO_BestEver = m_w->PSO_BestEver->text().toDouble();
  uspex_gui.calc.vcnebType = m_w->vcnebType->text().toInt();
  uspex_gui.calc._vcnebtype_img_num = m_w->vcnebtype_img_num->isChecked();
  uspex_gui.calc._vcnebtype_spring = m_w->vcnebtype_spring->isChecked();
  uspex_gui.calc.numImages = m_w->numImages->text().toInt();
  uspex_gui.calc.numSteps = m_w->numSteps->text().toInt();
  uspex_gui.calc.optFreezing = m_w->optFreezing->isChecked();
  uspex_gui.calc.optimizerType = m_w->optimizerType->currentIndex() + 1;
  uspex_gui.calc.dt = m_w->dt->text().toDouble();
  uspex_gui.calc.ConvThreshold = m_w->ConvThreshold->text().toDouble();
  uspex_gui.calc.VarPathLength = m_w->VarPathLength->text().toDouble();
  uspex_gui.calc.optRelaxType = m_w->optRelaxType->currentIndex() + 1;
  uspex_gui.calc.K_min = m_w->K_min->text().toDouble();
  uspex_gui.calc.K_max = m_w->K_max->text().toDouble();
  uspex_gui.calc.Kconstant = m_w->Kconstant->text().toDouble();
  uspex_gui.calc.optMethodCIDI = m_w->optMethodCIDI->currentIndex();
  uspex_gui.calc.startCIDIStep = m_w->startCIDIStep->text().toInt();
  uspex_gui.calc.PrintStep = m_w->PrintStep->text().toInt();
  uspex_gui.calc.FormatType = m_w->FormatType->currentIndex() + 1;
  uspex_gui.calc.reconstruct = m_w->reconstruct->text().toInt();
  uspex_gui.calc.thicknessS = m_w->thicknessS->text().toDouble();
  uspex_gui.calc.thicknessB = m_w->thicknessB->text().toDouble();

  /* StoichiometryStart is gint* in struct — skip sync */
  (void) m_w->StoichiometryStart;
  uspex_gui.calc.E_AB = m_w->E_AB->text().toDouble();
  uspex_gui.calc.Mu_A = m_w->Mu_A->text().toDouble();
  uspex_gui.calc.Mu_B = m_w->Mu_B->text().toDouble();

  /* Save per-format lattice storage to calc.Latticevalues (only when AUTO_LAT is unchecked) */
  g_free(uspex_gui.calc.Latticevalues);
  uspex_gui.calc.Latticevalues = NULL;
  uspex_gui.calc._nlattice_line = 0;
  uspex_gui.calc._nlattice_vals = 0;
  if (!uspex_gui.auto_C_lat)
  {
    if (m_lattice_lattice && m_lattice_lattice_lines > 0)
    {
      int total = m_lattice_lattice_lines * m_lattice_lattice_vals;
      uspex_gui.calc.Latticevalues = (double *) g_malloc(total * sizeof(gdouble));
      memcpy(uspex_gui.calc.Latticevalues, m_lattice_lattice, total * sizeof(gdouble));
      uspex_gui.calc._nlattice_line = m_lattice_lattice_lines;
      uspex_gui.calc._nlattice_vals = m_lattice_lattice_vals;
    } else if (m_lattice_crystal && m_lattice_crystal_n > 0)
    {
      uspex_gui.calc.Latticevalues = (double *) g_malloc(m_lattice_crystal_n * sizeof(gdouble));
      memcpy(uspex_gui.calc.Latticevalues, m_lattice_crystal, m_lattice_crystal_n * sizeof(gdouble));
      uspex_gui.calc._nlattice_line = 1;
      uspex_gui.calc._nlattice_vals = m_lattice_crystal_n;
    } else if (m_lattice_volume && m_lattice_volume_n > 0)
    {
      uspex_gui.calc.Latticevalues = (double *) g_malloc(sizeof(gdouble));
      uspex_gui.calc.Latticevalues[0] = m_lattice_volume[0];
      uspex_gui.calc._nlattice_line = 1;
      uspex_gui.calc._nlattice_vals = 1;
    }
  }
}

void UspexDialog::on_run()
{
  sync();
  QMessageBox::information(this, tr("USPEX"),
                           tr("USPEX execution not yet implemented in Qt version.\n"
                              "Settings have been saved to the model."));
  close();
}

void UspexDialog::on_close()
{
  sync();
  close();
}

/* ============================================================
 * Signal handlers — interdependent widget logic
 * ============================================================ */

/* Helper: compute calculationType integer from dim/mag/mol/var flags */
/* Valid ctype values (the 18 allowed types) */
static gint compute_calculationType(int dim, bool mag, bool mol, bool var, bool negDim2)
{
  int base = dim * 100;
  /* negDim2 means dim was stored as positive 2 but represents -2 */
  if (negDim2 && dim > 0)
    base = -base;
  if (mag)
  {
    if (base < 0)
      base -= 1000; /* magnetic for 2D crystals: -200 -> -1200 */
    else
      base += 1000; /* magnetic for normal: 300 -> 1300 */
  }
  if (mol)
    base += 10;
  if (var)
    base += 1;
  return base;
}

/* Helper: extract dim/mag/mol/var from calculationType integer */
static void parse_calculationType(int ctype, int &dim, bool &mag, bool &mol, bool &var, bool &negDim2)
{
  int val = ctype;
  negDim2 = false;
  if (val < 0)
  {
    val = -val;
    negDim2 = true;
  }
  mag = (val >= 1000);
  if (mag)
    val -= 1000;
  var = (val % 10 != 0);
  mol = ((val / 10) % 10 != 0);
  dim = val / 100;
  if (negDim2 && dim == 2)
    dim = -2;
}

/* Helper: find combo index matching a ctype value */
/* Map combo item text to ctype value. "s300" -> 1300, "-200" -> -200, etc. */
static int combo_item_to_ctype(const QString &text)
{
  if (text.startsWith("-s"))
  {
    int v = text.mid(2).toInt();
    return -v;
  }
  if (text.startsWith("s"))
  {
    return 1000 + text.mid(1).toInt();
  }
  if (text.startsWith("-"))
  {
    return -text.mid(1).toInt();
  }
  return text.toInt();
}

/* Set combo to ctype. Returns true if found, false if not (invalid combo). */
static bool set_calculationType_combo(QComboBox *combo, int ctype)
{
  for (int i = 0; i < combo->count(); i++)
  {
    if (combo_item_to_ctype(combo->itemText(i)) == ctype)
    {
      combo->setCurrentIndex(i);
      return true;
    }
  }
  return false;
}

/* Helper: determine if SPECIFIC tab should be visible
 * Shown when:
 *   1. Method is META(1), VCNEB(2), or PSO(3), OR
 *   2. calculationType indicates molecular/surface/polymer mode
 *      (310, 311, 110, 200, s200, 201, s201) */
static bool should_show_specific(int methodIdx, int calcType)
{
  /* Rule 1: META, VCNEB, PSO always show SPECIFIC */
  if (methodIdx == 1 || methodIdx == 2 || methodIdx == 3)
    return true;
  /* Rule 2: molecular (31x), polymer (110), surface (2xx) */
  /* Negative values = 2D crystals (m200, sm200, m201, sm201) — don't show SPECIFIC */
  if (calcType < 0)
    return false;
  int base = calcType % 1000; /* strip magnetic prefix (s* = 1xxx) */
  if (base == 310 || base == 311)
    return true; /* 310, 311 */
  if (base == 110)
    return true; /* 110 */
  if (base == 200 || base == 201)
    return true; /* 200, 201, s200, s201 */
  return false;
}

/* Helper: enable/disable BoltzTraP fields based on thermoelectric optimization */
void UspexDialog::update_boltztrap_visibility()
{
  bool have_ZT = false;
  if (uspex_gui.calc.optType == 14)
  {
    have_ZT = true;
  } else
  {
    /* Check both the calc field and the live QLineEdit text */
    QString newOpt = QString::fromUtf8(uspex_gui.calc.new_optType ? (gchar *) uspex_gui.calc.new_optType : "");
    newOpt += " " + m_w->new_optType->text();
    if (newOpt.contains("ZT") || newOpt.contains("14"))
    {
      have_ZT = true;
    }
  }
  /* Disable entire BoltzTraP frame */
  m_w->TE_goal->setDisabled(!have_ZT);
  m_w->BoltzTraP_T_max->setDisabled(!have_ZT);
  m_w->BoltzTraP_T_delta->setDisabled(!have_ZT);
  m_w->BoltzTraP_T_efcut->setDisabled(!have_ZT);
  m_w->cmd_BoltzTraP->setDisabled(!have_ZT);
  m_w->cmd_BoltzTraP_button->setDisabled(!have_ZT);
  m_w->TE_T_interest->setDisabled(!have_ZT);
  m_w->TE_threshold->setDisabled(!have_ZT);
}

void UspexDialog::on_method_changed(int index)
{
  bool showSpecific = should_show_specific(index, uspex_gui.calc.calculationType);
  m_notebook->setTabVisible(4, showSpecific);

  auto lockSpecific = [&](bool lock) {
    m_w->FullRelax->setDisabled(lock);
    m_w->maxVectorLength->setDisabled(lock);
    m_w->GaussianWidth->setDisabled(lock);
    m_w->GaussianHeight->setDisabled(lock);
    m_w->meta_model->setDisabled(lock);
    m_w->meta_model_button->setDisabled(lock);
    m_w->PSO_softMut->setDisabled(lock);
    m_w->PSO_BestStruc->setDisabled(lock);
    m_w->PSO_BestEver->setDisabled(lock);
    m_w->vcnebtype_method->setDisabled(lock);
    m_w->vcnebType->setDisabled(lock);
    m_w->vcnebtype_img_num->setDisabled(lock);
    m_w->vcnebtype_spring->setDisabled(lock);
    m_w->optReadImages->setDisabled(lock);
    m_w->numImages->setDisabled(lock);
    m_w->numSteps->setDisabled(lock);
    m_w->optFreezing->setDisabled(lock);
    m_w->optimizerType->setDisabled(lock);
    m_w->dt->setDisabled(lock);
    m_w->ConvThreshold->setDisabled(lock);
    m_w->VarPathLength->setDisabled(lock);
    m_w->optRelaxType->setDisabled(lock);
    m_w->K_min->setDisabled(lock);
    m_w->K_max->setDisabled(lock);
    m_w->Kconstant->setDisabled(lock);
    m_w->optMethodCIDI->setDisabled(lock);
    m_w->startCIDIStep->setDisabled(lock);
    m_w->pickupImages->setDisabled(lock);
    m_w->PrintStep->setDisabled(lock);
    m_w->FormatType->setDisabled(lock);
    m_w->img_model->setDisabled(lock);
    m_w->img_model_button->setDisabled(lock);
    m_w->mol_model->setDisabled(lock);
    m_w->num_mol->setDisabled(lock);
    m_w->mol_model_button->setDisabled(lock);
    m_w->mol_gdis->setDisabled(lock);
    m_w->curr_mol->setDisabled(lock);
    m_w->mol_gulp->setDisabled(lock);
    m_w->mol_apply_button->setDisabled(lock);
    m_w->substrate_model->setDisabled(lock);
    m_w->substrate_model_button->setDisabled(lock);
    m_w->reconstruct->setDisabled(lock);
    m_w->thicknessS->setDisabled(lock);
    m_w->thicknessB->setDisabled(lock);
    m_w->StoichiometryStart->setDisabled(lock);
    m_w->E_AB->setDisabled(lock);
    m_w->Mu_A->setDisabled(lock);
    m_w->Mu_B->setDisabled(lock);
  };

  if (!showSpecific)
  {
    lockSpecific(true);
  } else
  {
    lockSpecific(false); /* unlock all first */
    /* Re-apply MOL-specific enable/disable after unlocking */
    if (m_w->calctype_mol->isChecked())
    {
      on_mol_model_changed(m_w->mol_model->currentIndex());
    } else
    {
      m_w->num_mol->setDisabled(true);
      m_w->mol_model_button->setDisabled(true);
      m_w->mol_gdis->setDisabled(true);
      m_w->curr_mol->setDisabled(true);
      m_w->mol_gulp->setDisabled(true);
      m_w->mol_apply_button->setDisabled(true);
    }
    /* Re-apply CI/DI-dependent states after unlocking */
    on_cidi_method_changed(m_w->optMethodCIDI->currentIndex());
    /* Kconstant: disabled unless VCNEB with Var_Spring checked */
    m_w->Kconstant->setDisabled(m_w->vcnebtype_spring->isChecked());
  } /* end else */

  /* Enforce VCNEB/CI-DI dependent states regardless of showSpecific */
  {
    /* Lock VCNEB fields when method is not VCNEB */
    bool isVcneb = (index == 2);
    m_w->vcnebtype_method->setDisabled(!isVcneb);
    m_w->vcnebType->setDisabled(!isVcneb);
    m_w->vcnebtype_img_num->setDisabled(!isVcneb);
    m_w->vcnebtype_spring->setDisabled(!isVcneb);
    m_w->optReadImages->setDisabled(!isVcneb);
    m_w->numImages->setDisabled(!isVcneb);
    m_w->numSteps->setDisabled(!isVcneb);
    m_w->optFreezing->setDisabled(!isVcneb);
    m_w->optimizerType->setDisabled(!isVcneb);
    m_w->dt->setDisabled(!isVcneb);
    m_w->ConvThreshold->setDisabled(!isVcneb);
    m_w->VarPathLength->setDisabled(!isVcneb);
    m_w->optRelaxType->setDisabled(!isVcneb);

    bool springChecked = isVcneb ? m_w->vcnebtype_spring->isChecked() : false;
    m_w->K_min->setDisabled(!isVcneb || !springChecked);
    m_w->K_max->setDisabled(!isVcneb || !springChecked);
    m_w->Kconstant->setDisabled(!isVcneb || springChecked);

    int cidiIdx = isVcneb ? m_w->optMethodCIDI->currentIndex() : 0;
    m_w->optMethodCIDI->setDisabled(!isVcneb);
    m_w->startCIDIStep->setDisabled(!isVcneb || cidiIdx == 0);
    m_w->pickupImages->setDisabled(!isVcneb || cidiIdx == 0);
    m_w->PrintStep->setDisabled(!isVcneb);
    m_w->FormatType->setDisabled(!isVcneb);
  }

  /* Re-apply VCNEB-dependent states only when method IS VCNEB */
  if (index == 2)
  {
    on_vcnebtype_changed();
  }

  if (index == 1)
  { /* META */
    /* Unlock Metadynamics fields */
    m_w->FullRelax->setDisabled(false);
    m_w->maxVectorLength->setDisabled(false);
    m_w->GaussianWidth->setDisabled(false);
    m_w->GaussianHeight->setDisabled(false);
    m_w->meta_model->setDisabled(false);
    m_w->meta_model_button->setDisabled(false);
    /* FullRelax default: 2 */
    if (uspex_gui.calc.FullRelax == 0)
      uspex_gui.calc.FullRelax = 2;
    m_w->FullRelax->setCurrentIndex(uspex_gui.calc.FullRelax);
    /* Populate meta_model combo */
    m_w->meta_model->blockSignals(true);
    {
      while (m_w->meta_model->count() > 0)
        m_w->meta_model->removeItem(0);
      m_w->meta_model->addItem(tr("From VASP5 POSCAR file"));
      int idx = 1;
      for (GSList *l = sysenv.mal; l; l = g_slist_next(l))
      {
        struct model_pak *data = (struct model_pak *) l->data;
        if (g_slist_length(data->cores) > 0)
        {
          QString name = data->basename ? QString::fromUtf8(data->basename) : QString::number(idx);
          m_w->meta_model->addItem(QString("%1: %2").arg(idx).arg(name));
          idx++;
        }
      }
    }
    m_w->meta_model->blockSignals(false);
    /* Lock PSO and VCNEB fields */
    m_w->PSO_softMut->setDisabled(true);
    m_w->PSO_BestStruc->setDisabled(true);
    m_w->PSO_BestEver->setDisabled(true);
    m_w->vcnebtype_method->setDisabled(true);
    m_w->vcnebType->setDisabled(true);
    m_w->vcnebtype_img_num->setDisabled(true);
    m_w->vcnebtype_spring->setDisabled(true);
    m_w->optReadImages->setDisabled(true);
    m_w->numImages->setDisabled(true);
    m_w->numSteps->setDisabled(true);
    m_w->optFreezing->setDisabled(true);
    m_w->optimizerType->setDisabled(true);
    m_w->dt->setDisabled(true);
    m_w->ConvThreshold->setDisabled(true);
    m_w->VarPathLength->setDisabled(true);
    m_w->optRelaxType->setDisabled(true);
    m_w->K_min->setDisabled(true);
    m_w->K_max->setDisabled(true);
    m_w->Kconstant->setDisabled(true);
    m_w->optMethodCIDI->setDisabled(true);
    m_w->startCIDIStep->setDisabled(true);
    m_w->pickupImages->setDisabled(true);
    m_w->PrintStep->setDisabled(true);
    m_w->FormatType->setDisabled(true);
    m_w->img_model->setDisabled(true);
    m_w->img_model_button->setDisabled(true);
    m_w->mol_model->setDisabled(true);
    m_w->num_mol->setDisabled(true);
    m_w->mol_model_button->setDisabled(true);
    m_w->mol_gdis->setDisabled(true);
    m_w->curr_mol->setDisabled(true);
    m_w->mol_gulp->setDisabled(true);
    m_w->mol_apply_button->setDisabled(true);
    m_w->substrate_model->setDisabled(true);
    m_w->substrate_model_button->setDisabled(true);
    m_w->reconstruct->setDisabled(true);
    m_w->thicknessS->setDisabled(true);
    m_w->thicknessB->setDisabled(true);
    m_w->StoichiometryStart->setDisabled(true);
    m_w->E_AB->setDisabled(true);
    m_w->Mu_A->setDisabled(true);
    m_w->Mu_B->setDisabled(true);
  } else if (index == 2)
  { /* VCNEB */
    m_w->vcnebtype_method->setDisabled(false);
    m_w->vcnebType->setDisabled(false);
    m_w->vcnebtype_img_num->setDisabled(false);
    m_w->vcnebtype_spring->setDisabled(false);
    m_w->optReadImages->setDisabled(false);
    m_w->numImages->setDisabled(false);
    m_w->numSteps->setDisabled(false);
    m_w->optFreezing->setDisabled(false);
    m_w->optimizerType->setDisabled(false);
    m_w->dt->setDisabled(false);
    m_w->ConvThreshold->setDisabled(false);
    m_w->VarPathLength->setDisabled(false);
    m_w->optRelaxType->setDisabled(false);
    m_w->K_min->setDisabled(false);
    m_w->K_max->setDisabled(false);
    m_w->Kconstant->setDisabled(false);
    m_w->optMethodCIDI->setDisabled(false);
    m_w->startCIDIStep->setDisabled(false);
    m_w->pickupImages->setDisabled(false);
    m_w->PrintStep->setDisabled(false);
    m_w->FormatType->setDisabled(false);
    m_w->img_model->setDisabled(false);
    m_w->img_model_button->setDisabled(false);
    m_w->FullRelax->setDisabled(true);
    m_w->maxVectorLength->setDisabled(true);
    m_w->GaussianWidth->setDisabled(true);
    m_w->GaussianHeight->setDisabled(true);
    m_w->meta_model->setDisabled(true);
    m_w->meta_model_button->setDisabled(true);
    m_w->PSO_softMut->setDisabled(true);
    m_w->PSO_BestStruc->setDisabled(true);
    m_w->PSO_BestEver->setDisabled(true);
    m_w->mol_model->setDisabled(true);
    m_w->num_mol->setDisabled(true);
    m_w->mol_model_button->setDisabled(true);
    m_w->mol_gdis->setDisabled(true);
    m_w->curr_mol->setDisabled(true);
    m_w->mol_gulp->setDisabled(true);
    m_w->mol_apply_button->setDisabled(true);
    m_w->substrate_model->setDisabled(true);
    m_w->substrate_model_button->setDisabled(true);
    m_w->reconstruct->setDisabled(true);
    m_w->thicknessS->setDisabled(true);
    m_w->thicknessB->setDisabled(true);
    m_w->StoichiometryStart->setDisabled(true);
    m_w->E_AB->setDisabled(true);
    m_w->Mu_A->setDisabled(true);
    m_w->Mu_B->setDisabled(true);
  } else if (index == 3)
  { /* PSO */
    m_w->PSO_softMut->setDisabled(false);
    m_w->PSO_BestStruc->setDisabled(false);
    m_w->PSO_BestEver->setDisabled(false);
    m_w->FullRelax->setDisabled(true);
    m_w->maxVectorLength->setDisabled(true);
    m_w->GaussianWidth->setDisabled(true);
    m_w->GaussianHeight->setDisabled(true);
    m_w->meta_model->setDisabled(true);
    m_w->meta_model_button->setDisabled(true);
    m_w->vcnebtype_method->setDisabled(true);
    m_w->vcnebType->setDisabled(true);
    m_w->vcnebtype_img_num->setDisabled(true);
    m_w->vcnebtype_spring->setDisabled(true);
    m_w->optReadImages->setDisabled(true);
    m_w->numImages->setDisabled(true);
    m_w->numSteps->setDisabled(true);
    m_w->optFreezing->setDisabled(true);
    m_w->optimizerType->setDisabled(true);
    m_w->dt->setDisabled(true);
    m_w->ConvThreshold->setDisabled(true);
    m_w->VarPathLength->setDisabled(true);
    m_w->optRelaxType->setDisabled(true);
    m_w->K_min->setDisabled(true);
    m_w->K_max->setDisabled(true);
    m_w->Kconstant->setDisabled(true);
    m_w->optMethodCIDI->setDisabled(true);
    m_w->startCIDIStep->setDisabled(true);
    m_w->pickupImages->setDisabled(true);
    m_w->PrintStep->setDisabled(true);
    m_w->FormatType->setDisabled(true);
    m_w->img_model->setDisabled(true);
    m_w->img_model_button->setDisabled(true);
    /* MOL frame widgets are handled by on_type_changed, not here */
    m_w->substrate_model->setDisabled(true);
    m_w->substrate_model_button->setDisabled(true);
    m_w->reconstruct->setDisabled(true);
    m_w->thicknessS->setDisabled(true);
    m_w->thicknessB->setDisabled(true);
    m_w->StoichiometryStart->setDisabled(true);
    m_w->E_AB->setDisabled(true);
    m_w->Mu_A->setDisabled(true);
    m_w->Mu_B->setDisabled(true);
  } else
  { /* index == 0: USPEX — lock all SPECIFIC tab widgets */
    m_w->FullRelax->setDisabled(true);
    m_w->maxVectorLength->setDisabled(true);
    m_w->GaussianWidth->setDisabled(true);
    m_w->GaussianHeight->setDisabled(true);
    m_w->meta_model->setDisabled(true);
    m_w->meta_model_button->setDisabled(true);
    m_w->PSO_softMut->setDisabled(true);
    m_w->PSO_BestStruc->setDisabled(true);
    m_w->PSO_BestEver->setDisabled(true);
    m_w->vcnebtype_method->setDisabled(true);
    m_w->vcnebType->setDisabled(true);
    m_w->vcnebtype_img_num->setDisabled(true);
    m_w->vcnebtype_spring->setDisabled(true);
    m_w->optReadImages->setDisabled(true);
    m_w->numImages->setDisabled(true);
    m_w->numSteps->setDisabled(true);
    m_w->optFreezing->setDisabled(true);
    m_w->optimizerType->setDisabled(true);
    m_w->dt->setDisabled(true);
    m_w->ConvThreshold->setDisabled(true);
    m_w->VarPathLength->setDisabled(true);
    m_w->optRelaxType->setDisabled(true);
    m_w->K_min->setDisabled(true);
    m_w->K_max->setDisabled(true);
    m_w->Kconstant->setDisabled(true);
    m_w->optMethodCIDI->setDisabled(true);
    m_w->startCIDIStep->setDisabled(true);
    m_w->pickupImages->setDisabled(true);
    m_w->PrintStep->setDisabled(true);
    m_w->FormatType->setDisabled(true);
    m_w->img_model->setDisabled(true);
    m_w->img_model_button->setDisabled(true);
    /* MOL/Surfaces frame widgets handled by on_type_changed */
    /* substrate_model, substrate_model_button, reconstruct, thicknessS, thicknessB, StoichiometryStart, E_AB, Mu_A,
     * Mu_B */
    /* Lock MOL frame if current type is NOT molecular */
    {
      int dim2;
      bool mag2, mol2, var2, nd2;
      parse_calculationType((int) uspex_gui.calc.calculationType, dim2, mag2, mol2, var2, nd2);
      if (!mol2)
      {
        m_w->mol_model->setDisabled(true);
        m_w->num_mol->setDisabled(true);
        m_w->mol_model_button->setDisabled(true);
        m_w->mol_gdis->setDisabled(true);
        m_w->curr_mol->setDisabled(true);
        m_w->mol_gulp->setDisabled(true);
        m_w->mol_apply_button->setDisabled(true);
      }
    }
  } /* end else (method-specific) */

  /* Apply initial disabled states based on VAR, auto flags */
  bool varChecked = m_w->calctype_var->isChecked();
  m_w->numSpecies->setDisabled(!varChecked);
  m_w->blockSpecies->setDisabled(!varChecked);
  m_w->Species_apply_button->setDisabled(!varChecked);
  m_w->Species_delete_button->setDisabled(!varChecked);

  /* Apply auto flag locking directly */
  bool ab = m_w->auto_bonds->isChecked();
  m_w->goodBonds->setDisabled(ab);
  m_w->bond_d->setDisabled(ab);
  m_w->apply_bonds->setDisabled(ab);
  m_w->remove_bonds->setDisabled(ab);

  bool acl = m_w->auto_C_lat->isChecked();
  m_w->Latticevalues->setDisabled(acl);
  m_w->latticevalue->setDisabled(acl);
  m_w->latticeformat->setDisabled(acl);

  bool aci = m_w->auto_C_ion->isChecked();
  m_w->IonDistances->setDisabled(aci);
  m_w->distances->setDisabled(aci);
  m_w->apply_distances->setDisabled(aci);

  bool ast = m_w->auto_step->isChecked();
  m_w->ai_input->setDisabled(ast);
  m_w->ai_input_button->setDisabled(ast);
  m_w->ai_opt->setDisabled(ast);
  m_w->ai_opt_button->setDisabled(ast);

  /* Ver 10.4 / NEW toggle effects */
  bool v104 = m_w->sel_v1030_1->isChecked();
  m_w->sel_octave->setDisabled(v104);
  m_w->sel_new_opt->setChecked(v104);
  m_w->sel_new_opt->setDisabled(v104);
  m_w->new_optType->setDisabled(!v104);
  m_w->optType->setDisabled(v104);

  /* TPS (index 4) — enable TPS-specific fields */
  if (index == 4)
  {
    m_w->numIterations->setDisabled(false);
    m_w->speciesSymbol->setDisabled(false);
    m_w->mass->setDisabled(false);
    m_w->amplitudeShoot_AB->setDisabled(false);
    m_w->amplitudeShoot_BA->setDisabled(false);
    m_w->magnitudeShoot_success->setDisabled(false);
    m_w->magnitudeShoot_failure->setDisabled(false);
    m_w->shiftRatio->setDisabled(false);
    m_w->orderParaType->setDisabled(false);
    m_w->opCriteria_start->setDisabled(false);
    m_w->opCriteria_end->setDisabled(false);
    m_w->cmdOrderParameter->setDisabled(false);
    m_w->cmdOrderParameter_button->setDisabled(false);
    m_w->cmdEnthalpyTemperature->setDisabled(false);
    m_w->cmdEnthalpyTemperature_button->setDisabled(false);
    m_w->orderParameterFile->setDisabled(false);
    m_w->orderParameterFile_button->setDisabled(false);
    m_w->enthalpyTemperatureFile->setDisabled(false);
    m_w->enthalpyTemperatureFile_button->setDisabled(false);
    m_w->trajectoryFile->setDisabled(false);
    m_w->trajectoryFile_button->setDisabled(false);
    m_w->MDrestartFile->setDisabled(false);
    m_w->MDrestartFile_button->setDisabled(false);
  } else
  {
    /* Lock all TPS fields */
    m_w->numIterations->setDisabled(true);
    m_w->speciesSymbol->setDisabled(true);
    m_w->mass->setDisabled(true);
    m_w->amplitudeShoot_AB->setDisabled(true);
    m_w->amplitudeShoot_BA->setDisabled(true);
    m_w->magnitudeShoot_success->setDisabled(true);
    m_w->magnitudeShoot_failure->setDisabled(true);
    m_w->shiftRatio->setDisabled(true);
    m_w->orderParaType->setDisabled(true);
    m_w->opCriteria_start->setDisabled(true);
    m_w->opCriteria_end->setDisabled(true);
    m_w->cmdOrderParameter->setDisabled(true);
    m_w->cmdOrderParameter_button->setDisabled(true);
    m_w->cmdEnthalpyTemperature->setDisabled(true);
    m_w->cmdEnthalpyTemperature_button->setDisabled(true);
    m_w->orderParameterFile->setDisabled(true);
    m_w->orderParameterFile_button->setDisabled(true);
    m_w->enthalpyTemperatureFile->setDisabled(true);
    m_w->enthalpyTemperatureFile_button->setDisabled(true);
    m_w->trajectoryFile->setDisabled(true);
    m_w->trajectoryFile_button->setDisabled(true);
    m_w->MDrestartFile->setDisabled(true);
    m_w->MDrestartFile_button->setDisabled(true);
  }

  /* BoltzTraP — enable when thermoelectric optimization requested */
  update_boltztrap_visibility();

  /* FORCE VCNEB/CI-DI dependent states — always, regardless of method */
  m_w->K_min->setDisabled(true);
  m_w->K_max->setDisabled(true);
  m_w->Kconstant->setDisabled(true);
  m_w->startCIDIStep->setDisabled(true);
  m_w->pickupImages->setDisabled(true);
}

void UspexDialog::on_dim_changed(int value)
{
  if (value == -1)
  {
    int curCtype = combo_item_to_ctype(m_w->calculationType->currentText());
    int d;
    bool mg, ml, vr, nd;
    parse_calculationType(curCtype, d, mg, ml, vr, nd);
    if (d == 0)
      value = -2;
    else
      value = 0;
  }

  bool mag = m_w->calctype_mag->isChecked();
  bool mol = m_w->calctype_mol->isChecked();
  bool var = m_w->calctype_var->isChecked();
  bool negDim2 = (value == -2);
  int ctype = compute_calculationType(value, mag, mol, var, negDim2);

  /* If computed ctype doesn't exist in combo, revert DIM to what
   * the combo currently shows (guaranteed valid). */
  if (!set_calculationType_combo(m_w->calculationType, ctype))
  {
    int curCtype = combo_item_to_ctype(m_w->calculationType->currentText());
    int dim;
    bool mg2, ml2, vr2, nd2;
    parse_calculationType(curCtype, dim, mg2, ml2, vr2, nd2);
    m_w->calctype_dim->blockSignals(true);
    m_w->calctype_dim->setValue(dim);
    m_w->calctype_dim->blockSignals(false);
    return;
  }

  uspex_gui._dim = value;

  uspex_gui.calc.calculationType = (uspex_type) ctype;
  on_specifictab_visibility();

  /* Update SPECIFIC tab visibility */
  int methodIdx = m_w->calculationMethod->currentIndex();
  m_notebook->setTabVisible(4, should_show_specific(methodIdx, uspex_gui.calc.calculationType));
}

void UspexDialog::on_mag_toggled(bool checked)
{
  m_w->calctype_mag_2->blockSignals(true);
  m_w->calctype_mag_2->setChecked(checked);
  m_w->calctype_mag_2->blockSignals(false);

  uspex_gui.calc._calctype_mag = checked;

  int dim;
  bool mag, mol, var, negDim2;
  parse_calculationType((int) uspex_gui.calc.calculationType, dim, mag, mol, var, negDim2);
  int ctype = compute_calculationType(dim, checked, mol, var, negDim2);

  /* Validate: if ctype doesn't exist, revert MAG toggle */
  if (!set_calculationType_combo(m_w->calculationType, ctype))
  {
    m_w->calctype_mag->blockSignals(true);
    m_w->calctype_mag->setChecked(!checked);
    m_w->calctype_mag->blockSignals(false);
    m_w->calctype_mag_2->blockSignals(true);
    m_w->calctype_mag_2->setChecked(!checked);
    m_w->calctype_mag_2->blockSignals(false);
    return;
  }

  uspex_gui.calc._calctype_mag = checked;

  m_w->mag_nm->setDisabled(!checked);
  m_w->mag_fmls->setDisabled(!checked);
  m_w->mag_afml->setDisabled(!checked);
  m_w->mag_fmlh->setDisabled(!checked);
  m_w->mag_fmhs->setDisabled(!checked);
  m_w->mag_afmh->setDisabled(!checked);
  m_w->mag_aflh->setDisabled(!checked);
  m_w->fracSpinMut->setDisabled(!checked);

  on_specifictab_visibility();
}

void UspexDialog::on_mol_toggled(bool checked)
{
  int dim;
  bool mag, mol, var, negDim2;
  parse_calculationType((int) uspex_gui.calc.calculationType, dim, mag, mol, var, negDim2);
  int ctype = compute_calculationType(dim, mag, checked, var, negDim2);

  /* Validate: if ctype doesn't exist, revert MOL toggle */
  if (!set_calculationType_combo(m_w->calculationType, ctype))
  {
    m_w->calctype_mol->blockSignals(true);
    m_w->calctype_mol->setChecked(!checked);
    m_w->calctype_mol->blockSignals(false);
    return;
  }

  uspex_gui.calc._calctype_mol = checked;

  m_w->checkMolecules->setDisabled(!checked);
  m_w->checkConnectivity->setDisabled(!checked);
  m_w->fracRotMut->setDisabled(!checked);
  m_w->MolCenters->setDisabled(!checked);
  m_w->centers->setDisabled(!checked);
  m_w->centers_button->setDisabled(!checked);
  /* Enable/disable entire Molecules frame widgets */
  if (m_w->molFrame)
  {
    if (checked)
    {
      /* MOL turned on — enable mol_model combo so user can select */
      m_w->mol_model->setDisabled(false);
      /* Apply per-combo enable/disable logic */
      on_mol_model_changed(m_w->mol_model->currentIndex());
    } else
    {
      /* MOL turned off — disable all Molecules frame widgets */
      m_w->num_mol->setDisabled(true);
      m_w->mol_model_button->setDisabled(true);
      m_w->mol_gdis->setDisabled(true);
      m_w->curr_mol->setDisabled(true);
      m_w->mol_gulp->setDisabled(true);
      m_w->mol_apply_button->setDisabled(true);
    }
  }

  on_specifictab_visibility();

  /* Update SPECIFIC tab visibility */
  int methodIdx = m_w->calculationMethod->currentIndex();
  m_notebook->setTabVisible(4, should_show_specific(methodIdx, uspex_gui.calc.calculationType));
}

void UspexDialog::on_var_toggled(bool checked)
{
  int dim;
  bool mag, mol, var, negDim2;
  parse_calculationType((int) uspex_gui.calc.calculationType, dim, mag, mol, var, negDim2);
  int ctype = compute_calculationType(dim, mag, mol, checked, negDim2);

  /* Validate: if ctype doesn't exist, revert VAR toggle */
  if (!set_calculationType_combo(m_w->calculationType, ctype))
  {
    m_w->calctype_var->blockSignals(true);
    m_w->calctype_var->setChecked(!checked);
    m_w->calctype_var->blockSignals(false);
    return;
  }

  uspex_gui.calc._calctype_var = checked;

  m_w->numSpecies->setDisabled(!checked);
  m_w->blockSpecies->setDisabled(!checked);
  m_w->Species_apply_button->setDisabled(!checked);
  m_w->Species_delete_button->setDisabled(!checked);
  m_w->firstGeneMax->setDisabled(!checked);
  m_w->minAt->setDisabled(!checked);
  m_w->maxAt->setDisabled(!checked);
  m_w->fracTrans->setDisabled(!checked);
  m_w->howManyTrans->setDisabled(!checked);
  m_w->specificTrans->setDisabled(!checked);

  on_specifictab_visibility();
}

void UspexDialog::on_type_changed(int index)
{
  /* Parse the selected type text to get the ctype value */
  int ctype = combo_item_to_ctype(m_w->calculationType->itemText(index));

  /* Extract dim/mag/mol/var from ctype */
  int dim;
  bool mag, mol, var, negDim2;
  parse_calculationType(ctype, dim, mag, mol, var, negDim2);

  /* Sync DIM spinner */
  m_w->calctype_dim->blockSignals(true);
  m_w->calctype_dim->setValue(dim);
  m_w->calctype_dim->blockSignals(false);
  uspex_gui._dim = dim;

  /* Sync MAG toggle */
  m_w->calctype_mag->blockSignals(true);
  m_w->calctype_mag->setChecked(mag);
  m_w->calctype_mag->blockSignals(false);
  m_w->calctype_mag_2->blockSignals(true);
  m_w->calctype_mag_2->setChecked(mag);
  m_w->calctype_mag_2->blockSignals(false);
  uspex_gui.calc._calctype_mag = mag;

  /* Sync MOL toggle */
  m_w->calctype_mol->blockSignals(true);
  m_w->calctype_mol->setChecked(mol);
  m_w->calctype_mol->blockSignals(false);
  uspex_gui.calc._calctype_mol = mol;

  /* Sync VAR toggle */
  m_w->calctype_var->blockSignals(true);
  m_w->calctype_var->setChecked(var);
  m_w->calctype_var->blockSignals(false);
  uspex_gui.calc._calctype_var = var;

  /* Update stored ctype */
  uspex_gui.calc.calculationType = (uspex_type) ctype;

  /* Update field visibility based on flags */
  m_w->mag_nm->setDisabled(!mag);
  m_w->mag_fmls->setDisabled(!mag);
  m_w->mag_afml->setDisabled(!mag);
  m_w->mag_fmlh->setDisabled(!mag);
  m_w->mag_fmhs->setDisabled(!mag);
  m_w->mag_afmh->setDisabled(!mag);
  m_w->mag_aflh->setDisabled(!mag);
  m_w->fracSpinMut->setDisabled(!mag);

  m_w->checkMolecules->setDisabled(!mol);
  m_w->checkConnectivity->setDisabled(!mol);
  m_w->fracRotMut->setDisabled(!mol);
  m_w->MolCenters->setDisabled(!mol);
  m_w->centers->setDisabled(!mol);
  m_w->centers_button->setDisabled(!mol);

  /* Enable/disable Molecules frame widgets */
  if (mol)
  {
    m_w->mol_model->setDisabled(false);
    on_mol_model_changed(m_w->mol_model->currentIndex());
  } else
  {
    m_w->mol_model->setDisabled(true);
    m_w->num_mol->setDisabled(true);
    m_w->mol_model_button->setDisabled(true);
    m_w->mol_gdis->setDisabled(true);
    m_w->curr_mol->setDisabled(true);
    m_w->mol_gulp->setDisabled(true);
    m_w->mol_apply_button->setDisabled(true);
  }

  m_w->numSpecies->setDisabled(!var);
  m_w->blockSpecies->setDisabled(!var);
  m_w->Species_apply_button->setDisabled(!var);
  m_w->Species_delete_button->setDisabled(!var);
  m_w->firstGeneMax->setDisabled(!var);
  m_w->minAt->setDisabled(!var);
  m_w->maxAt->setDisabled(!var);
  m_w->fracTrans->setDisabled(!var);
  m_w->howManyTrans->setDisabled(!var);
  m_w->specificTrans->setDisabled(!var);

  on_specifictab_visibility();
  update_numSpecies_combo();

  /* Update SPECIFIC tab visibility */
  int methodIdx = m_w->calculationMethod->currentIndex();
  m_notebook->setTabVisible(4, should_show_specific(methodIdx, ctype));

  /* Lock VCNEB fields when method is not VCNEB */
  if (methodIdx != 2)
  {
    m_w->Kconstant->setDisabled(true);
    m_w->VarPathLength->setDisabled(true);
  }
}

void UspexDialog::on_atomType_changed(int index)
{
  /* Parse selected atomType to fill @Sym/@Z/@Num/@Val fields */
  QString text = m_w->atomType->itemText(index);
  if (text == tr("ADD ATOMTYPE"))
  {
    /* Reset to defaults */
    m_w->atom_sym->setText(uspex_gui._tmp_atom_sym);
    m_w->atom_typ->setText(QString::number(uspex_gui._tmp_atom_typ));
    m_w->atom_num->setText(QString::number(uspex_gui._tmp_atom_num));
    m_w->atom_val->setText(QString::number(uspex_gui._tmp_atom_val));
    return;
  }
  /* Parse "Symbol(num) (V=valence)" */
  int num, val;
  QRegularExpression re(QStringLiteral("^([A-Za-z]+)\\((\\d+)\\) \\(V=(\\d+)\\)$"));
  QRegularExpressionMatch match = re.match(text);
  if (match.hasMatch())
  {
    QString sym = match.captured(1);
    num = match.captured(2).toInt();
    val = match.captured(3).toInt();
    int typ = elem_symbol_test(sym.toUtf8().constData());
    g_strlcpy(uspex_gui._tmp_atom_sym, sym.toUtf8().constData(), 3);
    uspex_gui._tmp_atom_sym[2] = '\0';
    uspex_gui._tmp_atom_typ = typ;
    uspex_gui._tmp_atom_num = num;
    uspex_gui._tmp_atom_val = val;
    m_w->atom_sym->setText(sym);
    m_w->atom_typ->setText(QString::number(typ));
    m_w->atom_num->setText(QString::number(num));
    m_w->atom_val->setText(QString::number(val));
  }
}

void UspexDialog::on_apply_atom()
{
  int index = m_w->atomType->currentIndex();
  QString text = m_w->atomType->itemText(index);

  /* Read @Sym, @Num, @Val from fields */
  QString sym = m_w->atom_sym->text();
  bool ok;
  int num = m_w->atom_num->text().toInt(&ok);
  if (!ok)
    num = 0;
  int val = m_w->atom_val->text().toInt(&ok);
  if (!ok)
    val = 0;
  int typ = elem_symbol_test(sym.toUtf8().constData());

  if (text == tr("ADD ATOMTYPE"))
  {
    /* Check for duplicates */
    for (int i = 0; i < index; i++)
    {
      QString item = m_w->atomType->itemText(i);
      QRegularExpression re(QStringLiteral("^([A-Za-z]+)\\("));
      QRegularExpressionMatch m = re.match(item);
      if (m.hasMatch())
      {
        int existingTyp = elem_symbol_test(m.captured(1).toUtf8().constData());
        if (existingTyp == typ)
        {
          /* Duplicate — just select it */
          m_w->atomType->setCurrentIndex(i);
          return;
        }
      }
    }
    /* Add new atomType */
    QString newItem = QString("%1(%2) (V=%3)").arg(sym).arg(num).arg(val);
    m_w->atomType->insertItem(index, newItem);
    m_w->atomType->setCurrentIndex(index);

    /* Update calc.atomType and calc.numSpecies */
    gint *newAtomType = (gint *) g_malloc((uspex_gui.calc._nspecies + 1) * sizeof(gint));
    gint *newNumSpecies = (gint *) g_malloc((uspex_gui.calc._nspecies + 1) * sizeof(gint));
    for (int i = 0; i < index; i++)
    {
      QString item = m_w->atomType->itemText(i);
      QRegularExpression re(QStringLiteral("^([A-Za-z]+)\\((\\d+)\\)"));
      QRegularExpressionMatch m = re.match(item);
      if (m.hasMatch())
      {
        newAtomType[i] = elem_symbol_test(m.captured(1).toUtf8().constData());
        newNumSpecies[i] = m.captured(2).toInt();
      }
    }
    newAtomType[index] = typ;
    newNumSpecies[index] = num;
    for (int i = index + 1; i < uspex_gui.calc._nspecies; i++)
    {
      QString item = m_w->atomType->itemText(i + 1);
      QRegularExpression re(QStringLiteral("^([A-Za-z]+)\\((\\d+)\\)"));
      QRegularExpressionMatch m = re.match(item);
      if (m.hasMatch())
      {
        newAtomType[i + 1] = elem_symbol_test(m.captured(1).toUtf8().constData());
        newNumSpecies[i + 1] = m.captured(2).toInt();
      }
    }
    g_free(uspex_gui.calc.atomType);
    uspex_gui.calc.atomType = newAtomType;
    if (!uspex_gui.calc._calctype_var)
    {
      g_free(uspex_gui.calc.numSpecies);
      uspex_gui.calc.numSpecies = newNumSpecies;
    } else
    {
      g_free(newNumSpecies);
    }
    uspex_gui.calc._nspecies++;

    /* Update numSpecies combo */
    update_numSpecies_combo();
  } else
  {
    /* Modify existing atomType */
    QString newItem = QString("%1(%2) (V=%3)").arg(sym).arg(num).arg(val);
    m_w->atomType->setItemText(index, newItem);

    /* Update calc arrays */
    QRegularExpression re(QStringLiteral("^([A-Za-z]+)\\((\\d+)\\)"));
    QRegularExpressionMatch m = re.match(text);
    if (m.hasMatch())
    {
      /* Check for duplicates at other indices */
      for (int i = 0; i < uspex_gui.calc._nspecies; i++)
      {
        if (i == index)
          continue;
        QString item = m_w->atomType->itemText(i);
        QRegularExpressionMatch m2 = re.match(item);
        if (m2.hasMatch())
        {
          int existingTyp = elem_symbol_test(m2.captured(1).toUtf8().constData());
          if (existingTyp == typ)
          {
            /* Restore old text */
            m_w->atomType->setItemText(index, text);
            return;
          }
        }
      }
    }

    uspex_gui.calc.atomType[index] = typ;
    if (!uspex_gui.calc._calctype_var)
    {
      uspex_gui.calc.numSpecies[index] = num;
      update_numSpecies_combo();
    }
  }
}

void UspexDialog::on_remove_atom()
{
  int index = m_w->atomType->currentIndex();
  QString text = m_w->atomType->itemText(index);

  if (text == tr("ADD ATOMTYPE"))
    return; /* Can't delete */
  if (uspex_gui.calc._nspecies <= 1)
    return; /* Can't delete last */

  m_w->atomType->blockSignals(true);
  m_w->atomType->removeItem(index);
  m_w->atomType->blockSignals(false);

  /* Rebuild calc arrays */
  gint *newAtomType = (gint *) g_malloc((uspex_gui.calc._nspecies - 1) * sizeof(gint));
  gint *newNumSpecies = (gint *) g_malloc((uspex_gui.calc._nspecies - 1) * sizeof(gint));
  int idx = 0;
  for (int i = 0; i < m_w->atomType->count(); i++)
  {
    QString item = m_w->atomType->itemText(i);
    QRegularExpression re(QStringLiteral("^([A-Za-z]+)\\((\\d+)\\)"));
    QRegularExpressionMatch m = re.match(item);
    if (m.hasMatch())
    {
      newAtomType[idx] = elem_symbol_test(m.captured(1).toUtf8().constData());
      newNumSpecies[idx] = m.captured(2).toInt();
      idx++;
    }
  }
  g_free(uspex_gui.calc.atomType);
  uspex_gui.calc.atomType = newAtomType;
  if (!uspex_gui.calc._calctype_var)
  {
    g_free(uspex_gui.calc.numSpecies);
    uspex_gui.calc.numSpecies = newNumSpecies;
  } else
  {
    g_free(newNumSpecies);
  }
  uspex_gui.calc._nspecies--;

  /* Select previous or first */
  int selIdx = (index > 0) ? index - 1 : 0;
  m_w->atomType->setCurrentIndex(selIdx);

  update_numSpecies_combo();
}

void UspexDialog::update_numSpecies_combo()
{
  if (!uspex_gui.calc.numSpecies || !ptr_valid(uspex_gui.calc.numSpecies))
    return;

  m_w->numSpecies->blockSignals(true);
  m_w->numSpecies->clear();

  int numItems = uspex_gui.calc._calctype_mol ? uspex_gui.calc._var_nspecies : uspex_gui.calc._nspecies;
  QString text = QString::number(uspex_gui.calc.numSpecies[0]);
  for (int i = 1; i < numItems; i++)
  {
    text += " " + QString::number(uspex_gui.calc.numSpecies[i]);
  }
  m_w->numSpecies->addItem(text);
  m_w->numSpecies->addItem(tr("ADD SPECIES BLOCK"));
  m_w->numSpecies->blockSignals(false);
}

void UspexDialog::on_specifictab_visibility()
{
  int dim;
  bool mag, mol, var, negDim2;
  parse_calculationType((int) uspex_gui.calc.calculationType, dim, mag, mol, var, negDim2);

  if (dim == 3)
  {
    m_w->PhaseDiagram->setDisabled(false);
  } else
  {
    m_w->PhaseDiagram->setDisabled(true);
  }

  if (dim == 2)
  {
    m_w->thicknessS->setDisabled(false);
    m_w->thicknessB->setDisabled(false);
    m_w->reconstruct->setDisabled(false);
    m_w->StoichiometryStart->setDisabled(false);
    m_w->E_AB->setDisabled(false);
    m_w->Mu_A->setDisabled(false);
    m_w->Mu_B->setDisabled(false);
    m_w->substrate_model->setDisabled(false);
    m_w->substrate_model_button->setDisabled(false);
  } else
  {
    m_w->thicknessS->setDisabled(true);
    m_w->thicknessB->setDisabled(true);
    m_w->reconstruct->setDisabled(true);
    m_w->StoichiometryStart->setDisabled(true);
    m_w->E_AB->setDisabled(true);
    m_w->Mu_A->setDisabled(true);
    m_w->Mu_B->setDisabled(true);
    m_w->substrate_model->setDisabled(true);
    m_w->substrate_model_button->setDisabled(true);
  }

  if (dim == 0)
  {
    m_w->numberparents->setDisabled(false);
  } else
  {
    m_w->numberparents->setDisabled(true);
  }
}

void UspexDialog::on_auto_bonds_toggled(bool checked)
{
  uspex_gui.auto_bonds = checked;
  m_w->goodBonds->setDisabled(checked);
  m_w->bond_d->setDisabled(checked);
  m_w->apply_bonds->setDisabled(checked);
  m_w->remove_bonds->setDisabled(checked);
}

void UspexDialog::on_auto_C_lat_toggled(bool checked)
{
  uspex_gui.auto_C_lat = checked;
  if (checked)
  {
    /* Automatic — lock widgets */
    m_w->Latticevalues->setDisabled(true);
    m_w->latticevalue->setDisabled(true);
    m_w->latticeformat->setDisabled(true);
  } else
  {
    /* Manual — unlock and ensure Latticevalues array exists */
    m_w->Latticevalues->setDisabled(false);
    m_w->latticevalue->setDisabled(false);
    m_w->latticeformat->setDisabled(false);
    if (uspex_gui.calc.Latticevalues == NULL || !ptr_valid(uspex_gui.calc.Latticevalues))
    {
      if (uspex_gui.calc._nlattice_line == 0)
      {
        uspex_gui.calc._nlattice_line = 1;
        uspex_gui.calc._nlattice_vals = 1;
      }
      uspex_gui.calc.Latticevalues =
          (double *) g_malloc0(uspex_gui.calc._nlattice_line * uspex_gui.calc._nlattice_vals * sizeof(gdouble));
    }
  }
}

void UspexDialog::on_auto_C_ion_toggled(bool checked)
{
  uspex_gui.auto_C_ion = checked;
  m_w->IonDistances->setDisabled(checked);
  m_w->distances->setDisabled(checked);
  m_w->apply_distances->setDisabled(checked);
}

void UspexDialog::on_iondistances_selected(int index)
{
  /* Copy selected combo line to DIST field */
  if (index < 0 || index >= m_w->IonDistances->count())
    return;
  m_w->distances->setText(m_w->IonDistances->itemText(index));
}

void UspexDialog::on_apply_distances()
{
  /* Apply modified ion distances back to combo and calc.IonDistances */
  int index = m_w->IonDistances->currentIndex();
  if (index < 0)
    return;

  QString text = m_w->distances->text();
  QString trimmed = text.trimmed();
  if (trimmed.isEmpty())
    return;

  QStringList parts = trimmed.split(' ', Qt::SkipEmptyParts);
  if (parts.isEmpty())
    return;

  /* Ensure IonDistances array exists */
  if (!ptr_valid(uspex_gui.calc.IonDistances))
  {
    g_free(uspex_gui.calc.IonDistances);
    uspex_gui.calc.IonDistances = nullptr;
  }

  int nspecies = uspex_gui.calc._nspecies;
  if (nspecies <= 0)
    nspecies = parts.size();

  /* Realloc if needed */
  int total = nspecies * nspecies;
  if (!ptr_valid(uspex_gui.calc.IonDistances))
  {
    uspex_gui.calc.IonDistances = (double *) g_malloc0(total * sizeof(double));
  }

  /* Update the selected row */
  for (int j = 0; j < parts.size() && j < nspecies; j++)
  {
    bool ok;
    double val = parts[j].toDouble(&ok);
    if (ok)
      uspex_gui.calc.IonDistances[index * nspecies + j] = val;
  }

  /* Update combo line */
  QString row;
  for (int j = 0; j < nspecies; j++)
  {
    if (j > 0)
      row += " ";
    row += QString::number(uspex_gui.calc.IonDistances[index * nspecies + j], 'f', 4);
  }
  m_w->IonDistances->setItemText(index, row);
}

void UspexDialog::on_auto_step_toggled(bool checked)
{
  uspex_gui.auto_step = checked;
  m_w->ai_input->setDisabled(checked);
  m_w->ai_input_button->setDisabled(checked);
  m_w->ai_opt->setDisabled(checked);
  m_w->ai_opt_button->setDisabled(checked);
}

void UspexDialog::on_use_specific_toggled(bool checked)
{
  if (!checked)
    return;
  uspex_gui.have_specific = TRUE;
  m_w->use_specific->setChecked(true);
  m_w->set_specific->setChecked(false);
  /* Enable SPE folder, disable INP/OPT/LIB/EXE */
  m_w->spe_folder->setDisabled(false);
  m_w->spe_folder_button->setDisabled(false);
  m_w->ai_input->setDisabled(true);
  m_w->ai_input_button->setDisabled(true);
  m_w->ai_opt->setDisabled(true);
  m_w->ai_opt_button->setDisabled(true);
  m_w->ai_lib->setDisabled(true);
  m_w->ai_lib_button->setDisabled(true);
  m_w->ai_lib_flavor->setDisabled(true);
  m_w->ai_lib_sel->setDisabled(true);
}

void UspexDialog::on_set_specific_toggled(bool checked)
{
  if (!checked)
    return;
  uspex_gui.have_specific = FALSE;
  m_w->set_specific->setChecked(true);
  m_w->use_specific->setChecked(false);
  /* Disable SPE folder, enable INP/OPT/LIB/EXE */
  m_w->spe_folder->setDisabled(true);
  m_w->spe_folder_button->setDisabled(true);
  m_w->ai_input->setDisabled(false);
  m_w->ai_input_button->setDisabled(false);
  m_w->ai_opt->setDisabled(false);
  m_w->ai_opt_button->setDisabled(false);
  m_w->ai_lib->setDisabled(false);
  m_w->ai_lib_button->setDisabled(false);
  m_w->ai_lib_flavor->setDisabled(false);
  m_w->ai_lib_sel->setDisabled(false);
}

void UspexDialog::on_spe_folder_button_clicked()
{
  QString folder = QFileDialog::getExistingDirectory(this, tr("Select the Specific folder"), QString());
  if (folder.isEmpty())
    return;
  m_w->spe_folder->setText(folder);
}

/* ============================================================
 * Step navigation and AI field handlers
 * ============================================================ */

void UspexDialog::on_num_opt_steps_changed(int value)
{
  /* Resize step arrays */
  gint old_n = uspex_gui.calc._num_opt_steps;
  gint new_n = value;

  /* Resize _isfixed */
  gboolean *new_isfixed = g_new(gboolean, new_n);
  for (gint i = 0; i < qMin(old_n, new_n); i++)
    new_isfixed[i] = uspex_gui.calc._isfixed[i];
  for (gint i = old_n; i < new_n; i++)
    new_isfixed[i] = FALSE;
  g_free(uspex_gui.calc._isfixed);
  uspex_gui.calc._isfixed = new_isfixed;

  /* Resize abinitioCode */
  gint *new_abinitioCode = g_new(gint, new_n);
  for (gint i = 0; i < qMin(old_n, new_n); i++)
    new_abinitioCode[i] = uspex_gui.calc.abinitioCode[i];
  for (gint i = old_n; i < new_n; i++)
    new_abinitioCode[i] = 1;
  g_free(uspex_gui.calc.abinitioCode);
  uspex_gui.calc.abinitioCode = new_abinitioCode;

  /* Resize KresolStart */
  gdouble *new_kresol = g_new(gdouble, new_n);
  for (gint i = 0; i < qMin(old_n, new_n); i++)
    new_kresol[i] = uspex_gui.calc.KresolStart[i];
  for (gint i = old_n; i < new_n; i++)
    new_kresol[i] = 0.2;
  g_free(uspex_gui.calc.KresolStart);
  uspex_gui.calc.KresolStart = new_kresol;

  /* Resize vacuumSize */
  gdouble *new_vacuum = g_new(gdouble, new_n);
  for (gint i = 0; i < qMin(old_n, new_n); i++)
    new_vacuum[i] = uspex_gui.calc.vacuumSize[i];
  for (gint i = old_n; i < new_n; i++)
    new_vacuum[i] = 10.0;
  g_free(uspex_gui.calc.vacuumSize);
  uspex_gui.calc.vacuumSize = new_vacuum;

  /* Resize _tmp_ai_input */
  gchar **new_ai_input = g_new(gchar *, new_n);
  for (gint i = 0; i < qMin(old_n, new_n); i++)
    new_ai_input[i] = uspex_gui._tmp_ai_input[i];
  for (gint i = old_n; i < new_n; i++)
    new_ai_input[i] = NULL;
  g_free(uspex_gui._tmp_ai_input);
  uspex_gui._tmp_ai_input = new_ai_input;

  /* Resize _tmp_ai_opt */
  gchar **new_ai_opt = g_new(gchar *, new_n);
  for (gint i = 0; i < qMin(old_n, new_n); i++)
    new_ai_opt[i] = uspex_gui._tmp_ai_opt[i];
  for (gint i = old_n; i < new_n; i++)
    new_ai_opt[i] = NULL;
  g_free(uspex_gui._tmp_ai_opt);
  uspex_gui._tmp_ai_opt = new_ai_opt;

  /* Resize _tmp_ai_lib_folder */
  gchar **new_ai_lib = g_new(gchar *, new_n);
  for (gint i = 0; i < qMin(old_n, new_n); i++)
    new_ai_lib[i] = uspex_gui._tmp_ai_lib_folder[i];
  for (gint i = old_n; i < new_n; i++)
    new_ai_lib[i] = NULL;
  g_free(uspex_gui._tmp_ai_lib_folder);
  uspex_gui._tmp_ai_lib_folder = new_ai_lib;

  /* Resize _tmp_ai_lib_sel */
  gchar **new_ai_lib_sel = g_new(gchar *, new_n);
  for (gint i = 0; i < qMin(old_n, new_n); i++)
    new_ai_lib_sel[i] = uspex_gui._tmp_ai_lib_sel[i];
  for (gint i = old_n; i < new_n; i++)
    new_ai_lib_sel[i] = NULL;
  g_free(uspex_gui._tmp_ai_lib_sel);
  uspex_gui._tmp_ai_lib_sel = new_ai_lib_sel;

  /* Resize _tmp_commandExecutable */
  gchar **new_exe = g_new(gchar *, new_n);
  for (gint i = 0; i < qMin(old_n, new_n); i++)
    new_exe[i] = uspex_gui._tmp_commandExecutable[i];
  for (gint i = old_n; i < new_n; i++)
    new_exe[i] = NULL;
  g_free(uspex_gui._tmp_commandExecutable);
  uspex_gui._tmp_commandExecutable = new_exe;

  uspex_gui.calc._num_opt_steps = new_n;

  /* Clamp curr_step */
  m_w->curr_step->setRange(1, new_n);
  m_w->curr_step->setValue(qMin(m_w->curr_step->value(), new_n));
}

void UspexDialog::on_curr_step_changed(int value)
{
  gint idx = value - 1; /* 0-based */
  if (idx < 0 || idx >= uspex_gui.calc._num_opt_steps)
    return;

  /* Load step data */
  m_w->isfixed->setChecked(uspex_gui.calc._isfixed[idx]);

  m_w->abinitioCode->blockSignals(true);
  {
    int code = uspex_gui.calc.abinitioCode[idx];
    if (code < 0 || code >= m_w->abinitioCode->count())
      code = 1;
    m_w->abinitioCode->setCurrentIndex(code);
  }
  m_w->abinitioCode->blockSignals(false);

  m_w->KresolStart->setText(QString::number(uspex_gui.calc.KresolStart[idx], 'f', 4));
  m_w->vacuumSize->setText(QString::number(uspex_gui.calc.vacuumSize[idx], 'f', 4));

  /* commandExecutable: use per-step value if _isCmdList, else use [0] */
  if (uspex_gui._tmp_commandExecutable[idx])
    m_w->commandExecutable->setText(QString::fromUtf8(uspex_gui._tmp_commandExecutable[idx]));
  else
    m_w->commandExecutable->setText("N/A");

  /* ai_input */
  if (uspex_gui._tmp_ai_input[idx])
    m_w->ai_input->setText(QString::fromUtf8(uspex_gui._tmp_ai_input[idx]));
  else
    m_w->ai_input->setText("N/A");

  /* ai_opt */
  if (uspex_gui._tmp_ai_opt[idx])
    m_w->ai_opt->setText(QString::fromUtf8(uspex_gui._tmp_ai_opt[idx]));
  else
    m_w->ai_opt->setText("N/A");

  /* ai_lib */
  if (uspex_gui._tmp_ai_lib_folder[idx])
    m_w->ai_lib->setText(QString::fromUtf8(uspex_gui._tmp_ai_lib_folder[idx]));
  else
    m_w->ai_lib->setText("N/A");

  /* Sync library flavor and sel for this step */
  sync_ai_lib_flavor();
  sync_ai_lib_sel();
}

void UspexDialog::on_ai_input_button_clicked()
{
  gint step = m_w->curr_step->value();
  QString stepStr = QString::number(step);
  QString filename = QFileDialog::getOpenFileName(this, tr("Select Specific INPUT file") + " " + stepStr, QString(),
                                                  tr("INPUT files (INPUT_*);;All files (*)"));
  if (filename.isEmpty())
    return;

  gint idx = step - 1;
  g_free(uspex_gui._tmp_ai_input[idx]);
  uspex_gui._tmp_ai_input[idx] = g_strdup(filename.toUtf8().constData());
  m_w->ai_input->setText(filename);
}

void UspexDialog::on_ai_opt_button_clicked()
{
  gint step = m_w->curr_step->value();
  QString stepStr = QString::number(step);
  QString filename = QFileDialog::getOpenFileName(this, tr("Select Specific OPTION file") + " " + stepStr, QString(),
                                                  tr("OPTION files (OPTION_*);;All files (*)"));
  if (filename.isEmpty())
    return;

  gint idx = step - 1;
  g_free(uspex_gui._tmp_ai_opt[idx]);
  uspex_gui._tmp_ai_opt[idx] = g_strdup(filename.toUtf8().constData());
  m_w->ai_opt->setText(filename);
}

void UspexDialog::on_load_abinitio_exe_clicked()
{
  gint step = m_w->curr_step->value();
  QString filename = QFileDialog::getOpenFileName(this, tr("Select ab initio command"), QString(),
                                                  tr("CommandExecutable files (CommandExecutable);;All files (*)"));
  if (filename.isEmpty())
    return;

  gint idx = step - 1;
  g_free(uspex_gui._tmp_commandExecutable[idx]);
  uspex_gui._tmp_commandExecutable[idx] = g_strdup(filename.toUtf8().constData());
  m_w->commandExecutable->setText(filename);
}

void UspexDialog::on_ai_lib_button_clicked()
{
  /* Apply library flavor: populate FLAVOR combo with detected elements */
  sync_ai_lib_flavor();
}

void UspexDialog::on_apply_step_clicked()
{
  gint idx = m_w->curr_step->value() - 1;
  if (idx < 0 || idx >= uspex_gui.calc._num_opt_steps)
    return;

  /* Save step values */
  uspex_gui.calc._isfixed[idx] = m_w->isfixed->isChecked();
  uspex_gui.calc.abinitioCode[idx] = m_w->abinitioCode->currentIndex();
  uspex_gui.calc.KresolStart[idx] = m_w->KresolStart->text().toDouble();
  uspex_gui.calc.vacuumSize[idx] = m_w->vacuumSize->text().toDouble();

  g_free(uspex_gui._tmp_commandExecutable[idx]);
  uspex_gui._tmp_commandExecutable[idx] = g_strdup(m_w->commandExecutable->text().toUtf8().constData());

  /* Apply library flavor */
  apply_ai_lib_flavor();
}

/* ============================================================
 * USPEX launch button handlers
 * ============================================================ */

void UspexDialog::on_load_uspex_exe_clicked()
{
  QString filename = QFileDialog::getOpenFileName(this, tr("Select USPEX Executable"), QString(),
                                                  tr("USPEX executables (uspex_exec);;All files (*)"));
  if (filename.isEmpty())
    return;
  m_w->job_uspex_exe->setText(filename);
}

void UspexDialog::on_uspex_path_dialog_clicked()
{
  QString folder = QFileDialog::getExistingDirectory(this, tr("Select working folder"), QString());
  if (folder.isEmpty())
    return;
  m_w->job_path->setText(folder);
}

void UspexDialog::on_load_remote_folder_clicked()
{
  QString folder = QFileDialog::getExistingDirectory(this, tr("Select identical remote folder"), QString());
  if (folder.isEmpty())
    return;
  m_w->remoteFolder->setText(folder);
}

/* ========================
 * Library flavor helpers 
 * ======================== */

void UspexDialog::sync_ai_lib_flavor()
{
  gint step = m_w->curr_step->value() - 1;
  if (step < 0 || step >= uspex_gui.calc._num_opt_steps)
    return;

  m_w->ai_lib_flavor->blockSignals(true);
  {
    while (m_w->ai_lib_flavor->count() > 0)
      m_w->ai_lib_flavor->removeItem(0);

    QString libPath = m_w->ai_lib->text();
    if (libPath.isEmpty() || libPath == "N/A")
    {
      /* No folder: keep manually entered flavor or add N/A */
      if (!uspex_gui._tmp_ai_lib_sel[step] || !uspex_gui._tmp_ai_lib_sel[step][0])
        m_w->ai_lib_flavor->addItem("N/A");
      else
        m_w->ai_lib_flavor->addItem(QString::fromUtf8(uspex_gui._tmp_ai_lib_sel[step]));
      m_w->ai_lib_flavor->addItem("N/A");
      m_w->ai_lib_flavor->setCurrentIndex(0);
    } else
    {
      /* Scan folder for matching element files */
      QDir dir(libPath);
      if (dir.exists())
      {
        QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        int code = m_w->abinitioCode->currentIndex();

        /* Add AUTO for DFTB */
        if (code == 15 && !entries.isEmpty())
          m_w->ai_lib_flavor->addItem("AUTO");

        for (const QString &entry : entries)
        {
          bool match = false;
          switch (code)
          {
          case 1:
          case 2:
          case 8:
          case 10:
          case 11:
          case 18: /* VASP, SIESTA, QE, ATK, CASTEP, Abinit */
            for (int i = 0; i < uspex_gui.calc._nspecies; i++)
            {
              QString sym = QString::fromUtf8(elements[uspex_gui.calc.atomType[i]].symbol);
              if (entry.startsWith(sym))
              {
                match = true;
                break;
              }
            }
            break;
          case 3: /* GULP */
            match = entry.endsWith(".lib");
            break;
          case 12: /* Tinker */
            match = entry.endsWith(".prm");
            break;
          case 7: /* CP2K */
            match = !entry.startsWith(".");
            break;
          default:
            match = false;
            break;
          }
          if (match)
            m_w->ai_lib_flavor->addItem(entry);
        }
      }
      if (m_w->ai_lib_flavor->count() == 0)
        m_w->ai_lib_flavor->addItem("N/A");
    }
  }
  m_w->ai_lib_flavor->blockSignals(false);
}

void UspexDialog::sync_ai_lib_sel()
{
  gint step = m_w->curr_step->value() - 1;
  if (step < 0 || step >= uspex_gui.calc._num_opt_steps)
    return;

  if (uspex_gui._tmp_ai_lib_sel[step])
    m_w->ai_lib_sel->setText(QString::fromUtf8(uspex_gui._tmp_ai_lib_sel[step]));
  else
    m_w->ai_lib_sel->setText("N/A");
}

void UspexDialog::apply_ai_lib_flavor()
{
  gint step = m_w->curr_step->value() - 1;
  if (step < 0 || step >= uspex_gui.calc._num_opt_steps)
    return;

  QString flavor = m_w->ai_lib_flavor->currentText();
  if (flavor.isEmpty())
    return;

  g_free(uspex_gui._tmp_ai_lib_sel[step]);
  uspex_gui._tmp_ai_lib_sel[step] = g_strdup(flavor.toUtf8().constData());
}

void UspexDialog::on_spacegroup_active_toggled(bool checked)
{
  uspex_gui.calc.doSpaceGroup = checked;
  m_w->SymTolerance->setDisabled(!checked);
}

/* Load new format's saved data into the combo */
void UspexDialog::lattice_format_switch(int oldIdx, int newIdx)
{
  /* Sync _nlattice_line/_nlattice_vals to the new format's storage */
  switch (newIdx)
  {
  case 0: /* Volume */
    uspex_gui.calc._nlattice_line = 1;
    uspex_gui.calc._nlattice_vals = 1;
    break;
  case 1: /* Lattice */
    if (m_lattice_lattice)
    {
      uspex_gui.calc._nlattice_line = m_lattice_lattice_lines;
      uspex_gui.calc._nlattice_vals = m_lattice_lattice_vals;
    } else
    {
      uspex_gui.calc._nlattice_line = (uspex_gui._dim == 3) ? 3 : 2;
      uspex_gui.calc._nlattice_vals = (uspex_gui._dim == 3) ? 3 : 2;
    }
    break;
  case 2: /* Crystal */
    uspex_gui.calc._nlattice_line = 1;
    uspex_gui.calc._nlattice_vals = m_lattice_crystal_n > 0 ? m_lattice_crystal_n : 6;
    break;
  }
}

void UspexDialog::on_latticeformat_changed(int index)
{
  /* Always repopulate combo when format changes, regardless of auto mode */
  /* Seed from model if per-format storage is empty */
  if (!m_lattice_volume && !m_lattice_lattice && !m_lattice_crystal)
  {
    struct model_pak *mdl = qt_get_active_model();
    if (mdl)
    {
      m_lattice_volume = (double *) g_malloc(sizeof(double));
      m_lattice_volume[0] = mdl->volume;
      m_lattice_volume_n = 1;
      m_lattice_crystal = (double *) g_malloc0(6 * sizeof(double));
      m_lattice_crystal[0] = mdl->pbc[0];
      m_lattice_crystal[1] = mdl->pbc[1];
      m_lattice_crystal[2] = mdl->pbc[2];
      m_lattice_crystal[3] = mdl->pbc[3] * R2D;
      m_lattice_crystal[4] = mdl->pbc[4] * R2D;
      m_lattice_crystal[5] = mdl->pbc[5] * R2D;
      m_lattice_crystal_n = 6;
      m_lattice_lattice = (double *) g_malloc0(9 * sizeof(double));
      memcpy(m_lattice_lattice, mdl->latmat, 9 * sizeof(double));
      m_lattice_lattice_lines = 3;
      m_lattice_lattice_vals = 3;
    }
  }

  /* Save current format data, load new format data */
  int oldIdx = m_w->latticeformat->currentIndex();
  lattice_format_switch(oldIdx, index);

  /* Repopulate Latticevalues combo based on new format */
  m_w->Latticevalues->blockSignals(true);
  {
    while (m_w->Latticevalues->count() > 0)
      m_w->Latticevalues->removeItem(0);

    switch (index)
    {
    case 0: /* Volume — 1 value */
      uspex_gui.calc._nlattice_line = 1;
      uspex_gui.calc._nlattice_vals = 1;
      if (m_lattice_volume && m_lattice_volume_n > 0)
      {
        m_w->Latticevalues->addItem(QString::number(m_lattice_volume[0], 'f', 4));
      } else
      {
        m_w->Latticevalues->addItem("0.0000");
      }
      break;
    case 1: /* Lattice — _nlattice_line rows, _nlattice_vals cols */
      /* _nlattice_line/_nlattice_vals already set by lattice_format_switch */
      for (int i = 0; i < uspex_gui.calc._nlattice_line; i++)
      {
        QString row;
        for (int j = 0; j < uspex_gui.calc._nlattice_vals; j++)
        {
          if (j > 0)
            row += " ";
          if (m_lattice_lattice &&
              i * uspex_gui.calc._nlattice_vals + j < m_lattice_lattice_lines * m_lattice_lattice_vals)
            row += QString::number(m_lattice_lattice[i * uspex_gui.calc._nlattice_vals + j], 'f', 4);
          else
            row += "0.0000";
        }
        m_w->Latticevalues->addItem(row);
      }
      break;
    case 2: /* Crystal — 6 values */
      uspex_gui.calc._nlattice_line = 1;
      uspex_gui.calc._nlattice_vals = 6;
      {
        QString row;
        for (int j = 0; j < 6; j++)
        {
          if (j > 0)
            row += " ";
          if (m_lattice_crystal && j < m_lattice_crystal_n)
            row += QString::number(m_lattice_crystal[j], 'f', 4);
          else
            row += "0.0000";
        }
        m_w->Latticevalues->addItem(row);
      }
      break;
    }
    m_w->Latticevalues->setCurrentIndex(0);
  }
  m_w->Latticevalues->blockSignals(false);

  /* Copy first combo line to VALUES field */
  if (m_w->Latticevalues->count() > 0)
    m_w->latticevalue->setText(m_w->Latticevalues->itemText(0));
}

void UspexDialog::on_latticevalues_selected(int index)
{
  /* Copy selected combo line to VALUES field */
  if (index < 0 || index >= m_w->Latticevalues->count())
    return;
  m_w->latticevalue->setText(m_w->Latticevalues->itemText(index));
}

void UspexDialog::on_apply_latticevalue()
{
  /* Apply modified lattice values back to combo and per-format storage */
  int index = m_w->Latticevalues->currentIndex();
  if (index < 0)
    return;

  QString text = m_w->latticevalue->text();

  /* Replace the combo line */
  m_w->Latticevalues->setItemText(index, text);

  /* Parse the text */
  QString trimmed = text.trimmed();
  if (trimmed.isEmpty())
    return;

  QStringList parts = trimmed.split(' ', Qt::SkipEmptyParts);
  if (parts.isEmpty())
    return;

  /* Get current format from FORMAT combo */
  int formatIdx = m_w->latticeformat->currentIndex();

  switch (formatIdx)
  {
  case 0: /* Volume — 1 value */
    uspex_gui.calc._nlattice_line = 1;
    uspex_gui.calc._nlattice_vals = 1;
    if (m_lattice_volume == nullptr)
    {
      m_lattice_volume = (double *) g_malloc(sizeof(gdouble));
    }
    m_lattice_volume_n = 1;
    {
      bool ok;
      m_lattice_volume[0] = parts[0].toDouble(&ok);
    }
    break;
  case 1: /* Lattice — _nlattice_line rows, _nlattice_vals cols */
    if (uspex_gui._dim == 3)
    {
      uspex_gui.calc._nlattice_line = 3;
      uspex_gui.calc._nlattice_vals = 3;
    } else
    {
      uspex_gui.calc._nlattice_line = 2;
      uspex_gui.calc._nlattice_vals = 2;
    }
    {
      int nlines = m_w->Latticevalues->count();
      int nvals = uspex_gui.calc._nlattice_vals;
      int total = nlines * nvals;
      gdouble *newData = (double *) g_malloc0(total * sizeof(gdouble));
      for (int r = 0; r < nlines; r++)
      {
        QString rline = m_w->Latticevalues->itemText(r);
        QStringList rparts = rline.trimmed().split(' ', Qt::SkipEmptyParts);
        for (int c = 0; c < rparts.size() && c < nvals; c++)
        {
          bool ok;
          newData[r * nvals + c] = rparts[c].toDouble(&ok);
        }
      }
      g_free(m_lattice_lattice);
      m_lattice_lattice = newData;
      m_lattice_lattice_lines = nlines;
      m_lattice_lattice_vals = nvals;
    }
    break;
  case 2: /* Crystal — 6 values */
    uspex_gui.calc._nlattice_line = 1;
    uspex_gui.calc._nlattice_vals = 6;
    {
      int total = 6;
      gdouble *newData = (double *) g_malloc0(total * sizeof(gdouble));
      for (int j = 0; j < parts.size() && j < total; j++)
      {
        bool ok;
        newData[j] = parts[j].toDouble(&ok);
      }
      g_free(m_lattice_crystal);
      m_lattice_crystal = newData;
      m_lattice_crystal_n = total;
    }
    break;
  }
}

void UspexDialog::on_v1030_toggled(bool checked)
{
  uspex_gui.have_v1030 = checked;

  /* Sync both Ver 10.4 checkboxes */
  m_w->sel_v1030_1->blockSignals(true);
  m_w->sel_v1030_3->blockSignals(true);
  m_w->sel_v1030_1->setChecked(checked);
  m_w->sel_v1030_3->setChecked(checked);
  m_w->sel_v1030_1->blockSignals(false);
  m_w->sel_v1030_3->blockSignals(false);

  if (checked)
  {
    /* Ver 10.4: lock octave, force NEW on and locked */
    m_w->sel_octave->setDisabled(true);
    m_w->sel_new_opt->setChecked(true);
    m_w->sel_new_opt->setDisabled(true);
    m_w->new_optType->setDisabled(false);
    m_w->optType->setDisabled(true);
  } else
  {
    /* Pre-10.4: unlock octave, NEW off and unlocked */
    m_w->sel_octave->setDisabled(false);
    m_w->sel_new_opt->setChecked(false);
    m_w->sel_new_opt->setDisabled(false);
    m_w->new_optType->setDisabled(true);
    m_w->optType->setDisabled(false);
  }
  uspex_gui.have_new_optType = checked;
}

void UspexDialog::on_optType_changed(int index)
{
  /* Map combo index to optType value (combo has gaps: 1-12, 14, 17, 1101-1111) */
  static const int optTypeValues[] = {1,  2,    3,    4,    5,    6,    7,    8,    9,    10,   11,   12,  14,
                                      17, 1101, 1102, 1103, 1104, 1105, 1106, 1107, 1108, 1109, 1110, 1111};
  int nValues = sizeof(optTypeValues) / sizeof(optTypeValues[0]);
  if (index >= 0 && index < nValues)
    uspex_gui.calc.optType = (uspex_opt) optTypeValues[index];
  else
    uspex_gui.calc.optType = (uspex_opt) (index + 1);
  update_boltztrap_visibility();
}

void UspexDialog::on_new_optType_changed(const QString &text) { update_boltztrap_visibility(); }

void UspexDialog::on_meta_model_changed(int index)
{
  /* Enable button only when "From VASP5 POSCAR file" (index 0) is selected */
  m_w->meta_model_button->setDisabled(index != 0);
}

void UspexDialog::on_meta_model_button_clicked()
{
  /* Open file dialog for POSCAR */
  QString filename = QFileDialog::getOpenFileName(this, tr("Select META starting point"), QString(),
                                                  tr("POSCAR files (POSCAR*);;All files (*)"));
  if (filename.isEmpty())
    return;

  /* Add selected file as first item in combo, select it */
  m_w->meta_model->blockSignals(true);
  {
    int idx = m_w->meta_model->count();
    m_w->meta_model->insertItem(0, filename);
    m_w->meta_model->setCurrentIndex(0);
  }
  m_w->meta_model->blockSignals(false);
}

void UspexDialog::on_vcnebtype_changed()
{
  /* Only apply VCNEB-dependent states when method IS VCNEB */
  if (m_w->calculationMethod->currentIndex() != 2)
    return;

  /* Sync vcnebType from method/Var_Iname/Var_Spring */
  int type = m_w->vcnebtype_method->currentIndex() == 1 ? 200 : 100;
  if (m_w->vcnebtype_img_num->isChecked())
    type += 10;
  if (m_w->vcnebtype_spring->isChecked())
    type += 1;
  uspex_gui.calc.vcnebType = type;
  m_w->vcnebType->setText(QString::number(type));

  /* Var_Iname → enable/disable VarPathLength */
  m_w->VarPathLength->setDisabled(!m_w->vcnebtype_img_num->isChecked());

  /* Var_Spring → enable/disable K_min/K_max, lock/unlock Kconstant */
  if (m_w->vcnebtype_spring->isChecked())
  {
    m_w->K_min->setDisabled(false);
    m_w->K_max->setDisabled(false);
    m_w->Kconstant->setDisabled(true);
  } else
  {
    m_w->K_min->setDisabled(true);
    m_w->K_max->setDisabled(true);
    m_w->Kconstant->setDisabled(false);
  }
}

void UspexDialog::on_cidi_method_changed(int index)
{
  /* Disable Start CI/DI and Pickup when optMethodCIDI is 0 (No CI/DI) */
  bool disabled = (index == 0);
  m_w->startCIDIStep->setDisabled(disabled);
  m_w->pickupImages->setDisabled(disabled);
}

void UspexDialog::on_img_model_changed(int index)
{
  /* Enable button only when "From file" (index 0) is selected */
  m_w->img_model_button->setDisabled(index != 0);
}

void UspexDialog::on_img_model_button_clicked()
{
  /* Build file filter based on FormatType selection */
  int fmt = m_w->FormatType->currentIndex();
  QString filter;
  switch (fmt)
  {
  case 0:
    filter = tr("XCRYSTDENS files (*.xsf);;All files (*)");
    break;
  case 1:
    filter = tr("VASP POSCAR files (*POSCAR*);;All files (*)");
    break;
  case 2:
    filter = tr("XYZ files (*.xyz);;All files (*)");
    break;
  default:
    filter = tr("Structure files (*.xsf *.POSCAR *.xyz);;All files (*)");
    break;
  }
  QString filename = QFileDialog::getOpenFileName(this, tr("Select VCNEB image model"), QString(), filter);
  if (filename.isEmpty())
    return;

  /* Add selected file as first item in combo, select it */
  m_w->img_model->blockSignals(true);
  {
    int idx = m_w->img_model->count();
    m_w->img_model->insertItem(0, filename);
    m_w->img_model->setCurrentIndex(0);
  }
  m_w->img_model->blockSignals(false);
}

void UspexDialog::on_FormatType_changed(int index)
{
  /* Sync calc.FormatType from combo index (index 0 = value 1, etc.) */
  uspex_gui.calc.FormatType = index + 1;
}

void UspexDialog::on_substrate_model_changed(int index)
{
  /* Enable button only when "From VASP5 POSCAR file" (index 0) is selected */
  m_w->substrate_model_button->setDisabled(index != 0);
}

void UspexDialog::on_substrate_model_button_clicked()
{
  /* Open file dialog for POSCAR files */
  QString filename = QFileDialog::getOpenFileName(this, tr("Select Substrate structure"), QString(),
                                                  tr("VASP POSCAR files (*POSCAR*);;All files (*)"));
  if (filename.isEmpty())
    return;

  /* Add selected file as first item in combo, select it */
  m_w->substrate_model->blockSignals(true);
  {
    int idx = m_w->substrate_model->count();
    m_w->substrate_model->insertItem(0, filename);
    m_w->substrate_model->setCurrentIndex(0);
  }
  m_w->substrate_model->blockSignals(false);
}

void UspexDialog::on_mol_model_changed(int index)
{
  /* If MOL toggle is off, disable all Molecules frame widgets */
  if (!m_w->calctype_mol->isChecked())
  {
    m_w->num_mol->setDisabled(true);
    m_w->mol_model_button->setDisabled(true);
    m_w->mol_gdis->setDisabled(true);
    m_w->curr_mol->setDisabled(true);
    m_w->mol_gulp->setDisabled(true);
    m_w->mol_apply_button->setDisabled(true);
    return;
  }

  /* index 0 = "Already provided" → disable all other widgets */
  /* index 1 = "From MOL_ folder" → enable N_MOLS + Open button */
  /* index 2 = "From GDIS models" → enable all except Open button */
  bool fromMolFolder = (index == 1);
  bool fromGDIS = (index == 2);

  m_w->num_mol->setDisabled(!fromMolFolder && !fromGDIS);
  m_w->mol_model_button->setDisabled(!fromMolFolder);
  m_w->mol_gdis->setDisabled(!fromGDIS);
  m_w->curr_mol->setDisabled(!fromGDIS);
  m_w->mol_gulp->setDisabled(!fromGDIS);
  m_w->mol_apply_button->setDisabled(!fromGDIS);
}

void UspexDialog::on_num_mol_changed(int value)
{
  /* Resize _tmp_mols arrays and update curr_mol range */
  gint old_n = uspex_gui.calc._nmolecules;
  gint new_n = value;

  gint *new_mols_gdis = g_new(gint, new_n);
  gboolean *new_mols_gulp = g_new(gboolean, new_n);

  for (gint i = 0; i < qMin(old_n, new_n); i++)
  {
    new_mols_gdis[i] = uspex_gui._tmp_mols_gdis[i];
    new_mols_gulp[i] = uspex_gui._tmp_mols_gulp[i];
  }
  for (gint i = old_n; i < new_n; i++)
  {
    new_mols_gdis[i] = 0;
    new_mols_gulp[i] = FALSE;
  }

  g_free(uspex_gui._tmp_mols_gdis);
  g_free(uspex_gui._tmp_mols_gulp);
  uspex_gui._tmp_mols_gdis = new_mols_gdis;
  uspex_gui._tmp_mols_gulp = new_mols_gulp;
  uspex_gui.calc._nmolecules = new_n;

  /* Update curr_mol range: 1 to N_MOLS */
  m_w->curr_mol->setRange(1, new_n);
  m_w->curr_mol->setValue(qMin(m_w->curr_mol->value(), new_n));
}

void UspexDialog::on_curr_mol_changed(int value)
{
  /* Load molecule data for current index */
  gint idx = value - 1; /* 0-based */
  if (idx < 0 || idx >= uspex_gui.calc._nmolecules)
    return;

  m_w->mol_gdis->blockSignals(true);
  {
    m_w->mol_gdis->setCurrentIndex(uspex_gui._tmp_mols_gdis[idx]);
    m_w->mol_gulp->setChecked(uspex_gui._tmp_mols_gulp[idx]);
  }
  m_w->mol_gdis->blockSignals(false);
}

void UspexDialog::on_mol_apply_clicked()
{
  /* Store GDIS_MOL selection and GULP_FORM flag for current molecule */
  gint idx = m_w->curr_mol->value() - 1; /* 0-based */
  if (idx < 0 || idx >= uspex_gui.calc._nmolecules)
    return;

  gint gdis_idx;
  m_w->mol_gdis->blockSignals(true);
  {
    gdis_idx = m_w->mol_gdis->currentIndex();
  }
  m_w->mol_gdis->blockSignals(false);

  uspex_gui._tmp_mols_gdis[idx] = gdis_idx;
  uspex_gui._tmp_mols_gulp[idx] = m_w->mol_gulp->isChecked();
}

void UspexDialog::on_mol_model_button_clicked()
{
  int idx = m_w->mol_model->currentIndex();

  if (idx == 1)
  {
    /* "From MOL_ folder" → open folder dialog */
    QString folder = QFileDialog::getExistingDirectory(this, tr("Select MOL_x folder"), QString());
    if (folder.isEmpty())
      return;

    /* Replace "From MOL_ folder" with selected path, re-add "From GDIS models" */
    m_w->mol_model->blockSignals(true);
    {
      m_w->mol_model->setItemText(1, folder);
      /* Ensure "From GDIS models" exists at index 2 */
      while (m_w->mol_model->count() < 3)
        m_w->mol_model->addItem(tr("From GDIS models"));
      m_w->mol_model->setCurrentIndex(1);
    }
    m_w->mol_model->blockSignals(false);
  } else if (idx == 0)
  {
    /* "Already provided" → should not happen, but handle gracefully */
  }
}

void UspexDialog::on_save()
{
  sync();
  save_uspex_calc();
  QMessageBox::information(this, tr("USPEX"), tr("Settings saved."));
}

void UspexDialog::on_file_entry_button_clicked()
{
  /* Open file dialog for USPEX Parameters.txt */
  QString filename = QFileDialog::getOpenFileName(this, tr("Select USPEX Parameters file"), QString(),
                                                  tr("USPEX Parameters files (Parameters.txt);;All files (*)"));
  if (filename.isEmpty())
    return;

  /* Set file_entry_path so refresh() preserves it */
  g_free(uspex_gui.file_entry_path);
  uspex_gui.file_entry_path = g_strdup(filename.toUtf8().constData());
  m_w->file_entry->setText(filename);

  /* Load parameters from the selected file */
  gchar *gfilename = g_strdup(filename.toUtf8().constData());
  gint safe_nspecies = uspex_gui.calc._nspecies;
  uspex_calc_struct *test_calc = read_uspex_parameters(gfilename, safe_nspecies);
  if (test_calc != NULL)
  {
    copy_uspex_parameters(test_calc, &(uspex_gui.calc));
    refresh();
    free_uspex_parameters(test_calc);
    g_free(test_calc);
  }
  g_free(gfilename);
}

/* Bridge function */
extern "C" void qt_show_uspex_dialog(void)
{
  extern QWidget *get_main_window_widget();
  struct model_pak *model = qt_get_active_model();
  UspexDialog *dlg = new UspexDialog(get_main_window_widget(), model);
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->show();
  dlg->raise();
  dlg->activateWindow();
}
