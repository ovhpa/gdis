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

#ifndef DOCKINGDIALOG_H
#define DOCKINGDIALOG_H

#include <QDialog>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>

class DockingDialog : public QDialog
{
  Q_OBJECT

public:
  explicit DockingDialog(QWidget *parent = nullptr);

public slots:
  void on_create_docking();

private:
  void setupUI();

  /* Translational sampling */
  QCheckBox *m_chkTransSampling;
  QDoubleSpinBox *m_spinCellX, *m_spinCellY;
  QSpinBox *m_spinGridX, *m_spinGridY;

  /* Rotational sampling */
  QCheckBox *m_chkRotSampling;
  QSpinBox *m_spinRotX, *m_spinRotY, *m_spinRotZ;

  /* Rigid body */
  QCheckBox *m_chkRigidBody;
  QCheckBox *m_chkRigidX, *m_chkRigidY, *m_chkRigidZ;

  /* Control */
  QCheckBox *m_chkNoExecute;

  /* Reference to the surface model */
  struct model_pak *m_surface;
};

#endif // DOCKINGDIALOG_H
