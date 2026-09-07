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
 * Iso-Surfaces dialog for GDIS Qt6 GUI
 */

#include "isosurfacesdialog.h"
#include "gdis_api.h"
#include "glcanvas.h"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QVBoxLayout>

IsoSurfacesDialog::IsoSurfacesDialog(struct model_pak *model, QWidget *parent) : QDialog(parent), m_model(model)
{
  setWindowTitle("Iso-Surfaces");
  setMinimumWidth(400);

  auto *mainLayout = new QVBoxLayout(this);

  /* === Iso-surface type === */
  auto *typeGroup = new QGroupBox("Iso-surface type", this);
  auto *typeLayout = new QFormLayout(typeGroup);

  m_methodCombo = new QComboBox(this);
  m_methodCombo->addItems({"Molecular surface", "Hirshfeld surface", "Electron density", "Promolecule isosurface"});
  /* Get initial method from C side */
  gint init_method, init_colour;
  qt_ms_get_state(&init_method, &init_colour);
  switch (init_method)
  {
  case 2:
    m_methodCombo->setCurrentIndex(2);
    break;
  case 11:
    m_methodCombo->setCurrentIndex(1);
    break;
  case 16:
    m_methodCombo->setCurrentIndex(3);
    break;
  default:
    m_methodCombo->setCurrentIndex(0);
    break;
  }
  QObject::connect(m_methodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                   &IsoSurfacesDialog::on_method_changed);
  typeLayout->addRow("Type:", m_methodCombo);
  mainLayout->addWidget(typeGroup);

  /* === Colour method === */
  auto *colourGroup = new QGroupBox("Colour method", this);
  auto *colourLayout = new QFormLayout(colourGroup);

  m_colourCombo = new QComboBox(this);
  m_colourCombo->addItems({"Default", "AFM", "Electrostatic", "Curvedness", "Shape Index", "De"});
  switch (init_colour)
  {
  case 9:
    m_colourCombo->setCurrentIndex(1);
    break;
  case 10:
    m_colourCombo->setCurrentIndex(2);
    break;
  case 14:
    m_colourCombo->setCurrentIndex(3);
    break;
  case 15:
    m_colourCombo->setCurrentIndex(4);
    break;
  case 13:
    m_colourCombo->setCurrentIndex(5);
    break;
  default:
    m_colourCombo->setCurrentIndex(0);
    break;
  }
  colourLayout->addRow("Method:", m_colourCombo);
  mainLayout->addWidget(colourGroup);

  /* === Parameters === */
  auto *paramGroup = new QGroupBox("Parameters", this);
  auto *paramLayout = new QFormLayout(paramGroup);

  m_gridSizeSpin = new QDoubleSpinBox(this);
  m_gridSizeSpin->setRange(0.05, 10.0);
  m_gridSizeSpin->setSingleStep(0.05);
  m_gridSizeSpin->setValue(qt_ms_get_grid_size());
  paramLayout->addRow("Triangulation grid size:", m_gridSizeSpin);

  m_blurSpin = new QDoubleSpinBox(this);
  m_blurSpin->setRange(0.05, 1.0);
  m_blurSpin->setSingleStep(0.05);
  m_blurSpin->setValue(qt_ms_get_blur());
  paramLayout->addRow("Molecular surface blurring:", m_blurSpin);

  m_edenSpin = new QDoubleSpinBox(this);
  m_edenSpin->setRange(0.001, 1.0);
  m_edenSpin->setSingleStep(0.001);
  m_edenSpin->setValue(qt_ms_get_eden());
  paramLayout->addRow("Electron density value:", m_edenSpin);

  mainLayout->addWidget(paramGroup);

  /* === Electrostatic potential scale === */
  m_epotVbox = new QWidget(this);
  auto *epotLayout = new QFormLayout(m_epotVbox);

  m_epotAutoScale = new QCheckBox("Electrostatic autoscaling", this);
  m_epotAutoScale->setChecked(qt_ms_get_epot_autoscale());
  QObject::connect(m_epotAutoScale, &QCheckBox::toggled, this, &IsoSurfacesDialog::update_epot_sensitive);
  epotLayout->addRow("", m_epotAutoScale);

  gdouble epot_min, epot_max;
  gint epot_div;
  qt_ms_get_epot(&epot_min, &epot_max, &epot_div);

  auto *hbox = new QHBoxLayout();
  auto *minLabel = new QLabel("minimum ", this);
  m_epotMinEdit = new QLineEdit(this);
  m_epotMinEdit->setText(QString::number(epot_min, 'f', 6));
  hbox->addWidget(minLabel);
  hbox->addWidget(m_epotMinEdit);
  epotLayout->addRow("", hbox);

  hbox = new QHBoxLayout();
  auto *maxLabel = new QLabel("maximum ", this);
  m_epotMaxEdit = new QLineEdit(this);
  m_epotMaxEdit->setText(QString::number(epot_max, 'f', 6));
  hbox->addWidget(maxLabel);
  hbox->addWidget(m_epotMaxEdit);
  epotLayout->addRow("", hbox);

  hbox = new QHBoxLayout();
  auto *divLabel = new QLabel("divisions ", this);
  m_epotDivEdit = new QLineEdit(this);
  m_epotDivEdit->setText(QString::number(epot_div));
  hbox->addWidget(divLabel);
  hbox->addWidget(m_epotDivEdit);
  epotLayout->addRow("", hbox);

  mainLayout->addWidget(m_epotVbox);
  m_epotVbox->setVisible(false);

  /* === Buttons === */
  auto *btnLayout = new QHBoxLayout();
  auto *calcBtn = new QPushButton("Calculate", this);
  auto *delBtn = new QPushButton("Delete", this);
  auto *closeBtn = new QPushButton("Close", this);
  btnLayout->addWidget(calcBtn);
  btnLayout->addWidget(delBtn);
  btnLayout->addStretch();
  btnLayout->addWidget(closeBtn);
  mainLayout->addLayout(btnLayout);

  QObject::connect(calcBtn, &QPushButton::clicked, this, &IsoSurfacesDialog::on_calculate);
  QObject::connect(delBtn, &QPushButton::clicked, this, &IsoSurfacesDialog::on_delete);
  QObject::connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void IsoSurfacesDialog::on_method_changed(int index)
{
  switch (index)
  {
  case 0:
    qt_ms_set_method(1);
    break;
  case 1:
    qt_ms_set_method(11);
    break;
  case 2:
    qt_ms_set_method(2);
    break;
  case 3:
    qt_ms_set_method(16);
    break;
  }
  extern void redraw_canvas(gint);
  redraw_canvas(2); /* ALL */
  extern void *qt_get_gl_canvas(void);
  auto *canvas = static_cast<GLCanvas *>(qt_get_gl_canvas());
  if (canvas)
    canvas->update();
}

void IsoSurfacesDialog::update_epot_sensitive()
{
  bool sensitive = !m_epotAutoScale->isChecked();
  m_epotMinEdit->setEnabled(sensitive);
  m_epotMaxEdit->setEnabled(sensitive);
  m_epotDivEdit->setEnabled(sensitive);
  qt_ms_set_epot_autoscale(m_epotAutoScale->isChecked());
}

void IsoSurfacesDialog::on_calculate()
{
  /* Set colour mode from combo */
  switch (m_colourCombo->currentIndex())
  {
  case 0:
    qt_ms_set_colour(8);
    break;
  case 1:
    qt_ms_set_colour(9);
    break;
  case 2:
    qt_ms_set_colour(10);
    break;
  case 3:
    qt_ms_set_colour(14);
    break;
  case 4:
    qt_ms_set_colour(15);
    break;
  case 5:
    qt_ms_set_colour(13);
    break;
  }

  /* Set parameters */
  qt_ms_set_blur(m_blurSpin->value());
  qt_ms_set_eden(m_edenSpin->value());
  qt_ms_set_grid_size(m_gridSizeSpin->value());

  /* Set electrostatic scale */
  qt_ms_set_epot(m_epotMinEdit->text().toDouble(), m_epotMaxEdit->text().toDouble(), m_epotDivEdit->text().toInt());

  /* Call the C calculation function */
  qt_ms_calculate();

  /* Force repaint */
  extern void *qt_get_gl_canvas(void);
  auto *canvas = static_cast<GLCanvas *>(qt_get_gl_canvas());
  if (canvas)
    canvas->update();

  /* Read back values for autoscaling */
  gdouble epot_min, epot_max;
  gint epot_div;
  qt_ms_get_epot(&epot_min, &epot_max, &epot_div);
  m_epotMinEdit->setText(QString::number(epot_min, 'f', 6));
  m_epotMaxEdit->setText(QString::number(epot_max, 'f', 6));
  m_epotDivEdit->setText(QString::number(epot_div));

  /* Show/hide epot controls */
  update_epot_sensitive();
  gint method, colour;
  qt_ms_get_state(&method, &colour);
  m_epotVbox->setVisible(colour == 10);
}

void IsoSurfacesDialog::on_delete()
{
  qt_ms_delete();
  extern void *qt_get_gl_canvas(void);
  auto *canvas = static_cast<GLCanvas *>(qt_get_gl_canvas());
  if (canvas)
    canvas->update();
  // Don't close the dialog
}

/* Qt bridge: show iso-surfaces dialog */
extern "C" void qt_show_isosurfaces_dialog(struct model_pak *model)
{
  extern QWidget *get_main_window_widget();
  IsoSurfacesDialog *dlg = new IsoSurfacesDialog(model, get_main_window_widget());
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->show();
}
