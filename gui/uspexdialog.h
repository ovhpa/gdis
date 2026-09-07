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

#ifndef USPEXDIALOG_H
#define USPEXDIALOG_H

#include <QDialog>
#include <QTabWidget>

/* Forward declaration of pimpl struct — defined in uspexdialog.cpp */
struct UspexWidgets;

class UspexDialog : public QDialog
{
  Q_OBJECT

public:
  explicit UspexDialog(QWidget *parent = nullptr, struct model_pak *model = nullptr);
  ~UspexDialog();

private Q_SLOTS:
  void on_run();
  void on_close();
  void on_save();
  /* Signal handlers for interdependent widgets */
  void on_method_changed(int index);
  void on_dim_changed(int value);
  void on_mag_toggled(bool checked);
  void on_mol_toggled(bool checked);
  void on_var_toggled(bool checked);
  void on_type_changed(int index);
  void on_atomType_changed(int index);
  void on_apply_atom();
  void on_remove_atom();
  void update_numSpecies_combo();
  void on_auto_bonds_toggled(bool checked);
  void on_auto_C_lat_toggled(bool checked);
  void on_auto_C_ion_toggled(bool checked);
  void on_iondistances_selected(int index);
  void on_apply_distances();
  void on_auto_step_toggled(bool checked);
  void on_use_specific_toggled(bool checked);
  void on_set_specific_toggled(bool checked);
  void on_spe_folder_button_clicked();
  void on_curr_step_changed(int value);
  void on_num_opt_steps_changed(int value);
  void on_ai_input_button_clicked();
  void on_ai_opt_button_clicked();
  void on_load_abinitio_exe_clicked();
  void on_ai_lib_button_clicked();
  void on_apply_step_clicked();
  void on_load_uspex_exe_clicked();
  void on_uspex_path_dialog_clicked();
  void on_load_remote_folder_clicked();
  void sync_ai_lib_flavor();
  void sync_ai_lib_sel();
  void apply_ai_lib_flavor();
  void on_spacegroup_active_toggled(bool checked);
  void on_latticeformat_changed(int index);
  void on_latticevalues_selected(int index);
  void on_apply_latticevalue();
  void on_v1030_toggled(bool checked);
  void update_boltztrap_visibility();
  void on_optType_changed(int index);
  void on_new_optType_changed(const QString &text);
  void on_meta_model_changed(int index);
  void on_meta_model_button_clicked();
  void on_vcnebtype_changed();
  void on_cidi_method_changed(int index);
  void on_img_model_changed(int index);
  void on_img_model_button_clicked();
  void on_FormatType_changed(int index);
  void on_substrate_model_changed(int index);
  void on_substrate_model_button_clicked();
  void on_mol_model_changed(int index);
  void on_num_mol_changed(int value);
  void on_curr_mol_changed(int value);
  void on_mol_apply_clicked();
  void on_mol_model_button_clicked();
  void on_file_entry_button_clicked();
  void on_specifictab_visibility();

private:
  void setupSystemPage();
  void setupStructurePage();
  void setupCalculationPage();
  void setupAdvancedPage();
  void setupSpecificPage();
  void refresh();
  void sync();

  QTabWidget *m_notebook;

  /* Pimpl - all widget pointers live here to avoid header include issues */
  UspexWidgets *m_w;

  friend void wire_up_signals(UspexDialog *dlg);

  /* Separate storage for each lattice format */
  void lattice_format_switch(int oldIdx, int newIdx);
  double *m_lattice_volume = nullptr;
  double *m_lattice_lattice = nullptr;
  double *m_lattice_crystal = nullptr;
  int m_lattice_volume_n = 0;
  int m_lattice_lattice_lines = 0;
  int m_lattice_lattice_vals = 0;
  int m_lattice_crystal_n = 0;
  struct model_pak *m_model = nullptr;
};

void wire_up_signals(UspexDialog *dlg);

#endif // USPEXDIALOG_H
