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

#ifndef VASPDialog_H
#define VaspDialog_H

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
#include <QMessageBox>
#include <QPlainTextEdit>

class VaspDialog : public QDialog
{
  Q_OBJECT

public:
  explicit VaspDialog(QWidget *parent = nullptr);
  ~VaspDialog() override;

  void refresh();
  void populatePoscarFromModel();

private slots:
  void on_save();
  void on_run();
  void on_close();
  void updateKpointsModeEnabled();
  void on_prec_changed(int index);
  void on_algo_changed(int index);
  void on_ialgo_changed(int index);
  void on_mixer_changed(int index);
  void on_mixpre_changed(int index);
  void on_inimix_changed(int index);
  void on_ibrion_changed(int index);
  void on_kpoints_mode_changed(int index);
  void on_calculation_type_changed(int index);
  void on_apply_simple();
  void on_potcar_mode_changed();
  void sync();
  void poscar_sync();
  void write_poscar(FILE *output);
  void write_kpoints(FILE *output);
  void write_potcar(FILE *output);
  void apply_tooltips();

private:
  void setupPresetPage();
  void setupConvergencePage();
  void setupElectronicPage1();
  void setupElectronicPage2();
  void setupIonicPage();
  void setupKpointsPage();
  void updatePoscarAtoms();
  void setupPotcarPage();
  void setupExecPage();
  void potcar_folder_get_info(gchar *folderPath);
  void potcar_folder_register(gchar *item);
  void applyParallelConstraints();

  /* Tab widgets */
  QTabWidget *m_notebook = nullptr;

  /* PRESET page */
  QComboBox *m_simpleCalculCombo = nullptr;
  QCheckBox *m_simpleRgeomCheck = nullptr;
  QSpinBox *m_simpleDimSpin = nullptr;
  QComboBox *m_simpleSystemCombo = nullptr;
  QLineEdit *m_simplePoscarEdit = nullptr;
  QComboBox *m_simpleKgridCombo = nullptr;
  QLineEdit *m_simpleSpeciesEdit = nullptr;
  QLineEdit *m_simplePotcarEdit = nullptr;
  QPushButton *m_simplePotcarBtn = nullptr;
  QSpinBox *m_simpleNpSpin = nullptr;
  QSpinBox *m_simpleNcoreSpin = nullptr;
  QSpinBox *m_simpleKparSpin = nullptr;
  QPushButton *m_simpleApplyBtn = nullptr;
  QPlainTextEdit *m_simpleMessageText = nullptr;

  /* CONVERGENCE page */
  QLineEdit *m_nameEdit = nullptr;
  QLineEdit *m_fileEntryEdit = nullptr;
  QPushButton *m_fileEntryBtn = nullptr;
  QComboBox *m_precCombo = nullptr;
  QCheckBox *m_usePrecCheck = nullptr;
  QLineEdit *m_encutEdit = nullptr;
  QLineEdit *m_enaugEdit = nullptr;
  QLineEdit *m_ediffEdit = nullptr;
  QComboBox *m_algoCombo = nullptr;
  QCheckBox *m_ldiagCheck = nullptr;
  QLineEdit *m_nsimEdit = nullptr;
  QLineEdit *m_vtimeEdit = nullptr;
  QLineEdit *m_iwavprEdit = nullptr;
  QComboBox *m_ialgoCombo = nullptr;
  QCheckBox *m_autoElecCheck = nullptr;
  QLineEdit *m_nbandsEdit = nullptr;
  QLineEdit *m_nelectEdit = nullptr;
  QCheckBox *m_iniwavCheck = nullptr;
  QLineEdit *m_istartEdit = nullptr;
  QLineEdit *m_ichargEdit = nullptr;
  QComboBox *m_mixerCombo = nullptr;
  QCheckBox *m_autoMixerCheck = nullptr;
  QLineEdit *m_nelmEdit = nullptr;
  QLineEdit *m_nelmdlEdit = nullptr;
  QLineEdit *m_nelminEdit = nullptr;
  QComboBox *m_mixpreCombo = nullptr;
  QLineEdit *m_amixEdit = nullptr;
  QLineEdit *m_bmixEdit = nullptr;
  QLineEdit *m_aminEdit = nullptr;
  QComboBox *m_inimixCombo = nullptr;
  QLineEdit *m_maxmixEdit = nullptr;
  QLineEdit *m_amixMagEdit = nullptr;
  QLineEdit *m_bmixMagEdit = nullptr;
  QLineEdit *m_wcEdit = nullptr;
  QLineEdit *m_lmaxmixEdit = nullptr;
  QLineEdit *m_lmaxpawEdit = nullptr;
  QCheckBox *m_addgridCheck = nullptr;
  QCheckBox *m_autoGridCheck = nullptr;

  /* ELECT-I page */
  QLineEdit *m_ismearEdit = nullptr;
  QLineEdit *m_sigmaEdit = nullptr;
  QCheckBox *m_kgammaCheck = nullptr;
  QLineEdit *m_kspacingEdit = nullptr;
  QLineEdit *m_fermweEdit = nullptr;
  QLineEdit *m_fermdoEdit = nullptr;
  QComboBox *m_lrealCombo = nullptr;
  QLineEdit *m_convRoptEdit = nullptr;
  QComboBox *m_elecLrealCombo = nullptr;

  QComboBox *m_ggaCombo = nullptr;
  QCheckBox *m_voskownCheck = nullptr;
  QCheckBox *m_lasphCheck = nullptr;
  QCheckBox *m_spinCheck = nullptr;
  QCheckBox *m_lnoncollCheck = nullptr;
  QCheckBox *m_lsorbitCheck = nullptr;
  QLineEdit *m_saxisEdit = nullptr;
  QCheckBox *m_ggaCompatCheck = nullptr;
  QLineEdit *m_nupdownEdit = nullptr;
  QLineEdit *m_magmomEdit = nullptr;
  QCheckBox *m_lmetaggaCheck = nullptr;
  QLineEdit *m_metaggaEdit = nullptr;
  QComboBox *m_metaggaCombo = nullptr;
  QCheckBox *m_lmixtauCheck = nullptr;
  QLineEdit *m_lmaxtauEdit = nullptr;
  QLineEdit *m_cmbjEdit = nullptr;
  QLineEdit *m_cmbjaEdit = nullptr;
  QLineEdit *m_cmbjbEdit = nullptr;
  QComboBox *m_ldauPrintCombo = nullptr;
  QLineEdit *m_dipolEdit = nullptr;
  QCheckBox *m_ldauCheck = nullptr;
  QComboBox *m_ldauTypeCombo = nullptr;
  QLineEdit *m_ldaulEdit = nullptr;
  QLineEdit *m_ldauuEdit = nullptr;
  QLineEdit *m_ldaujEdit = nullptr;
  QCheckBox *m_ldipolCheck = nullptr;
  QComboBox *m_idipolCombo = nullptr;
  QLineEdit *m_epsilonEdit = nullptr;
  QLineEdit *m_efieldEdit = nullptr;
  QCheckBox *m_lmonoCheck = nullptr;

  /* ELECT-II page */
  QComboBox *m_lorbitCombo = nullptr;
  QLineEdit *m_nedosEdit = nullptr;
  QLineEdit *m_eminEdit = nullptr;
  QLineEdit *m_emaxEdit = nullptr;
  QLineEdit *m_efermiEdit = nullptr;
  QCheckBox *m_havePawCheck = nullptr;
  QLineEdit *m_rwigsEdit = nullptr;
  QCheckBox *m_lopticsCheck = nullptr;
  QCheckBox *m_lepsilonCheck = nullptr;
  QCheckBox *m_lrpaCheck = nullptr;
  QCheckBox *m_lnablaCheck = nullptr;

  QLineEdit *m_cshiftEdit = nullptr;
  QLineEdit *m_ngxEdit = nullptr;
  QLineEdit *m_ngyEdit = nullptr;
  QLineEdit *m_ngzEdit = nullptr;
  QLineEdit *m_ngxfEdit = nullptr;
  QLineEdit *m_ngyfEdit = nullptr;
  QLineEdit *m_ngzfEdit = nullptr;

  /* IONIC page */
  QLineEdit *m_nswEdit = nullptr;
  QComboBox *m_ibrionCombo = nullptr;
  QComboBox *m_isifCombo = nullptr;
  QCheckBox *m_relaxIonsCheck = nullptr;
  QCheckBox *m_relaxShapeCheck = nullptr;
  QCheckBox *m_relaxVolumeCheck = nullptr;
  QLineEdit *m_ediffgEdit = nullptr;
  QLineEdit *m_pstressEdit = nullptr;
  QLineEdit *m_nfreeEdit = nullptr;
  QLineEdit *m_potimEdit = nullptr;
  QLineEdit *m_tebegEdit = nullptr;
  QLineEdit *m_teendEdit = nullptr;
  QLineEdit *m_smassEdit = nullptr;
  QLineEdit *m_nblockEdit = nullptr;
  QLineEdit *m_kblockEdit = nullptr;
  QLineEdit *m_npacoEdit = nullptr;
  QLineEdit *m_apacoEdit = nullptr;
  QComboBox *m_poscarFreeCombo = nullptr;
  QCheckBox *m_poscarSdCheck = nullptr;
  QLineEdit *m_isymEdit = nullptr;
  QLineEdit *m_symPrecEdit = nullptr;
  QLineEdit *m_poscarA0Edit = nullptr;
  QCheckBox *m_poscarDirectCheck = nullptr;
  QLineEdit *m_poscarUxEdit = nullptr;
  QLineEdit *m_poscarUyEdit = nullptr;
  QLineEdit *m_poscarUzEdit = nullptr;
  QLineEdit *m_poscarVxEdit = nullptr;
  QLineEdit *m_poscarVyEdit = nullptr;
  QLineEdit *m_poscarVzEdit = nullptr;
  QLineEdit *m_poscarWxEdit = nullptr;
  QLineEdit *m_poscarWyEdit = nullptr;
  QLineEdit *m_poscarWzEdit = nullptr;
  QCheckBox *m_poscarTxCheck = nullptr;
  QCheckBox *m_poscarTyCheck = nullptr;
  QCheckBox *m_poscarTzCheck = nullptr;
  QComboBox *m_poscarAtomsCombo = nullptr;
  QLineEdit *m_poscarIndexEdit = nullptr;
  QLineEdit *m_poscarSymbolEdit = nullptr;
  QLineEdit *m_poscarXEdit = nullptr;
  QLineEdit *m_poscarYEdit = nullptr;
  QLineEdit *m_poscarZEdit = nullptr;
  QPushButton *m_poscarApplyBtn = nullptr;
  QPushButton *m_poscarDeleteBtn = nullptr;

  /* KPOINTS page */
  QLineEdit *m_kpointsIsmearEdit = nullptr;
  QCheckBox *m_kpointsGammaCheck = nullptr;
  QLineEdit *m_kpointsKspacingEdit = nullptr;
  QComboBox *m_kpointsModeCombo = nullptr;
  QCheckBox *m_kpointsCartCheck = nullptr;
  QSpinBox *m_kpointsKxSpin = nullptr;
  QSpinBox *m_kpointsKySpin = nullptr;
  QSpinBox *m_kpointsKzSpin = nullptr;
  QLineEdit *m_kpointsNkptsEdit = nullptr;
  QDoubleSpinBox *m_kpointsSxSpin = nullptr;
  QLineEdit *m_kpointsSyEdit = nullptr;
  QLineEdit *m_kpointsSzEdit = nullptr;
  QCheckBox *m_tetraCheck = nullptr;
  QComboBox *m_kpointsKptsCombo = nullptr;
  QPushButton *m_kpointsApplyBtn = nullptr;
  QPushButton *m_kpointsDelBtn = nullptr;
  QLineEdit *m_kpointsIndexEdit = nullptr;
  QLineEdit *m_kpointsXEdit = nullptr;
  QLineEdit *m_kpointsYEdit = nullptr;
  QLineEdit *m_kpointsZEdit = nullptr;
  QLineEdit *m_kpointsWEdit = nullptr;
  QLineEdit *m_tetraTotalEdit = nullptr;
  QLineEdit *m_tetraVolumeEdit = nullptr;
  QComboBox *m_tetraCombo = nullptr;
  QPushButton *m_tetraApplyBtn = nullptr;
  QPushButton *m_tetraDelBtn = nullptr;
  QLineEdit *m_tetraIndexEdit = nullptr;
  QLineEdit *m_tetraWEdit = nullptr;
  QLineEdit *m_tetraPtAEdit = nullptr;
  QLineEdit *m_tetraPtBEdit = nullptr;
  QLineEdit *m_tetraPtCEdit = nullptr;
  QLineEdit *m_tetraPtDEdit = nullptr;
  QGroupBox *m_tetraFrame = nullptr;

  /* POTCAR page */
  QLineEdit *m_potcarSpeciesEdit = nullptr;
  QRadioButton *m_potcarSelectFileRadio = nullptr;
  QRadioButton *m_potcarSelectFolderRadio = nullptr;
  QLineEdit *m_potcarFileEdit = nullptr;
  QPushButton *m_potcarFileBtn = nullptr;
  QLineEdit *m_potcarFolderEdit = nullptr;
  QPushButton *m_potcarFolderBtn = nullptr;
  QComboBox *m_potcarFlavorCombo = nullptr;
  QPushButton *m_potcarApplyBtn = nullptr;
  QLineEdit *m_potcarDetectedSpeciesEdit = nullptr;
  QLineEdit *m_potcarDetectedFlavorEdit = nullptr;

  /* EXEC page */
  QLineEdit *m_jobVaspExeEdit = nullptr;
  QPushButton *m_jobVaspExeBtn = nullptr;
  QLineEdit *m_jobMpirunEdit = nullptr;
  QPushButton *m_jobMpirunBtn = nullptr;
  QLineEdit *m_jobPathEdit = nullptr;
  QPushButton *m_jobPathBtn = nullptr;
  QSpinBox *m_jobNprocSpin = nullptr;
  QSpinBox *m_ncoreSpin = nullptr;
  QSpinBox *m_kparSpin = nullptr;
  QCheckBox *m_lplaneCheck = nullptr;
  QCheckBox *m_lscaluCheck = nullptr;
  QCheckBox *m_lscalapackCheck = nullptr;
  QCheckBox *m_lwaveCheck = nullptr;
  QCheckBox *m_lchargCheck = nullptr;
  QCheckBox *m_lvtotCheck = nullptr;
  QCheckBox *m_lvharCheck = nullptr;
  QCheckBox *m_lelfCheck = nullptr;

  /* Bottom button bar */
  QPushButton *m_buttonSave = nullptr;
  QPushButton *m_buttonExec = nullptr;
  QPushButton *m_buttonClose = nullptr;
};

#endif // VaspDialog_H
