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

#include "plotsdialog.h"

#include "gdis.h"
#include "gdis_api.h"

/* Forward declarations from plots.h (included via gdis.h chain) */
extern "C" {
struct plot_pak;
void plot_load_data(struct plot_pak *, struct task_pak *);
void plot_prepare_data(struct plot_pak *);
void plot_draw_graph(struct plot_pak *, struct task_pak *);
void plot_show_graph(struct plot_pak *);
}
#include "task.h"
#include "plots.h"
#include "graph.h"
#include "model.h"
#include "interface.h"
#include "gui_shorts.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>

extern struct sysenv_pak sysenv;

static void plot_type_from_radio(QRadioButton *rb, struct plot_pak *plot, plot_type energyType, plot_type forceType,
                                 plot_type volumeType, plot_type pressureType)
{
  if (rb == nullptr)
    return;
  if (rb->isChecked())
  {
    plot->type = static_cast<plot_type>(rb->property("plotType").toInt());
  }
}

PlotsDialog::PlotsDialog(struct model_pak *model, QWidget *parent) : QDialog(parent), m_model(model), m_plot(nullptr)
{
  setWindowTitle(tr("Plots: %1").arg(QString::fromUtf8(model->basename)));
  setMinimumSize(400, 300);

  if (!model->plot)
  {
    QMessageBox::warning(this, tr("Plots"), tr("No plot data available for this model."));
    deleteLater();
    return;
  }

  m_plot = (struct plot_pak *) model->plot;
  setupUI();
}

PlotsDialog::~PlotsDialog() {}

void PlotsDialog::setupUI()
{
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(8, 6, 8, 6);
  mainLayout->setSpacing(6);

  auto *tabWidget = new QTabWidget();

  /* Button group to make all radio buttons mutually exclusive across tabs */
  m_buttonGroup = new QButtonGroup(this);

  populateDynamicsTab(tabWidget);
  populateElectronicTab(tabWidget);
  populateFrequencyTab(tabWidget);

  mainLayout->addWidget(tabWidget);

  /* X/Y tics row */
  auto *ticsLayout = new QHBoxLayout();
  ticsLayout->setSpacing(8);
  ticsLayout->addWidget(new QLabel(tr("X tics:")));
  m_xticsSpin = new QSpinBox();
  m_xticsSpin->setRange(1, 50);
  m_xticsSpin->setValue((int) m_plot->xtics);
  ticsLayout->addWidget(m_xticsSpin);
  ticsLayout->addWidget(new QLabel(tr("Y tics:")));
  m_yticsSpin = new QSpinBox();
  m_yticsSpin->setRange(1, 50);
  m_yticsSpin->setValue((int) m_plot->ytics);
  ticsLayout->addWidget(m_yticsSpin);
  ticsLayout->addStretch();
  mainLayout->addLayout(ticsLayout);

  /* Buttons */
  auto *btnLayout = new QHBoxLayout();
  btnLayout->setSpacing(6);
  auto *execBtn = new QPushButton(tr("Execute"));
  auto *closeBtn = new QPushButton(tr("Close"));
  btnLayout->addStretch();
  btnLayout->addWidget(execBtn);
  btnLayout->addWidget(closeBtn);
  mainLayout->addLayout(btnLayout);

  connect(execBtn, &QPushButton::clicked, this, &PlotsDialog::on_execute);
  connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);

  /* Restore previous selection */
  plot_type currentType = m_plot->type;
  if (currentType == PLOT_ENERGY && m_rbEnergy)
    m_rbEnergy->setChecked(true);
  else if (currentType == PLOT_FORCE && m_rbForce)
    m_rbForce->setChecked(true);
  else if (currentType == PLOT_VOLUME && m_rbVolume)
    m_rbVolume->setChecked(true);
  else if (currentType == PLOT_PRESSURE && m_rbPressure)
    m_rbPressure->setChecked(true);
  else if (currentType == PLOT_DOS && m_rbDOS)
    m_rbDOS->setChecked(true);
  else if (currentType == PLOT_BAND && m_rbBand)
    m_rbBand->setChecked(true);
  else if (currentType == PLOT_BANDOS && m_rbBandDOS)
    m_rbBandDOS->setChecked(true);
  else if (currentType == PLOT_FREQUENCY && m_rbVibrational)
    m_rbVibrational->setChecked(true);
  else if (currentType == PLOT_RAMAN && m_rbRaman)
    m_rbRaman->setChecked(true);
}

void PlotsDialog::populateDynamicsTab(QTabWidget *tabWidget)
{
  auto *tab = new QWidget();
  auto *layout = new QVBoxLayout(tab);
  layout->setContentsMargins(8, 6, 8, 6);
  layout->setSpacing(6);

  auto *infoHBox = new QHBoxLayout();
  infoHBox->addWidget(new QLabel(tr("Ionic steps:")));
  infoHBox->addWidget(new QLabel(QString::number(m_model->num_frames)));
  infoHBox->addStretch();
  layout->addLayout(infoHBox);

  auto *group = new QGroupBox(tr("Plot type"));
  auto *groupLayout = new QVBoxLayout(group);
  groupLayout->setSpacing(4);

  m_rbEnergy = new QRadioButton(tr("Energy / step"));
  m_buttonGroup->addButton(m_rbEnergy);
  m_rbEnergy->setProperty("plotType", PLOT_ENERGY);
  groupLayout->addWidget(m_rbEnergy);

  m_rbForce = new QRadioButton(tr("Forces / step"));
  m_buttonGroup->addButton(m_rbForce);
  m_rbForce->setProperty("plotType", PLOT_FORCE);
  groupLayout->addWidget(m_rbForce);

  m_rbVolume = new QRadioButton(tr("Volume / step"));
  m_buttonGroup->addButton(m_rbVolume);
  m_rbVolume->setProperty("plotType", PLOT_VOLUME);
  groupLayout->addWidget(m_rbVolume);

  m_rbPressure = new QRadioButton(tr("Pressure / step"));
  m_buttonGroup->addButton(m_rbPressure);
  m_rbPressure->setProperty("plotType", PLOT_PRESSURE);
  groupLayout->addWidget(m_rbPressure);

  /* Disable unavailable options */
  if (!(m_plot->plot_mask & PLOT_ENERGY))
    m_rbEnergy->setEnabled(false);
  if (!(m_plot->plot_mask & PLOT_FORCE))
    m_rbForce->setEnabled(false);
  if (!(m_plot->plot_mask & PLOT_VOLUME))
    m_rbVolume->setEnabled(false);
  if (!(m_plot->plot_mask & PLOT_PRESSURE))
    m_rbPressure->setEnabled(false);

  if (m_model->num_frames <= 1)
    group->setEnabled(false);

  layout->addWidget(group);
  layout->addStretch();

  tabWidget->addTab(tab, tr("Dynamics"));
}

void PlotsDialog::populateElectronicTab(QTabWidget *tabWidget)
{
  auto *tab = new QWidget();
  auto *layout = new QVBoxLayout(tab);
  layout->setContentsMargins(8, 6, 8, 6);
  layout->setSpacing(6);

  auto *infoHBox1 = new QHBoxLayout();
  infoHBox1->addWidget(new QLabel(tr("DOS present:")));
  infoHBox1->addWidget(new QLabel(QString::number(m_plot->ndos)));
  infoHBox1->addStretch();
  layout->addLayout(infoHBox1);

  auto *infoHBox2 = new QHBoxLayout();
  infoHBox2->addWidget(new QLabel(tr("BAND present:")));
  infoHBox2->addWidget(new QLabel(QString::number(m_plot->nbands)));
  infoHBox2->addStretch();
  layout->addLayout(infoHBox2);

  auto *group = new QGroupBox(tr("Plot type"));
  auto *groupLayout = new QVBoxLayout(group);
  groupLayout->setSpacing(4);

  m_rbDOS = new QRadioButton(tr("Density Of States"));
  m_buttonGroup->addButton(m_rbDOS);
  m_rbDOS->setProperty("plotType", PLOT_DOS);
  groupLayout->addWidget(m_rbDOS);

  m_rbBand = new QRadioButton(tr("Band structure"));
  m_buttonGroup->addButton(m_rbBand);
  m_rbBand->setProperty("plotType", PLOT_BAND);
  groupLayout->addWidget(m_rbBand);

  m_rbBandDOS = new QRadioButton(tr("Band / DOS"));
  m_buttonGroup->addButton(m_rbBandDOS);
  m_rbBandDOS->setProperty("plotType", PLOT_BANDOS);
  groupLayout->addWidget(m_rbBandDOS);

  /* Disable unavailable options */
  if (!(m_plot->plot_mask & PLOT_DOS))
    m_rbDOS->setEnabled(false);
  if (!(m_plot->plot_mask & PLOT_BAND))
    m_rbBand->setEnabled(false);
  if ((m_plot->plot_mask & PLOT_BANDOS) ^ PLOT_BANDOS)
    m_rbBandDOS->setEnabled(false);

  if (!(m_plot->plot_mask & (PLOT_BAND | PLOT_DOS | PLOT_BANDOS)))
    group->setEnabled(false);

  layout->addWidget(group);
  layout->addStretch();

  tabWidget->addTab(tab, tr("Electronic"));
}

void PlotsDialog::populateFrequencyTab(QTabWidget *tabWidget)
{
  auto *tab = new QWidget();
  auto *layout = new QVBoxLayout(tab);
  layout->setContentsMargins(8, 6, 8, 6);
  layout->setSpacing(6);

  auto *infoHBox1 = new QHBoxLayout();
  infoHBox1->addWidget(new QLabel(tr("FREQs present:")));
  infoHBox1->addWidget(new QLabel(QString::number(m_plot->nfreq)));
  infoHBox1->addStretch();
  layout->addLayout(infoHBox1);

  auto *infoHBox2 = new QHBoxLayout();
  infoHBox2->addWidget(new QLabel(tr("RAMAN present:")));
  infoHBox2->addWidget(new QLabel(QString::number(m_plot->nraman)));
  infoHBox2->addStretch();
  layout->addLayout(infoHBox2);

  auto *group = new QGroupBox(tr("Plot type"));
  auto *groupLayout = new QVBoxLayout(group);
  groupLayout->setSpacing(4);

  m_rbVibrational = new QRadioButton(tr("Vibrational"));
  m_buttonGroup->addButton(m_rbVibrational);
  m_rbVibrational->setProperty("plotType", PLOT_FREQUENCY);
  groupLayout->addWidget(m_rbVibrational);

  m_rbRaman = new QRadioButton(tr("Raman"));
  m_buttonGroup->addButton(m_rbRaman);
  m_rbRaman->setProperty("plotType", PLOT_RAMAN);
  groupLayout->addWidget(m_rbRaman);

  /* Disable unavailable options */
  if (!(m_plot->plot_mask & PLOT_FREQUENCY))
    m_rbVibrational->setEnabled(false);
  if (!(m_plot->plot_mask & PLOT_RAMAN))
    m_rbRaman->setEnabled(false);

  if (!(m_plot->plot_mask & (PLOT_FREQUENCY | PLOT_RAMAN)))
    group->setEnabled(false);

  layout->addWidget(group);
  layout->addStretch();

  tabWidget->addTab(tab, tr("Frequency"));
}

void PlotsDialog::on_execute()
{
  fprintf(stderr, "[DEBUG] on_execute called!\n");
  if (!m_model || !m_plot)
    return;

  /* Determine selected plot type from radio buttons */
  plot_type selectedType = PLOT_NONE;
  if (m_rbEnergy && m_rbEnergy->isChecked())
    selectedType = PLOT_ENERGY;
  else if (m_rbForce && m_rbForce->isChecked())
    selectedType = PLOT_FORCE;
  else if (m_rbVolume && m_rbVolume->isChecked())
    selectedType = PLOT_VOLUME;
  else if (m_rbPressure && m_rbPressure->isChecked())
    selectedType = PLOT_PRESSURE;
  else if (m_rbDOS && m_rbDOS->isChecked())
    selectedType = PLOT_DOS;
  else if (m_rbBand && m_rbBand->isChecked())
    selectedType = PLOT_BAND;
  else if (m_rbBandDOS && m_rbBandDOS->isChecked())
    selectedType = PLOT_BANDOS;
  else if (m_rbVibrational && m_rbVibrational->isChecked())
    selectedType = PLOT_FREQUENCY;
  else if (m_rbRaman && m_rbRaman->isChecked())
    selectedType = PLOT_RAMAN;

  if (selectedType == PLOT_NONE)
  {
    QMessageBox::warning(this, tr("Plots"), tr("No plot type selected."));
    return;
  }

  /* Update plot structure */
  m_plot->type = selectedType;
  m_plot->xtics = m_xticsSpin->value();
  m_plot->ytics = m_yticsSpin->value();
  m_plot->data_changed = TRUE;

  fprintf(stderr, "[DEBUG] Plot type=%d selectedType=%d plot_mask=%d\n", m_plot->type, selectedType, m_plot->plot_mask);

  /* Lock model and run plot tasks synchronously */
  m_model->locked = TRUE;
  m_plot->model = m_model;

  extern void plot_load_data(struct plot_pak *, struct task_pak *);
  extern void plot_prepare_data(struct plot_pak *);
  extern void plot_draw_graph(struct plot_pak *, struct task_pak *);
  extern void plot_show_graph(struct plot_pak *);

  /* Create a dummy task struct for plot_load_data which needs task->progress */
  struct task_pak dummyTask;
  memset(&dummyTask, 0, sizeof(dummyTask));
  dummyTask.progress = 0.0;

  /* Run PLOT-INIT first (load + prepare data) */
  plot_load_data(m_plot, &dummyTask);
  plot_prepare_data(m_plot);

  /* Run PLOT-DRAW second (draw the graph) */
  fprintf(stderr, "[DEBUG] Before plot_draw_graph: type=%d\n", m_plot->type);
  plot_draw_graph(m_plot, &dummyTask);
  fprintf(stderr, "[DEBUG] After plot_draw_graph\n");
  plot_show_graph(m_plot);

  m_model->locked = FALSE;

  /* Refresh tree and canvas */
  extern void qt_refresh_all(void);
  qt_refresh_all();
  extern void qt_force_canvas_refresh(void);
  qt_force_canvas_refresh();

  /* Dialog stays open for next plot */
}

/* Bridge function */
extern "C" void qt_show_plots_dialog(void)
{
  extern struct sysenv_pak sysenv;
  extern QWidget *get_main_window_widget();
  extern void plot_initialize(struct model_pak *);
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (!model)
    return;

  /* Initialize plot data if not already done */
  if (!model->plot)
  {
    plot_initialize(model);
    if (!model->plot)
      return;
  }

  PlotsDialog *dlg = new PlotsDialog(model, get_main_window_widget());
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->show();
}
