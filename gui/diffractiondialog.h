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

#ifndef DIFFRACTIONDIALOG_H
#define DIFFRACTIONDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSplitter>

class DiffractionDialog : public QDialog
{
  Q_OBJECT

public:
  explicit DiffractionDialog(struct model_pak *model, QWidget *parent = nullptr);

private slots:
  void on_calculate();

private:
  void setup_ui();

  struct model_pak *m_model;

  /* Radiation controls */
  QComboBox *m_radiationCombo;
  QLineEdit *m_wavelengthEdit;

  /* Broadening controls */
  QComboBox *m_broadeningCombo;
  QDoubleSpinBox *m_mixingSpin;

  /* 2theta controls */
  QDoubleSpinBox *m_thetaMinSpin;
  QDoubleSpinBox *m_thetaMaxSpin;
  QDoubleSpinBox *m_thetaStepSpin;

  /* U V W controls */
  QDoubleSpinBox *m_uSpin;
  QDoubleSpinBox *m_vSpin;
  QDoubleSpinBox *m_wSpin;

  /* Output */
  QLineEdit *m_filenameEdit;
  QCheckBox *m_allFramesCheck;
};

#endif // DIFFRACTIONDIALOG_H
