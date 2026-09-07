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

#ifndef MEASUREMENT_DIALOG_H
#define MEASUREMENT_DIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QTreeView>
#include <QStandardItemModel>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QGroupBox>
#include <QKeyEvent>

class MeasurementDialog : public QDialog
{
  Q_OBJECT

public:
  explicit MeasurementDialog(QWidget *parent = nullptr);

protected:
  void keyPressEvent(QKeyEvent *event) override;

signals:
  /* Emitted when a measurement is created via canvas click */
  void canvasMeasurementCreated();

private Q_SLOTS:
  void on_select_all();
  void on_delete_selected();
  void on_dump();
  void on_tree_selection_changed();
  void on_create_bond();
  void on_create_distance();
  void on_create_angle();
  void on_create_torsion();
  void on_search_match();
  void on_search_type_changed();

public:
  /* Public for C callback access */
  void refresh_tree();

private:
  void populate_tree();
  void update_selected_measurement();
  void setup_manual_tab(QWidget *page);
  void setup_search_tab(QWidget *page);

  QTabWidget *m_tabWidget;
  QTreeView *m_treeView;
  QStandardItemModel *m_model;

  /* Manual tab widgets */
  QLineEdit *m_valueEntry;

  /* Search tab widgets */
  QComboBox *m_searchTypeCombo;
  QComboBox *m_match1Combo;
  QComboBox *m_match2Combo;
  QComboBox *m_match3Combo;
  QDoubleSpinBox *m_spinMin12;
  QDoubleSpinBox *m_spinMax12;
  QDoubleSpinBox *m_spinMin23;
  QDoubleSpinBox *m_spinMax23;
  QDoubleSpinBox *m_spinMinAngle;
  QDoubleSpinBox *m_spinMaxAngle;
  QPushButton *m_searchBtn;

  /* Tree view buttons */
  QPushButton *m_selectBtn;
  QPushButton *m_deleteBtn;
  QPushButton *m_dumpBtn;
};

#endif /* MEASUREMENT_DIALOG_H */
