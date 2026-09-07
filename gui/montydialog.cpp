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
 * Monty Configuration Dialog for GDIS Qt6 GUI
 * Monty setup dialog with 6 tabs:
 * Crystal Graph, Input, Output, Model, Monitor, Run
 */

#include "montydialog.h"
#include "gdis_api.h"

#include "gdis.h"
#include "file.h"
#include "parse.h"
#include "task.h"

#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>

extern struct sysenv_pak sysenv;

/* Monty task bridge */
extern "C" void qt_monty_task(struct model_pak *);

MontyDialog::MontyDialog(struct model_pak *model, QWidget *parent) : QDialog(parent), m_model(model)
{
  setWindowTitle(tr("MONTY configuration"));
  resize(750, 550);

  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(8, 6, 8, 6);
  mainLayout->setSpacing(6);

  m_notebook = new QTabWidget(this);
  m_notebook->setTabPosition(QTabWidget::North);
  m_notebook->setDocumentMode(true);
  mainLayout->addWidget(m_notebook);

  setupCrystalGraphPage();
  setupInputPage();
  setupOutputPage();
  setupModelPage();
  setupMonitorPage();
  setupRunPage();

  /* Button bar */
  auto *btnLayout = new QHBoxLayout();
  btnLayout->addStretch();

  auto *runBtn = new QPushButton(tr("Run"), this);
  auto *closeBtn = new QPushButton(tr("Close"), this);
  btnLayout->addWidget(runBtn);
  btnLayout->addWidget(closeBtn);
  mainLayout->addLayout(btnLayout);

  connect(runBtn, &QPushButton::clicked, this, &MontyDialog::on_run);
  connect(closeBtn, &QPushButton::clicked, this, &MontyDialog::on_close);

  refresh();
}

MontyDialog::~MontyDialog() {}

void MontyDialog::refresh()
{
  if (!m_model)
    return;

  /* Crystal graph */
  m_imageXSpin->setValue(m_model->monty.image_x);
  m_imageYSpin->setValue(m_model->monty.image_y);
  m_imageZSpin->setValue(m_model->monty.image_z);

  /* Input */
  m_cgfEdit->setText(QString::fromUtf8(m_model->monty.input_cgf ? m_model->monty.input_cgf : ""));
  m_surfaceEdit->setText(QString::fromUtf8(m_model->monty.input_surface ? m_model->monty.input_surface : ""));
  m_hklsEdit->setPlainText(QString::fromUtf8(m_model->monty.hkls ? m_model->monty.hkls : ""));
  m_outputDirsEdit->setPlainText(QString::fromUtf8(m_model->monty.output_dirs ? m_model->monty.output_dirs : ""));
  m_supersaturationsEdit->setPlainText(
      QString::fromUtf8(m_model->monty.supersaturations ? m_model->monty.supersaturations : ""));

  int euIdx = 0;
  if (m_model->monty.energy_unit && strstr(m_model->monty.energy_unit, "kJ"))
    euIdx = 1;
  m_energyUnitCombo->setCurrentIndex(euIdx);

  m_esolvEdit->setText(QString::fromUtf8(m_model->monty.esolv ? m_model->monty.esolv : ""));

  /* Output */
  m_outputExtEdit->setText(QString::fromUtf8(m_model->monty.output_extension ? m_model->monty.output_extension : ""));
  m_writeSurfaceCheck->setChecked(m_model->monty.write_surface);
  m_writeXyzCheck->setChecked(m_model->monty.write_xyz);
  m_writeMatlabCheck->setChecked(m_model->monty.write_matlab);
  m_writeMsiCheck->setChecked(m_model->monty.write_msi);

  /* Model */
  m_spiralCheck->setChecked(m_model->monty.spiral);
  m_xstepsSpin->setValue(m_model->monty.xsteps);
  m_ystepsSpin->setValue(m_model->monty.ysteps);
  m_temperatureSpin->setValue(m_model->monty.temperature);
  m_kineticsSpin->setValue(m_model->monty.kinetics);

  /* Monitor */
  m_multiFrameXyzCheck->setChecked(m_model->monty.multi_frame_xyz);
  m_monitorHeightCheck->setChecked(m_model->monty.monitor_height);
  m_monitorEnergyCheck->setChecked(m_model->monty.monitor_energy);
  m_monitorHhcorrCheck->setChecked(m_model->monty.monitor_hhcorr);
  m_monitorDiffusionCheck->setChecked(m_model->monty.monitor_diffusion_profile);

  /* Run */
  m_randomSeedEdit->setText(QString::fromUtf8(m_model->monty.random_seed ? m_model->monty.random_seed : ""));
  m_rowsSpin->setValue(m_model->monty.rows);
  m_colsSpin->setValue(m_model->monty.cols);
  m_layersSpin->setValue(m_model->monty.layers);
  m_incrementSpin->setValue(m_model->monty.increment);
  m_relaxSpin->setValue(m_model->monty.relax);
  m_cyclesSpin->setValue(m_model->monty.cycles);
  m_movesSpin->setValue(m_model->monty.moves);
}

void MontyDialog::setupCrystalGraphPage()
{
  auto *page = new QWidget();
  auto *vbox = new QVBoxLayout(page);
  vbox->setSpacing(8);

  auto *frame = new QGroupBox(tr("Crystal Graph Cutoffs"), page);
  auto *form = new QFormLayout(frame);
  form->setSpacing(4);

  m_imageXSpin = new QDoubleSpinBox();
  m_imageXSpin->setRange(1.0, 1e10);
  m_imageXSpin->setSingleStep(1.0);
  form->addRow(tr("Cutoff in a-direction (A)"), m_imageXSpin);
  connect(m_imageXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          [this](double v) { m_model->monty.image_x = v; });

  m_imageYSpin = new QDoubleSpinBox();
  m_imageYSpin->setRange(1.0, 1e10);
  m_imageYSpin->setSingleStep(1.0);
  form->addRow(tr("Cutoff in b-direction (A)"), m_imageYSpin);
  connect(m_imageYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          [this](double v) { m_model->monty.image_y = v; });

  m_imageZSpin = new QDoubleSpinBox();
  m_imageZSpin->setRange(1.0, 1e10);
  m_imageZSpin->setSingleStep(1.0);
  form->addRow(tr("Cutoff in c-direction (A)"), m_imageZSpin);
  connect(m_imageZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          [this](double v) { m_model->monty.image_z = v; });

  vbox->addWidget(frame);

  auto *calcBtn = new QPushButton(tr("Calculate crystal graph"), page);
  vbox->addWidget(calcBtn);
  connect(calcBtn, &QPushButton::clicked, this, &MontyDialog::on_calculate_crystal_graph);

  m_notebook->addTab(page, tr("Crystal Graph"));
}

void MontyDialog::setupInputPage()
{
  auto *page = new QWidget();
  auto *vbox = new QVBoxLayout(page);
  vbox->setSpacing(8);

  /* File selection table */
  auto *form = new QFormLayout();
  form->setSpacing(4);

  auto *cgfHbox = new QHBoxLayout();
  m_cgfEdit = new QLineEdit();
  cgfHbox->addWidget(m_cgfEdit);
  auto *cgfBtn = new QPushButton(tr("Browse"));
  cgfHbox->addWidget(cgfBtn);
  form->addRow(tr("Input CGF Filename"), cgfHbox);
  connect(cgfBtn, &QPushButton::clicked, this, &MontyDialog::on_select_cgf);

  auto *surfHbox = new QHBoxLayout();
  m_surfaceEdit = new QLineEdit();
  surfHbox->addWidget(m_surfaceEdit);
  auto *surfBtn = new QPushButton(tr("Browse"));
  surfHbox->addWidget(surfBtn);
  form->addRow(tr("Input Surface Filename"), surfHbox);
  connect(surfBtn, &QPushButton::clicked, this, &MontyDialog::on_select_surface);

  vbox->addLayout(form);

  /* Horizontal split for HKL, output dirs, supersaturations */
  auto *hbox = new QHBoxLayout();

  auto *hklFrame = new QGroupBox(tr("HKL Values"), page);
  auto *hklVbox = new QVBoxLayout(hklFrame);
  m_hklsEdit = new QTextEdit();
  m_hklsEdit->setMinimumHeight(120);
  hklVbox->addWidget(m_hklsEdit);
  hbox->addWidget(hklFrame, 1);

  auto *dirFrame = new QGroupBox(tr("Output directories"), page);
  auto *dirVbox = new QVBoxLayout(dirFrame);
  m_outputDirsEdit = new QTextEdit();
  m_outputDirsEdit->setMinimumHeight(120);
  dirVbox->addWidget(m_outputDirsEdit);
  hbox->addWidget(dirFrame, 1);

  auto *supFrame = new QGroupBox(tr("Driving forces"), page);
  auto *supVbox = new QVBoxLayout(supFrame);
  m_supersaturationsEdit = new QTextEdit();
  m_supersaturationsEdit->setMinimumHeight(120);
  supVbox->addWidget(m_supersaturationsEdit);
  hbox->addWidget(supFrame, 1);

  vbox->addLayout(hbox);

  /* Energy unit radio buttons */
  auto *euFrame = new QGroupBox(tr("Energy unit"), page);
  auto *euVbox = new QVBoxLayout(euFrame);
  euVbox->setSpacing(4);

  /* Use combo for energy unit */
  m_energyUnitCombo = new QComboBox();
  m_energyUnitCombo->addItems({tr("kcal/mol"), tr("kJ/mol")});
  euVbox->addWidget(m_energyUnitCombo);

  connect(m_energyUnitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &MontyDialog::on_energy_unit_changed);

  vbox->addWidget(euFrame);

  /* Solvation energy */
  auto *form2 = new QFormLayout();
  form2->setSpacing(4);
  m_esolvEdit = new QLineEdit();
  form2->addRow(tr("Solvation energy"), m_esolvEdit);
  connect(m_esolvEdit, &QLineEdit::textChanged, this, [this](const QString &t) {
    g_free(m_model->monty.esolv);
    m_model->monty.esolv = g_strdup(t.toUtf8().constData());
  });
  vbox->addLayout(form2);

  m_notebook->addTab(page, tr("Input"));
}

void MontyDialog::setupOutputPage()
{
  auto *page = new QWidget();
  auto *vbox = new QVBoxLayout(page);
  vbox->setSpacing(8);

  auto *form = new QFormLayout();
  form->setSpacing(4);

  m_outputExtEdit = new QLineEdit();
  form->addRow(tr("File extension"), m_outputExtEdit);
  connect(m_outputExtEdit, &QLineEdit::textChanged, this, [this](const QString &t) {
    g_free(m_model->monty.output_extension);
    m_model->monty.output_extension = g_strdup(t.toUtf8().constData());
  });
  vbox->addLayout(form);

  auto *frame = new QGroupBox(tr("Output file types"), page);
  auto *vbox2 = new QVBoxLayout(frame);
  vbox2->setSpacing(4);

  m_writeSurfaceCheck = new QCheckBox(tr("Write Monty surface"));
  m_writeXyzCheck = new QCheckBox(tr("Write XYZ surface"));
  m_writeMatlabCheck = new QCheckBox(tr("Write Matlab surface"));
  m_writeMsiCheck = new QCheckBox(tr("Write MSI surface"));

  vbox2->addWidget(m_writeSurfaceCheck);
  vbox2->addWidget(m_writeXyzCheck);
  vbox2->addWidget(m_writeMatlabCheck);
  vbox2->addWidget(m_writeMsiCheck);

  connect(m_writeSurfaceCheck, &QCheckBox::toggled, this, [this](bool v) { m_model->monty.write_surface = v; });
  connect(m_writeXyzCheck, &QCheckBox::toggled, this, [this](bool v) { m_model->monty.write_xyz = v; });
  connect(m_writeMatlabCheck, &QCheckBox::toggled, this, [this](bool v) { m_model->monty.write_matlab = v; });
  connect(m_writeMsiCheck, &QCheckBox::toggled, this, [this](bool v) { m_model->monty.write_msi = v; });

  vbox->addWidget(frame);
  m_notebook->addTab(page, tr("Output"));
}

void MontyDialog::setupModelPage()
{
  auto *page = new QWidget();
  auto *vbox = new QVBoxLayout(page);
  vbox->setSpacing(8);

  auto *frame = new QGroupBox(tr("Model parameters"), page);
  auto *form = new QFormLayout(frame);
  form->setSpacing(4);

  m_spiralCheck = new QCheckBox(tr("Spiral growth"));
  form->addRow("", m_spiralCheck);
  connect(m_spiralCheck, &QCheckBox::toggled, this, [this](bool v) { m_model->monty.spiral = v; });

  m_xstepsSpin = new QDoubleSpinBox();
  m_xstepsSpin->setRange(-1e10, 1e10);
  m_xstepsSpin->setSingleStep(1.0);
  form->addRow(tr("X-steps"), m_xstepsSpin);
  connect(m_xstepsSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          [this](double v) { m_model->monty.xsteps = v; });

  m_ystepsSpin = new QDoubleSpinBox();
  m_ystepsSpin->setRange(-1e10, 1e10);
  m_ystepsSpin->setSingleStep(1.0);
  form->addRow(tr("Y-steps"), m_ystepsSpin);
  connect(m_ystepsSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          [this](double v) { m_model->monty.ysteps = v; });

  m_temperatureSpin = new QDoubleSpinBox();
  m_temperatureSpin->setRange(0.0, 1e10);
  m_temperatureSpin->setSingleStep(1.0);
  form->addRow(tr("Temperature (K)"), m_temperatureSpin);
  connect(m_temperatureSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          [this](double v) { m_model->monty.temperature = v; });

  m_kineticsSpin = new QDoubleSpinBox();
  m_kineticsSpin->setRange(0.0, 1.0);
  m_kineticsSpin->setSingleStep(0.01);
  form->addRow(tr("Kinetics factor"), m_kineticsSpin);
  connect(m_kineticsSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          [this](double v) { m_model->monty.kinetics = v; });

  vbox->addWidget(frame);
  m_notebook->addTab(page, tr("Model"));
}

void MontyDialog::setupMonitorPage()
{
  auto *page = new QWidget();
  auto *vbox = new QVBoxLayout(page);
  vbox->setSpacing(8);

  auto *frame = new QGroupBox(tr("Monitoring options"), page);
  auto *vbox2 = new QVBoxLayout(frame);
  vbox2->setSpacing(4);

  m_multiFrameXyzCheck = new QCheckBox(tr("Create a multi-frame .xyz file"));
  m_monitorHeightCheck = new QCheckBox(tr("Monitor average height"));
  m_monitorEnergyCheck = new QCheckBox(tr("Monitor surface energy"));
  m_monitorHhcorrCheck = new QCheckBox(tr("Monitor height correlation"));
  m_monitorDiffusionCheck = new QCheckBox(tr("Monitor diffusion profile"));

  vbox2->addWidget(m_multiFrameXyzCheck);
  vbox2->addWidget(m_monitorHeightCheck);
  vbox2->addWidget(m_monitorEnergyCheck);
  vbox2->addWidget(m_monitorHhcorrCheck);
  vbox2->addWidget(m_monitorDiffusionCheck);

  connect(m_multiFrameXyzCheck, &QCheckBox::toggled, this, [this](bool v) { m_model->monty.multi_frame_xyz = v; });
  connect(m_monitorHeightCheck, &QCheckBox::toggled, this, [this](bool v) { m_model->monty.monitor_height = v; });
  connect(m_monitorEnergyCheck, &QCheckBox::toggled, this, [this](bool v) { m_model->monty.monitor_energy = v; });
  connect(m_monitorHhcorrCheck, &QCheckBox::toggled, this, [this](bool v) { m_model->monty.monitor_hhcorr = v; });
  connect(m_monitorDiffusionCheck, &QCheckBox::toggled, this,
          [this](bool v) { m_model->monty.monitor_diffusion_profile = v; });

  vbox->addWidget(frame);
  m_notebook->addTab(page, tr("Monitor"));
}

void MontyDialog::setupRunPage()
{
  auto *page = new QWidget();
  auto *vbox = new QVBoxLayout(page);
  vbox->setSpacing(8);

  auto *frame = new QGroupBox(tr("Run parameters"), page);
  auto *form = new QFormLayout(frame);
  form->setSpacing(4);

  m_randomSeedEdit = new QLineEdit();
  form->addRow(tr("Random Seed"), m_randomSeedEdit);
  connect(m_randomSeedEdit, &QLineEdit::textChanged, this, [this](const QString &t) {
    g_free(m_model->monty.random_seed);
    m_model->monty.random_seed = g_strdup(t.toUtf8().constData());
  });

  m_rowsSpin = new QDoubleSpinBox();
  m_rowsSpin->setRange(5.0, 1e10);
  m_rowsSpin->setSingleStep(1.0);
  form->addRow(tr("Rows"), m_rowsSpin);
  connect(m_rowsSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          [this](double v) { m_model->monty.rows = v; });

  m_colsSpin = new QDoubleSpinBox();
  m_colsSpin->setRange(5.0, 1e10);
  m_colsSpin->setSingleStep(1.0);
  form->addRow(tr("Columns"), m_colsSpin);
  connect(m_colsSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          [this](double v) { m_model->monty.cols = v; });

  m_layersSpin = new QDoubleSpinBox();
  m_layersSpin->setRange(5.0, 1e10);
  m_layersSpin->setSingleStep(1.0);
  form->addRow(tr("Layers"), m_layersSpin);
  connect(m_layersSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          [this](double v) { m_model->monty.layers = v; });

  m_incrementSpin = new QDoubleSpinBox();
  m_incrementSpin->setRange(2.0, 1e10);
  m_incrementSpin->setSingleStep(1.0);
  form->addRow(tr("Increment"), m_incrementSpin);
  connect(m_incrementSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          [this](double v) { m_model->monty.increment = v; });

  m_relaxSpin = new QDoubleSpinBox();
  m_relaxSpin->setRange(0.0, 1000.0);
  m_relaxSpin->setSingleStep(1.0);
  form->addRow(tr("Relaxation steps"), m_relaxSpin);
  connect(m_relaxSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          [this](double v) { m_model->monty.relax = v; });

  m_cyclesSpin = new QDoubleSpinBox();
  m_cyclesSpin->setRange(0.0, 1000.0);
  m_cyclesSpin->setSingleStep(1.0);
  form->addRow(tr("Cycles"), m_cyclesSpin);
  connect(m_cyclesSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          [this](double v) { m_model->monty.cycles = v; });

  m_movesSpin = new QDoubleSpinBox();
  m_movesSpin->setRange(0.0, 1000.0);
  m_movesSpin->setSingleStep(1.0);
  form->addRow(tr("Moves"), m_movesSpin);
  connect(m_movesSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          [this](double v) { m_model->monty.moves = v; });

  vbox->addWidget(frame);
  m_notebook->addTab(page, tr("Run"));
}

void MontyDialog::on_select_cgf()
{
  QString fileName =
      QFileDialog::getOpenFileName(this, tr("Select a CGF File"), m_cgfEdit->text(), tr("CGF files (*.cgf *.CGF)"));
  if (fileName.isEmpty())
    return;
  m_cgfEdit->setText(fileName);
  g_free(m_model->monty.input_cgf);
  m_model->monty.input_cgf = g_strdup(fileName.toUtf8().constData());
}

void MontyDialog::on_energy_unit_changed(int idx)
{
  if (!m_model)
    return;
  g_free(m_model->monty.energy_unit);
  m_model->monty.energy_unit = g_strdup(idx == 0 ? "kcal/mol" : "kJ/mol");
}

void MontyDialog::on_select_surface()
{
  QString fileName = QFileDialog::getOpenFileName(this, tr("Select a Surface File"), m_surfaceEdit->text(),
                                                  tr("Monty surface files (*.monty2)"));
  if (fileName.isEmpty())
    return;
  m_surfaceEdit->setText(fileName);
  g_free(m_model->monty.input_surface);
  m_model->monty.input_surface = g_strdup(fileName.toUtf8().constData());
}

void MontyDialog::on_calculate_crystal_graph()
{
  /* Placeholder: crystal graph calculation would go here */
  QMessageBox::information(this, tr("Crystal Graph"),
                           tr("Crystal graph calculation not yet implemented in Qt version."));
}

void MontyDialog::on_run()
{
  if (!m_model)
    return;
  if (!sysenv.monty_path)
  {
    QMessageBox::warning(this, tr("Monty"),
                         tr("Monty executable was not found.\n"
                            "Set the path in View > Executable paths."));
    return;
  }

  qt_monty_task(m_model);
  close();
}

void MontyDialog::on_close()
{
  /* Save text fields */
  if (!m_model)
    return;
  g_free(m_model->monty.hkls);
  m_model->monty.hkls = g_strdup(m_hklsEdit->toPlainText().toUtf8().constData());
  g_free(m_model->monty.output_dirs);
  m_model->monty.output_dirs = g_strdup(m_outputDirsEdit->toPlainText().toUtf8().constData());
  g_free(m_model->monty.supersaturations);
  m_model->monty.supersaturations = g_strdup(m_supersaturationsEdit->toPlainText().toUtf8().constData());
  g_free(m_model->monty.input_cgf);
  m_model->monty.input_cgf = g_strdup(m_cgfEdit->text().toUtf8().constData());
  g_free(m_model->monty.input_surface);
  m_model->monty.input_surface = g_strdup(m_surfaceEdit->text().toUtf8().constData());
  g_free(m_model->monty.esolv);
  m_model->monty.esolv = g_strdup(m_esolvEdit->text().toUtf8().constData());
  g_free(m_model->monty.output_extension);
  m_model->monty.output_extension = g_strdup(m_outputExtEdit->text().toUtf8().constData());
  g_free(m_model->monty.random_seed);
  m_model->monty.random_seed = g_strdup(m_randomSeedEdit->text().toUtf8().constData());

  close();
}

/* Bridge function — called from mainwindow.cpp */
extern "C" void qt_show_monty_dialog(struct model_pak *model)
{
  extern QWidget *get_main_window_widget();
  MontyDialog *dlg = new MontyDialog(model, get_main_window_widget());
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->show();
  dlg->raise();
  dlg->activateWindow();
}
