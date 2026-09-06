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

#include "dockingdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QMessageBox>
#include <QDir>
#include <QInputDialog>

/* Include C headers */
#undef slots
#undef signals
#undef public
#undef emit

#include "gdis.h"
#include "interface.h"

/* External C declarations */
extern "C" {
extern struct sysenv_pak sysenv;
extern void docking_project_create(struct model_pak *model);
extern void gui_text_show(gint type, const gchar *text);
}

DockingDialog::DockingDialog(QWidget *parent) : QDialog(parent), m_surface(nullptr)
{
  setWindowTitle(tr("Docking setup"));
  setMinimumSize(500, 400);

  /* Check that we have a valid surface model */
  m_surface = static_cast<struct model_pak *>(sysenv.active_model);
  if (!m_surface)
  {
    gui_text_show(1, "Please load a surface first.\n");
    reject();
    return;
  }
  if (m_surface->periodic != 2)
  {
    gui_text_show(1, "Your model is not a surface.\n");
    reject();
    return;
  }

  setupUI();
}

void DockingDialog::setupUI()
{
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setSpacing(8);
  mainLayout->setContentsMargins(8, 8, 8, 8);

  /* Title */
  auto *titleGroup = new QGroupBox(tr("Docking setup: %1").arg(m_surface->basename));
  auto *titleLayout = new QVBoxLayout(titleGroup);
  titleLayout->setContentsMargins(8, 4, 8, 4);
  mainLayout->addWidget(titleGroup);

  /* Split into two columns: left=sampling, right=rigid+controls */
  auto *hbox = new QHBoxLayout();
  hbox->setSpacing(12);

  /* ===== LEFT COLUMN: Sampling controls ===== */
  auto *leftLayout = new QVBoxLayout();
  leftLayout->setSpacing(8);

  /* Translational sampling */
  auto *transGroup = new QGroupBox(tr("Translational sampling"));
  auto *transLayout = new QVBoxLayout(transGroup);
  transLayout->setSpacing(4);

  m_chkTransSampling = new QCheckBox(tr("Enable translational sampling"));
  m_chkTransSampling->setChecked(dock_grid_on);
  transLayout->addWidget(m_chkTransSampling);

  auto *cellLayout = new QHBoxLayout();
  cellLayout->setSpacing(4);
  cellLayout->addWidget(new QLabel(tr("axes fractions:")));
  m_spinCellX = new QDoubleSpinBox();
  m_spinCellX->setRange(0.0, 1.0);
  m_spinCellX->setSingleStep(0.05);
  m_spinCellX->setValue(dock_cell[0]);
  cellLayout->addWidget(m_spinCellX);
  m_spinCellY = new QDoubleSpinBox();
  m_spinCellY->setRange(0.0, 1.0);
  m_spinCellY->setSingleStep(0.05);
  m_spinCellY->setValue(dock_cell[1]);
  cellLayout->addWidget(m_spinCellY);
  transLayout->addLayout(cellLayout);

  auto *gridLayout = new QHBoxLayout();
  gridLayout->setSpacing(4);
  gridLayout->addWidget(new QLabel(tr("grid size:")));
  m_spinGridX = new QSpinBox();
  m_spinGridX->setRange(1, 10);
  m_spinGridX->setValue(static_cast<gint>(dock_grid[0]));
  gridLayout->addWidget(m_spinGridX);
  m_spinGridY = new QSpinBox();
  m_spinGridY->setRange(1, 10);
  m_spinGridY->setValue(static_cast<gint>(dock_grid[1]));
  gridLayout->addWidget(m_spinGridY);
  transLayout->addLayout(gridLayout);

  leftLayout->addWidget(transGroup);

  /* Rotational sampling */
  auto *rotGroup = new QGroupBox(tr("Rotational sampling"));
  auto *rotLayout = new QVBoxLayout(rotGroup);
  rotLayout->setSpacing(4);

  m_chkRotSampling = new QCheckBox(tr("Enable rotational sampling"));
  m_chkRotSampling->setChecked(dock_rotate_on);
  rotLayout->addWidget(m_chkRotSampling);

  auto *rotXLayout = new QHBoxLayout();
  rotXLayout->addWidget(new QLabel(tr("x axis:")));
  m_spinRotX = new QSpinBox();
  m_spinRotX->setRange(1, 60);
  m_spinRotX->setValue(static_cast<gint>(dock_rotate[0]));
  rotXLayout->addWidget(m_spinRotX);
  rotLayout->addLayout(rotXLayout);

  auto *rotYLayout = new QHBoxLayout();
  rotYLayout->addWidget(new QLabel(tr("y axis:")));
  m_spinRotY = new QSpinBox();
  m_spinRotY->setRange(1, 60);
  m_spinRotY->setValue(static_cast<gint>(dock_rotate[1]));
  rotYLayout->addWidget(m_spinRotY);
  rotLayout->addLayout(rotYLayout);

  auto *rotZLayout = new QHBoxLayout();
  rotZLayout->addWidget(new QLabel(tr("z axis:")));
  m_spinRotZ = new QSpinBox();
  m_spinRotZ->setRange(1, 60);
  m_spinRotZ->setValue(static_cast<gint>(dock_rotate[2]));
  rotZLayout->addWidget(m_spinRotZ);
  rotLayout->addLayout(rotZLayout);

  leftLayout->addWidget(rotGroup);
  hbox->addLayout(leftLayout);

  /* ===== RIGHT COLUMN: Rigid body + controls ===== */
  auto *rightLayout = new QVBoxLayout();
  rightLayout->setSpacing(8);

  /* Rigid body docking */
  auto *rigidGroup = new QGroupBox(tr("Rigid body docking"));
  auto *rigidLayout = new QVBoxLayout(rigidGroup);
  rigidLayout->setSpacing(4);

  m_chkRigidBody = new QCheckBox(tr("Treat as a rigid body"));
  m_chkRigidBody->setChecked(dock_rigid_on);
  rigidLayout->addWidget(m_chkRigidBody);

  m_chkRigidX = new QCheckBox(tr("Allow translation in x"));
  m_chkRigidX->setChecked(dock_rigid_x);
  rigidLayout->addWidget(m_chkRigidX);

  m_chkRigidY = new QCheckBox(tr("Allow translation in y"));
  m_chkRigidY->setChecked(dock_rigid_y);
  rigidLayout->addWidget(m_chkRigidY);

  m_chkRigidZ = new QCheckBox(tr("Allow translation in z"));
  m_chkRigidZ->setChecked(dock_rigid_z);
  rigidLayout->addWidget(m_chkRigidZ);

  rightLayout->addWidget(rigidGroup);

  /* Control options */
  auto *ctrlGroup = new QGroupBox(tr("Control options"));
  auto *ctrlLayout = new QVBoxLayout(ctrlGroup);
  ctrlLayout->setSpacing(4);

  m_chkNoExecute = new QCheckBox(tr("Create project files then stop"));
  m_chkNoExecute->setChecked(dock_no_execute);
  ctrlLayout->addWidget(m_chkNoExecute);

  rightLayout->addWidget(ctrlGroup);

  hbox->addLayout(rightLayout);
  mainLayout->addLayout(hbox);

  /* Buttons */
  auto *buttonLayout = new QHBoxLayout();
  auto *createBtn = new QPushButton(tr("Create"));
  auto *cancelBtn = new QPushButton(tr("Cancel"));
  buttonLayout->addWidget(createBtn);
  buttonLayout->addWidget(cancelBtn);
  mainLayout->addLayout(buttonLayout);

  connect(createBtn, &QPushButton::clicked, this, &DockingDialog::on_create_docking);
  connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void DockingDialog::on_create_docking()
{
  /* Update global variables from dialog values */
  dock_grid_on = m_chkTransSampling->isChecked() ? 1 : 0;
  dock_cell[0] = m_spinCellX->value();
  dock_cell[1] = m_spinCellY->value();
  dock_grid[0] = static_cast<gdouble>(m_spinGridX->value());
  dock_grid[1] = static_cast<gdouble>(m_spinGridY->value());

  dock_rotate_on = m_chkRotSampling->isChecked() ? 1 : 0;
  dock_rotate[0] = static_cast<gdouble>(m_spinRotX->value());
  dock_rotate[1] = static_cast<gdouble>(m_spinRotY->value());
  dock_rotate[2] = static_cast<gdouble>(m_spinRotZ->value());

  dock_rigid_on = m_chkRigidBody->isChecked() ? 1 : 0;
  dock_rigid_x = m_chkRigidX->isChecked() ? 1 : 0;
  dock_rigid_y = m_chkRigidY->isChecked() ? 1 : 0;
  dock_rigid_z = m_chkRigidZ->isChecked() ? 1 : 0;

  dock_no_execute = m_chkNoExecute->isChecked() ? 1 : 0;

  /* Call the C docking function */
  docking_project_create(m_surface);

  accept();
}

/* Qt bridge function */
extern "C" void qt_dock_dialog(void)
{
  extern QWidget *get_main_window_widget();
  DockingDialog *dlg = new DockingDialog(get_main_window_widget());
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->show();
}
