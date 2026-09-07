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

#ifndef MONTYDIALOG_H
#define MONTYDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QGroupBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QMessageBox>

struct model_pak;

class MontyDialog : public QDialog
{
  Q_OBJECT

public:
  explicit MontyDialog(struct model_pak *model, QWidget *parent = nullptr);
  ~MontyDialog() override;

  void refresh();

private slots:
  void on_run();
  void on_close();
  void on_select_cgf();
  void on_select_surface();
  void on_energy_unit_changed(int index);
  void on_calculate_crystal_graph();

private:
  void setupCrystalGraphPage();
  void setupInputPage();
  void setupOutputPage();
  void setupModelPage();
  void setupMonitorPage();
  void setupRunPage();

  struct model_pak *m_model = nullptr;
  QTabWidget *m_notebook = nullptr;

  /* Input page widgets */
  QLineEdit *m_cgfEdit = nullptr;
  QLineEdit *m_surfaceEdit = nullptr;
  QTextEdit *m_hklsEdit = nullptr;
  QTextEdit *m_outputDirsEdit = nullptr;
  QTextEdit *m_supersaturationsEdit = nullptr;
  QComboBox *m_energyUnitCombo = nullptr;
  QLineEdit *m_esolvEdit = nullptr;

  /* Output page widgets */
  QLineEdit *m_outputExtEdit = nullptr;
  QCheckBox *m_writeSurfaceCheck = nullptr;
  QCheckBox *m_writeXyzCheck = nullptr;
  QCheckBox *m_writeMatlabCheck = nullptr;
  QCheckBox *m_writeMsiCheck = nullptr;

  /* Model page widgets */
  QCheckBox *m_spiralCheck = nullptr;
  QDoubleSpinBox *m_xstepsSpin = nullptr;
  QDoubleSpinBox *m_ystepsSpin = nullptr;
  QDoubleSpinBox *m_temperatureSpin = nullptr;
  QDoubleSpinBox *m_kineticsSpin = nullptr;

  /* Monitor page widgets */
  QCheckBox *m_multiFrameXyzCheck = nullptr;
  QCheckBox *m_monitorHeightCheck = nullptr;
  QCheckBox *m_monitorEnergyCheck = nullptr;
  QCheckBox *m_monitorHhcorrCheck = nullptr;
  QCheckBox *m_monitorDiffusionCheck = nullptr;

  /* Run page widgets */
  QLineEdit *m_randomSeedEdit = nullptr;
  QDoubleSpinBox *m_rowsSpin = nullptr;
  QDoubleSpinBox *m_colsSpin = nullptr;
  QDoubleSpinBox *m_layersSpin = nullptr;
  QDoubleSpinBox *m_incrementSpin = nullptr;
  QDoubleSpinBox *m_relaxSpin = nullptr;
  QDoubleSpinBox *m_cyclesSpin = nullptr;
  QDoubleSpinBox *m_movesSpin = nullptr;

  /* Crystal graph widgets */
  QDoubleSpinBox *m_imageXSpin = nullptr;
  QDoubleSpinBox *m_imageYSpin = nullptr;
  QDoubleSpinBox *m_imageZSpin = nullptr;
};

#endif // MONTYDIALOG_H
