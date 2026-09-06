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

#ifndef PLOTSDIALOG_H
#define PLOTSDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QRadioButton>
#include <QSpinBox>
#include <QLabel>
#include <QGroupBox>
#include <QButtonGroup>

class QTabWidget;

class PlotsDialog : public QDialog
{
  Q_OBJECT

public:
  explicit PlotsDialog(struct model_pak *model, QWidget *parent = nullptr);
  ~PlotsDialog();

private:
  Q_SLOT void on_execute();

private:
  void setupUI();
  void populateDynamicsTab(QTabWidget *tabWidget);
  void populateElectronicTab(QTabWidget *tabWidget);
  void populateFrequencyTab(QTabWidget *tabWidget);

  struct model_pak *m_model;
  struct plot_pak *m_plot;

  QRadioButton *m_rbEnergy;
  QRadioButton *m_rbForce;
  QRadioButton *m_rbVolume;
  QRadioButton *m_rbPressure;

  QRadioButton *m_rbDOS;
  QRadioButton *m_rbBand;
  QRadioButton *m_rbBandDOS;

  QRadioButton *m_rbVibrational;
  QRadioButton *m_rbRaman;

  QSpinBox *m_xticsSpin;
  QSpinBox *m_yticsSpin;
  QButtonGroup *m_buttonGroup;
};

#endif // PLOTSDIALOG_H
