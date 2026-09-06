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
 * SIESTA Configuration Dialog for GDIS Qt6 GUI
 * SIESTA setup dialog with 5 tabs:
 * File Handler, Electronic Structure, SCF, Geometry, File I/O
 */

#include "siestadialog.h"
#include "gdis_api.h"

#include "gdis.h"
#include "file.h"
#include "parse.h"
#include "task.h"

#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>

/* Bridge functions from qt_api.c (C linkage) */
extern "C" void qt_siesta_task(struct model_pak *);

extern struct sysenv_pak sysenv;

SiestaDialog::SiestaDialog(struct model_pak *model, QWidget *parent) : QDialog(parent), m_model(model)
{
  setWindowTitle(tr("Siesta Setup"));
  resize(800, 600);

  m_mainLayout = new QVBoxLayout(this);
  m_mainLayout->setContentsMargins(8, 6, 8, 6);
  m_mainLayout->setSpacing(6);

  /* File handler — outside tabs, at top of dialog */
  setupFileHandlerPage();

  m_notebook = new QTabWidget(this);
  m_notebook->setTabPosition(QTabWidget::North);
  m_notebook->setDocumentMode(true);
  m_mainLayout->addWidget(m_notebook);

  setupElectronicStructurePage();
  setupSCFPage();
  setupGeometryPage();
  setupFileIOPage();

  /* Button bar */
  auto *btnLayout = new QHBoxLayout();
  btnLayout->addStretch();

  auto *saveBtn = new QPushButton(tr("Save"), this);
  auto *runBtn = new QPushButton(tr("Run"), this);
  auto *closeBtn = new QPushButton(tr("Close"), this);
  btnLayout->addWidget(saveBtn);
  btnLayout->addWidget(runBtn);
  btnLayout->addWidget(closeBtn);
  m_mainLayout->addLayout(btnLayout);

  connect(saveBtn, &QPushButton::clicked, this, &SiestaDialog::on_save);
  connect(runBtn, &QPushButton::clicked, this, &SiestaDialog::on_run);
  connect(closeBtn, &QPushButton::clicked, this, &SiestaDialog::on_close);

  refresh();
}

SiestaDialog::~SiestaDialog() {}

void SiestaDialog::refresh()
{
  if (!m_model)
    return;

  /* File handler */
  m_writeCsvCheck->setChecked(false); /* siestafileWRITE is a global */
  m_filenameEdit->setText(QString::fromUtf8(m_model->siesta.modelfilename ? m_model->siesta.modelfilename : ""));

  /* Electronic Structure */
  int basisIdx = 0;
  auto bs = m_model->siesta.basis_set;
  switch (bs)
  {
  case siesta_pak::DZ_ZETA:
    basisIdx = 1;
    break;
  case siesta_pak::SZP_ZETA:
    basisIdx = 2;
    break;
  case siesta_pak::DZP_ZETA:
    basisIdx = 3;
    break;
  case siesta_pak::CUSTOM_ZETA:
    basisIdx = 4;
    break;
  default:
    basisIdx = 0;
    break;
  }
  m_basisSetCombo->setCurrentIndex(basisIdx);
  on_basis_set_changed(basisIdx);

  m_customZetaSpin->setValue(m_model->siesta.custom_zeta);
  m_customZetaPolSpin->setValue(m_model->siesta.custom_zeta_polarisation);
  m_splitZetaNormSpin->setValue(m_model->siesta.split_zeta_norm);
  m_energyShiftSpin->setValue(m_model->siesta.energy_shift);

  /* Ensure FClast defaults to total atoms, and NumCGsteps has a sane default */
  m_model->siesta.md_fc_last = m_model->num_atoms;
  if (m_model->siesta.md_num_cg_steps <= 0)
    m_model->siesta.md_num_cg_steps = 16842752;
  m_meshCutoffSpin->setValue(m_model->siesta.mesh_cutoff);
  m_electronicTempSpin->setValue(m_model->siesta.electronic_temperature);
  m_spinPolarisedCheck->setChecked(m_model->siesta.spin_polarised);
  if (m_isPeriodicCheck)
    m_isPeriodicCheck->setChecked(m_model->siesta.is_periodic);
  if (m_kgridCutoffSpin)
    m_kgridCutoffSpin->setValue(m_model->siesta.kgrid_cutoff);

  /* SCF */
  m_noOfCyclesSpin->setValue(m_model->siesta.no_of_cycles);
  m_mixingWeightSpin->setValue(m_model->siesta.mixing_weight);
  m_pulayMixingCheck->setChecked(m_model->siesta.pulay_mixing);
  on_pulay_toggled(m_model->siesta.pulay_mixing);
  m_noOfPulayMatricesSpin->setValue(m_model->siesta.no_of_pulay_matrices);
  m_divideAndConquerCheck->setChecked(m_model->siesta.diag_divide_and_conquer);

  /* Geometry */
  int geomIdx = 0;
  auto rt = m_model->siesta.run_type;
  switch (rt)
  {
  case siesta_pak::OPTIMISATION:
    geomIdx = 1;
    break;
  case siesta_pak::MOLECULAR_DYNAMICS:
    geomIdx = 2;
    break;
  case siesta_pak::PHONON_CALCULATION:
    geomIdx = 3;
    break;
  default:
    geomIdx = 0;
    break;
  }
  m_geomRunTypeCombo->setCurrentIndex(geomIdx);
  on_geom_runtype_changed(geomIdx);

  m_numberOfStepsSpin->setValue(m_model->siesta.number_of_steps);
  m_optimiseCellCheck->setChecked(m_model->siesta.md_variable_cell);

  int mdIdx = 0;
  auto mdRun = m_model->siesta.md_type_of_run;
  switch (mdRun)
  {
  case siesta_pak::VERLET_MDRUN:
    mdIdx = 1;
    break;
  case siesta_pak::NOSE_MDRUN:
    mdIdx = 2;
    break;
  case siesta_pak::PARRINELLOPAHMAN_MDRUN:
    mdIdx = 3;
    break;
  case siesta_pak::NOSEPARRINELLOPAHMAN_MDRUN:
    mdIdx = 4;
    break;
  case siesta_pak::ANNEAL_MDRUN:
    mdIdx = 5;
    break;
  case siesta_pak::FC_MDRUN:
    mdIdx = 6;
    break;
  case siesta_pak::PHONON_MDRUN:
    mdIdx = 7;
    break;
  default:
    mdIdx = 0;
    break;
  }
  m_mdRunTypeCombo->setCurrentIndex(mdIdx);

  m_initialTempSpin->setValue(m_model->siesta.md_inital_temperature);
  m_targetTempSpin->setValue(m_model->siesta.md_target_temperature);
  m_targetPressureSpin->setValue(m_model->siesta.pressure);
  m_initialTimeStepSpin->setValue(m_model->siesta.md_inital_time_step);
  m_finalTimeStepSpin->setValue(m_model->siesta.md_final_time_step);
  m_lengthTimeStepSpin->setValue(m_model->siesta.md_length_time_step);
  m_restartCheck->setChecked(m_model->siesta.use_saved_data);
  m_maxCGDispSpin->setValue(m_model->siesta.md_max_cg_displacement);
  m_maxForceTolSpin->setValue(m_model->siesta.md_max_force_tol);
  m_maxStressTolSpin->setValue(m_model->siesta.md_max_stress_tol);
  m_targetPressureOptSpin->setValue(m_model->siesta.md_target_pressure);
  m_stressXXSpin->setValue(m_model->siesta.md_target_stress_xx);
  m_stressYYSpin->setValue(m_model->siesta.md_target_stress_yy);
  m_stressZZSpin->setValue(m_model->siesta.md_target_stress_zz);
  m_stressXYSpin->setValue(m_model->siesta.md_target_stress_xy);
  m_stressXZSpin->setValue(m_model->siesta.md_target_stress_xz);
  m_stressYZSpin->setValue(m_model->siesta.md_target_stress_yz);
  m_finiteDiffStepSpin->setValue(m_model->siesta.finite_diff_step_size);

  /* File I/O */
  m_longOutputCheck->setChecked(m_model->siesta.long_output);
  on_long_output_toggled(m_model->siesta.long_output);
  m_dosSpin->setValue(m_model->siesta.density_of_states);
  m_densityOnMeshSpin->setValue(m_model->siesta.density_on_mesh);
  m_electrostaticPotSpin->setValue(m_model->siesta.electrostatic_pot_on_mesh);
  m_writeCoorStepCheck->setChecked(m_model->siesta.file_output_write_coor_step);
  m_writeForcesCheck->setChecked(m_model->siesta.file_output_write_forces);
  m_writeKpointsCheck->setChecked(m_model->siesta.file_output_write_kpoints);
  m_writeEigenvaluesCheck->setChecked(m_model->siesta.file_output_write_eigenvalues);
  m_writeKbandsCheck->setChecked(m_model->siesta.file_output_write_kbands);
  m_writeBandsCheck->setChecked(m_model->siesta.file_output_write_bands);
  m_writeWavefunctionsCheck->setChecked(m_model->siesta.file_output_write_wavefunctions);
  m_writeMullikenSpin->setValue((int) m_model->siesta.file_output_write_mullikenpop);
  m_writeDmCheck->setChecked(m_model->siesta.file_output_write_dm);
  m_writeXmolCheck->setChecked(m_model->siesta.file_output_write_coor_xmol);
  m_writeCeriusCheck->setChecked(m_model->siesta.file_output_write_coor_cerius);
  m_writeMdXmolCheck->setChecked(m_model->siesta.file_output_write_md_xmol);
  m_writeMdHistoryCheck->setChecked(m_model->siesta.file_output_write_md_history);
}

void SiestaDialog::setupFileHandlerPage()
{
  /* This is called from the constructor where mainLayout is in scope.
   * We use the member m_mainLayout instead. */
  auto *vbox = new QVBoxLayout();
  vbox->setSpacing(4);

  m_writeCsvCheck = new QCheckBox(tr("Write csv"));
  vbox->addWidget(m_writeCsvCheck);

  auto *frame = new QGroupBox(tr("File Specifics"));
  auto *form = new QFormLayout(frame);
  form->setSpacing(4);
  form->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
  form->setLabelAlignment(Qt::AlignLeft);

  m_filenameEdit = new QLineEdit();
  form->addRow(tr("Filename"), m_filenameEdit);
  connect(m_filenameEdit, &QLineEdit::textChanged, this, [this](const QString &t) {
    g_free(m_model->siesta.modelfilename);
    m_model->siesta.modelfilename = g_strdup(t.toUtf8().constData());
  });

  vbox->addWidget(frame);
  m_mainLayout->addLayout(vbox);
}

void SiestaDialog::setupElectronicStructurePage()
{
  auto *page = new QWidget();
  auto *mainLayout = new QVBoxLayout(page);
  mainLayout->setSpacing(8);

  /* Basis Set + Custom Zeta Config (stacked vertically) */
  {
    auto *frame = new QGroupBox(tr("Basis Set"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    m_basisSetCombo = new QComboBox();
    m_basisSetCombo->addItems({tr("Single zeta"), tr("Double zeta"), tr("Single zeta polarised"),
                               tr("Double zeta polarised"), tr("Custom Zeta")});
    vbox->addWidget(m_basisSetCombo);
    connect(m_basisSetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &SiestaDialog::on_basis_set_changed);

    /* Custom Zeta Config — inside Basis Set frame, below radio buttons */
    m_customZetaFrame = new QGroupBox(tr("Custom Zeta Config"), frame);
    auto *zetaVbox = new QVBoxLayout(m_customZetaFrame);
    zetaVbox->setSpacing(4);
    m_customZetaFrame->setDisabled(true);

    auto *zetaForm = new QFormLayout();
    zetaForm->setSpacing(4);
    zetaForm->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    zetaForm->setLabelAlignment(Qt::AlignLeft);

    m_customZetaSpin = new QSpinBox();
    m_customZetaSpin->setRange(1, 5);
    m_customZetaSpin->setValue((int) m_model->siesta.custom_zeta);
    zetaForm->addRow(tr("Zeta level"), m_customZetaSpin);
    connect(m_customZetaSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) {
      m_model->siesta.custom_zeta = v;
      if (v > 4)
        QMessageBox::warning(this, tr("SIESTA"), tr("More than 4 zeta levels — heavy computation!"));
    });

    m_customZetaPolSpin = new QSpinBox();
    m_customZetaPolSpin->setRange(0, 5);
    m_customZetaPolSpin->setValue((int) m_model->siesta.custom_zeta_polarisation);
    zetaForm->addRow(tr("Polarisation level"), m_customZetaPolSpin);
    connect(m_customZetaPolSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) {
      m_model->siesta.custom_zeta_polarisation = v;
      if (v > 3)
        QMessageBox::warning(this, tr("SIESTA"), tr("More than 3 polarisations?"));
    });

    zetaVbox->addLayout(zetaForm);
    vbox->addWidget(m_customZetaFrame);

    mainLayout->addWidget(frame);
  }

  /* Special inputs */
  {
    auto *frame = new QGroupBox(tr("Special inputs"), page);
    auto *form = new QFormLayout(frame);
    form->setSpacing(4);
    form->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    form->setLabelAlignment(Qt::AlignLeft);

    m_splitZetaNormSpin = new QDoubleSpinBox();
    m_splitZetaNormSpin->setRange(0.01, 1.0);
    m_splitZetaNormSpin->setSingleStep(0.01);
    m_splitZetaNormSpin->setValue(m_model->siesta.split_zeta_norm);
    form->addRow(tr("Split Zeta norm"), m_splitZetaNormSpin);
    connect(m_splitZetaNormSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double v) { m_model->siesta.split_zeta_norm = v; });

    m_energyShiftSpin = new QDoubleSpinBox();
    m_energyShiftSpin->setRange(0.001, 0.05);
    m_energyShiftSpin->setSingleStep(0.001);
    m_energyShiftSpin->setValue(m_model->siesta.energy_shift);
    form->addRow(tr("Energy Shift (Ryd)"), m_energyShiftSpin);
    connect(m_energyShiftSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double v) { m_model->siesta.energy_shift = v; });

    m_meshCutoffSpin = new QSpinBox();
    m_meshCutoffSpin->setRange(40, 1000);
    m_meshCutoffSpin->setSingleStep(5);
    m_meshCutoffSpin->setValue((int) m_model->siesta.mesh_cutoff);
    form->addRow(tr("Mesh Cutoff (Ryd)"), m_meshCutoffSpin);
    connect(m_meshCutoffSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int v) { m_model->siesta.mesh_cutoff = v; });

    m_electronicTempSpin = new QSpinBox();
    m_electronicTempSpin->setRange(0, 500);
    m_electronicTempSpin->setSingleStep(1);
    m_electronicTempSpin->setValue((int) m_model->siesta.electronic_temperature);
    form->addRow(tr("Electronic temp (K)"), m_electronicTempSpin);
    connect(m_electronicTempSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int v) { m_model->siesta.electronic_temperature = v; });

    m_spinPolarisedCheck = new QCheckBox(tr("Spin Polarised"));
    m_spinPolarisedCheck->setChecked(m_model->siesta.spin_polarised);
    form->addRow("", m_spinPolarisedCheck);
    connect(m_spinPolarisedCheck, &QCheckBox::toggled, this, [this](bool v) { m_model->siesta.spin_polarised = v; });

    mainLayout->addWidget(frame);
  }

  /* Periodic — only shown when model is periodic */
  if (m_model->periodic)
  {
    auto *frame = new QGroupBox(tr("Periodic"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    m_isPeriodicCheck = new QCheckBox(tr("Is it Periodic?"));
    m_isPeriodicCheck->setChecked(m_model->siesta.is_periodic);
    vbox->addWidget(m_isPeriodicCheck);
    connect(m_isPeriodicCheck, &QCheckBox::toggled, this, [this](bool v) {
      m_model->siesta.is_periodic = v;
      m_kgridCutoffSpin->parentWidget()->setVisible(v);
    });

    auto *form = new QFormLayout();
    form->setSpacing(4);
    form->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    form->setLabelAlignment(Qt::AlignLeft);

    m_kgridCutoffSpin = new QDoubleSpinBox();
    m_kgridCutoffSpin->setRange(1.0, 50.0);
    m_kgridCutoffSpin->setSingleStep(1.0);
    m_kgridCutoffSpin->setValue(m_model->siesta.kgrid_cutoff);
    form->addRow(tr("kgrid Cutoff"), m_kgridCutoffSpin);
    connect(m_kgridCutoffSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double v) { m_model->siesta.kgrid_cutoff = v; });

    frame->setVisible(m_model->siesta.is_periodic);
    vbox->addLayout(form);

    mainLayout->addWidget(frame);
  }

  m_notebook->addTab(page, tr("Electronic Structure"));
}

void SiestaDialog::setupSCFPage()
{
  auto *page = new QWidget();
  auto *vbox = new QVBoxLayout(page);
  vbox->setSpacing(8);

  /* Method */
  {
    auto *frame = new QGroupBox(tr("Method"), page);
    auto *form = new QFormLayout(frame);
    form->setSpacing(4);
    form->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    form->setLabelAlignment(Qt::AlignLeft);

    m_noOfCyclesSpin = new QSpinBox();
    m_noOfCyclesSpin->setRange(1, 100);
    m_noOfCyclesSpin->setValue((int) m_model->siesta.no_of_cycles);
    form->addRow(tr("Number of Cycles"), m_noOfCyclesSpin);
    connect(m_noOfCyclesSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int v) { m_model->siesta.no_of_cycles = v; });

    m_mixingWeightSpin = new QDoubleSpinBox();
    m_mixingWeightSpin->setRange(0.01, 0.5);
    m_mixingWeightSpin->setSingleStep(0.01);
    m_mixingWeightSpin->setValue(m_model->siesta.mixing_weight);
    form->addRow(tr("Mixing Weight"), m_mixingWeightSpin);
    connect(m_mixingWeightSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double v) { m_model->siesta.mixing_weight = v; });

    vbox->addWidget(frame);
  }

  /* Pulay Mixing checkbox — left-aligned below Method frame */
  {
    auto *hbox = new QHBoxLayout();
    hbox->setContentsMargins(0, 0, 0, 0);
    m_pulayMixingCheck = new QCheckBox(tr("Pulay Mixing"));
    m_pulayMixingCheck->setChecked(m_model->siesta.pulay_mixing);
    hbox->addWidget(m_pulayMixingCheck);
    hbox->addStretch();
    connect(m_pulayMixingCheck, &QCheckBox::toggled, this, &SiestaDialog::on_pulay_toggled);
    vbox->addLayout(hbox);
  }

  /* Pulay options — directly below Method frame, not in a separate frame */
  {
    m_noOfPulayMatricesSpin = new QSpinBox();
    m_noOfPulayMatricesSpin->setRange(1, 1000);
    m_noOfPulayMatricesSpin->setSingleStep(5);
    m_noOfPulayMatricesSpin->setValue((int) m_model->siesta.no_of_pulay_matrices);
    auto *pulayForm = new QFormLayout();
    pulayForm->setSpacing(4);
    pulayForm->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    pulayForm->setLabelAlignment(Qt::AlignLeft);
    pulayForm->addRow(tr("Number of Pulay Matrices"), m_noOfPulayMatricesSpin);
    connect(m_noOfPulayMatricesSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int v) { m_model->siesta.no_of_pulay_matrices = v; });

    /* Initially disabled when Pulay Mixing is off */
    m_noOfPulayMatricesSpin->setEnabled(m_model->siesta.pulay_mixing);
    vbox->addLayout(pulayForm);
  }

  /* Speed Hacks */
  {
    auto *frame = new QGroupBox(tr("Speed Hacks"), page);
    auto *vbox2 = new QVBoxLayout(frame);
    vbox2->setSpacing(4);

    m_divideAndConquerCheck = new QCheckBox(tr("Divide and Conquer"));
    m_divideAndConquerCheck->setChecked(m_model->siesta.diag_divide_and_conquer);
    vbox2->addWidget(m_divideAndConquerCheck);
    connect(m_divideAndConquerCheck, &QCheckBox::toggled, this,
            [this](bool v) { m_model->siesta.diag_divide_and_conquer = v; });

    vbox->addWidget(frame);
  }

  m_notebook->addTab(page, tr("SCF"));
}

void SiestaDialog::setupGeometryPage()
{
  auto *page = new QWidget();
  auto *mainLayout = new QVBoxLayout(page);
  mainLayout->setSpacing(8);

  /* Run type */
  {
    auto *frame = new QGroupBox(tr("Method"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    m_geomRunTypeCombo = new QComboBox();
    m_geomRunTypeCombo->addItems(
        {tr("Single Point"), tr("Optimisation"), tr("Molecular Dynamics"), tr("Phonon Calculation")});
    vbox->addWidget(m_geomRunTypeCombo);
    connect(m_geomRunTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &SiestaDialog::on_geom_runtype_changed);
    mainLayout->addWidget(frame);
  }

  /* Stacked widget for geometry sub-pages */
  m_geomStack = new QStackedWidget();

  /* Single Point page */
  {
    auto *spPage = new QWidget();
    auto *vbox = new QVBoxLayout(spPage);
    vbox->setSpacing(4);
    auto *label = new QLabel(tr("Number of Steps — locked to Zero\n(single point)"));
    label->setWordWrap(true);
    vbox->addWidget(label);
    m_geomStack->addWidget(spPage);
  }

  /* Optimisation page */
  {
    auto *optPage = new QWidget();
    auto *mainVbox = new QVBoxLayout(optPage);
    mainVbox->setSpacing(8);

    /* Options frame — wraps all optimisation settings */
    auto *optionsFrame = new QGroupBox(tr("Options"), optPage);
    auto *optionsVbox = new QVBoxLayout(optionsFrame);
    optionsVbox->setSpacing(8);

    /* Parameters form */
    auto *form = new QFormLayout();
    form->setSpacing(4);
    form->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    form->setLabelAlignment(Qt::AlignLeft);

    m_numberOfStepsSpin = new QSpinBox();
    m_numberOfStepsSpin->setRange(2, 1000);
    m_numberOfStepsSpin->setValue((int) m_model->siesta.number_of_steps);
    form->addRow(tr("Number of Steps"), m_numberOfStepsSpin);
    connect(m_numberOfStepsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int v) { m_model->siesta.number_of_steps = v; });

    m_targetPressureOptSpin = new QDoubleSpinBox();
    m_targetPressureOptSpin->setRange(-5.0, 5.0);
    m_targetPressureOptSpin->setSingleStep(0.1);
    m_targetPressureOptSpin->setValue(m_model->siesta.md_target_pressure);
    form->addRow(tr("Target Pressure"), m_targetPressureOptSpin);
    connect(m_targetPressureOptSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double v) { m_model->siesta.md_target_pressure = v; });

    optionsVbox->addLayout(form);

    /* Optimise cell — left-aligned checkbox */
    {
      auto *hbox = new QHBoxLayout();
      hbox->setContentsMargins(0, 0, 0, 0);
      m_optimiseCellCheck = new QCheckBox(tr("Optimise cell"));
      m_optimiseCellCheck->setChecked(m_model->siesta.md_variable_cell);
      hbox->addWidget(m_optimiseCellCheck);
      hbox->addStretch();
      connect(m_optimiseCellCheck, &QCheckBox::toggled, this, [this](bool v) { m_model->siesta.md_variable_cell = v; });
      optionsVbox->addLayout(hbox);
    }

    /* Stress tensors — 2 columns */
    {
      auto *stressFrame = new QGroupBox(tr("Stress Tensors"), optionsFrame);
      auto *stressHbox = new QHBoxLayout(stressFrame);
      stressHbox->setSpacing(16);

      /* Left column: xx, yy, zz */
      auto *leftForm = new QFormLayout();
      leftForm->setSpacing(4);
      leftForm->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
      leftForm->setLabelAlignment(Qt::AlignLeft);

      m_stressXXSpin = new QDoubleSpinBox();
      m_stressXXSpin->setRange(-5.0, 5.0);
      m_stressXXSpin->setSingleStep(0.1);
      m_stressXXSpin->setValue(m_model->siesta.md_target_stress_xx);
      leftForm->addRow(tr("xx"), m_stressXXSpin);

      m_stressYYSpin = new QDoubleSpinBox();
      m_stressYYSpin->setRange(-5.0, 5.0);
      m_stressYYSpin->setSingleStep(0.1);
      m_stressYYSpin->setValue(m_model->siesta.md_target_stress_yy);
      leftForm->addRow(tr("yy"), m_stressYYSpin);

      m_stressZZSpin = new QDoubleSpinBox();
      m_stressZZSpin->setRange(-5.0, 5.0);
      m_stressZZSpin->setSingleStep(0.1);
      m_stressZZSpin->setValue(m_model->siesta.md_target_stress_zz);
      leftForm->addRow(tr("zz"), m_stressZZSpin);

      stressHbox->addLayout(leftForm);

      /* Right column: xy, xz, yz */
      auto *rightForm = new QFormLayout();
      rightForm->setSpacing(4);
      rightForm->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
      rightForm->setLabelAlignment(Qt::AlignLeft);

      m_stressXYSpin = new QDoubleSpinBox();
      m_stressXYSpin->setRange(-5.0, 5.0);
      m_stressXYSpin->setSingleStep(0.1);
      m_stressXYSpin->setValue(m_model->siesta.md_target_stress_xy);
      rightForm->addRow(tr("xy"), m_stressXYSpin);

      m_stressXZSpin = new QDoubleSpinBox();
      m_stressXZSpin->setRange(-5.0, 5.0);
      m_stressXZSpin->setSingleStep(0.1);
      m_stressXZSpin->setValue(m_model->siesta.md_target_stress_xz);
      rightForm->addRow(tr("xz"), m_stressXZSpin);

      m_stressYZSpin = new QDoubleSpinBox();
      m_stressYZSpin->setRange(-5.0, 5.0);
      m_stressYZSpin->setSingleStep(0.1);
      m_stressYZSpin->setValue(m_model->siesta.md_target_stress_yz);
      rightForm->addRow(tr("yz"), m_stressYZSpin);

      stressHbox->addLayout(rightForm);

      optionsVbox->addWidget(stressFrame);
    }

    /* Termination Options */
    auto *termFrame = new QGroupBox(tr("Termination Options"), optionsFrame);
    auto *termForm = new QFormLayout(termFrame);
    termForm->setSpacing(4);
    termForm->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    termForm->setLabelAlignment(Qt::AlignLeft);

    m_maxCGDispSpin = new QDoubleSpinBox();
    m_maxCGDispSpin->setRange(0.0, 2.0);
    m_maxCGDispSpin->setSingleStep(0.01);
    m_maxCGDispSpin->setValue(m_model->siesta.md_max_cg_displacement);
    termForm->addRow(tr("Max CG displacement"), m_maxCGDispSpin);
    connect(m_maxCGDispSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double v) { m_model->siesta.md_max_cg_displacement = v; });

    m_maxForceTolSpin = new QDoubleSpinBox();
    m_maxForceTolSpin->setRange(0.0, 2.0);
    m_maxForceTolSpin->setSingleStep(0.01);
    m_maxForceTolSpin->setValue(m_model->siesta.md_max_force_tol);
    termForm->addRow(tr("Max Force Tolerance"), m_maxForceTolSpin);
    connect(m_maxForceTolSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double v) { m_model->siesta.md_max_force_tol = v; });

    m_maxStressTolSpin = new QDoubleSpinBox();
    m_maxStressTolSpin->setRange(0.0, 2.0);
    m_maxStressTolSpin->setSingleStep(0.1);
    m_maxStressTolSpin->setValue(m_model->siesta.md_max_stress_tol);
    termForm->addRow(tr("Max Stress Tolerance"), m_maxStressTolSpin);
    connect(m_maxStressTolSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double v) { m_model->siesta.md_max_stress_tol = v; });

    optionsVbox->addWidget(termFrame);

    /* Job Options */
    auto *jobFrame = new QGroupBox(tr("Job Options"), optionsFrame);
    auto *jobVbox = new QVBoxLayout(jobFrame);
    jobVbox->setSpacing(4);

    m_restartCheck = new QCheckBox(tr("Restart (Use saved data)"));
    m_restartCheck->setChecked(m_model->siesta.use_saved_data);
    jobVbox->addWidget(m_restartCheck);
    connect(m_restartCheck, &QCheckBox::toggled, this, [this](bool v) { m_model->siesta.use_saved_data = v; });

    optionsVbox->addWidget(jobFrame);
    mainVbox->addWidget(optionsFrame);

    m_geomStack->addWidget(optPage);
  }

  /* Molecular Dynamics page */
  {
    auto *mdPage = new QWidget();
    auto *vbox = new QVBoxLayout(mdPage);
    vbox->setSpacing(8);

    /* Run Type */
    auto *runTypeFrame = new QGroupBox(tr("Run Type"), mdPage);
    auto *runTypeVbox = new QVBoxLayout(runTypeFrame);
    runTypeVbox->setSpacing(4);

    m_mdRunTypeCombo = new QComboBox();
    m_mdRunTypeCombo->addItems(
        {tr("Verlet"), tr("Nose"), tr("Parrinello-Rahman"), tr("Nose-Parrinello-Rahman"), tr("Anneal"), tr("Phonon")});
    runTypeVbox->addWidget(m_mdRunTypeCombo);
    connect(m_mdRunTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &SiestaDialog::on_md_runtype_changed);
    vbox->addWidget(runTypeFrame);

    /* Temperature */
    auto *tempFrame = new QGroupBox(tr("Temperature"), mdPage);
    auto *tempForm = new QFormLayout(tempFrame);
    tempForm->setSpacing(4);

    m_initialTempSpin = new QSpinBox();
    m_initialTempSpin->setRange(0, 500);
    m_initialTempSpin->setSingleStep(1);
    m_initialTempSpin->setValue((int) m_model->siesta.md_inital_temperature);
    tempForm->addRow(tr("Initial Temperature"), m_initialTempSpin);
    connect(m_initialTempSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int v) { m_model->siesta.md_inital_temperature = v; });

    m_targetTempSpin = new QSpinBox();
    m_targetTempSpin->setRange(0, 500);
    m_targetTempSpin->setSingleStep(1);
    m_targetTempSpin->setValue((int) m_model->siesta.md_target_temperature);
    tempForm->addRow(tr("Target Temperature"), m_targetTempSpin);
    connect(m_targetTempSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int v) { m_model->siesta.md_target_temperature = v; });

    vbox->addWidget(tempFrame);

    /* Pressure */
    auto *pressFrame = new QGroupBox(tr("Pressure"), mdPage);
    auto *pressForm = new QFormLayout(pressFrame);
    pressForm->setSpacing(4);

    m_targetPressureSpin = new QDoubleSpinBox();
    m_targetPressureSpin->setRange(-10.0, 10.0);
    m_targetPressureSpin->setSingleStep(0.01);
    m_targetPressureSpin->setValue(m_model->siesta.pressure);
    pressForm->addRow(tr("Target Pressure"), m_targetPressureSpin);
    connect(m_targetPressureSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double v) { m_model->siesta.pressure = v; });

    vbox->addWidget(pressFrame);

    /* Time */
    auto *timeFrame = new QGroupBox(tr("Time"), mdPage);
    auto *timeForm = new QFormLayout(timeFrame);
    timeForm->setSpacing(4);

    m_initialTimeStepSpin = new QSpinBox();
    m_initialTimeStepSpin->setRange(0, 500);
    m_initialTimeStepSpin->setSingleStep(1);
    m_initialTimeStepSpin->setValue((int) m_model->siesta.md_inital_time_step);
    timeForm->addRow(tr("Initial Timestep"), m_initialTimeStepSpin);
    connect(m_initialTimeStepSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int v) { m_model->siesta.md_inital_time_step = v; });

    m_finalTimeStepSpin = new QSpinBox();
    m_finalTimeStepSpin->setRange(0, 500);
    m_finalTimeStepSpin->setSingleStep(1);
    m_finalTimeStepSpin->setValue((int) m_model->siesta.md_final_time_step);
    timeForm->addRow(tr("Final Timestep"), m_finalTimeStepSpin);
    connect(m_finalTimeStepSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int v) { m_model->siesta.md_final_time_step = v; });

    m_lengthTimeStepSpin = new QDoubleSpinBox();
    m_lengthTimeStepSpin->setRange(0.1, 200.0);
    m_lengthTimeStepSpin->setSingleStep(0.1);
    m_lengthTimeStepSpin->setValue(m_model->siesta.md_length_time_step);
    timeForm->addRow(tr("Length of Timestep"), m_lengthTimeStepSpin);
    connect(m_lengthTimeStepSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double v) { m_model->siesta.md_length_time_step = v; });

    vbox->addWidget(timeFrame);

    /* Job Options */
    auto *jobFrame = new QGroupBox(tr("Job Options"), mdPage);
    auto *jobVbox = new QVBoxLayout(jobFrame);
    jobVbox->setSpacing(4);

    m_restartCheck = new QCheckBox(tr("Restart (Use saved data)"));
    m_restartCheck->setChecked(m_model->siesta.use_saved_data);
    jobVbox->addWidget(m_restartCheck);
    connect(m_restartCheck, &QCheckBox::toggled, this, [this](bool v) { m_model->siesta.use_saved_data = v; });

    vbox->addWidget(jobFrame);
    m_geomStack->addWidget(mdPage);
  }

  /* Phonon page */
  {
    auto *phononPage = new QWidget();
    auto *vbox = new QVBoxLayout(phononPage);
    vbox->setSpacing(8);

    auto *diffFrame = new QGroupBox(tr("Differencing"), phononPage);
    auto *diffForm = new QFormLayout(diffFrame);
    diffForm->setSpacing(4);

    m_finiteDiffStepSpin = new QDoubleSpinBox();
    m_finiteDiffStepSpin->setRange(0.1, 10.0);
    m_finiteDiffStepSpin->setSingleStep(0.1);
    m_finiteDiffStepSpin->setValue(m_model->siesta.finite_diff_step_size);
    diffForm->addRow(tr("Finite Difference step size"), m_finiteDiffStepSpin);
    connect(m_finiteDiffStepSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double v) { m_model->siesta.finite_diff_step_size = v; });

    vbox->addWidget(diffFrame);

    auto *jobFrame = new QGroupBox(tr("Job Options"), phononPage);
    auto *jobVbox = new QVBoxLayout(jobFrame);
    jobVbox->setSpacing(4);

    m_restartCheck = new QCheckBox(tr("Restart (Use saved data)"));
    m_restartCheck->setChecked(m_model->siesta.use_saved_data);
    jobVbox->addWidget(m_restartCheck);
    connect(m_restartCheck, &QCheckBox::toggled, this, [this](bool v) { m_model->siesta.use_saved_data = v; });

    vbox->addWidget(jobFrame);
    m_geomStack->addWidget(phononPage);
  }

  mainLayout->addWidget(m_geomStack);
  m_notebook->addTab(page, tr("Geometry"));
}

void SiestaDialog::setupFileIOPage()
{
  auto *page = new QWidget();
  auto *vbox = new QVBoxLayout(page);
  vbox->setSpacing(8);

  /* Files */
  {
    auto *frame = new QGroupBox(tr("Files"), page);
    auto *vbox2 = new QVBoxLayout(frame);
    vbox2->setSpacing(4);

    m_longOutputCheck = new QCheckBox(tr("Long output"));
    m_longOutputCheck->setChecked(m_model->siesta.long_output);
    vbox2->addWidget(m_longOutputCheck);
    connect(m_longOutputCheck, &QCheckBox::toggled, this, &SiestaDialog::on_long_output_toggled);

    vbox->addWidget(frame);
  }

  /* Mesh potential */
  {
    auto *frame = new QGroupBox(tr("Mesh potential"), page);
    auto *form = new QFormLayout(frame);
    form->setSpacing(4);
    form->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    form->setLabelAlignment(Qt::AlignLeft);

    m_dosSpin = new QDoubleSpinBox();
    m_dosSpin->setRange(0.1, 10.0);
    m_dosSpin->setSingleStep(0.1);
    m_dosSpin->setValue(m_model->siesta.density_of_states);
    form->addRow(tr("Density of states"), m_dosSpin);
    connect(m_dosSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double v) { m_model->siesta.density_of_states = v; });

    m_densityOnMeshSpin = new QDoubleSpinBox();
    m_densityOnMeshSpin->setRange(0.1, 10.0);
    m_densityOnMeshSpin->setSingleStep(0.1);
    m_densityOnMeshSpin->setValue(m_model->siesta.density_on_mesh);
    form->addRow(tr("Density on mesh"), m_densityOnMeshSpin);
    connect(m_densityOnMeshSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double v) { m_model->siesta.density_on_mesh = v; });

    m_electrostaticPotSpin = new QDoubleSpinBox();
    m_electrostaticPotSpin->setRange(0.1, 10.0);
    m_electrostaticPotSpin->setSingleStep(0.1);
    m_electrostaticPotSpin->setValue(m_model->siesta.electrostatic_pot_on_mesh);
    form->addRow(tr("Electrostatic pot on mesh"), m_electrostaticPotSpin);
    connect(m_electrostaticPotSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double v) { m_model->siesta.electrostatic_pot_on_mesh = v; });

    vbox->addWidget(frame);
  }

  /* Output Options — 2 columns */
  {
    auto *frame = new QGroupBox(tr("Output Options"), page);
    auto *hbox = new QHBoxLayout(frame);
    hbox->setSpacing(24);

    /* Left column: 4 toggles */
    {
      auto *leftVbox = new QVBoxLayout();
      leftVbox->setSpacing(4);

      m_writeCoorStepCheck = new QCheckBox(tr("WriteCoorStep"));
      m_writeForcesCheck = new QCheckBox(tr("WriteForces"));
      m_writeKpointsCheck = new QCheckBox(tr("WriteKpoints"));
      m_writeEigenvaluesCheck = new QCheckBox(tr("WriteEigenvalues"));

      leftVbox->addWidget(m_writeCoorStepCheck);
      leftVbox->addWidget(m_writeForcesCheck);
      leftVbox->addWidget(m_writeKpointsCheck);
      leftVbox->addWidget(m_writeEigenvaluesCheck);
      hbox->addLayout(leftVbox);
    }

    /* Right column: 3 toggles + 1 spinner */
    {
      auto *rightVbox = new QVBoxLayout();
      rightVbox->setSpacing(4);

      m_writeBandsCheck = new QCheckBox(tr("WriteBands"));
      m_writeKbandsCheck = new QCheckBox(tr("WriteKbands"));
      m_writeWavefunctionsCheck = new QCheckBox(tr("WriteWaveFunctions"));

      rightVbox->addWidget(m_writeBandsCheck);
      rightVbox->addWidget(m_writeKbandsCheck);
      rightVbox->addWidget(m_writeWavefunctionsCheck);

      auto *mullikenForm = new QFormLayout();
      mullikenForm->setSpacing(4);
      mullikenForm->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
      mullikenForm->setLabelAlignment(Qt::AlignLeft);
      m_writeMullikenSpin = new QSpinBox();
      m_writeMullikenSpin->setRange(0, 3);
      m_writeMullikenSpin->setSingleStep(1);
      mullikenForm->addRow(tr("WriteMullikenPop"), m_writeMullikenSpin);
      rightVbox->addLayout(mullikenForm);

      hbox->addLayout(rightVbox);
    }

    connect(m_writeCoorStepCheck, &QCheckBox::toggled, this,
            [this](bool v) { m_model->siesta.file_output_write_coor_step = v; });
    connect(m_writeForcesCheck, &QCheckBox::toggled, this,
            [this](bool v) { m_model->siesta.file_output_write_forces = v; });
    connect(m_writeKpointsCheck, &QCheckBox::toggled, this,
            [this](bool v) { m_model->siesta.file_output_write_kpoints = v; });
    connect(m_writeEigenvaluesCheck, &QCheckBox::toggled, this,
            [this](bool v) { m_model->siesta.file_output_write_eigenvalues = v; });
    connect(m_writeBandsCheck, &QCheckBox::toggled, this,
            [this](bool v) { m_model->siesta.file_output_write_bands = v; });
    connect(m_writeKbandsCheck, &QCheckBox::toggled, this,
            [this](bool v) { m_model->siesta.file_output_write_kbands = v; });
    connect(m_writeWavefunctionsCheck, &QCheckBox::toggled, this,
            [this](bool v) { m_model->siesta.file_output_write_wavefunctions = v; });
    connect(m_writeMullikenSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int v) { m_model->siesta.file_output_write_mullikenpop = v; });

    vbox->addWidget(frame);
  }

  /* Extra Output Options */
  {
    auto *frame = new QGroupBox(tr("Extra Output Options"), page);
    auto *vbox2 = new QVBoxLayout(frame);
    vbox2->setSpacing(4);

    m_writeDmCheck = new QCheckBox(tr("Write density matrix"));
    m_writeXmolCheck = new QCheckBox(tr("Write Xmol coordinates"));
    m_writeCeriusCheck = new QCheckBox(tr("Write cerius coordinates"));
    m_writeMdXmolCheck = new QCheckBox(tr("Write MD xmol"));
    m_writeMdHistoryCheck = new QCheckBox(tr("Write MD history"));

    vbox2->addWidget(m_writeDmCheck);
    vbox2->addWidget(m_writeXmolCheck);
    vbox2->addWidget(m_writeCeriusCheck);
    vbox2->addWidget(m_writeMdXmolCheck);
    vbox2->addWidget(m_writeMdHistoryCheck);

    connect(m_writeDmCheck, &QCheckBox::toggled, this, [this](bool v) { m_model->siesta.file_output_write_dm = v; });
    connect(m_writeXmolCheck, &QCheckBox::toggled, this,
            [this](bool v) { m_model->siesta.file_output_write_coor_xmol = v; });
    connect(m_writeCeriusCheck, &QCheckBox::toggled, this,
            [this](bool v) { m_model->siesta.file_output_write_coor_cerius = v; });
    connect(m_writeMdXmolCheck, &QCheckBox::toggled, this,
            [this](bool v) { m_model->siesta.file_output_write_md_xmol = v; });
    connect(m_writeMdHistoryCheck, &QCheckBox::toggled, this,
            [this](bool v) { m_model->siesta.file_output_write_md_history = v; });

    vbox->addWidget(frame);
  }

  m_notebook->addTab(page, tr("File I/O"));
}

void SiestaDialog::on_basis_set_changed(int index)
{
  if (!m_model)
    return;
  switch (index)
  {
  case 0:
    m_model->siesta.basis_set = siesta_pak::SZ_ZETA;
    break;
  case 1:
    m_model->siesta.basis_set = siesta_pak::DZ_ZETA;
    break;
  case 2:
    m_model->siesta.basis_set = siesta_pak::SZP_ZETA;
    break;
  case 3:
    m_model->siesta.basis_set = siesta_pak::DZP_ZETA;
    break;
  case 4:
    m_model->siesta.basis_set = siesta_pak::CUSTOM_ZETA;
    break;
  default:
    m_model->siesta.basis_set = siesta_pak::SZ_ZETA;
    break;
  }
  m_customZetaFrame->setDisabled(index != 4);
}

void SiestaDialog::on_geom_runtype_changed(int index)
{
  if (!m_model)
    return;
  switch (index)
  {
  case 0:
    m_model->siesta.run_type = siesta_pak::SINGLE_POINT;
    break;
  case 1:
    m_model->siesta.run_type = siesta_pak::OPTIMISATION;
    break;
  case 2:
    m_model->siesta.run_type = siesta_pak::MOLECULAR_DYNAMICS;
    break;
  case 3:
    m_model->siesta.run_type = siesta_pak::PHONON_CALCULATION;
    break;
  default:
    m_model->siesta.run_type = siesta_pak::SINGLE_POINT;
    break;
  }
  m_geomStack->setCurrentIndex(index);
}

void SiestaDialog::on_md_runtype_changed(int index)
{
  if (!m_model)
    return;
  switch (index)
  {
  case 0:
    m_model->siesta.md_type_of_run = siesta_pak::CG_MDRUN;
    break;
  case 1:
    m_model->siesta.md_type_of_run = siesta_pak::VERLET_MDRUN;
    break;
  case 2:
    m_model->siesta.md_type_of_run = siesta_pak::NOSE_MDRUN;
    break;
  case 3:
    m_model->siesta.md_type_of_run = siesta_pak::PARRINELLOPAHMAN_MDRUN;
    break;
  case 4:
    m_model->siesta.md_type_of_run = siesta_pak::NOSEPARRINELLOPAHMAN_MDRUN;
    break;
  case 5:
    m_model->siesta.md_type_of_run = siesta_pak::ANNEAL_MDRUN;
    break;
  case 6:
    m_model->siesta.md_type_of_run = siesta_pak::FC_MDRUN;
    break;
  case 7:
    m_model->siesta.md_type_of_run = siesta_pak::PHONON_MDRUN;
    break;
  default:
    m_model->siesta.md_type_of_run = siesta_pak::CG_MDRUN;
    break;
  }
}

void SiestaDialog::on_long_output_toggled(bool checked)
{
  if (!m_model)
    return;
  m_model->siesta.long_output = checked;
  if (checked)
  {
    m_model->siesta.file_output_write_coor_step = true;
    m_model->siesta.file_output_write_forces = true;
    m_model->siesta.file_output_write_kpoints = true;
    m_model->siesta.file_output_write_eigenvalues = true;
    m_model->siesta.file_output_write_bands = true;
    m_model->siesta.file_output_write_kbands = true;
    m_model->siesta.file_output_write_wavefunctions = true;
    m_model->siesta.file_output_write_mullikenpop = 1;
  } else
  {
    m_model->siesta.file_output_write_coor_step = false;
    m_model->siesta.file_output_write_forces = false;
    m_model->siesta.file_output_write_kpoints = false;
    m_model->siesta.file_output_write_eigenvalues = false;
    m_model->siesta.file_output_write_bands = false;
    m_model->siesta.file_output_write_kbands = false;
    m_model->siesta.file_output_write_wavefunctions = false;
    m_model->siesta.file_output_write_mullikenpop = 0;
  }
  /* Update checkbox states */
  m_writeCoorStepCheck->setChecked(m_model->siesta.file_output_write_coor_step);
  m_writeForcesCheck->setChecked(m_model->siesta.file_output_write_forces);
  m_writeKpointsCheck->setChecked(m_model->siesta.file_output_write_kpoints);
  m_writeEigenvaluesCheck->setChecked(m_model->siesta.file_output_write_eigenvalues);
  m_writeBandsCheck->setChecked(m_model->siesta.file_output_write_bands);
  m_writeKbandsCheck->setChecked(m_model->siesta.file_output_write_kbands);
  m_writeWavefunctionsCheck->setChecked(m_model->siesta.file_output_write_wavefunctions);
  m_writeMullikenSpin->setValue((int) m_model->siesta.file_output_write_mullikenpop);
}

void SiestaDialog::on_pulay_toggled(bool checked)
{
  if (!m_model)
    return;
  m_model->siesta.pulay_mixing = checked;
  if (m_noOfPulayMatricesSpin)
    m_noOfPulayMatricesSpin->setEnabled(checked);
}

void SiestaDialog::on_save()
{
  if (!m_model)
    return;
  QString fname = m_filenameEdit->text();
  if (!fname.endsWith(".fdf", Qt::CaseInsensitive))
    fname += ".fdf";
  g_free(m_model->siesta.modelfilename);
  m_model->siesta.modelfilename = g_strdup(fname.toUtf8().constData());
  m_filenameEdit->setText(fname);

  /* Generate the .fdf file */
  gchar *fdfPath = g_build_filename(sysenv.cwd, m_model->siesta.modelfilename, NULL);
  gint ret = write_fdf(fdfPath, m_model);
  g_free(fdfPath);

  if (ret == 0)
    QMessageBox::information(this, tr("SIESTA"), tr("FDF file saved successfully."));
  else
    QMessageBox::warning(this, tr("SIESTA"), tr("Failed to save FDF file."));
}

void SiestaDialog::on_run()
{
  if (!m_model)
    return;
  QString fname = m_filenameEdit->text();
  if (!fname.endsWith(".fdf", Qt::CaseInsensitive))
    fname += ".fdf";
  g_free(m_model->siesta.modelfilename);
  m_model->siesta.modelfilename = g_strdup(fname.toUtf8().constData());
  m_filenameEdit->setText(fname);

  /* First save the FDF file */
  gchar *fdfPath = g_build_filename(sysenv.cwd, m_model->siesta.modelfilename, NULL);
  gint ret = write_fdf(fdfPath, m_model);
  g_free(fdfPath);

  if (ret != 0)
  {
    QMessageBox::warning(this, tr("SIESTA"), tr("Failed to save FDF file. Aborting."));
    return;
  }

  /* Build and execute the SIESTA command */
  if (!sysenv.siesta_path)
  {
    QMessageBox::warning(this, tr("SIESTA"),
                         tr("SIESTA executable was not found.\n"
                            "Set the path in View > Executable paths."));
    return;
  }

  /* Generate output filename from input filename */
  gchar *outPath = parse_extension_set(m_model->siesta.modelfilename, "out");
  outPath = g_build_filename(sysenv.cwd, outPath, NULL);

  /* Command: siesta < input.fdf > output.out */
  gchar *cmd = g_strdup_printf("%s/%s < %s > %s 2>&1", sysenv.siesta_path, sysenv.siesta_exe, fdfPath, outPath);
  g_free(fdfPath);

  /* Queue the task — uses same async mechanism as GAMESS/GULP */
  qt_siesta_task(m_model);

  close();
}

void SiestaDialog::on_close()
{
  /* Save all text fields */
  if (!m_model)
    return;
  g_free(m_model->siesta.modelfilename);
  m_model->siesta.modelfilename = g_strdup(m_filenameEdit->text().toUtf8().constData());
  close();
}

/* Bridge function — called from mainwindow.cpp */
extern "C" void qt_show_siesta_dialog(struct model_pak *model)
{
  extern QWidget *get_main_window_widget();
  SiestaDialog *dlg = new SiestaDialog(model, get_main_window_widget());
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->show();
  dlg->raise();
  dlg->activateWindow();
}
