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

#ifndef DISLOCATIONDIALOG_H
#define DISLOCATIONDIALOG_H

#include <QDialog>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>

class DislocationDialog : public QDialog
{
  Q_OBJECT

public:
  explicit DislocationDialog(QWidget *parent = nullptr);

public slots:
  void on_build_defect();

private:
  void setupUI();

  /* Geometry */
  QDoubleSpinBox *m_spinOrientX, *m_spinOrientY, *m_spinOrientZ;
  QDoubleSpinBox *m_spinBurgersX, *m_spinBurgersY, *m_spinBurgersZ;
  QDoubleSpinBox *m_spinOriginX, *m_spinOriginY;
  QDoubleSpinBox *m_spinCenterX, *m_spinCenterY;
  QSpinBox *m_spinRegionX, *m_spinRegionY;

  /* Options */
  QCheckBox *m_chkCleave;
  QCheckBox *m_chkCluster;

  /* Neutralization combo */
  QComboBox *m_comboNeutral;
};

#endif // DISLOCATIONDIALOG_H
