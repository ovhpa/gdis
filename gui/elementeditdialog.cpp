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
 * Element Edit dialog for GDIS Qt6 GUI
 */

#include "elementeditdialog.h"
#include "gdis_api.h"
#include "svg_utils.h"

#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

ElementEditDialog::ElementEditDialog(int element_number, QWidget *parent)
    : QDialog(parent), m_element_number(element_number)
{
  setWindowTitle("Element Edit");
  setMinimumWidth(380);

  load_element_data(element_number);

  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setSpacing(8);
  mainLayout->setContentsMargins(12, 12, 12, 12);

  /* Info frame */
  auto *infoFrame = new QFrame(this);
  infoFrame->setFrameShape(QFrame::StyledPanel);
  auto *infoLayout = new QFormLayout(infoFrame);

  m_nameLabel = new QLabel(this);
  m_symbolLabel = new QLabel(this);
  m_numberLabel = new QLabel(this);
  m_weightLabel = new QLabel(this);

  m_nameLabel->setText(QString("Name: %1").arg(m_name));
  m_symbolLabel->setText(QString("Symbol: %1").arg(m_symbol));
  m_numberLabel->setText(QString("Number: %1").arg(m_number));
  m_weightLabel->setText(QString("Weight: %1").arg(m_weight, 0, 'f', 4));

  infoLayout->addRow("", m_nameLabel);
  infoLayout->addRow("", m_symbolLabel);
  infoLayout->addRow("", m_numberLabel);
  infoLayout->addRow("", m_weightLabel);
  mainLayout->addWidget(infoFrame);

  /* Edit frame */
  auto *editFrame = new QFrame(this);
  editFrame->setFrameShape(QFrame::StyledPanel);
  auto *editLayout = new QFormLayout(editFrame);

  m_covaSpin = new QDoubleSpinBox(this);
  m_covaSpin->setRange(0.1, 3.0);
  m_covaSpin->setSingleStep(0.01);
  m_covaSpin->setValue(m_cova);
  editLayout->addRow("Ionic/Covalent radius:", m_covaSpin);

  m_vdwSpin = new QDoubleSpinBox(this);
  m_vdwSpin->setRange(0.1, 4.0);
  m_vdwSpin->setSingleStep(0.01);
  m_vdwSpin->setValue(m_vdw);
  editLayout->addRow("VdW radius:", m_vdwSpin);

  /* Colour button */
  auto *colourRow = new QHBoxLayout();
  m_colourButton = new QPushButton(this);
  m_colourButton->setText("Colour: ");
  update_colour_preview();
  QObject::connect(m_colourButton, &QPushButton::clicked, this, [this]() {
    QColorDialog *dlg = new QColorDialog(this);
    QColor qc((int) (m_colour[0] * 255), (int) (m_colour[1] * 255), (int) (m_colour[2] * 255));
    if (dlg->exec() == QDialog::Accepted)
    {
      qc = dlg->selectedColor();
      m_colour[0] = qc.redF();
      m_colour[1] = qc.greenF();
      m_colour[2] = qc.blueF();
      update_colour_preview();
    }
    dlg->deleteLater();
  });
  colourRow->addWidget(m_colourButton);
  editLayout->addRow("Colour:", colourRow);

  mainLayout->addWidget(editFrame);

  /* Action buttons frame */
  auto *actionFrame = new QFrame(this);
  actionFrame->setFrameShape(QFrame::StyledPanel);
  auto *actionLayout = new QVBoxLayout(actionFrame);
  actionLayout->setSpacing(4);
  actionLayout->setContentsMargins(8, 8, 8, 8);

  add_action_button(actionLayout, "Apply to current model", [this]() {
    m_cova = m_covaSpin->value();
    m_vdw = m_vdwSpin->value();
    qt_elem_apply(m_element_number, m_cova, m_vdw, m_colour[0], m_colour[1], m_colour[2], 1);
  });
  add_action_button(actionLayout, "Apply globally", [this]() {
    m_cova = m_covaSpin->value();
    m_vdw = m_vdwSpin->value();
    qt_elem_apply(m_element_number, m_cova, m_vdw, m_colour[0], m_colour[1], m_colour[2], 0);
  });
  add_action_button(actionLayout, "Reset values", [this]() { qt_elem_reset(m_element_number); });

  mainLayout->addWidget(actionFrame);

  /* Close button */
  auto *closeBtn = new QPushButton("Close", this);
  QObject::connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
  mainLayout->addWidget(closeBtn);

  /* Sync spinbox changes */
  QObject::connect(m_covaSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                   [this](double) { m_cova = m_covaSpin->value(); });
  QObject::connect(m_vdwSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                   [this](double) { m_vdw = m_vdwSpin->value(); });
}

void ElementEditDialog::load_element_data(int number)
{
  qt_get_elem_data(number, &m_number, m_symbol, m_name, &m_weight, &m_cova, &m_vdw, &m_charge, &m_colour[0],
                   &m_colour[1], &m_colour[2]);
  m_element_number = number;
}

void ElementEditDialog::update_colour_preview()
{
  m_colourButton->setStyleSheet(QString("QPushButton { "
                                        "background-color: rgb(%1, %2, %3); "
                                        "border: 1px solid #333; "
                                        "padding: 4px; }")
                                    .arg((int) (m_colour[0] * 255))
                                    .arg((int) (m_colour[1] * 255))
                                    .arg((int) (m_colour[2] * 255)));
}

/* Qt bridge: show element edit dialog */
extern "C" void qt_show_element_dialog(int number)
{
  extern QWidget *get_main_window_widget();
  ElementEditDialog *dlg = new ElementEditDialog(number, get_main_window_widget());
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->show();
}
