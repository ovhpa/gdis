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

#include "dislocationdialog.h"

#include "gdis.h"
#include "defect.h"
#include "interface.h"

#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>

/* External C declarations */
extern struct sysenv_pak sysenv;
extern void defect_new(struct defect_pak *defect, struct model_pak *model);
extern void gui_defect_default(void);

/* Defect global from gui_defect.c */
extern struct defect_pak defect;

DislocationDialog::DislocationDialog(QWidget *parent) : QDialog(parent)
{
  setWindowTitle(tr("Dislocation builder"));
  setMinimumSize(450, 500);
  /* Restore defaults before loading current values */
  gui_defect_default();
  setupUI();
}

void DislocationDialog::setupUI()
{
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setSpacing(8);
  mainLayout->setContentsMargins(8, 8, 8, 8);

  /* ===== Geometry frame ===== */
  auto *geoGroup = new QGroupBox(tr("Geometry"));
  auto *geoLayout = new QFormLayout(geoGroup);
  geoLayout->setSpacing(6);
  geoLayout->setContentsMargins(8, 8, 8, 8);

  /* Orientation vector */
  m_spinOrientX = new QDoubleSpinBox();
  m_spinOrientX->setRange(-20.0, 20.0);
  m_spinOrientX->setValue(defect.orient[0]);
  m_spinOrientX->setSingleStep(1.0);
  m_spinOrientX->setDecimals(0);
  geoLayout->addRow(tr("Orientation vector x:"), m_spinOrientX);

  m_spinOrientY = new QDoubleSpinBox();
  m_spinOrientY->setRange(-20.0, 20.0);
  m_spinOrientY->setValue(defect.orient[1]);
  m_spinOrientY->setSingleStep(1.0);
  m_spinOrientY->setDecimals(0);
  geoLayout->addRow(tr("Orientation vector y:"), m_spinOrientY);

  m_spinOrientZ = new QDoubleSpinBox();
  m_spinOrientZ->setRange(-20.0, 20.0);
  m_spinOrientZ->setValue(defect.orient[2]);
  m_spinOrientZ->setSingleStep(1.0);
  m_spinOrientZ->setDecimals(0);
  geoLayout->addRow(tr("Orientation vector z:"), m_spinOrientZ);

  /* Burgers vector */
  m_spinBurgersX = new QDoubleSpinBox();
  m_spinBurgersX->setRange(-20.0, 20.0);
  m_spinBurgersX->setValue(defect.burgers[0]);
  m_spinBurgersX->setSingleStep(0.05);
  m_spinBurgersX->setDecimals(2);
  geoLayout->addRow(tr("Burgers vector x:"), m_spinBurgersX);

  m_spinBurgersY = new QDoubleSpinBox();
  m_spinBurgersY->setRange(-20.0, 20.0);
  m_spinBurgersY->setValue(defect.burgers[1]);
  m_spinBurgersY->setSingleStep(0.05);
  m_spinBurgersY->setDecimals(2);
  geoLayout->addRow(tr("Burgers vector y:"), m_spinBurgersY);

  m_spinBurgersZ = new QDoubleSpinBox();
  m_spinBurgersZ->setRange(-20.0, 20.0);
  m_spinBurgersZ->setValue(defect.burgers[2]);
  m_spinBurgersZ->setSingleStep(0.05);
  m_spinBurgersZ->setDecimals(2);
  geoLayout->addRow(tr("Burgers vector z:"), m_spinBurgersZ);

  /* Defect origin */
  m_spinOriginX = new QDoubleSpinBox();
  m_spinOriginX->setRange(0.0, 1.0);
  m_spinOriginX->setValue(defect.origin[0]);
  m_spinOriginX->setSingleStep(0.05);
  m_spinOriginX->setDecimals(2);
  geoLayout->addRow(tr("Defect origin x:"), m_spinOriginX);

  m_spinOriginY = new QDoubleSpinBox();
  m_spinOriginY->setRange(0.0, 1.0);
  m_spinOriginY->setValue(defect.origin[1]);
  m_spinOriginY->setSingleStep(0.05);
  m_spinOriginY->setDecimals(2);
  geoLayout->addRow(tr("Defect origin y:"), m_spinOriginY);

  /* Defect center */
  m_spinCenterX = new QDoubleSpinBox();
  m_spinCenterX->setRange(0.0, 1.0);
  m_spinCenterX->setValue(defect.center[0]);
  m_spinCenterX->setSingleStep(0.05);
  m_spinCenterX->setDecimals(2);
  geoLayout->addRow(tr("Defect center x:"), m_spinCenterX);

  m_spinCenterY = new QDoubleSpinBox();
  m_spinCenterY->setRange(0.0, 1.0);
  m_spinCenterY->setValue(defect.center[1]);
  m_spinCenterY->setSingleStep(0.05);
  m_spinCenterY->setDecimals(2);
  geoLayout->addRow(tr("Defect center y:"), m_spinCenterY);

  /* Region sizes */
  m_spinRegionX = new QSpinBox();
  m_spinRegionX->setRange(0, 999);
  m_spinRegionX->setValue(static_cast<gint>(defect.region[0]));
  geoLayout->addRow(tr("Region size 1:"), m_spinRegionX);

  m_spinRegionY = new QSpinBox();
  m_spinRegionY->setRange(0, 999);
  m_spinRegionY->setValue(static_cast<gint>(defect.region[1]));
  geoLayout->addRow(tr("Region size 2:"), m_spinRegionY);

  mainLayout->addWidget(geoGroup);

  /* ===== Options frame ===== */
  auto *optGroup = new QGroupBox(tr("Options"));
  auto *optLayout = new QVBoxLayout(optGroup);
  optLayout->setSpacing(6);
  optLayout->setContentsMargins(8, 8, 8, 8);

  /* Charge neutralization */
  auto *neutLayout = new QHBoxLayout();
  neutLayout->addWidget(new QLabel(tr("Charge neutralization:")));
  m_comboNeutral = new QComboBox();
  m_comboNeutral->addItem("none");
  neutLayout->addWidget(m_comboNeutral);
  optLayout->addLayout(neutLayout);

  m_chkCleave = new QCheckBox(tr("Allow bond cleaving"));
  m_chkCleave->setChecked(defect.cleave);
  optLayout->addWidget(m_chkCleave);

  m_chkCluster = new QCheckBox(tr("Discard periodicity"));
  m_chkCluster->setChecked(defect.cluster);
  optLayout->addWidget(m_chkCluster);

  mainLayout->addWidget(optGroup);

  /* Buttons */
  auto *buttonLayout = new QHBoxLayout();
  auto *buildBtn = new QPushButton(tr("Build"));
  auto *cancelBtn = new QPushButton(tr("Cancel"));
  buttonLayout->addWidget(buildBtn);
  buttonLayout->addWidget(cancelBtn);
  mainLayout->addLayout(buttonLayout);

  connect(buildBtn, &QPushButton::clicked, this, &DislocationDialog::on_build_defect);
  connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void DislocationDialog::on_build_defect()
{
  /* Update defect globals from dialog values */
  defect.orient[0] = m_spinOrientX->value();
  defect.orient[1] = m_spinOrientY->value();
  defect.orient[2] = m_spinOrientZ->value();

  defect.burgers[0] = m_spinBurgersX->value();
  defect.burgers[1] = m_spinBurgersY->value();
  defect.burgers[2] = m_spinBurgersZ->value();

  defect.origin[0] = m_spinOriginX->value();
  defect.origin[1] = m_spinOriginY->value();

  defect.center[0] = m_spinCenterX->value();
  defect.center[1] = m_spinCenterY->value();

  defect.region[0] = static_cast<gdouble>(m_spinRegionX->value());
  defect.region[1] = static_cast<gdouble>(m_spinRegionY->value());

  defect.cleave = m_chkCleave->isChecked() ? TRUE : FALSE;
  defect.cluster = m_chkCluster->isChecked() ? TRUE : FALSE;
  /* neutral = 0 (none) — default, combo not fully implemented */
  defect.neutral = 0;

  /* Call the defect builder */
  struct model_pak *model = static_cast<struct model_pak *>(sysenv.active_model);
  if (!model)
    return;
  if (model->periodic != 3)
  {
    /* Show error via GDIS text output */
    extern void gui_text_show(gint, const gchar *);
    gui_text_show(1, "Your model is not 3D periodic.\n");
    return;
  }

  defect_new(&defect, model);
  accept();
}

/* Qt bridge function */
extern "C" void qt_defect_dialog(void)
{
  extern QWidget *get_main_window_widget();
  DislocationDialog *dlg = new DislocationDialog(get_main_window_widget());
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->show();
}
