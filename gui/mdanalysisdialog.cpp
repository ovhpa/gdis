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

#include "mdanalysisdialog.h"

#include "gdis.h"
#include "graph.h"

/* Forward declaration — elem.c defines find_unique */
extern "C" GSList *find_unique(gint, struct model_pak *);

extern "C" {
#include "analysis.h"
}
#include "interface.h"
#include "gui_shorts.h"
#include "edit.h"
#include "coords.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QGroupBox>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QMessageBox>
#include <QList>

extern struct elem_pak elements[];
extern struct sysenv_pak sysenv;

static void exec_analysis_task(struct analysis_pak *analysis, struct model_pak *model)
{
  const char *type = analysis->rdf_normalize ? "RDF" : "Pair count";

  if (g_ascii_strncasecmp(type, "RDF", 3) == 0 || g_ascii_strncasecmp(type, "Pair", 4) == 0)
  {
    analysis->rdf_normalize = (g_ascii_strncasecmp(type, "RDF", 3) == 0);
    task_new("RDF", (gpointer) &analysis_plot_rdf, analysis, (gpointer) &analysis_show, model, model);
    return;
  }
  if (g_ascii_strncasecmp(type, "VACF", 4) == 0)
  {
    task_new("VACF", (gpointer) &analysis_plot_vacf, analysis, (gpointer) &analysis_show, model, model);
    return;
  }
  if (g_ascii_strncasecmp(type, "Temp", 4) == 0)
  {
    task_new("Temp", (gpointer) &analysis_plot_temp, analysis, (gpointer) &analysis_show, model, model);
    return;
  }
  if (g_ascii_strncasecmp(type, "Pot", 3) == 0)
  {
    task_new("Energy", (gpointer) &analysis_plot_pe, analysis, (gpointer) &analysis_show, model, model);
    return;
  }
  if (g_ascii_strncasecmp(type, "Kin", 3) == 0)
  {
    task_new("Energy", (gpointer) &analysis_plot_ke, analysis, (gpointer) &analysis_show, model, model);
    return;
  }
  if (g_ascii_strncasecmp(type, "Meas", 4) == 0)
  {
    task_new("Measure", (gpointer) &analysis_plot_meas, analysis, (gpointer) &analysis_show, model, model);
    return;
  }
}

MDAnalysisDialog::MDAnalysisDialog(void *model, QWidget *parent) : QDialog(parent), m_model(model), m_analysis(nullptr)
{
  struct model_pak *m = (struct model_pak *) model;
  setWindowTitle(tr("MD Analysis"));
  setMinimumSize(400, 200);

  if (!m || !m->animation)
  {
    QMessageBox::warning(this, tr("MD Analysis"), tr("No animation data available."));
    deleteLater();
    return;
  }

  if (analysis_init(m))
  {
    QMessageBox::warning(this, tr("MD Analysis"), tr("Failed to initialize analysis."));
    deleteLater();
    return;
  }

  m_analysis = (struct analysis_pak *) m->analysis;

  setupUI();
}

MDAnalysisDialog::~MDAnalysisDialog() {}

struct model_pak *MDAnalysisDialog::getModel() const { return (struct model_pak *) m_model; }

struct analysis_pak *MDAnalysisDialog::getAnalysis() const { return (struct analysis_pak *) m_analysis; }

void MDAnalysisDialog::setupUI()
{
  auto *mainLayout = new QHBoxLayout(this);

  /* --- Left pane: model info + calculation menu --- */
  auto *leftVBox = new QVBoxLayout();
  leftVBox->setContentsMargins(0, 0, 4, 0);

  /* Model group — fixed height to match Analysis Interval */
  auto *modelGroup = new QGroupBox(tr("Model"));
  auto *modelLayout = new QHBoxLayout(modelGroup);
  modelLayout->setContentsMargins(8, 6, 8, 6);
  modelLayout->addWidget(new QLabel(getModel()->basename));
  modelGroup->setFixedHeight(80);
  leftVBox->addWidget(modelGroup);

  /* Calculate group — fixed height to match Analysis Atoms */
  auto *calcGroup = new QGroupBox(tr("Calculate"));
  auto *calcLayout = new QHBoxLayout(calcGroup);
  calcLayout->setContentsMargins(8, 6, 8, 6);

  auto *performLabel = new QLabel(tr("Perform "));
  m_calcCombo = new QComboBox();

  /* Build calculation list based on available data */
  if (getModel()->gulp.trj_file && g_file_test(getModel()->gulp.trj_file, G_FILE_TEST_EXISTS))
  {
    m_calcCombo->addItems({"Pair count", "RDF", "Measurements", "VACF", "Temperature", "Potential E", "Kinetic E"});
  } else
  {
    m_calcCombo->addItems({"Pair count", "RDF", "Measurements"});
  }

  calcLayout->addWidget(performLabel);
  calcLayout->addWidget(m_calcCombo);
  calcGroup->setFixedHeight(80);
  leftVBox->addWidget(calcGroup);

  leftVBox->addStretch();
  mainLayout->addLayout(leftVBox);

  /* --- Right pane: RDF parameters + atom selection + buttons --- */
  auto *rightVBox = new QVBoxLayout();
  rightVBox->setSpacing(6);

  /* Analysis Interval group — fixed height to match Model */
  auto *intervalGroup = new QGroupBox(tr("Analysis Interval"));
  auto *intervalLayout = new QVBoxLayout(intervalGroup);
  intervalLayout->setContentsMargins(8, 6, 8, 6);
  intervalGroup->setFixedHeight(80);

  auto *intervalHBox = new QHBoxLayout();
  m_startSpin = new QDoubleSpinBox();
  m_startSpin->setRange(0.0, 10.0 * getModel()->rmax);
  m_startSpin->setValue(getAnalysis()->start);
  m_startSpin->setSingleStep(0.1);
  m_startSpin->setDecimals(1);
  m_startSpin->setSuffix(tr(" Å"));

  m_stopSpin = new QDoubleSpinBox();
  m_stopSpin->setRange(0.1, 10.0 * getModel()->rmax);
  m_stopSpin->setValue(getAnalysis()->stop);
  m_stopSpin->setSingleStep(0.1);
  m_stopSpin->setDecimals(1);
  m_stopSpin->setSuffix(tr(" Å"));

  m_stepSpin = new QDoubleSpinBox();
  m_stepSpin->setRange(0.1, getModel()->rmax);
  m_stepSpin->setValue(getAnalysis()->step);
  m_stepSpin->setSingleStep(0.1);
  m_stepSpin->setDecimals(1);
  m_stepSpin->setSuffix(tr(" Å"));

  intervalHBox->addWidget(new QLabel(tr("Start:")));
  intervalHBox->addWidget(m_startSpin);
  intervalHBox->addWidget(new QLabel(tr("Stop:")));
  intervalHBox->addWidget(m_stopSpin);
  intervalHBox->addWidget(new QLabel(tr("Step:")));
  intervalHBox->addWidget(m_stepSpin);
  intervalLayout->addLayout(intervalHBox);
  rightVBox->addWidget(intervalGroup);

  /* Analysis Atoms group — fixed height to match Calculate */
  auto *atomsGroup = new QGroupBox(tr("Analysis Atoms"));
  auto *atomsLayout = new QVBoxLayout(atomsGroup);
  atomsLayout->setContentsMargins(8, 6, 8, 6);
  atomsGroup->setFixedHeight(80);

  auto *atomsHBox = new QHBoxLayout();
  m_atom1Combo = new QComboBox();
  m_atom1Combo->setEditable(true);
  m_atom1Combo->setInsertPolicy(QComboBox::NoInsert);
  m_atom2Combo = new QComboBox();
  m_atom2Combo->setEditable(true);
  m_atom2Combo->setInsertPolicy(QComboBox::NoInsert);

  populateAtomCombos();

  atomsHBox->addWidget(new QLabel(tr("Atom 1:")));
  atomsHBox->addWidget(m_atom1Combo, 1);
  atomsHBox->addWidget(new QLabel(tr("Atom 2:")));
  atomsHBox->addWidget(m_atom2Combo, 1);
  atomsLayout->addLayout(atomsHBox);
  rightVBox->addWidget(atomsGroup);

  rightVBox->addStretch();

  /* Buttons: bottom-right of right pane */
  auto *btnLayout = new QHBoxLayout();
  btnLayout->setSpacing(6);
  auto *execBtn = new QPushButton(tr("Execute"));
  auto *closeBtn = new QPushButton(tr("Close"));
  btnLayout->addStretch();
  btnLayout->addWidget(execBtn);
  btnLayout->addWidget(closeBtn);
  rightVBox->addLayout(btnLayout);

  mainLayout->addLayout(rightVBox);

  connect(execBtn, &QPushButton::clicked, this, &MDAnalysisDialog::on_execute);
  connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
}

void MDAnalysisDialog::populateAtomCombos()
{
  m_atomList.clear();
  m_atomList << "Any";

  GSList *list = find_unique(LABEL, getModel());
  for (GSList *item = list; item; item = g_slist_next(item))
  {
    m_atomList << QString::fromUtf8((const char *) item->data);
  }
  g_slist_free(list);

  m_atom1Combo->addItems(m_atomList);
  m_atom2Combo->addItems(m_atomList);
}

void MDAnalysisDialog::on_execute()
{
  if (!m_model || !m_analysis)
    return;

  struct model_pak *model = getModel();
  struct analysis_pak *analysis = getAnalysis();

  /* Sync dialog values back to analysis struct */
  analysis->start = m_startSpin->value();
  analysis->stop = m_stopSpin->value();
  analysis->step = m_stepSpin->value();

  /* Set atom strings from combo box text */
  QString atom1Text = m_atom1Combo->currentText();
  QString atom2Text = m_atom2Combo->currentText();

  if (analysis->atom1)
    g_free(analysis->atom1);
  if (analysis->atom2)
    g_free(analysis->atom2);

  analysis->atom1 = g_strdup(atom1Text.toUtf8().constData());
  analysis->atom2 = g_strdup(atom2Text.toUtf8().constData());

  /* Duplicate analysis for the task */
  struct analysis_pak *analysisCopy = (struct analysis_pak *) analysis_dup(analysis);

  /* Determine task type */
  QString calcType = m_calcCombo->currentText();
  if (calcType == "RDF")
  {
    analysisCopy->rdf_normalize = TRUE;
    task_new("RDF", (gpointer) &analysis_plot_rdf, analysisCopy, (gpointer) &analysis_show, model, model);
  } else if (calcType == "Pair count")
  {
    analysisCopy->rdf_normalize = FALSE;
    task_new("Pair", (gpointer) &analysis_plot_rdf, analysisCopy, (gpointer) &analysis_show, model, model);
  } else if (calcType == "VACF")
  {
    task_new("VACF", (gpointer) &analysis_plot_vacf, analysisCopy, (gpointer) &analysis_show, model, model);
  } else if (calcType == "Temperature")
  {
    task_new("Temp", (gpointer) &analysis_plot_temp, analysisCopy, (gpointer) &analysis_show, model, model);
  } else if (calcType == "Potential E")
  {
    task_new("Energy", (gpointer) &analysis_plot_pe, analysisCopy, (gpointer) &analysis_show, model, model);
  } else if (calcType == "Kinetic E")
  {
    task_new("Energy", (gpointer) &analysis_plot_ke, analysisCopy, (gpointer) &analysis_show, model, model);
  } else if (calcType == "Measurements")
  {
    task_new("Measure", (gpointer) &analysis_plot_meas, analysisCopy, (gpointer) &analysis_show, model, model);
  }
}

/* Bridge function called from MainWindow */
extern "C" void qt_show_md_analysis_dialog(void)
{
  extern struct sysenv_pak sysenv;
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (!model)
    return;
  if (!model->animation)
    return;

  extern QWidget *get_main_window_widget();
  MDAnalysisDialog *dlg = new MDAnalysisDialog(model, get_main_window_widget());
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->show();
}
