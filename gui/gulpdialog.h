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

#ifndef GULPDIALOG_H
#define GULPDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QRadioButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QLabel>
#include <QGroupBox>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QSlider>
#include <QButtonGroup>

struct model_pak;

class GulpDialog : public QDialog
{
  Q_OBJECT

public:
  explicit GulpDialog(struct model_pak *model, QWidget *parent = nullptr);
  ~GulpDialog();

private slots:
  void on_execute();
  void on_close();
  void on_jobname_changed();

public:
  void syncEnergyToUI();

private:
  void setupUI();
  void populateControlTab(QTabWidget *tabWidget);
  void populateFilesTab(QTabWidget *tabWidget);
  void populateOptimisationTab(QTabWidget *tabWidget);
  void populatePotentialsTab(QTabWidget *tabWidget);
  void populateElementsTab(QTabWidget *tabWidget);
  void populateUnprocessedTab(QTabWidget *tabWidget);
  void populateVibrationalTab(QTabWidget *tabWidget);
  void populateSolvationTab(QTabWidget *tabWidget);
  void populateCommonFrames();

  void syncFromModel();
  void syncToModel();

  struct model_pak *m_model;
  QTabWidget *m_tabWidget;
  QButtonGroup *m_runGroup;
  QButtonGroup *m_constraintGroup;
  QButtonGroup *m_coulombGroup;
  QButtonGroup *m_ensembleGroup;
  QButtonGroup *m_primaryOptGroup;
  QButtonGroup *m_secondaryOptGroup;
  QButtonGroup *m_switchGroup;

  /* Control tab */
  QRadioButton *m_rbSingle, *m_rbOptimize, *m_rbDynamics;
  QRadioButton *m_rbConp, *m_rbConv;
  QRadioButton *m_rbMole, *m_rbMolmec, *m_rbMolq, *m_rbNobuild;
  QCheckBox *m_chkFix, *m_chkNoautobond;
  QLineEdit *m_editTemp, *m_editPressure;
  QRadioButton *m_rbNVE, *m_rbNVT, *m_rbNPT;
  QLineEdit *m_editTimestep, *m_editEquilibration, *m_editProduction;
  QLineEdit *m_editSample, *m_editWrite;
  QCheckBox *m_chkNoExec, *m_chkNosym;
  QCheckBox *m_chkNoEatt, *m_chkQeq;

  /* Files tab */
  QLineEdit *m_editInputFile, *m_editDumpFile, *m_editTrajFile;
  QCheckBox *m_chkPrintCharge;

  /* Optimisation tab */
  QRadioButton *m_rbBfgsOpt, *m_rbConjOpt, *m_rbRfoOpt;
  QRadioButton *m_rbNoOpt2, *m_rbBfgsOpt2, *m_rbConjOpt2, *m_rbRfoOpt2;
  QRadioButton *m_rbCycle, *m_rbGnorm;
  QLineEdit *m_editSwitchValue;
  QSpinBox *m_spinMaxCyc;

  /* Potentials tab */
  QTextEdit *m_editPotentials;
  QLineEdit *m_editLibFile;

  /* Elements tab */
  QTextEdit *m_editElements, *m_editSpecies;

  /* Unprocessed tab */
  QCheckBox *m_chkOutputExtraKeywords, *m_chkOutputExtra;
  QLineEdit *m_editExtraKeywords;
  QTextEdit *m_editExtra;

  /* Vibrational tab */
  QCheckBox *m_chkPhonon, *m_chkEigen;
  QTextEdit *m_editKpoints;
  QCheckBox *m_chkShowEigenvectors;
  QDoubleSpinBox *m_spinPhononScaling;
  QSpinBox *m_spinAnimResolution;
  QLineEdit *m_editMovieName;
  QSlider *m_phononSlider;

  /* Solvation tab */
  QComboBox *m_comboSolvationModel;
  QDoubleSpinBox *m_spinSolventEpsilon, *m_spinSolventRadius;
  QDoubleSpinBox *m_spinSolventDelta, *m_spinSolventRmax;
  QDoubleSpinBox *m_spinSmoothing;
  QComboBox *m_comboShapeApprox;
  QSpinBox *m_spinIndexK, *m_spinIndexL;
  QSpinBox *m_spinSegments;

  /* Common frames */
  QLineEdit *m_editJobName;

  /* Energy display fields (updated by proc_gulp_task) */
  QLineEdit *m_editEnergy;
  QLineEdit *m_editSbe;
  QLineEdit *m_editSdipole;
  QLineEdit *m_editEsurf;
  QLineEdit *m_editEatt;
};

#endif // GULPDIALOG_H
