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
 * Z-matrix Editor Dialog for GDIS Qt6 GUI
 */

#ifndef ZMATRIXDIALOG_H
#define ZMATRIXDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

struct model_pak;
struct zmat_pak;

class ZMatrixDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ZMatrixDialog(struct model_pak *model, QWidget *parent = nullptr);

private slots:
  /* Line selection changed */
  void on_line_changed();

  /* Value entry changed */
  void on_value_entry_changed(int idx);

  /* Variable editing */
  void on_variable_name_changed();
  void on_variable_value_changed();

  /* Buttons */
  void on_recompute_geometry();
  void on_build_from_selection();

private:
  void populate_text_edit();
  void update_line_entries();
  void update_variable_combo();
  void sync_entry_to_zval(int idx);
  void apply_entry_changes();

  struct model_pak *m_model;
  struct zmat_pak *m_zmat;

  /* Text display */
  QTextEdit *m_textEdit;

  /* Line selector and value editors */
  QSpinBox *m_lineSpin;
  QLineEdit *m_valueEntry[3];

  /* Variable editing */
  QComboBox *m_variableCombo;
  QLineEdit *m_variableValueEntry;

  /* Buttons */
  QPushButton *m_recomputeBtn;
  QPushButton *m_buildBtn;
};

#endif /* ZMATRIXDIALOG_H */
