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

#ifndef MDANALYSISDIALOG_H
#define MDANALYSISDIALOG_H

#include <QDialog>

class QComboBox;
class QDoubleSpinBox;

class MDAnalysisDialog : public QDialog
{
  Q_OBJECT

public:
  explicit MDAnalysisDialog(void *model, QWidget *parent = nullptr);
  ~MDAnalysisDialog();

private slots:
  void on_execute();

private:
  struct model_pak *getModel() const;
  struct analysis_pak *getAnalysis() const;
  void setupUI();
  void populateAtomCombos();

  void *m_model;
  void *m_analysis;

  QComboBox *m_calcCombo;
  QComboBox *m_atom1Combo;
  QComboBox *m_atom2Combo;
  QDoubleSpinBox *m_startSpin;
  QDoubleSpinBox *m_stopSpin;
  QDoubleSpinBox *m_stepSpin;

  QStringList m_atomList;
};

#endif // MDANALYSISDIALOG_H
