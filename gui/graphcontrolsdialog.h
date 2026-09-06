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

#ifndef GRAPHCONTROLSDIALOG_H
#define GRAPHCONTROLSDIALOG_H

#include <QDialog>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

class GraphControlsDialog : public QDialog
{
  Q_OBJECT

public:
  explicit GraphControlsDialog(struct graph_pak *graph, struct model_pak *model, QWidget *parent = nullptr);
  ~GraphControlsDialog();

private slots:
  void on_apply();
  void on_close();
  void on_set_changed(int);
  void on_auto_x_toggled(bool);
  void on_auto_y_toggled(bool);

private:
  void setupUI();
  void syncFromGraph();
  void applyToGraph();
  void auto_x_range();
  void auto_y_range();

  struct graph_pak *m_graph;
  struct model_pak *m_model;

  /* Title & Axis controls */
  QLineEdit *m_titleEdit;
  QLineEdit *m_subTitleEdit;
  QLineEdit *m_xTitleEdit;
  QLineEdit *m_yTitleEdit;
  QSpinBox *m_titleFontSize;
  QSpinBox *m_subTitleFontSize;
  QSpinBox *m_xTitleFontSize;
  QSpinBox *m_yTitleFontSize;
  QDoubleSpinBox *m_xminSpin;
  QDoubleSpinBox *m_xmaxSpin;
  QDoubleSpinBox *m_yminSpin;
  QDoubleSpinBox *m_ymaxSpin;
  QSpinBox *m_xticsSpin;
  QSpinBox *m_yticsSpin;
  QCheckBox *m_autoXCheck;
  QCheckBox *m_autoYCheck;

  /* Data Set Properties — disabled for now */
  QSpinBox *m_setSpin;
  QComboBox *m_typeCombo;
  QCheckBox *m_byValueCheck;
  QSpinBox *m_numSpin;
  QLineEdit *m_sizeEdit;
  QLineEdit *m_idxEdit;
  QComboBox *m_symbolCombo;
  QDoubleSpinBox *m_xValSpin;
  QDoubleSpinBox *m_yValSpin;
  QComboBox *m_lineCombo;
  QComboBox *m_colorCombo;

  bool m_byValueEnabled;
};

#endif // GRAPHCONTROLSDIALOG_H
