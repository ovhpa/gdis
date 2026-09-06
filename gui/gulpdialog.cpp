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

#include "gulpdialog.h"
#include "gdis_api.h"

#include "gdis.h"
#include <QTimer>

/* Map from model pointer to GulpDialog — used by task callbacks for energy updates */
static struct model_pak *g_gulp_dialog_models[4096] = {nullptr};
static GulpDialog *g_gulp_dialogs[4096] = {nullptr};
static int g_gulp_dialog_count = 0;

/* Qt-aware: update energy display from background thread */
extern "C" void qt_update_gulp_energy(struct model_pak *model)
{
  if (!model)
    return;
  GulpDialog *dlg = nullptr;
  for (int i = 0; i < g_gulp_dialog_count; i++)
  {
    if (g_gulp_dialog_models[i] == model)
    {
      dlg = g_gulp_dialogs[i];
      break;
    }
  }
  if (!dlg)
    return;
  /* Post to main thread */
  QTimer::singleShot(0, [dlg]() { dlg->syncEnergyToUI(); });
}

#include "gui_shorts.h"
extern "C" {
#include "edit.h"
}

/* Helper: add radio button to group */
static void add_radio(QButtonGroup *group, QRadioButton *rb)
{
  if (group)
    group->addButton(rb);
}

/* Helper: create a check box */
static QCheckBox *make_check(const char *label, QWidget *parent) { return new QCheckBox(label, parent); }

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QTimer>
#include <QComboBox>
#include <QPushButton>
#include <QSlider>
#include <QButtonGroup>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>

GulpDialog::GulpDialog(struct model_pak *model, QWidget *parent) : QDialog(parent), m_model(model), m_tabWidget(nullptr)
{
  setWindowTitle(tr("GULP Configuration"));
  resize(900, 650);

  setupUI();
  syncFromModel();
}

GulpDialog::~GulpDialog() {}

void GulpDialog::setupUI()
{
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(8, 6, 8, 6);
  mainLayout->setSpacing(6);

  /* Tab widget */
  m_tabWidget = new QTabWidget();
  mainLayout->addWidget(m_tabWidget);

  /* Button groups for mutually exclusive radio buttons */
  m_runGroup = new QButtonGroup(this);
  m_constraintGroup = new QButtonGroup(this);
  m_coulombGroup = new QButtonGroup(this);
  m_ensembleGroup = new QButtonGroup(this);
  m_primaryOptGroup = new QButtonGroup(this);
  m_secondaryOptGroup = new QButtonGroup(this);
  m_switchGroup = new QButtonGroup(this);

  populateControlTab(m_tabWidget);
  populateFilesTab(m_tabWidget);
  populateOptimisationTab(m_tabWidget);
  populatePotentialsTab(m_tabWidget);
  populateElementsTab(m_tabWidget);
  populateUnprocessedTab(m_tabWidget);
  populateVibrationalTab(m_tabWidget);
  populateSolvationTab(m_tabWidget);

  /* Common frames (Files + Details) below tabs */
  populateCommonFrames();

  /* Execute/Close buttons */
  auto *btnLayout = new QHBoxLayout();
  btnLayout->addStretch();
  auto *execBtn = new QPushButton(tr("Execute"));
  auto *closeBtn = new QPushButton(tr("Close"));
  btnLayout->addWidget(execBtn);
  btnLayout->addWidget(closeBtn);
  mainLayout->addLayout(btnLayout);

  connect(execBtn, &QPushButton::clicked, this, &GulpDialog::on_execute);
  connect(closeBtn, &QPushButton::clicked, this, &GulpDialog::on_close);
}

void GulpDialog::populateControlTab(QTabWidget *tabWidget)
{
  auto *tab = new QWidget();
  auto *layout = new QVBoxLayout(tab);
  layout->setContentsMargins(8, 6, 8, 6);
  layout->setSpacing(6);

  auto *hbox = new QHBoxLayout();
  hbox->setSpacing(8);
  layout->addLayout(hbox);

  auto *leftCol = new QVBoxLayout();
  auto *rightCol = new QVBoxLayout();
  hbox->addLayout(leftCol);
  hbox->addLayout(rightCol);

  /* --- LEFT COLUMN --- */

  /* Run type */
  auto *runGroup = new QGroupBox(tr("Run type"));
  auto *runLayout = new QVBoxLayout(runGroup);
  runLayout->setSpacing(4);
  m_rbSingle = new QRadioButton(tr("Single point"));
  m_rbOptimize = new QRadioButton(tr("Optimise"));
  m_rbDynamics = new QRadioButton(tr("Dynamics"));
  add_radio(m_runGroup, m_rbSingle);
  add_radio(m_runGroup, m_rbOptimize);
  add_radio(m_runGroup, m_rbDynamics);
  runLayout->addWidget(m_rbSingle);
  runLayout->addWidget(m_rbOptimize);
  runLayout->addWidget(m_rbDynamics);
  leftCol->addWidget(runGroup);

  /* Constraint */
  auto *constraintGroup = new QGroupBox(tr("Constraint"));
  auto *constraintLayout = new QVBoxLayout(constraintGroup);
  constraintLayout->setSpacing(4);
  m_rbConp = new QRadioButton(tr("Constant pressure"));
  m_rbConv = new QRadioButton(tr("Constant volume"));
  add_radio(m_constraintGroup, m_rbConp);
  add_radio(m_constraintGroup, m_rbConv);
  constraintLayout->addWidget(m_rbConp);
  constraintLayout->addWidget(m_rbConv);
  leftCol->addWidget(constraintGroup);

  /* Molecule options */
  auto *molGroup = new QGroupBox(tr("Molecule options"));
  auto *molLayout = new QVBoxLayout(molGroup);
  molLayout->setSpacing(4);
  m_rbMole = new QRadioButton(tr("Coulomb subtract all intramolecular"));
  m_rbMolmec = new QRadioButton(tr("Coulomb subtract 1-2 and 1-3 intramolecular"));
  m_rbMolq = new QRadioButton(tr("Build but retain coulomb interactions"));
  m_rbNobuild = new QRadioButton(tr("Molecule building off"));
  add_radio(m_coulombGroup, m_rbMole);
  add_radio(m_coulombGroup, m_rbMolmec);
  add_radio(m_coulombGroup, m_rbMolq);
  add_radio(m_coulombGroup, m_rbNobuild);
  molLayout->addWidget(m_rbMole);
  molLayout->addWidget(m_rbMolmec);
  molLayout->addWidget(m_rbMolq);
  molLayout->addWidget(m_rbNobuild);
  leftCol->addWidget(molGroup);

  /* Connectivity checks */
  m_chkFix = make_check("Fix the initial connectivity", tab);
  m_chkNoautobond = make_check("Automatic connectivity off", tab);
  leftCol->addWidget(m_chkFix);
  leftCol->addWidget(m_chkNoautobond);

  /* Temperature & Pressure — text fields, not spinners */
  auto *tempGroup = new QGroupBox(tr("Temperature & Pressure"));
  auto *tempLayout = new QFormLayout(tempGroup);
  tempLayout->setSpacing(4);
  m_editTemp = new QLineEdit();
  m_editTemp->setPlaceholderText("Temperature (K)");
  m_editPressure = new QLineEdit();
  m_editPressure->setPlaceholderText("Pressure (bar)");
  tempLayout->addRow(tr("Temperature (K)"), m_editTemp);
  tempLayout->addRow(tr("Pressure (bar)"), m_editPressure);
  leftCol->addWidget(tempGroup);

  /* --- RIGHT COLUMN --- */

  /* Dynamics ensemble */
  auto *dynGroup = new QGroupBox(tr("Dynamics"));
  auto *dynLayout = new QVBoxLayout(dynGroup);
  dynLayout->setSpacing(4);
  m_rbNVE = new QRadioButton(tr("NVE"));
  m_rbNVT = new QRadioButton(tr("NVT"));
  m_rbNPT = new QRadioButton(tr("NPT"));
  add_radio(m_ensembleGroup, m_rbNVE);
  add_radio(m_ensembleGroup, m_rbNVT);
  add_radio(m_ensembleGroup, m_rbNPT);
  dynLayout->addWidget(m_rbNVE);
  dynLayout->addWidget(m_rbNVT);
  dynLayout->addWidget(m_rbNPT);
  rightCol->addWidget(dynGroup);

  /* Dynamics timing */
  auto *timingLayout = new QFormLayout();
  timingLayout->setSpacing(4);
  m_editTimestep = new QLineEdit();
  m_editTimestep->setPlaceholderText("Time step (fs)");
  m_editEquilibration = new QLineEdit();
  m_editEquilibration->setPlaceholderText("Equilibration");
  m_editProduction = new QLineEdit();
  m_editProduction->setPlaceholderText("Production");
  m_editSample = new QLineEdit();
  m_editSample->setPlaceholderText("Sample");
  m_editWrite = new QLineEdit();
  m_editWrite->setPlaceholderText("Write");
  timingLayout->addRow(tr("Time step (fs)"), m_editTimestep);
  timingLayout->addRow(tr("Equilibration"), m_editEquilibration);
  timingLayout->addRow(tr("Production"), m_editProduction);
  timingLayout->addRow(tr("Sample"), m_editSample);
  timingLayout->addRow(tr("Write"), m_editWrite);
  rightCol->addLayout(timingLayout);

  /* Keyword toggles */
  auto *toggleGroup = new QGroupBox(tr("Options"));
  auto *toggleLayout = new QVBoxLayout(toggleGroup);
  toggleLayout->setSpacing(4);
  m_chkNoExec = make_check("Create input file then stop", tab);
  m_chkNosym = make_check("Build cell then discard symmetry", tab);
  m_chkNoEatt = make_check("No attachment energy calculation", tab);
  m_chkQeq = make_check("QEq electronegativity equalisation", tab);
  toggleLayout->addWidget(m_chkNoExec);
  toggleLayout->addWidget(m_chkNosym);
  toggleLayout->addWidget(m_chkNoEatt);
  toggleLayout->addWidget(m_chkQeq);
  rightCol->addWidget(toggleGroup);

  rightCol->addStretch();
  layout->addLayout(hbox);
  tabWidget->addTab(tab, tr("Control"));
}

void GulpDialog::populateFilesTab(QTabWidget *tabWidget)
{
  auto *tab = new QWidget();
  auto *layout = new QVBoxLayout(tab);
  layout->setContentsMargins(8, 6, 8, 6);
  layout->setSpacing(6);

  auto *filesGroup = new QGroupBox(tr("Files"));
  auto *filesLayout = new QFormLayout(filesGroup);
  filesLayout->setSpacing(4);
  m_editInputFile = new QLineEdit();
  m_editDumpFile = new QLineEdit();
  m_editTrajFile = new QLineEdit();
  filesLayout->addRow(tr("Input file"), m_editInputFile);
  filesLayout->addRow(tr("Dump file"), m_editDumpFile);
  filesLayout->addRow(tr("Trajectory file"), m_editTrajFile);
  layout->addWidget(filesGroup);

  auto *optGroup = new QGroupBox(tr("Options"));
  auto *optLayout = new QVBoxLayout(optGroup);
  optLayout->setSpacing(4);
  m_chkPrintCharge = make_check("Print charge with coordinates", tab);
  optLayout->addWidget(m_chkPrintCharge);
  layout->addWidget(optGroup);

  layout->addStretch();
  tabWidget->addTab(tab, tr("Files"));
}

void GulpDialog::populateOptimisationTab(QTabWidget *tabWidget)
{
  auto *tab = new QWidget();
  auto *layout = new QVBoxLayout(tab);
  layout->setContentsMargins(8, 6, 8, 6);
  layout->setSpacing(6);

  /* Top row: 3 columns side by side */
  auto *topHBox = new QHBoxLayout();
  topHBox->setSpacing(8);

  /* Column 1: Primary optimiser */
  auto *primGroup = new QGroupBox(tr("Primary optimiser"));
  auto *primLayout = new QVBoxLayout(primGroup);
  primLayout->setSpacing(4);
  m_rbBfgsOpt = new QRadioButton(tr("bfgs"));
  m_rbConjOpt = new QRadioButton(tr("conj"));
  m_rbRfoOpt = new QRadioButton(tr("rfo"));
  add_radio(m_primaryOptGroup, m_rbBfgsOpt);
  add_radio(m_primaryOptGroup, m_rbConjOpt);
  add_radio(m_primaryOptGroup, m_rbRfoOpt);
  primLayout->addWidget(m_rbBfgsOpt);
  primLayout->addWidget(m_rbConjOpt);
  primLayout->addWidget(m_rbRfoOpt);
  topHBox->addWidget(primGroup, 1);

  /* Column 2: Secondary optimiser */
  auto *secGroup = new QGroupBox(tr("Secondary optimiser"));
  auto *secLayout = new QVBoxLayout(secGroup);
  secLayout->setSpacing(4);
  m_rbNoOpt2 = new QRadioButton(tr("none"));
  m_rbBfgsOpt2 = new QRadioButton(tr("bfgs"));
  m_rbConjOpt2 = new QRadioButton(tr("conj"));
  m_rbRfoOpt2 = new QRadioButton(tr("rfo"));
  add_radio(m_secondaryOptGroup, m_rbNoOpt2);
  add_radio(m_secondaryOptGroup, m_rbBfgsOpt2);
  add_radio(m_secondaryOptGroup, m_rbConjOpt2);
  add_radio(m_secondaryOptGroup, m_rbRfoOpt2);
  secLayout->addWidget(m_rbNoOpt2);
  secLayout->addWidget(m_rbBfgsOpt2);
  secLayout->addWidget(m_rbConjOpt2);
  secLayout->addWidget(m_rbRfoOpt2);
  topHBox->addWidget(secGroup, 1);

  /* Column 3: Switching criteria */
  auto *switchGroup = new QGroupBox(tr("Switching criteria"));
  auto *switchLayout = new QVBoxLayout(switchGroup);
  switchLayout->setSpacing(4);
  m_rbCycle = new QRadioButton(tr("cycle"));
  m_rbGnorm = new QRadioButton(tr("gnorm"));
  add_radio(m_switchGroup, m_rbCycle);
  add_radio(m_switchGroup, m_rbGnorm);
  switchLayout->addWidget(m_rbCycle);
  switchLayout->addWidget(m_rbGnorm);

  auto *valueLayout = new QHBoxLayout();
  valueLayout->addWidget(new QLabel(tr("Value")));
  m_editSwitchValue = new QLineEdit();
  valueLayout->addWidget(m_editSwitchValue);
  switchLayout->addLayout(valueLayout);

  topHBox->addWidget(switchGroup, 1);
  layout->addLayout(topHBox);

  /* Bottom row: Maximum cycles — spans full width */
  auto *cycleGroup = new QGroupBox(tr("Optimization"));
  auto *cycleLayout = new QHBoxLayout(cycleGroup);
  cycleLayout->setSpacing(4);
  auto *cycleLabel = new QLabel(tr("Maximum cycles"));
  m_spinMaxCyc = new QSpinBox();
  m_spinMaxCyc->setRange(0, 500);
  m_spinMaxCyc->setSingleStep(10);
  cycleLayout->addWidget(cycleLabel);
  cycleLayout->addWidget(m_spinMaxCyc);
  cycleLayout->addStretch();
  layout->addWidget(cycleGroup);

  tabWidget->addTab(tab, tr("Optimisation"));
}

void GulpDialog::populatePotentialsTab(QTabWidget *tabWidget)
{
  auto *tab = new QWidget();
  auto *layout = new QVBoxLayout(tab);
  layout->setContentsMargins(8, 6, 8, 6);
  layout->setSpacing(6);

  auto *potGroup = new QGroupBox(tr("Potentials"));
  auto *potLayout = new QVBoxLayout(potGroup);
  m_editPotentials = new QTextEdit();
  m_editPotentials->setMinimumHeight(200);
  potLayout->addWidget(m_editPotentials);
  layout->addWidget(potGroup);

  auto *libGroup = new QGroupBox(tr("Potential Library"));
  auto *libLayout = new QHBoxLayout(libGroup);
  libLayout->setSpacing(4);
  auto *libLabel = new QLabel(tr("File"));
  m_editLibFile = new QLineEdit();
  libLayout->addWidget(libLabel);
  libLayout->addWidget(m_editLibFile);
  layout->addWidget(libGroup);

  layout->addStretch();
  tabWidget->addTab(tab, tr("Potentials"));
}

void GulpDialog::populateElementsTab(QTabWidget *tabWidget)
{
  auto *tab = new QWidget();
  auto *layout = new QHBoxLayout(tab);
  layout->setContentsMargins(8, 6, 8, 6);
  layout->setSpacing(8);

  auto *elemGroup = new QGroupBox(tr("Element"));
  auto *elemLayout = new QVBoxLayout(elemGroup);
  m_editElements = new QTextEdit();
  m_editElements->setMinimumHeight(300);
  elemLayout->addWidget(m_editElements);
  layout->addWidget(elemGroup, 1);

  auto *specGroup = new QGroupBox(tr("Species"));
  auto *specLayout = new QVBoxLayout(specGroup);
  m_editSpecies = new QTextEdit();
  m_editSpecies->setMinimumHeight(300);
  specLayout->addWidget(m_editSpecies);
  layout->addWidget(specGroup, 1);

  tabWidget->addTab(tab, tr("Elements"));
}

void GulpDialog::populateUnprocessedTab(QTabWidget *tabWidget)
{
  auto *tab = new QWidget();
  auto *layout = new QVBoxLayout(tab);
  layout->setContentsMargins(8, 6, 8, 6);
  layout->setSpacing(6);

  auto *kwGroup = new QGroupBox(tr("Keywords"));
  auto *kwLayout = new QVBoxLayout(kwGroup);
  kwLayout->setSpacing(4);
  m_chkOutputExtraKeywords = make_check("Pass keywords through to GULP", tab);
  m_editExtraKeywords = new QLineEdit();
  kwLayout->addWidget(m_chkOutputExtraKeywords);
  kwLayout->addWidget(new QLabel(tr("Keywords")));
  kwLayout->addWidget(m_editExtraKeywords);
  layout->addWidget(kwGroup);

  auto *optGroup = new QGroupBox(tr("Options"));
  auto *optLayout = new QVBoxLayout(optGroup);
  optLayout->setSpacing(4);
  m_chkOutputExtra = make_check("Pass options through to GULP", tab);
  m_editExtra = new QTextEdit();
  optLayout->addWidget(m_chkOutputExtra);
  optLayout->addWidget(m_editExtra);
  layout->addWidget(optGroup);

  layout->addStretch();
  tabWidget->addTab(tab, tr("Unprocessed"));
}

void GulpDialog::populateVibrationalTab(QTabWidget *tabWidget)
{
  auto *tab = new QWidget();
  auto *layout = new QVBoxLayout(tab);
  layout->setContentsMargins(8, 6, 8, 6);
  layout->setSpacing(6);

  /* Compute options + kpoints row */
  auto *computeHBox = new QHBoxLayout();
  computeHBox->setSpacing(8);

  auto *computeGroup = new QGroupBox(tr("Compute"));
  auto *computeLayout = new QVBoxLayout(computeGroup);
  computeLayout->setSpacing(4);
  m_chkPhonon = make_check("Compute vibrational modes", tab);
  m_chkEigen = make_check("Compute eigenvectors", tab);
  computeLayout->addWidget(m_chkPhonon);
  computeLayout->addWidget(m_chkEigen);
  computeHBox->addWidget(computeGroup, 1);

  if (m_model->periodic)
  {
    auto *kpGroup = new QGroupBox(tr("kpoints"));
    auto *kpLayout = new QVBoxLayout(kpGroup);
    m_editKpoints = new QTextEdit();
    kpLayout->addWidget(m_editKpoints);
    computeHBox->addWidget(kpGroup, 1);
  }

  layout->addLayout(computeHBox);

  /* Eigenvectors section */
  auto *eigenGroup = new QGroupBox(tr("Eigenvectors"));
  auto *eigenLayout = new QVBoxLayout(eigenGroup);
  eigenLayout->setSpacing(4);

  /* Slider + prev/next buttons */
  auto *sliderRow = new QHBoxLayout();
  sliderRow->addWidget(new QLabel(tr("Number")));
  m_phononSlider = new QSlider(Qt::Horizontal);
  m_phononSlider->setRange(0, 1);
  m_phononSlider->setDisabled(true); /* will be updated when phonon data loads */
  auto *prevBtn = new QPushButton(tr("<"));
  auto *nextBtn = new QPushButton(tr(">"));
  sliderRow->addWidget(m_phononSlider);
  sliderRow->addWidget(prevBtn);
  sliderRow->addWidget(nextBtn);
  eigenLayout->addLayout(sliderRow);

  /* IR/Raman info table */
  auto *infoLayout = new QFormLayout();
  infoLayout->setSpacing(4);
  infoLayout->addRow(tr("Frequency"), new QLabel("—"));
  auto *irBtn = new QPushButton(tr("IR intensity"));
  auto *ramanBtn = new QPushButton(tr("Raman intensity"));
  infoLayout->addRow(irBtn, new QLabel("—"));
  infoLayout->addRow(ramanBtn, new QLabel("—"));
  eigenLayout->addLayout(infoLayout);

  /* Display + scaling */
  m_chkShowEigenvectors = make_check("Display eigenvectors", tab);
  eigenLayout->addWidget(m_chkShowEigenvectors);

  auto *scalingLayout = new QHBoxLayout();
  scalingLayout->addWidget(new QLabel(tr("Eigenvector scaling")));
  m_spinPhononScaling = new QDoubleSpinBox();
  m_spinPhononScaling->setRange(0.1, 9.9);
  m_spinPhononScaling->setSingleStep(0.1);
  scalingLayout->addWidget(m_spinPhononScaling);
  eigenLayout->addLayout(scalingLayout);

  auto *animLayout = new QHBoxLayout();
  animLayout->addWidget(new QLabel(tr("Animation resolution")));
  m_spinAnimResolution = new QSpinBox();
  m_spinAnimResolution->setRange(10, 100);
  m_spinAnimResolution->setSingleStep(1);
  animLayout->addWidget(m_spinAnimResolution);
  eigenLayout->addLayout(animLayout);

  /* Movie name + buttons */
  auto *movieRow = new QHBoxLayout();
  movieRow->addWidget(new QLabel(tr("Movie name")));
  m_editMovieName = new QLineEdit();
  movieRow->addWidget(m_editMovieName, 1);
  auto *cameraBtn = new QPushButton(tr("📷"));
  auto *playBtn = new QPushButton(tr("▶"));
  auto *stopBtn = new QPushButton(tr("■"));
  movieRow->addWidget(cameraBtn);
  movieRow->addWidget(playBtn);
  movieRow->addWidget(stopBtn);
  eigenLayout->addLayout(movieRow);

  layout->addWidget(eigenGroup);
  tabWidget->addTab(tab, tr("Vibrational"));
}

void GulpDialog::populateSolvationTab(QTabWidget *tabWidget)
{
  auto *tab = new QWidget();
  auto *layout = new QVBoxLayout(tab);
  layout->setContentsMargins(8, 6, 8, 6);
  layout->setSpacing(6);

  /* Solvation model */
  auto *modelGroup = new QGroupBox(tr("Solvation model"));
  auto *modelLayout = new QVBoxLayout(modelGroup);
  modelLayout->setSpacing(4);
  m_comboSolvationModel = new QComboBox();
  m_comboSolvationModel->addItem("None");
  m_comboSolvationModel->addItem("COSMIC");
  m_comboSolvationModel->addItem("COSMO");
  modelLayout->addWidget(m_comboSolvationModel);
  layout->addWidget(modelGroup);

  /* Solvent parameters */
  auto *solventGroup = new QGroupBox(tr("Solvent parameters"));
  auto *solventLayout = new QFormLayout(solventGroup);
  solventLayout->setSpacing(4);
  m_spinSolventEpsilon = new QDoubleSpinBox();
  m_spinSolventEpsilon->setRange(1.0, 1000.0);
  m_spinSolventEpsilon->setSingleStep(0.1);
  m_spinSolventRadius = new QDoubleSpinBox();
  m_spinSolventRadius->setRange(0.1, 9.9);
  m_spinSolventRadius->setSingleStep(0.1);
  m_spinSolventDelta = new QDoubleSpinBox();
  m_spinSolventDelta->setRange(0.1, 9.9);
  m_spinSolventDelta->setSingleStep(0.1);
  m_spinSolventRmax = new QDoubleSpinBox();
  m_spinSolventRmax->setRange(1.0, 99.0);
  m_spinSolventRmax->setSingleStep(1.0);
  m_spinSmoothing = new QDoubleSpinBox();
  m_spinSmoothing->setRange(0.0, 2.0);
  m_spinSmoothing->setSingleStep(0.1);
  solventLayout->addRow(tr("Solvent epsilon"), m_spinSolventEpsilon);
  solventLayout->addRow(tr("Solvent radius"), m_spinSolventRadius);
  solventLayout->addRow(tr("Solvent delta"), m_spinSolventDelta);
  solventLayout->addRow(tr("Solvent rmax"), m_spinSolventRmax);
  solventLayout->addRow(tr("Smoothing"), m_spinSmoothing);
  layout->addWidget(solventGroup);

  /* Surface construction geometry */
  auto *surfaceGroup = new QGroupBox(tr("Surface construction"));
  auto *surfaceLayout = new QFormLayout(surfaceGroup);
  surfaceLayout->setSpacing(4);

  m_comboShapeApprox = new QComboBox();
  m_comboShapeApprox->addItem("Octahedron");
  m_comboShapeApprox->addItem("Dodecahedron");
  surfaceLayout->addRow(tr("Shape approximation"), m_comboShapeApprox);

  auto *indicesHBox = new QHBoxLayout();
  indicesHBox->addWidget(new QLabel(tr("Shape indices")));
  m_spinIndexK = new QSpinBox();
  m_spinIndexK->setRange(0, 99);
  indicesHBox->addWidget(m_spinIndexK);
  m_spinIndexL = new QSpinBox();
  m_spinIndexL->setRange(0, 99);
  indicesHBox->addWidget(m_spinIndexL);
  surfaceLayout->addRow(tr(""), indicesHBox);

  auto *segmentsRow = new QHBoxLayout();
  segmentsRow->addWidget(new QLabel(tr("Segments per atom")));
  m_spinSegments = new QSpinBox();
  m_spinSegments->setRange(1, 999);
  segmentsRow->addWidget(m_spinSegments);
  segmentsRow->addWidget(new QLabel(tr("(Points per atom shown at runtime)")));
  surfaceLayout->addRow(tr(""), segmentsRow);

  layout->addWidget(surfaceGroup);
  layout->addStretch();
  tabWidget->addTab(tab, tr("Solvation"));
}

void GulpDialog::populateCommonFrames()
{
  /* Files frame */
  auto *filesHBox = new QHBoxLayout();
  filesHBox->setSpacing(4);
  auto *jobLabel = new QLabel(tr("Job file name"));
  m_editJobName = new QLineEdit();
  filesHBox->addWidget(jobLabel);
  filesHBox->addWidget(m_editJobName, 1);

  auto *filesGroup = new QGroupBox(tr("Files"));
  auto *filesLayout = new QVBoxLayout(filesGroup);
  filesLayout->addLayout(filesHBox);

  /* Details frame */
  auto *detailsHBox = new QHBoxLayout();
  detailsHBox->setSpacing(8);

  auto *leftVBox = new QVBoxLayout();
  leftVBox->setSpacing(2);
  leftVBox->addWidget(new QLabel(tr("Structure name")));
  leftVBox->addWidget(new QLabel(tr("Total energy (eV)")));
  if (m_model->periodic == 2)
  {
    leftVBox->addWidget(new QLabel(tr("Surface bulk energy (eV)")));
    leftVBox->addWidget(new QLabel(tr("Surface dipole")));
    leftVBox->addWidget(new QLabel(tr("Surface energy")));
    leftVBox->addWidget(new QLabel(tr("Attachment energy")));
  }
  detailsHBox->addLayout(leftVBox);

  auto *rightVBox = new QVBoxLayout();
  rightVBox->setSpacing(2);
  auto *nameEdit = new QLineEdit(m_model->basename);
  nameEdit->setReadOnly(true);
  rightVBox->addWidget(nameEdit);

  m_editEnergy = new QLineEdit();
  m_editEnergy->setReadOnly(true);
  rightVBox->addWidget(m_editEnergy);

  if (m_model->periodic == 2)
  {
    m_editSbe = new QLineEdit();
    m_editSbe->setReadOnly(true);
    rightVBox->addWidget(m_editSbe);
    m_editSdipole = new QLineEdit();
    m_editSdipole->setReadOnly(true);
    rightVBox->addWidget(m_editSdipole);
    m_editEsurf = new QLineEdit();
    m_editEsurf->setReadOnly(true);
    rightVBox->addWidget(m_editEsurf);
    m_editEatt = new QLineEdit();
    m_editEatt->setReadOnly(true);
    rightVBox->addWidget(m_editEatt);
  }

  detailsHBox->addLayout(rightVBox);

  auto *detailsGroup = new QGroupBox(tr("Details"));
  auto *detailsLayout = new QVBoxLayout(detailsGroup);
  detailsLayout->addLayout(detailsHBox);

  auto *mainLayout = static_cast<QVBoxLayout *>(layout());
  mainLayout->addWidget(filesGroup);
  mainLayout->addWidget(detailsGroup);

  connect(m_editJobName, &QLineEdit::textChanged, this, &GulpDialog::on_jobname_changed);
}

void GulpDialog::syncFromModel()
{
  /* Run type */
  if (m_model->gulp.run == E_SINGLE)
    m_rbSingle->setChecked(true);
  else if (m_model->gulp.run == E_OPTIMIZE)
    m_rbOptimize->setChecked(true);
  else if (m_model->gulp.run == MD)
    m_rbDynamics->setChecked(true);
  else
    m_rbSingle->setChecked(true);

  /* Constraint */
  if (m_model->gulp.method == CONP)
    m_rbConp->setChecked(true);
  else
    m_rbConv->setChecked(true);

  /* Molecule options */
  if (m_model->gulp.coulomb == MOLE)
    m_rbMole->setChecked(true);
  else if (m_model->gulp.coulomb == MOLMEC)
    m_rbMolmec->setChecked(true);
  else if (m_model->gulp.coulomb == MOLQ)
    m_rbMolq->setChecked(true);
  else if (m_model->gulp.coulomb == NOBUILD)
    m_rbNobuild->setChecked(true);
  else
    m_rbMole->setChecked(true);

  m_chkFix->setChecked(m_model->gulp.fix);
  m_chkNoautobond->setChecked(m_model->gulp.noautobond);

  /* Temperature & Pressure — text fields */
  if (m_model->gulp.temperature)
    m_editTemp->setText(QString::fromUtf8(m_model->gulp.temperature));
  if (m_model->gulp.pressure)
    m_editPressure->setText(QString::fromUtf8(m_model->gulp.pressure));

  /* Dynamics ensemble */
  if (m_model->gulp.ensemble == NVE)
    m_rbNVE->setChecked(true);
  else if (m_model->gulp.ensemble == NVT)
    m_rbNVT->setChecked(true);
  else if (m_model->gulp.ensemble == NPT)
    m_rbNPT->setChecked(true);
  else
    m_rbNVE->setChecked(true);

  /* Dynamics timing */
  if (m_model->gulp.timestep)
    m_editTimestep->setText(m_model->gulp.timestep);
  if (m_model->gulp.equilibration)
    m_editEquilibration->setText(m_model->gulp.equilibration);
  if (m_model->gulp.production)
    m_editProduction->setText(m_model->gulp.production);
  if (m_model->gulp.sample)
    m_editSample->setText(m_model->gulp.sample);
  if (m_model->gulp.write)
    m_editWrite->setText(m_model->gulp.write);

  /* Keyword toggles */
  m_chkNoExec->setChecked(m_model->gulp.no_exec);
  m_chkNosym->setChecked(m_model->gulp.nosym);
  m_chkNoEatt->setChecked(m_model->gulp.no_eatt);
  m_chkQeq->setChecked(m_model->gulp.qeq);

  /* Files tab */
  if (m_model->gulp.temp_file)
    m_editInputFile->setText(m_model->gulp.temp_file);
  if (m_model->gulp.dump_file)
    m_editDumpFile->setText(m_model->gulp.dump_file);
  if (m_model->gulp.trj_file)
    m_editTrajFile->setText(m_model->gulp.trj_file);
  m_chkPrintCharge->setChecked(m_model->gulp.print_charge);

  /* Optimisation tab */
  if (m_model->gulp.optimiser == BFGS_OPT || m_model->gulp.optimiser == -1)
    m_rbBfgsOpt->setChecked(true);
  else if (m_model->gulp.optimiser == CONJ_OPT)
    m_rbConjOpt->setChecked(true);
  else if (m_model->gulp.optimiser == RFO_OPT)
    m_rbRfoOpt->setChecked(true);

  if (m_model->gulp.optimiser2 == SWITCH_OFF || m_model->gulp.optimiser2 == -1)
    m_rbNoOpt2->setChecked(true);
  else if (m_model->gulp.optimiser2 == BFGS_OPT)
    m_rbBfgsOpt2->setChecked(true);
  else if (m_model->gulp.optimiser2 == CONJ_OPT)
    m_rbConjOpt2->setChecked(true);
  else if (m_model->gulp.optimiser2 == RFO_OPT)
    m_rbRfoOpt2->setChecked(true);

  if (m_model->gulp.switch_type == CYCLE)
    m_rbCycle->setChecked(true);
  else
    m_rbGnorm->setChecked(true);

  if (m_model->gulp.switch_value > 0.0)
    m_editSwitchValue->setText(QString::number(m_model->gulp.switch_value, 'f', 6));
  m_spinMaxCyc->setValue(static_cast<int>(m_model->gulp.maxcyc));

  /* Potentials tab */
  if (m_model->gulp.potentials)
    m_editPotentials->setPlainText(m_model->gulp.potentials);
  if (m_model->gulp.libfile)
    m_editLibFile->setText(m_model->gulp.libfile);

  /* Elements tab */
  if (m_model->gulp.elements)
    m_editElements->setPlainText(m_model->gulp.elements);
  if (m_model->gulp.species)
    m_editSpecies->setPlainText(m_model->gulp.species);

  /* Unprocessed tab */
  m_chkOutputExtraKeywords->setChecked(m_model->gulp.output_extra_keywords);
  m_chkOutputExtra->setChecked(m_model->gulp.output_extra);
  if (m_model->gulp.extra_keywords)
    m_editExtraKeywords->setText(m_model->gulp.extra_keywords);
  if (m_model->gulp.extra)
    m_editExtra->setPlainText(m_model->gulp.extra);

  /* Vibrational tab */
  m_chkPhonon->setChecked(m_model->gulp.phonon);
  m_chkEigen->setChecked(m_model->gulp.eigen);
  if (m_editKpoints && m_model->gulp.kpoints && m_model->gulp.kpoints[0])
    m_editKpoints->setPlainText(m_model->gulp.kpoints);
  m_chkShowEigenvectors->setChecked(m_model->show_eigenvectors);

  extern struct sysenv_pak sysenv;
  m_spinPhononScaling->setValue(sysenv.render.phonon_scaling);
  m_spinAnimResolution->setValue(static_cast<int>(m_model->pulse_max));
  if (m_model->phonon_movie_name)
    m_editMovieName->setText(m_model->phonon_movie_name);

  /* Solvation tab — map enum values to combo index */
  switch (m_model->gulp.solvation_model)
  {
  case GULP_SOLVATION_COSMIC:
    m_comboSolvationModel->setCurrentIndex(1);
    break;
  case GULP_SOLVATION_COSMO:
    m_comboSolvationModel->setCurrentIndex(2);
    break;
  default:
    m_comboSolvationModel->setCurrentIndex(0);
    break;
  }

  m_spinSolventEpsilon->setValue(m_model->gulp.cosmo_solvent_epsilon);
  m_spinSolventRadius->setValue(m_model->gulp.cosmo_solvent_radius);
  m_spinSolventDelta->setValue(m_model->gulp.cosmo_solvent_delta);
  m_spinSolventRmax->setValue(m_model->gulp.cosmo_solvent_rmax);
  m_spinSmoothing->setValue(m_model->gulp.cosmo_smoothing);

  /* Common frames */
  if (m_model->gulp.temp_file)
  {
    QString jobName = QFileInfo(QString::fromUtf8(m_model->gulp.temp_file)).baseName();
    m_editJobName->setText(jobName);
  }
}

void GulpDialog::syncEnergyToUI()
{
  if (!m_editEnergy)
    return;
  m_editEnergy->setText(QString::number(m_model->gulp.energy, 'f', 6));
  if (m_model->periodic == 2)
  {
    if (m_editSbe)
      m_editSbe->setText(QString::number(m_model->gulp.esurf[0], 'f', 6));
    if (m_editSdipole)
      m_editSdipole->setText(QString::number(m_model->gulp.sdipole, 'f', 6) + " e.Angs");
    if (m_editEsurf)
      m_editEsurf->setText(QString::number(m_model->gulp.esurf[0], 'f', 6) + "    " + m_model->gulp.esurf_units);
    if (m_editEatt)
      m_editEatt->setText(QString::number(m_model->gulp.eatt[0], 'f', 6) + "    " + m_model->gulp.eatt_units);
  }
}

void GulpDialog::syncToModel()
{
  /* Run type */
  if (m_rbSingle->isChecked())
    m_model->gulp.run = E_SINGLE;
  else if (m_rbOptimize->isChecked())
    m_model->gulp.run = E_OPTIMIZE;
  else if (m_rbDynamics->isChecked())
    m_model->gulp.run = MD;

  /* Constraint */
  if (m_rbConp->isChecked())
    m_model->gulp.method = CONP;
  else
    m_model->gulp.method = CONV;

  /* Molecule options */
  if (m_rbMole->isChecked())
    m_model->gulp.coulomb = MOLE;
  else if (m_rbMolmec->isChecked())
    m_model->gulp.coulomb = MOLMEC;
  else if (m_rbMolq->isChecked())
    m_model->gulp.coulomb = MOLQ;
  else if (m_rbNobuild->isChecked())
    m_model->gulp.coulomb = NOBUILD;

  m_model->gulp.fix = m_chkFix->isChecked();
  m_model->gulp.noautobond = m_chkNoautobond->isChecked();

  /* Temperature & Pressure — text to string */
  g_free(m_model->gulp.temperature);
  m_model->gulp.temperature = g_strdup(m_editTemp->text().toUtf8().constData());
  g_free(m_model->gulp.pressure);
  m_model->gulp.pressure = g_strdup(m_editPressure->text().toUtf8().constData());

  /* Dynamics ensemble */
  if (m_rbNVE->isChecked())
    m_model->gulp.ensemble = NVE;
  else if (m_rbNVT->isChecked())
    m_model->gulp.ensemble = NVT;
  else if (m_rbNPT->isChecked())
    m_model->gulp.ensemble = NPT;

  /* Dynamics timing */
  g_free(m_model->gulp.timestep);
  m_model->gulp.timestep = g_strdup(m_editTimestep->text().toUtf8().constData());
  g_free(m_model->gulp.equilibration);
  m_model->gulp.equilibration = g_strdup(m_editEquilibration->text().toUtf8().constData());
  g_free(m_model->gulp.production);
  m_model->gulp.production = g_strdup(m_editProduction->text().toUtf8().constData());
  g_free(m_model->gulp.sample);
  m_model->gulp.sample = g_strdup(m_editSample->text().toUtf8().constData());
  g_free(m_model->gulp.write);
  m_model->gulp.write = g_strdup(m_editWrite->text().toUtf8().constData());

  /* Keyword toggles */
  m_model->gulp.no_exec = m_chkNoExec->isChecked();
  m_model->gulp.nosym = m_chkNosym->isChecked();
  m_model->gulp.no_eatt = m_chkNoEatt->isChecked();
  m_model->gulp.qeq = m_chkQeq->isChecked();

  /* Files tab */
  g_free(m_model->gulp.temp_file);
  m_model->gulp.temp_file = g_strdup(m_editInputFile->text().toUtf8().constData());
  g_free(m_model->gulp.dump_file);
  m_model->gulp.dump_file = g_strdup(m_editDumpFile->text().toUtf8().constData());
  g_free(m_model->gulp.trj_file);
  m_model->gulp.trj_file = g_strdup(m_editTrajFile->text().toUtf8().constData());
  m_model->gulp.print_charge = m_chkPrintCharge->isChecked();

  /* Optimisation tab — preserve original value; don't force bfgs when default (-1) */
  if (m_model->gulp.optimiser != -1)
  {
    if (m_rbBfgsOpt->isChecked())
      m_model->gulp.optimiser = BFGS_OPT;
    else if (m_rbConjOpt->isChecked())
      m_model->gulp.optimiser = CONJ_OPT;
    else if (m_rbRfoOpt->isChecked())
      m_model->gulp.optimiser = RFO_OPT;
  }

  if (m_rbNoOpt2->isChecked())
    m_model->gulp.optimiser2 = SWITCH_OFF;
  else if (m_rbBfgsOpt2->isChecked())
    m_model->gulp.optimiser2 = BFGS_OPT;
  else if (m_rbConjOpt2->isChecked())
    m_model->gulp.optimiser2 = CONJ_OPT;
  else if (m_rbRfoOpt2->isChecked())
    m_model->gulp.optimiser2 = RFO_OPT;

  if (m_rbCycle->isChecked())
    m_model->gulp.switch_type = CYCLE;
  else
    m_model->gulp.switch_type = GNORM;

  bool ok = false;
  m_model->gulp.switch_value = m_editSwitchValue->text().toDouble(&ok);
  if (!ok)
    m_model->gulp.switch_value = 1.0;
  m_model->gulp.maxcyc = static_cast<gdouble>(m_spinMaxCyc->value());

  /* Potentials tab */
  g_free(m_model->gulp.potentials);
  m_model->gulp.potentials = g_strdup(m_editPotentials->toPlainText().toUtf8().constData());
  g_free(m_model->gulp.libfile);
  m_model->gulp.libfile = g_strdup(m_editLibFile->text().toUtf8().constData());

  /* Elements tab */
  g_free(m_model->gulp.elements);
  m_model->gulp.elements = g_strdup(m_editElements->toPlainText().toUtf8().constData());
  g_free(m_model->gulp.species);
  m_model->gulp.species = g_strdup(m_editSpecies->toPlainText().toUtf8().constData());

  /* Unprocessed tab */
  m_model->gulp.output_extra_keywords = m_chkOutputExtraKeywords->isChecked();
  m_model->gulp.output_extra = m_chkOutputExtra->isChecked();
  g_free(m_model->gulp.extra_keywords);
  m_model->gulp.extra_keywords = g_strdup(m_editExtraKeywords->text().toUtf8().constData());
  g_free(m_model->gulp.extra);
  m_model->gulp.extra = g_strdup(m_editExtra->toPlainText().toUtf8().constData());

  /* Vibrational tab */
  m_model->gulp.phonon = m_chkPhonon->isChecked();
  m_model->gulp.eigen = m_chkEigen->isChecked();
  g_free(m_model->gulp.kpoints);
  m_model->gulp.kpoints = g_strdup(m_editKpoints->toPlainText().toUtf8().constData());
  m_model->show_eigenvectors = m_chkShowEigenvectors->isChecked();

  extern struct sysenv_pak sysenv;
  sysenv.render.phonon_scaling = m_spinPhononScaling->value();
  m_model->pulse_max = static_cast<gdouble>(m_spinAnimResolution->value());
  g_free(m_model->phonon_movie_name);
  m_model->phonon_movie_name = g_strdup(m_editMovieName->text().toUtf8().constData());

  /* Solvation tab — map combo index to enum values */
  switch (m_comboSolvationModel->currentIndex())
  {
  case 0:
    m_model->gulp.solvation_model = GULP_SOLVATION_NONE;
    break;
  case 1:
    m_model->gulp.solvation_model = GULP_SOLVATION_COSMIC;
    break;
  case 2:
    m_model->gulp.solvation_model = GULP_SOLVATION_COSMO;
    break;
  default:
    m_model->gulp.solvation_model = GULP_SOLVATION_NONE;
    break;
  }
  m_model->gulp.cosmo_solvent_epsilon = m_spinSolventEpsilon->value();
  m_model->gulp.cosmo_solvent_radius = m_spinSolventRadius->value();
  m_model->gulp.cosmo_solvent_delta = m_spinSolventDelta->value();
  m_model->gulp.cosmo_solvent_rmax = m_spinSolventRmax->value();
  m_model->gulp.cosmo_smoothing = m_spinSmoothing->value();
}

void GulpDialog::on_execute()
{
  syncToModel();

  /* Store only the basename — write_gulp builds the full path */
  if (!m_editJobName->text().isEmpty())
  {
    g_free(m_model->gulp.temp_file);
    m_model->gulp.temp_file = g_strdup((m_editJobName->text() + ".gin").toUtf8().constData());
  }

  {
    extern struct sysenv_pak sysenv;
    fprintf(stderr, "[GULP] temp_file=%s out_file=%s gulp_path=%s\n",
            m_model->gulp.temp_file ? m_model->gulp.temp_file : "(null)",
            m_model->gulp.out_file ? m_model->gulp.out_file : "(null)", sysenv.gulp_path ? sysenv.gulp_path : "(null)");
  }

  extern void qt_gulp_task(struct model_pak *);
  qt_gulp_task(m_model);
}

void GulpDialog::on_close()
{
  syncToModel();
  /* Unregister from energy update callbacks */
  for (int i = 0; i < g_gulp_dialog_count; i++)
  {
    if (g_gulp_dialog_models[i] == m_model)
    {
      g_gulp_dialog_count--;
      for (int j = i; j < g_gulp_dialog_count; j++)
      {
        g_gulp_dialog_models[j] = g_gulp_dialog_models[j + 1];
        g_gulp_dialogs[j] = g_gulp_dialogs[j + 1];
      }
      break;
    }
  }
  close();
}

void GulpDialog::on_jobname_changed() {}

/* Bridge function */
extern "C" void qt_show_gulp_dialog(void)
{
  extern struct sysenv_pak sysenv;
  extern QWidget *get_main_window_widget();
  extern void gulp_files_init(struct model_pak *);
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (!model)
    return;

  gulp_files_init(model);

  /* Clear dialog pointer */
  model->gulp.dialog = NULL;

  GulpDialog *dlg = new GulpDialog(model, get_main_window_widget());
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  /* Register for energy update callbacks */
  if (g_gulp_dialog_count < 4096)
  {
    g_gulp_dialog_models[g_gulp_dialog_count] = model;
    g_gulp_dialogs[g_gulp_dialog_count] = dlg;
    g_gulp_dialog_count++;
  }
  dlg->show();
}
