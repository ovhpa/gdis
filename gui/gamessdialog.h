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

#ifndef GAMESSDIALOG_H
#define GAMESSDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QGroupBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QStackedWidget>

struct model_pak;

class GamessDialog : public QDialog
{
  Q_OBJECT

public:
  explicit GamessDialog(struct model_pak *model, QWidget *parent = nullptr);
  ~GamessDialog() override;

  void refresh();

private slots:
  void on_run();
  void on_close();
  void on_dft_toggled(bool checked);

  /* Control page */
  void on_run_type_changed(int index);
  void on_scf_type_changed(int index);
  void on_units_changed(int index);

  /* Basis page */
  void on_basis_changed(int index);

  /* Optimisation page */
  void on_opt_type_changed(int index);

private:
  void setupControlPage();
  void setupBasisPage();
  void setupOptimisationPage();

  struct model_pak *m_model = nullptr;

  /* Control page widgets */
  QRadioButton *m_exeTypeRunRadio = nullptr;
  QRadioButton *m_exeTypeCheckRadio = nullptr;
  QRadioButton *m_exeTypeDebugRadio = nullptr;
  QComboBox *m_exeTypeCombo = nullptr;
  QComboBox *m_runTypeCombo = nullptr;
  QComboBox *m_scfTypeCombo = nullptr;
  QComboBox *m_unitsCombo = nullptr;
  QSpinBox *m_maxitSpin = nullptr;
  QCheckBox *m_dftCheck = nullptr;
  QComboBox *m_functionalCombo = nullptr;
  QSpinBox *m_timeLimitSpin = nullptr;
  QSpinBox *m_mwordsSpin = nullptr;
  QCheckBox *m_wideOutputCheck = nullptr;
  QSpinBox *m_totalChargeSpin = nullptr;
  QSpinBox *m_multiplicitySpin = nullptr;

  /* Basis page widgets */
  QComboBox *m_basisCombo = nullptr;
  QSpinBox *m_numPSpin = nullptr;
  QSpinBox *m_numDSpin = nullptr;
  QSpinBox *m_numFSpin = nullptr;
  QCheckBox *m_heavyDiffuseCheck = nullptr;
  QCheckBox *m_hydrogenDiffuseCheck = nullptr;

  /* Optimisation page widgets */
  QComboBox *m_optTypeCombo = nullptr;
  QSpinBox *m_nstepSpin = nullptr;

  /* Files page widgets */
  QLineEdit *m_tempFileEdit = nullptr;

  /* Details page widgets */
  QLineEdit *m_titleEdit = nullptr;
  QLineEdit *m_energyEdit = nullptr;

  QTabWidget *m_notebook = nullptr;
};

#endif // GAMESSDIALOG_H
