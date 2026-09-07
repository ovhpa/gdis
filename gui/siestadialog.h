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

#ifndef SIESTADIALOG_H
#define SIESTADIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QGroupBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QStackedWidget>
#include <QMessageBox>

struct model_pak;

class SiestaDialog : public QDialog
{
  Q_OBJECT

public:
  explicit SiestaDialog(struct model_pak *model, QWidget *parent = nullptr);
  ~SiestaDialog() override;

  void refresh();

private slots:
  void on_save();
  void on_run();
  void on_close();
  void on_basis_set_changed(int index);
  void on_geom_runtype_changed(int index);
  void on_md_runtype_changed(int index);
  void on_long_output_toggled(bool checked);
  void on_pulay_toggled(bool checked);

private:
  void setupFileHandlerPage();
  void setupElectronicStructurePage();
  void setupSCFPage();
  void setupGeometryPage();
  void setupFileIOPage();

  struct model_pak *m_model = nullptr;
  QVBoxLayout *m_mainLayout = nullptr;
  QTabWidget *m_notebook = nullptr;
  QStackedWidget *m_geomStack = nullptr;

  /* File handler */
  QCheckBox *m_writeCsvCheck = nullptr;
  QLineEdit *m_filenameEdit = nullptr;

  /* Electronic Structure */
  QComboBox *m_basisSetCombo = nullptr;
  QSpinBox *m_customZetaSpin = nullptr;
  QSpinBox *m_customZetaPolSpin = nullptr;
  QWidget *m_customZetaFrame = nullptr;
  QDoubleSpinBox *m_splitZetaNormSpin = nullptr;
  QDoubleSpinBox *m_energyShiftSpin = nullptr;
  QSpinBox *m_meshCutoffSpin = nullptr;
  QSpinBox *m_electronicTempSpin = nullptr;
  QCheckBox *m_spinPolarisedCheck = nullptr;
  QCheckBox *m_isPeriodicCheck = nullptr;
  QDoubleSpinBox *m_kgridCutoffSpin = nullptr;

  /* SCF */
  QSpinBox *m_noOfCyclesSpin = nullptr;
  QDoubleSpinBox *m_mixingWeightSpin = nullptr;
  QCheckBox *m_pulayMixingCheck = nullptr;
  QSpinBox *m_noOfPulayMatricesSpin = nullptr;
  QCheckBox *m_divideAndConquerCheck = nullptr;

  /* Geometry */
  QComboBox *m_geomRunTypeCombo = nullptr;
  QSpinBox *m_numberOfStepsSpin = nullptr;
  QComboBox *m_mdRunTypeCombo = nullptr;
  QSpinBox *m_initialTempSpin = nullptr;
  QSpinBox *m_targetTempSpin = nullptr;
  QDoubleSpinBox *m_targetPressureSpin = nullptr;
  QSpinBox *m_initialTimeStepSpin = nullptr;
  QSpinBox *m_finalTimeStepSpin = nullptr;
  QDoubleSpinBox *m_lengthTimeStepSpin = nullptr;
  QCheckBox *m_restartCheck = nullptr;
  QCheckBox *m_optimiseCellCheck = nullptr;
  QDoubleSpinBox *m_maxCGDispSpin = nullptr;
  QDoubleSpinBox *m_maxForceTolSpin = nullptr;
  QDoubleSpinBox *m_maxStressTolSpin = nullptr;
  QDoubleSpinBox *m_targetPressureOptSpin = nullptr;
  QDoubleSpinBox *m_stressXXSpin = nullptr;
  QDoubleSpinBox *m_stressYYSpin = nullptr;
  QDoubleSpinBox *m_stressZZSpin = nullptr;
  QDoubleSpinBox *m_stressXYSpin = nullptr;
  QDoubleSpinBox *m_stressXZSpin = nullptr;
  QDoubleSpinBox *m_stressYZSpin = nullptr;
  QDoubleSpinBox *m_finiteDiffStepSpin = nullptr;

  /* File I/O */
  QCheckBox *m_longOutputCheck = nullptr;
  QDoubleSpinBox *m_dosSpin = nullptr;
  QDoubleSpinBox *m_densityOnMeshSpin = nullptr;
  QDoubleSpinBox *m_electrostaticPotSpin = nullptr;
  QCheckBox *m_writeCoorStepCheck = nullptr;
  QCheckBox *m_writeForcesCheck = nullptr;
  QCheckBox *m_writeKpointsCheck = nullptr;
  QCheckBox *m_writeEigenvaluesCheck = nullptr;
  QCheckBox *m_writeKbandsCheck = nullptr;
  QCheckBox *m_writeBandsCheck = nullptr;
  QCheckBox *m_writeWavefunctionsCheck = nullptr;
  QSpinBox *m_writeMullikenSpin = nullptr;
  QCheckBox *m_writeDmCheck = nullptr;
  QCheckBox *m_writeXmolCheck = nullptr;
  QCheckBox *m_writeCeriusCheck = nullptr;
  QCheckBox *m_writeMdXmolCheck = nullptr;
  QCheckBox *m_writeMdHistoryCheck = nullptr;
};

#endif // SIESTADIALOG_H
