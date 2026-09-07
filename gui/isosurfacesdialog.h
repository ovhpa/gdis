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

#ifndef ISOSURFACESDIALOG_H
#define ISOSURFACESDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

struct model_pak;

class IsoSurfacesDialog : public QDialog
{
  Q_OBJECT

public:
  explicit IsoSurfacesDialog(struct model_pak *model, QWidget *parent = nullptr);

private slots:
  void on_method_changed(int index);
  void on_calculate();
  void on_delete();

private:
  void update_epot_sensitive();

  struct model_pak *m_model;

  /* UI widgets */
  QComboBox *m_methodCombo;
  QComboBox *m_colourCombo;
  QDoubleSpinBox *m_gridSizeSpin;
  QDoubleSpinBox *m_blurSpin;
  QDoubleSpinBox *m_edenSpin;
  QCheckBox *m_epotAutoScale;
  QLineEdit *m_epotMinEdit;
  QLineEdit *m_epotMaxEdit;
  QLineEdit *m_epotDivEdit;
  QWidget *m_epotVbox;
};

#endif /* ISOSURFACESDIALOG_H */
