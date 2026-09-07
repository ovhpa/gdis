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

#ifndef ELEMENT_EDIT_DIALOG_H
#define ELEMENT_EDIT_DIALOG_H

#include <QDialog>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>

class ElementEditDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ElementEditDialog(int element_number, QWidget *parent = nullptr);

private:
  void load_element_data(int number);
  void update_colour_preview();

  int m_element_number;

  QLabel *m_nameLabel;
  QLabel *m_symbolLabel;
  QLabel *m_numberLabel;
  QLabel *m_weightLabel;

  QDoubleSpinBox *m_covaSpin;
  QDoubleSpinBox *m_vdwSpin;

  QPushButton *m_colourButton;
  double m_colour[3];

  /* Stored element data */
  int m_number;
  char m_symbol[4];
  char m_name[32];
  double m_weight;
  double m_cova;
  double m_vdw;
  double m_charge;
};

#endif /* ELEMENT_EDIT_DIALOG_H */
