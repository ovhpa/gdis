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
 * Model Editing Dialog for GDIS Qt6 GUI
 */

#ifndef EDITDIALOG_H
#define EDITDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QTableWidget>
#include <QTreeWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QGroupBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>

class EditDialog : public QDialog
{
  Q_OBJECT

public:
  explicit EditDialog(QWidget *parent = nullptr);
  ~EditDialog() override;

  void refresh();
  void populate_spatial_list();
  void sync_transmat_to_cedit();
  void do_construct_transmat();

private slots:
  /* Builder callbacks */
  void on_add_atoms();
  void on_confine_atoms();
  void on_confine_molecules();
  void on_add_shells();
  void on_del_shells();
  void on_add_bond_single();
  void on_del_bonds();
  void on_toggle_bonding();
  void on_make_model();
  void on_make_supercell();
  void on_make_p1();
  void on_create_nanotube();

  /* Spatials callbacks */
  void on_define_vector();
  void on_define_plane();
  void on_define_ribbon();
  void on_delete_all_vectors();
  void on_delete_all_planes();
  void on_delete_all_spatial();
  void on_delete_selected_spatial();
  void delete_selected_spacial();
  void on_spatial_colour_all();
  void on_spatial_colour_select();

  /* Transformations callbacks */
  void on_construct_transmat();
  void on_apply_transmat_atoms();
  void on_apply_transmat_selected();
  void on_apply_transmat_lattice();
  void on_apply_latmat();
  void on_modify_periodicity();

  /* Regions callbacks */
  void on_region_move_up();
  void on_region_move_down();
  void on_region_add_1a();
  void on_region_add_2a();
  void on_region_add_1b();
  void on_region_add_2b();
  void on_region_growth_add();
  void on_region_growth_del();

  /* Labelling callbacks */
  void on_type_model();
  void on_make_rule();
  void on_import_ff();

  /* Library callbacks */
  void on_library_save();
  void on_library_load();

  /* Tree selection */
  void on_spatial_selection_changed(const QModelIndex &current, const QModelIndex &previous);

private:
  void setupBuilderPage();
  void setupSpatialsPage();
  void setupTransformationsPage();
  void setupRegionsPage();
  void setupLabellingPage();
  void setupLibraryPage();

  /* Builder widgets */
  QSpinBox *m_chiralitySpin[2];
  QLineEdit *m_basisEntry[2];
  QDoubleSpinBox *m_nanotubeLengthSpin;

  /* Spatials widgets */
  QTableWidget *m_spatialTable;
  QColor *m_spatialColour;
  QColor *m_labelColour;

  /* Transformations widgets */
  QTableWidget *m_transTable; // 5x4: 3x3 matrix + 3 translation + labels
  QLineEdit *m_axisAngleEntry;
  QLineEdit *m_refSpatialEntry;
  QComboBox *m_constructCombo;

  /* Labelling widgets */
  QComboBox *m_ffLabelCombo;
  QLineEdit *m_ffLabelEntry;
  QLineEdit *m_ffElementEntry;
  QLineEdit *m_ffDistanceEntry;
  QLineEdit *m_ffCountEntry;
  QLineEdit *m_ffImportEntry;

  /* Library widgets */
  QListWidget *m_libraryList;
  QLineEdit *m_libraryNameEntry;
  QPushButton *m_librarySaveBtn;

  QTabWidget *m_tabs;
};

#endif /* EDITDIALOG_H */
