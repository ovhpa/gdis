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

#ifndef MDIDIALOG_H
#define MDIDIALOG_H

#include <QDialog>
#include <QSpinBox>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QListWidget>
#include <QMessageBox>

class MdiDialog : public QDialog
{
  Q_OBJECT

public:
  explicit MdiDialog(QWidget *parent = nullptr);
  ~MdiDialog();

private slots:
  void on_create_box();
  void on_cancel();

private:
  void setupUI();

  /* Box dimension spinner */
  QSpinBox *m_spinBoxDim;

  /* List of component spinners (index 0 = solvent, 1+ = solutes) */
  QList<QSpinBox *> m_spinComponents;

  /* Labels for each component */
  QList<QLabel *> m_componentLabels;

  /* Reference to the solvent model (model 0) */
  struct model_pak *m_solvent;
};

#endif // MDIDIALOG_H
