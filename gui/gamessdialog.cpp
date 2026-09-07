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
 * GAMESS Configuration Dialog for GDIS Qt6 GUI
 */

#include "gamessdialog.h"
#include "gdis_api.h"

#include "gdis.h"
#include "gamess.h"
#include "file.h"
#include "parse.h"

#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>

extern struct sysenv_pak sysenv;

/* GAMESS task bridge */
extern "C" void qt_gamess_task(struct model_pak *);

/* Basis set definitions (from gamess.c / pak.h) */
static const struct {
  const char *label;
  int basis;
  int ngauss;
} basis_sets[] = {{"User Defined", GMS_USER, 0},
                  {"MNDO", GMS_MNDO, 0},
                  {"AM1", GMS_AM1, 0},
                  {"PM3", GMS_PM3, 0},
                  {"MINI", GMS_MINI, 0},
                  {"MIDI", GMS_MIDI, 0},
                  {"STO-2G", GMS_STO, 2},
                  {"STO-3G", GMS_STO, 3},
                  {"STO-4G", GMS_STO, 4},
                  {"STO-5G", GMS_STO, 5},
                  {"STO-6G", GMS_STO, 6},
                  {"3-21G", GMS_N21, 3},
                  {"6-21G", GMS_N21, 6},
                  {"4-31G", GMS_N31, 4},
                  {"5-31G", GMS_N31, 5},
                  {"6-31G", GMS_N31, 6},
                  {"6-311G", GMS_N311, 6},
                  {"DZV", GMS_DZV, 0},
                  {"DH", GMS_DH, 0},
                  {"TZV", GMS_TZV, 0},
                  {"MC", GMS_MC, 0},
                  {nullptr, 0, 0}};

GamessDialog::GamessDialog(struct model_pak *model, QWidget *parent) : QDialog(parent), m_model(model)
{
  setWindowTitle(tr("GAMESS configuration"));
  resize(700, 550);

  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(8, 6, 8, 6);
  mainLayout->setSpacing(6);

  auto *notebook = new QTabWidget(this);
  m_notebook = notebook;
  notebook->setTabPosition(QTabWidget::North);
  notebook->setDocumentMode(true);
  mainLayout->addWidget(notebook);

  setupControlPage();
  setupBasisPage();
  setupOptimisationPage();

  /* Files section — outside tabs */
  {
    auto *frame = new QGroupBox(tr("Files"), this);
    auto *form = new QFormLayout(frame);
    form->setSpacing(4);

    auto *edit = new QLineEdit();
    edit->setText(QString::fromUtf8(m_model->gamess.temp_file));
    m_tempFileEdit = edit;
    form->addRow(tr("Job input file"), edit);
    connect(edit, &QLineEdit::textChanged, this, [this](const QString &t) {
      g_free(m_model->gamess.temp_file);
      m_model->gamess.temp_file = g_strdup(t.toUtf8().constData());
    });
    mainLayout->addWidget(frame);
  }

  /* Details section — outside tabs */
  {
    auto *frame = new QGroupBox(tr("Details"), this);
    auto *form = new QFormLayout(frame);
    form->setSpacing(4);

    auto *titleEdit = new QLineEdit();
    titleEdit->setText(QString::fromUtf8(m_model->gamess.title));
    m_titleEdit = titleEdit;
    form->addRow(tr("Title"), titleEdit);
    connect(titleEdit, &QLineEdit::textChanged, this, [this](const QString &t) {
      g_free(m_model->gamess.title);
      m_model->gamess.title = g_strdup(t.toUtf8().constData());
    });

    auto *energyEdit = new QLineEdit();
    energyEdit->setReadOnly(true);
    m_energyEdit = energyEdit;
    form->addRow(tr("Total energy (Hartree)"), energyEdit);

    mainLayout->addWidget(frame);
  }

  /* Button bar */
  auto *btnLayout = new QHBoxLayout();
  btnLayout->addStretch();

  auto *runBtn = new QPushButton(tr("Run"), this);
  auto *closeBtn = new QPushButton(tr("Close"), this);
  btnLayout->addWidget(runBtn);
  btnLayout->addWidget(closeBtn);
  mainLayout->addLayout(btnLayout);

  connect(runBtn, &QPushButton::clicked, this, &GamessDialog::on_run);
  connect(closeBtn, &QPushButton::clicked, this, &GamessDialog::on_close);

  refresh();
}

GamessDialog::~GamessDialog() {}

void GamessDialog::refresh()
{
  if (!m_model)
    return;

  /* Control page — exe_type uses radio buttons */
  {
    int exeIdx = 0;
    if (m_model->gamess.exe_type == GMS_CHECK)
      exeIdx = 1;
    else if (m_model->gamess.exe_type == GMS_DEBUG)
      exeIdx = 2;
    if (exeIdx == 0 && m_exeTypeRunRadio)
      m_exeTypeRunRadio->setChecked(true);
    if (exeIdx == 1 && m_exeTypeCheckRadio)
      m_exeTypeCheckRadio->setChecked(true);
    if (exeIdx == 2 && m_exeTypeDebugRadio)
      m_exeTypeDebugRadio->setChecked(true);
  }

  int runIdx = 0;
  switch (m_model->gamess.run_type)
  {
  case GMS_GRADIENT:
    runIdx = 1;
    break;
  case GMS_HESSIAN:
    runIdx = 2;
    break;
  case GMS_OPTIMIZE:
    runIdx = 3;
    break;
  default:
    runIdx = 0;
    break;
  }
  m_runTypeCombo->setCurrentIndex(runIdx);

  int scfIdx = 0;
  if (m_model->gamess.scf_type == GMS_UHF)
    scfIdx = 1;
  else if (m_model->gamess.scf_type == GMS_ROHF)
    scfIdx = 2;
  m_scfTypeCombo->setCurrentIndex(scfIdx);

  int unitIdx = (m_model->gamess.units == GMS_BOHR) ? 1 : 0;
  m_unitsCombo->setCurrentIndex(unitIdx);

  m_maxitSpin->setValue((int) m_model->gamess.maxit);

  m_dftCheck->setChecked(m_model->gamess.dft);
  on_dft_toggled(m_model->gamess.dft);

  int funcIdx = 0;
  if (m_model->gamess.dft_functional == BLYP)
    funcIdx = 1;
  else if (m_model->gamess.dft_functional == B3LYP)
    funcIdx = 2;
  m_functionalCombo->setCurrentIndex(funcIdx);

  m_timeLimitSpin->setValue((int) m_model->gamess.time_limit);
  m_mwordsSpin->setValue((int) m_model->gamess.mwords);
  m_wideOutputCheck->setChecked(m_model->gamess.wide_output);

  m_totalChargeSpin->setValue((int) m_model->gamess.total_charge);
  m_multiplicitySpin->setValue((int) m_model->gamess.multiplicity);

  /* Basis page */
  int basisIdx = 0;
  for (int i = 0; basis_sets[i].label; i++)
  {
    if (m_model->gamess.basis == basis_sets[i].basis && m_model->gamess.ngauss == basis_sets[i].ngauss)
    {
      basisIdx = i;
      break;
    }
  }
  m_basisCombo->setCurrentIndex(basisIdx);

  m_numPSpin->setValue((int) m_model->gamess.num_p);
  m_numDSpin->setValue((int) m_model->gamess.num_d);
  m_numFSpin->setValue((int) m_model->gamess.num_f);
  m_heavyDiffuseCheck->setChecked(m_model->gamess.have_heavy_diffuse);
  m_hydrogenDiffuseCheck->setChecked(m_model->gamess.have_hydrogen_diffuse);

  /* Optimisation page */
  int optIdx = 0;
  if (m_model->gamess.opt_type == GMS_NR)
    optIdx = 1;
  else if (m_model->gamess.opt_type == GMS_RFO)
    optIdx = 2;
  else if (m_model->gamess.opt_type == GMS_SCHLEGEL)
    optIdx = 3;
  m_optTypeCombo->setCurrentIndex(optIdx);
  m_nstepSpin->setValue((int) m_model->gamess.nstep);

  /* Files page */
  m_tempFileEdit->setText(QString::fromUtf8(m_model->gamess.temp_file));

  /* Details page */
  if (g_ascii_strncasecmp(m_model->gamess.title, "none", 4) == 0)
    g_free(m_model->gamess.title), m_model->gamess.title = g_strdup(m_model->basename);
  m_titleEdit->setText(QString::fromUtf8(m_model->gamess.title));

  if (m_model->gamess.have_energy)
    m_energyEdit->setText(QString::number(m_model->gamess.energy, 'f', 6));
  else
    m_energyEdit->setText(tr("not yet calculated"));
  m_energyEdit->setReadOnly(true);
}

void GamessDialog::setupControlPage()
{
  auto *page = new QWidget();
  auto *mainLayout = new QVBoxLayout(page);
  mainLayout->setSpacing(8);

  /* Three-column layout */
  auto *hbox = new QHBoxLayout();
  auto *col1 = new QVBoxLayout();
  auto *col2 = new QVBoxLayout();
  auto *col3 = new QVBoxLayout();

  /* --- Column 1: Execution type, Run type --- */
  {
    auto *frame = new QGroupBox(tr("Execution type"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    auto *runRadio = new QRadioButton(tr("Run"));
    auto *checkRadio = new QRadioButton(tr("Check"));
    auto *debugRadio = new QRadioButton(tr("Debug"));
    vbox->addWidget(runRadio);
    vbox->addWidget(checkRadio);
    vbox->addWidget(debugRadio);
    col1->addWidget(frame);

    m_exeTypeRunRadio = runRadio;
    m_exeTypeCheckRadio = checkRadio;
    m_exeTypeDebugRadio = debugRadio;
    m_exeTypeCombo = nullptr; /* using radio buttons instead */
    connect(runRadio, &QRadioButton::toggled, this, [this]() {
      if (m_exeTypeCombo)
        return; /* skip if combo was set */
      m_model->gamess.exe_type = GMS_RUN;
    });
    connect(checkRadio, &QRadioButton::toggled, this, [this]() { m_model->gamess.exe_type = GMS_CHECK; });
    connect(debugRadio, &QRadioButton::toggled, this, [this]() { m_model->gamess.exe_type = GMS_DEBUG; });
  }

  {
    auto *frame = new QGroupBox(tr("Run type"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    auto *combo = new QComboBox();
    combo->addItems({tr("Single point"), tr("Gradient"), tr("Hessian"), tr("Optimize")});
    m_runTypeCombo = combo;
    vbox->addWidget(combo);
    col1->addWidget(frame);

    connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GamessDialog::on_run_type_changed);
  }

  /* --- Column 2: SCF type, Units, SCF options --- */
  {
    auto *frame = new QGroupBox(tr("SCF type"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    auto *combo = new QComboBox();
    combo->addItems({tr("RHF"), tr("UHF"), tr("ROHF")});
    m_scfTypeCombo = combo;
    vbox->addWidget(combo);
    col2->addWidget(frame);

    connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GamessDialog::on_scf_type_changed);
  }

  {
    auto *frame = new QGroupBox(tr("Input units"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    auto *combo = new QComboBox();
    combo->addItems({tr("Angstrom"), tr("Bohr")});
    m_unitsCombo = combo;
    vbox->addWidget(combo);
    col2->addWidget(frame);

    connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GamessDialog::on_units_changed);
  }

  {
    auto *frame = new QGroupBox(tr("SCF options"), page);
    auto *form = new QFormLayout(frame);
    form->setSpacing(4);

    auto *spin = new QSpinBox();
    spin->setRange(1, 150);
    spin->setValue((int) m_model->gamess.maxit);
    m_maxitSpin = spin;
    form->addRow(tr("Maximum iterations"), spin);
    connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) { m_model->gamess.maxit = v; });
    col2->addWidget(frame);
  }

  /* --- Column 3: DFT, Run control, Electronic config --- */
  {
    auto *frame = new QGroupBox(tr("DFT"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    auto *check = new QCheckBox(tr("DFT calculation"));
    check->setChecked(m_model->gamess.dft);
    m_dftCheck = check;
    vbox->addWidget(check);

    auto *hbox2 = new QHBoxLayout();
    hbox2->addWidget(new QLabel(tr("Functional")));
    auto *combo = new QComboBox();
    combo->addItems({tr("SVWN (LDA)"), tr("BLYP"), tr("B3LYP")});
    m_functionalCombo = combo;
    hbox2->addWidget(combo);
    vbox->addLayout(hbox2);

    col3->addWidget(frame);

    connect(check, &QCheckBox::toggled, this, [this](bool checked) {
      m_model->gamess.dft = checked;
      on_dft_toggled(checked);
    });
    connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
      m_model->gamess.dft = true;
      if (idx == 1)
        m_model->gamess.dft_functional = BLYP;
      else if (idx == 2)
        m_model->gamess.dft_functional = B3LYP;
      else
        m_model->gamess.dft_functional = SVWN;
    });
  }

  /* Fix: remove the duplicate check */
  {
    auto *frame = new QGroupBox(tr("Run control"), page);
    auto *form = new QFormLayout(frame);
    form->setSpacing(4);

    auto *timeSpin = new QSpinBox();
    timeSpin->setRange(0, 30000);
    timeSpin->setValue((int) m_model->gamess.time_limit);
    timeSpin->setSingleStep(10);
    m_timeLimitSpin = timeSpin;
    form->addRow(tr("Time limit (min)"), timeSpin);
    connect(timeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int v) { m_model->gamess.time_limit = v; });

    auto *memSpin = new QSpinBox();
    memSpin->setRange(1, 150);
    memSpin->setValue((int) m_model->gamess.mwords);
    m_mwordsSpin = memSpin;
    form->addRow(tr("Memory (megawords)"), memSpin);
    connect(memSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) { m_model->gamess.mwords = v; });

    auto *wideCheck = new QCheckBox(tr("Wide output"));
    wideCheck->setChecked(m_model->gamess.wide_output);
    m_wideOutputCheck = wideCheck;
    form->addRow("", wideCheck);
    connect(wideCheck, &QCheckBox::toggled, this, [this](bool v) { m_model->gamess.wide_output = v; });

    col3->addWidget(frame);
  }

  {
    auto *frame = new QGroupBox(tr("Electronic configuration"), page);
    auto *form = new QFormLayout(frame);
    form->setSpacing(4);

    auto *chargeSpin = new QSpinBox();
    chargeSpin->setRange(-5, 5);
    chargeSpin->setValue((int) m_model->gamess.total_charge);
    m_totalChargeSpin = chargeSpin;
    form->addRow(tr("Total charge"), chargeSpin);
    connect(chargeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int v) { m_model->gamess.total_charge = v; });

    auto *multSpin = new QSpinBox();
    multSpin->setRange(1, 5);
    multSpin->setValue((int) m_model->gamess.multiplicity);
    m_multiplicitySpin = multSpin;
    form->addRow(tr("Multiplicity"), multSpin);
    connect(multSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int v) { m_model->gamess.multiplicity = v; });

    col3->addWidget(frame);
  }

  hbox->addLayout(col1, 1);
  hbox->addLayout(col2, 1);
  hbox->addLayout(col3, 1);
  mainLayout->addLayout(hbox);

  m_notebook->addTab(page, tr("Control"));
}

void GamessDialog::setupBasisPage()
{
  auto *page = new QWidget();
  auto *mainLayout = new QHBoxLayout(page);
  mainLayout->setSpacing(8);

  /* Left: Basis set combo */
  {
    auto *frame = new QGroupBox(tr("Basis set"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    auto *combo = new QComboBox();
    for (int i = 0; basis_sets[i].label; i++)
      combo->addItem(QString::fromUtf8(basis_sets[i].label));
    m_basisCombo = combo;
    vbox->addWidget(combo);

    connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GamessDialog::on_basis_changed);
    mainLayout->addWidget(frame, 1);
  }

  /* Right: Polarization and diffuse functions */
  {
    auto *vbox = new QVBoxLayout();
    vbox->setSpacing(8);

    auto *polFrame = new QGroupBox(tr("Polarization functions"), page);
    auto *polForm = new QFormLayout(polFrame);
    polForm->setSpacing(4);

    auto *pSpin = new QSpinBox();
    pSpin->setRange(0, 3);
    m_numPSpin = pSpin;
    polForm->addRow(tr("p"), pSpin);
    connect(pSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) { m_model->gamess.num_p = v; });

    auto *dSpin = new QSpinBox();
    dSpin->setRange(0, 3);
    m_numDSpin = dSpin;
    polForm->addRow(tr("d"), dSpin);
    connect(dSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) { m_model->gamess.num_d = v; });

    auto *fSpin = new QSpinBox();
    fSpin->setRange(0, 1);
    m_numFSpin = fSpin;
    polForm->addRow(tr("f"), fSpin);
    connect(fSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) { m_model->gamess.num_f = v; });

    vbox->addWidget(polFrame);

    auto *diffFrame = new QGroupBox(tr("Diffuse functions"), page);
    auto *diffVbox = new QVBoxLayout(diffFrame);
    diffVbox->setSpacing(4);

    auto *heavyCheck = new QCheckBox(tr("Heavy atoms (s & p)"));
    heavyCheck->setChecked(m_model->gamess.have_heavy_diffuse);
    m_heavyDiffuseCheck = heavyCheck;
    diffVbox->addWidget(heavyCheck);
    connect(heavyCheck, &QCheckBox::toggled, this, [this](bool v) { m_model->gamess.have_heavy_diffuse = v; });

    auto *hCheck = new QCheckBox(tr("Hydrogen (s only)"));
    hCheck->setChecked(m_model->gamess.have_hydrogen_diffuse);
    m_hydrogenDiffuseCheck = hCheck;
    diffVbox->addWidget(hCheck);
    connect(hCheck, &QCheckBox::toggled, this, [this](bool v) { m_model->gamess.have_hydrogen_diffuse = v; });

    vbox->addWidget(diffFrame);
    mainLayout->addLayout(vbox, 1);
  }

  m_notebook->addTab(page, tr("Basis Set"));
}

void GamessDialog::setupOptimisationPage()
{
  auto *page = new QWidget();
  auto *mainLayout = new QVBoxLayout(page);
  mainLayout->setSpacing(8);

  auto *hbox = new QHBoxLayout();

  {
    auto *frame = new QGroupBox(tr("Optimiser"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    auto *combo = new QComboBox();
    combo->addItems({tr("Quadratic approximation"), tr("Newton-Raphson"), tr("RFO"), tr("Quasi-NR")});
    m_optTypeCombo = combo;
    vbox->addWidget(combo);

    connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GamessDialog::on_opt_type_changed);
    hbox->addWidget(frame, 1);
  }

  {
    auto *frame = new QGroupBox(tr("Optimization cycles"), page);
    auto *form = new QFormLayout(frame);
    form->setSpacing(4);

    auto *spin = new QSpinBox();
    spin->setRange(0, 500);
    spin->setValue((int) m_model->gamess.nstep);
    spin->setSingleStep(10);
    m_nstepSpin = spin;
    form->addRow(tr("Maximum cycles"), spin);
    connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) { m_model->gamess.nstep = v; });
    hbox->addWidget(frame, 1);
  }

  mainLayout->addLayout(hbox);

  m_notebook->addTab(page, tr("Optimisation"));
}

void GamessDialog::on_run()
{
  /* Save current settings to model */
  if (!m_model)
    return;

  /* Save file name */
  g_free(m_model->gamess.temp_file);
  m_model->gamess.temp_file = g_strdup(m_tempFileEdit->text().toUtf8().constData());

  /* Run GAMESS */
  if (!sysenv.gamess_path)
  {
    QMessageBox::warning(this, tr("GAMESS"),
                         tr("GAMESS executable was not found.\n"
                            "Set the path in View > Executable paths."));
    return;
  }

  qt_gamess_task(m_model);

  close();
}

void GamessDialog::on_close()
{
  /* Save file name */
  if (m_model)
  {
    g_free(m_model->gamess.temp_file);
    m_model->gamess.temp_file = g_strdup(m_tempFileEdit->text().toUtf8().constData());
  }
  close();
}

void GamessDialog::on_dft_toggled(bool checked)
{
  if (m_functionalCombo)
    m_functionalCombo->setEnabled(checked);
}

void GamessDialog::on_run_type_changed(int index)
{
  if (!m_model)
    return;
  switch (index)
  {
  case 0:
    m_model->gamess.run_type = GMS_ENERGY;
    break;
  case 1:
    m_model->gamess.run_type = GMS_GRADIENT;
    break;
  case 2:
    m_model->gamess.run_type = GMS_HESSIAN;
    break;
  case 3:
    m_model->gamess.run_type = GMS_OPTIMIZE;
    break;
  default:
    m_model->gamess.run_type = GMS_ENERGY;
    break;
  }
}

void GamessDialog::on_scf_type_changed(int index)
{
  if (!m_model)
    return;
  switch (index)
  {
  case 0:
    m_model->gamess.scf_type = GMS_RHF;
    break;
  case 1:
    m_model->gamess.scf_type = GMS_UHF;
    break;
  case 2:
    m_model->gamess.scf_type = GMS_ROHF;
    break;
  default:
    m_model->gamess.scf_type = GMS_RHF;
    break;
  }
}

void GamessDialog::on_units_changed(int index)
{
  if (!m_model)
    return;
  m_model->gamess.units = (index == 0) ? GMS_ANGS : GMS_BOHR;
}

void GamessDialog::on_basis_changed(int index)
{
  if (!m_model)
    return;
  for (int i = 0; basis_sets[i].label; i++)
  {
    if (i == index)
    {
      m_model->gamess.basis = (GMSBasisType) basis_sets[i].basis;
      m_model->gamess.ngauss = basis_sets[i].ngauss;
      break;
    }
  }
}

void GamessDialog::on_opt_type_changed(int index)
{
  if (!m_model)
    return;
  switch (index)
  {
  case 0:
    m_model->gamess.opt_type = GMS_QA;
    break;
  case 1:
    m_model->gamess.opt_type = GMS_NR;
    break;
  case 2:
    m_model->gamess.opt_type = GMS_RFO;
    break;
  case 3:
    m_model->gamess.opt_type = GMS_SCHLEGEL;
    break;
  default:
    m_model->gamess.opt_type = GMS_QA;
    break;
  }
}

/* Bridge function — called from mainwindow.cpp */
extern "C" void qt_show_gamess_dialog(struct model_pak *model)
{
  extern QWidget *get_main_window_widget();
  GamessDialog *dlg = new GamessDialog(model, get_main_window_widget());
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->show();
  dlg->raise();
  dlg->activateWindow();
}
