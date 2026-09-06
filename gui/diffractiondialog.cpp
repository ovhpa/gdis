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

#include "diffractiondialog.h"
#include "gdis_api.h"

#include <glib.h>

/* Include C headers — glib.h before Qt to avoid C++ exception specifier conflicts */
#include "gdis.h"
#include "coords.h"
#include "matrix.h"
#include "model.h"
#include "spatial.h"
#include "surface.h"
#include "graph.h"
#include "parse.h"
#include "file.h"
#include "task.h"
#include "numeric.h"
#include "glcanvas.h"

#include <QMessageBox>

/* External C bridge functions */
extern void gui_text_show(gint type, const gchar *msg);
extern void redraw_canvas(gint);
extern void qt_refresh_all(void);
extern void free_slist(GSList *list);
extern struct sysenv_pak sysenv;

/* Diffraction enums and globals — from gui_still_valid.c */
/* MEH = m*e*e/2*h*h */
#define MEH 0.026629795
enum { DIFF_XRAY, DIFF_NEUTRON, DIFF_ELECTRON, DIFF_GAUSSIAN, DIFF_LORENTZIAN, DIFF_PSEUDO_VOIGT };
gint diff_all_frames = TRUE;
extern struct elem_pak elements[];

/* Diffraction functions — moved from gui_still_valid.c */
/* Definitions at end of file with extern "C" linkage */

DiffractionDialog::DiffractionDialog(struct model_pak *model, QWidget *parent) : QDialog(parent), m_model(model)
{
  setWindowTitle("Powder Diffraction");
  setMinimumSize(500, 400);

  /* Store current values in model diffract struct */
  setup_ui();

  /* Sync Qt widgets to model values */
  m_radiationCombo->setCurrentIndex(m_model->diffract.radiation);
  m_wavelengthEdit->setText(QString("%1").arg(m_model->diffract.wavelength, 0, 'f', 6));
  m_broadeningCombo->setCurrentIndex(m_model->diffract.broadening);
  m_mixingSpin->setValue(m_model->diffract.asym);
  m_thetaMinSpin->setValue(m_model->diffract.theta[0]);
  m_thetaMaxSpin->setValue(m_model->diffract.theta[1]);
  m_thetaStepSpin->setValue(m_model->diffract.theta[2]);
  m_uSpin->setValue(m_model->diffract.u);
  m_vSpin->setValue(m_model->diffract.v);
  m_wSpin->setValue(m_model->diffract.w);

  m_filenameEdit->setText(m_model->basename);

  if (m_model->animation)
    m_allFramesCheck->setChecked(true);
  else
    m_allFramesCheck->setChecked(false);
}

void DiffractionDialog::setup_ui()
{
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setSpacing(6);
  mainLayout->setContentsMargins(6, 6, 6, 6);

  /* Radiation frame */
  auto *radFrame = new QGroupBox("Radiation", this);
  auto *radLayout = new QVBoxLayout(radFrame);
  radLayout->setSpacing(4);
  radLayout->setContentsMargins(4, 4, 4, 4);

  auto *radHbox = new QHBoxLayout();
  radHbox->setSpacing(6);
  radLayout->addLayout(radHbox);

  auto *radLabel = new QLabel("Radiation", radFrame);
  radHbox->addWidget(radLabel);

  m_radiationCombo = new QComboBox(radFrame);
  m_radiationCombo->addItems({"X-Rays", "Neutrons", "Electrons"});
  radHbox->addWidget(m_radiationCombo);

  auto *waveHbox = new QHBoxLayout();
  waveHbox->setSpacing(6);
  radLayout->addLayout(waveHbox);

  auto *waveLabel = new QLabel("Wavelength", radFrame);
  waveHbox->addWidget(waveLabel);

  m_wavelengthEdit = new QLineEdit(radFrame);
  m_wavelengthEdit->setFixedWidth(120);
  waveHbox->addWidget(m_wavelengthEdit);

  mainLayout->addWidget(radFrame);

  /* Broadening frame */
  auto *broadFrame = new QGroupBox("Broadening", this);
  auto *broadLayout = new QVBoxLayout(broadFrame);
  broadLayout->setSpacing(4);
  broadLayout->setContentsMargins(4, 4, 4, 4);

  auto *broadHbox = new QHBoxLayout();
  broadHbox->setSpacing(6);
  broadLayout->addLayout(broadHbox);

  auto *broadLabel = new QLabel("Broadening function", broadFrame);
  broadHbox->addWidget(broadLabel);

  m_broadeningCombo = new QComboBox(broadFrame);
  m_broadeningCombo->addItems({"Pseudo-Voigt", "Lorentzian", "Gaussian"});
  broadHbox->addWidget(m_broadeningCombo);

  auto *mixHbox = new QHBoxLayout();
  mixHbox->setSpacing(6);
  broadLayout->addLayout(mixHbox);

  auto *mixLabel = new QLabel("Mixing parameter", broadFrame);
  mixHbox->addWidget(mixLabel);

  m_mixingSpin = new QDoubleSpinBox(broadFrame);
  m_mixingSpin->setRange(0.0, 1.0);
  m_mixingSpin->setSingleStep(0.01);
  m_mixingSpin->setDecimals(2);
  m_mixingSpin->setFixedWidth(100);
  mixHbox->addWidget(m_mixingSpin);

  mainLayout->addWidget(broadFrame);

  /* Split pane: 2theta range | U V W */
  auto *splitter = new QSplitter(Qt::Horizontal, this);

  /* Left: 2theta range */
  auto *thetaFrame = new QGroupBox("2Theta range", splitter);
  auto *thetaLayout = new QVBoxLayout(thetaFrame);
  thetaLayout->setSpacing(4);
  thetaLayout->setContentsMargins(4, 4, 4, 4);

  auto addSpin = [&](QVBoxLayout *layout, const QString &label, QDoubleSpinBox *spin) {
    auto *hbox = new QHBoxLayout();
    hbox->setSpacing(6);
    auto *lbl = new QLabel(label, thetaFrame);
    hbox->addWidget(lbl);
    hbox->addWidget(spin);
    layout->addLayout(hbox);
  };

  m_thetaMinSpin = new QDoubleSpinBox(thetaFrame);
  m_thetaMinSpin->setRange(0.0, 160.0);
  m_thetaMinSpin->setSingleStep(0.1);
  m_thetaMinSpin->setDecimals(1);
  addSpin(thetaLayout, "  Min", m_thetaMinSpin);

  m_thetaMaxSpin = new QDoubleSpinBox(thetaFrame);
  m_thetaMaxSpin->setRange(0.0, 170.0);
  m_thetaMaxSpin->setSingleStep(0.1);
  m_thetaMaxSpin->setDecimals(1);
  addSpin(thetaLayout, "  Max", m_thetaMaxSpin);

  m_thetaStepSpin = new QDoubleSpinBox(thetaFrame);
  m_thetaStepSpin->setRange(0.01, 1.0);
  m_thetaStepSpin->setSingleStep(0.01);
  m_thetaStepSpin->setDecimals(2);
  addSpin(thetaLayout, "  Step", m_thetaStepSpin);

  mainLayout->addWidget(splitter);

  /* Right: U V W */
  auto *uvwFrame = new QGroupBox("U V W broadening", splitter);
  auto *uvwLayout = new QVBoxLayout(uvwFrame);
  uvwLayout->setSpacing(4);
  uvwLayout->setContentsMargins(4, 4, 4, 4);

  auto addUVW = [&](QVBoxLayout *layout, const QString &label, QDoubleSpinBox *spin) {
    auto *hbox = new QHBoxLayout();
    hbox->setSpacing(6);
    auto *lbl = new QLabel(label, uvwFrame);
    lbl->setFixedWidth(30);
    hbox->addWidget(lbl);
    hbox->addWidget(spin);
    layout->addLayout(hbox);
  };

  m_uSpin = new QDoubleSpinBox(uvwFrame);
  m_uSpin->setRange(-9.9, 9.9);
  m_uSpin->setSingleStep(0.05);
  m_uSpin->setDecimals(2);
  addUVW(uvwLayout, "U", m_uSpin);

  m_vSpin = new QDoubleSpinBox(uvwFrame);
  m_vSpin->setRange(-9.9, 9.9);
  m_vSpin->setSingleStep(0.05);
  m_vSpin->setDecimals(2);
  addUVW(uvwLayout, "V", m_vSpin);

  m_wSpin = new QDoubleSpinBox(uvwFrame);
  m_wSpin->setRange(-9.9, 9.9);
  m_wSpin->setSingleStep(0.05);
  m_wSpin->setDecimals(2);
  addUVW(uvwLayout, "W", m_wSpin);

  /* Output frame */
  auto *outFrame = new QGroupBox("Output", this);
  auto *outLayout = new QVBoxLayout(outFrame);
  outLayout->setSpacing(4);
  outLayout->setContentsMargins(4, 4, 4, 4);

  auto *fileHbox = new QHBoxLayout();
  fileHbox->setSpacing(6);
  outLayout->addLayout(fileHbox);

  auto *fileLabel = new QLabel("Filename", outFrame);
  fileHbox->addWidget(fileLabel);

  m_filenameEdit = new QLineEdit(outFrame);
  fileHbox->addWidget(m_filenameEdit);

  mainLayout->addWidget(outFrame);

  /* Check box for all frames */
  m_allFramesCheck = new QCheckBox("Calculate for all frames (output file only)", this);
  mainLayout->addWidget(m_allFramesCheck);

  /* Buttons */
  auto *btnLayout = new QHBoxLayout();
  btnLayout->setSpacing(6);
  mainLayout->addLayout(btnLayout);

  auto *calcBtn = new QPushButton("Calculate", this);
  calcBtn->setMinimumWidth(100);
  connect(calcBtn, &QPushButton::clicked, this, &DiffractionDialog::on_calculate);
  btnLayout->addWidget(calcBtn);

  auto *closeBtn = new QPushButton("Close", this);
  closeBtn->setMinimumWidth(100);
  connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
  btnLayout->addStretch();
  btnLayout->addWidget(closeBtn);
}

void DiffractionDialog::on_calculate()
{
  /* Read values from Qt widgets into model diffract struct */
  switch (m_radiationCombo->currentIndex())
  {
  case 0:
    m_model->diffract.radiation = 0;
    break; /* DIFF_XRAY */
  case 1:
    m_model->diffract.radiation = 1;
    break; /* DIFF_NEUTRON */
  case 2:
    m_model->diffract.radiation = 2;
    break; /* DIFF_ELECTRON */
  default:
    m_model->diffract.radiation = 0;
    break;
  }

  bool ok;
  double wl = m_wavelengthEdit->text().toDouble(&ok);
  if (!ok)
    wl = 1.54180;
  m_model->diffract.wavelength = fabs(wl);

  switch (m_broadeningCombo->currentIndex())
  {
  case 0:
    m_model->diffract.broadening = 3;
    break; /* DIFF_PSEUDO_VOIGT */
  case 1:
    m_model->diffract.broadening = 4;
    break; /* DIFF_LORENTZIAN */
  case 2:
    m_model->diffract.broadening = 3;
    break; /* DIFF_GAUSSIAN */
  default:
    m_model->diffract.broadening = 3;
    break;
  }

  m_model->diffract.asym = m_mixingSpin->value();
  m_model->diffract.theta[0] = m_thetaMinSpin->value();
  m_model->diffract.theta[1] = m_thetaMaxSpin->value();
  m_model->diffract.theta[2] = m_thetaStepSpin->value();
  m_model->diffract.u = m_uSpin->value();
  m_model->diffract.v = m_vSpin->value();
  m_model->diffract.w = m_wSpin->value();

  /* Store filename */
  g_free(m_model->diffract_filename);
  m_model->diffract_filename = g_strdup(m_filenameEdit->text().toStdString().c_str());

  /* Run calculation */
  qt_diffraction_calc(m_model);

  /* Notify Qt canvas to repaint */
  extern void *qt_get_gl_canvas(void);
  GLCanvas *canvas = static_cast<GLCanvas *>(qt_get_gl_canvas());
  if (canvas)
    canvas->update();
}
extern "C" gint dhkl_compare(gpointer ptr1, gpointer ptr2)
{
  struct plane_pak *plane1 = (struct plane_pak *) ptr1;
  struct plane_pak *plane2 = (struct plane_pak *) ptr2;

  if (plane1->dhkl > plane2->dhkl)
    return (-1);
  if (plane1->dhkl < plane2->dhkl)
    return (1);

  return (0);
}
/* NB: assumes the input plane list is Dhkl sorted */
extern "C" GSList *diff_get_unique_faces(GSList *list, struct model_pak *model)
{
  gdouble delta, dhkl;
  GSList *list1;
  struct plane_pak *plane = NULL, *refplane = NULL;

  dhkl = 999999999.9;
  list1 = list;
  while (list1)
  {
    plane = (struct plane_pak *) list1->data;
    list1 = g_slist_next(list1);

    /* determine if this is a repeated plane */
    delta = dhkl - plane->dhkl;
    if (delta > FRACTION_TOLERANCE)
    {
      /*
      printf("(%d %d %d) ", plane->index[0], plane->index[1], plane->index[2]);
          ret_list = g_slist_prepend(ret_list, plane);
      */

      /* this is a new plane */
      refplane = plane;
      dhkl = refplane->dhkl;
      refplane->multiplicity = 1;
    } else
    {
      /* related plane */
      list = g_slist_remove(list, plane);
      g_free(plane);
      /*
      printf("(%d %d %d) ", plane->index[0], plane->index[1], plane->index[2]);
      */
      if (refplane != NULL)
        refplane->multiplicity++; /*FIX f93659*/
    }
  }

  return (list);
}
#define DEBUG_CALC_SPECTRUM 0
extern "C" void diff_calc_spectrum(GSList *list, struct model_pak *model)
{
  gint i, n;
  gint cur_peak;
#ifdef UNUSED_BUT_SET
  gint old_peak;
#endif
  gdouble angle, sol, f, g, lpf, dr;
  gdouble c1, sqrt_c1, sqrt_pi, sina, cosa, sin2a, cos2a, tana, fwhm, bf, intensity;
  gdouble *spectrum;
  gpointer graph;
  GSList *item;
  struct plane_pak *plane;
  FILE *fp;
  /*NEW graph system*/
  g_data_x gx;
  g_data_y gy;
  gint idx;
  gdouble max;
  gchar *line;

  /* constant initialization */
  c1 = 4.0 * log(2.0);
  sqrt_c1 = sqrt(c1);
  sqrt_pi = sqrt(G_PI);

  /* initialize the output spectrum */
  n = 1 + (model->diffract.theta[1] - model->diffract.theta[0]) / model->diffract.theta[2];
  spectrum = (gdouble *) g_malloc0(n * sizeof(gdouble));

  /* initialize main peaks graph data */
  gy.y_size = n;
  gy.y = (gdouble *) g_malloc0(gy.y_size * sizeof(gdouble));
  gy.idx = (gint32 *) g_malloc0(gy.y_size * sizeof(gint32));
  gy.symbol = (graph_symbol *) g_malloc0(gy.y_size * sizeof(graph_symbol));
  gy.sym_color = NULL;
  max = -1.0 / 0.0; /*-inf*/

#if DEBUG_CALC_SPECTRUM
  printf("----------------------------------------------------------------------\n");
  printf("    hkl    :  m :  Dhkl  : 2theta :  lpf   :     |F|     :      I\n");
  printf("----------------------------------------------------------------------\n");
#endif

/* loop over all spplied peaks */
#ifdef UNUSED_BUT_SET
  old_peak = -1;
#endif
  for (item = list; item; item = g_slist_next(item))
  {
    plane = (struct plane_pak *) item->data;

    /* structure factor squared */
    f = plane->f[0] * plane->f[0] + plane->f[1] * plane->f[1];

    /* compute peak position and broadening parameters */
    sol = 0.5 / plane->dhkl;
    sina = sol * model->diffract.wavelength;
    angle = asin(sina);
    cosa = cos(angle);
    angle *= 2.0;
    cos2a = 1 - 2.0 * sina * sina;
    sin2a = 2.0 * sina * cosa;
    tana = sina / cosa;
    fwhm = sqrt(model->diffract.w + model->diffract.v * tana + model->diffract.u * tana * tana);

    /* lorentz polarization factor for powder (unpolarized incident beam) */
    /* TODO - include polarization factor (default 0.5) */
    if (model->diffract.radiation == DIFF_XRAY)
      lpf = 0.5 * (1.0 + cos2a * cos2a);
    else
      lpf = 1.0;
    lpf /= (sina * sin2a);

    /* determine if the current peak lies on the same bin as the previous peak - this */
    /* is the peak overlap problem (ie adding intensities over-emphasizes such peaks) */
    dr = (R2D * angle - model->diffract.theta[0]) / model->diffract.theta[2];
    /*
      cur_peak = (gint) dr;
    */
    cur_peak = nearest_int(dr);
#ifdef UNUSED_BUT_SET
    old_peak = cur_peak;
#endif
    /* fill out the spectrum with the current peak */
    for (i = 0; i < n; i++)
    {
      /* compute distance to peak maximum */

      /* absolute distance to peak */
      /*
          dr = R2D*angle - (model->diffract.theta[2] * (gdouble) i) - model->diffract.theta[0];
      */

      /* quantized distance to peak */
      dr = (gdouble) (i - cur_peak);
      dr *= model->diffract.theta[2];

      /* compute broadening factor - peak contribution at current spectrum interval */
      bf = 1.0 / model->diffract.theta[2];

      if (fabs(fwhm) > 0.00001)
      {
        switch (model->diffract.broadening)
        {
        case DIFF_GAUSSIAN:
          bf = sqrt_c1 * exp(-c1 * dr * dr / (fwhm * fwhm)) / (fwhm * sqrt_pi);
          break;

        case DIFF_LORENTZIAN:
          bf = fwhm / (2.0 * G_PI * (dr * dr + 0.25 * fwhm * fwhm));
          break;

        case DIFF_PSEUDO_VOIGT:
          g = model->diffract.asym;
          bf = g * 2.0 / (fwhm * G_PI * (1.0 + 4.0 * (dr * dr / (fwhm * fwhm))));
          bf += (1 - g) * sqrt_c1 * exp(-c1 * dr * dr / (fwhm * fwhm)) / (fwhm * G_PI);
          break;

        default:
          g_assert_not_reached();
        }
      } else
      {
        /* no contribution from peaks with zero FWHM outside their spectrum interval */
        if (cur_peak != i)
          bf = 0.0;
      }

      /* compute intensity for the current bin */
      intensity = plane->multiplicity * lpf * f * bf;
      spectrum[i] += intensity;
      /* add peak-only data */
      if ((cur_peak == i) && (intensity > 0.1))
      {
        /*this is a peak*/
        gy.y[i] += intensity;
        if (gy.y[i] > max)
          max = gy.y[i];
        /*compose a "name" for the peak 0xFF ensure the 8-bit*/
        gy.idx[i] = ((gint8) plane->index[0]) & 0xFF;
        gy.idx[i] += (((gint8) plane->index[1]) & 0xFF) << 8;
        gy.idx[i] += (((gint8) plane->index[2]) & 0xFF) << 16;
        gy.idx[i] += (((gint8) 0x55) & 0xFF) << 24; /*this serve as a marker*/
        /*square symbol (selectable)*/
        gy.symbol[i] = GRAPH_SYMB_SQUARE;
      }
    }

#if DEBUG_CALC_SPECTRUM
    printf("[%2d %2d %2d] : %2d : %6.4f : %6.3f : %6.3f : %11.4f : %11.4f\n", plane->index[0], plane->index[1],
           plane->index[2], plane->multiplicity, plane->dhkl, R2D * angle, lpf, sqrt(f), plane->multiplicity * lpf * f);
#endif
  }

  /* create the plots */
  if (model->animation && diff_all_frames)
  {
    /* cleanup to FIX memory leak (OVHPA) */
    g_free(gy.y);
    g_free(gy.idx);
    g_free(gy.symbol);
    /*
    printf("TODO - multiframe animation plotting.\n");
    */
  } else
  {
    /* try to make the 2theta scale nice */
    i = model->diffract.theta[1] - model->diffract.theta[0];
    if (i > 49)
      i /= 10;
    else if (i > 9)
      i /= 5;
    i++;

    /* create a new graph */
    switch (model->diffract.radiation)
    {
    case DIFF_ELECTRON:
      graph = graph_new("Electron", model);
      dat_graph_set_title("<big>Electron diffraction</big>", graph);
      break;

    case DIFF_NEUTRON:
      graph = graph_new("Neutron", model);
      dat_graph_set_title("<big>Neutron diffraction</big>", graph);
      break;

    default:
      graph = graph_new("X-Ray", model);
      dat_graph_set_title("<big>X-Ray diffraction</big>", graph);
    }
    /*prepare titles*/
    line = g_strdup_printf("<small> &#955; = %lf </small>", model->diffract.wavelength);
    dat_graph_set_sub_title(line, graph);
    g_free(line);
    dat_graph_set_x_title("2 &#952; (degree)", graph);
    dat_graph_set_y_title("intensities (a.u.)", graph);
    /*prepare data*/
    gx.x_size = n;
    gx.x = (gdouble *) g_malloc0(gx.x_size * sizeof(gdouble));
    gx.x[0] = 0.;
    for (idx = 1; idx < n; idx++)
      gx.x[idx] = gx.x[idx - 1] + model->diffract.theta[2];
    max = max * 1.05; /*adjust nicely*/
    gy.type = GRAPH_XY_TYPE;
    gy.mixed_symbol = FALSE;
    gy.line = GRAPH_LINE_SINGLE;
    gy.color = GRAPH_COLOR_DEFAULT;
    /*prepare graph*/
    dat_graph_set_x(gx, graph);
    dat_graph_add_y(gy, graph);
    /*next set is the spectra without selectable peaks*/
    for (idx = 0; idx < n; idx++)
    {
      gy.y[idx] = spectrum[idx];
      gy.idx[idx] = 0;
      gy.symbol[idx] = GRAPH_SYMB_NONE;
    }
    /*finish graph*/
    dat_graph_add_y(gy, graph);
    dat_graph_set_type(GRAPH_XY_TYPE, graph);
    dat_graph_set_limits(model->diffract.theta[0], model->diffract.theta[1], 0., max, graph);
    graph_set_yticks(FALSE, 2, graph);
    graph_set_xticks(TRUE, i, graph);
    graph_set_wavelength(model->diffract.wavelength, graph);
    g_free(gx.x);
    g_free(gy.y);
    g_free(gy.idx);
    g_free(gy.symbol);

    /* NEW - clear any other special objects displayed */
    model->picture_active = NULL;
    /* display the new graph */
    /* Refresh Qt tree to show new graph as child of model */
    extern void qt_refresh_all(void);
    qt_refresh_all();
  }

  /* save the spectrum */
  const gchar *outfile = model->diffract_filename ? model->diffract_filename : model->basename;
  fp = fopen(outfile, "at");
  if (!fp)
  {
    gui_text_show(ERROR, "Failed to open file for raw spectrum data.\n");
    g_free(spectrum); /*FIX 20cedb*/
    return;
  }
  for (i = 0; i < n; i++)
    fprintf(fp, "%5.4f %f\n", model->diffract.theta[0] + i * model->diffract.theta[2], spectrum[i]);
  fclose(fp);

  /* cleanup */
  g_free(spectrum);
  redraw_canvas(SINGLE);
}
/* FIXME - gain speed by prefeching for unique_atom_list */
extern "C" gdouble diff_get_sfc(gint type, gdouble sol, struct core_pak *core)
{
  gint i;
  gboolean flag;
  gdouble a, b, sfc, sol2;
  GSList *list;

  /* FIXME - CU will not match Cu in hash table lookup */
  list = (GSList *) g_hash_table_lookup(sysenv.sfc_table, elements[core->atom_code].symbol);
  if (!list)
  {
    printf("No sfc found for [%s], fudging...\n", core->atom_label);
    return ((gdouble) core->atom_code);
  }

  /* eforce list length */
  g_assert(g_slist_length(list) > 8);
  flag = FALSE;
  switch (type)
  {
  case DIFF_ELECTRON:
    flag = TRUE;
  case DIFF_XRAY:
    /* wave form factor */
    sfc = *((gdouble *) g_slist_nth_data(list, 8));
    sol2 = sol * sol;
    for (i = 0; i < 4; i++)
    {
      a = *((gdouble *) g_slist_nth_data(list, 2 * i));
      b = *((gdouble *) g_slist_nth_data(list, 2 * i + 1));
      sfc += a * exp(-b * sol2);
    }
    /* electron correction */
    if (flag)
      sfc = MEH * ((gdouble) core->atom_code - sfc) / sol2;
    break;

  case DIFF_NEUTRON:
    sfc = *((gdouble *) g_slist_nth_data(list, 9));
    break;

  default:
    printf("Unsupported radiation type, fudging...\n");
    sfc = core->atom_code;
    break;
  }

  /*
  printf("[%s] %d : %f\n", core->label, core->atom_code, sfc);
  */

  return (sfc);
}
/* TODO - merge common functionality with get_ranked_faces() in surface.c */
#define DEBUG_DIFF_RANK 0
extern "C" GSList *diff_get_ranked_faces(gdouble min, struct model_pak *model)
{
  gint h, k, l, hs, ks, ls;
  gdouble f1[3];
  GSList *list1;
  struct plane_pak *plane;

/* FIXME - should we have a Dhkl cutoff instead of a HKL limit?*/
#define HKL_LIMIT 19

/* enumerate all unique hkl within defined limit */
#if DEBUG_DIFF_RANK
  printf("trying:\n");
#endif
  list1 = NULL;
  for (hs = -1; hs < 2; hs += 2)
  {
    /* start at 0 for -ve and at 1 for +ve */
    for (h = (hs + 1) / 2; h < HKL_LIMIT; h++)
    {
      for (ks = -1; ks < 2; ks += 2)
      {
        for (k = (ks + 1) / 2; k < HKL_LIMIT; k++)
        {
          for (ls = -1; ls < 2; ls += 2)
          {
            for (l = (ls + 1) / 2; l < HKL_LIMIT; l++)
            {
#if DEBUG_DIFF_RANK
              printf("(%d %d %d)", hs * h, ks * k, ls * l);
#endif
              if (!h && !k && !l)
                continue;

              if (surf_sysabs(model, hs * h, ks * k, ls * l))
                continue;

#if DEBUG_DIFF_RANK
              printf("\n");
#endif

              /* add the plane */
              VEC3SET(f1, hs * h, ks * k, ls * l);
              plane = (struct plane_pak *) plane_new(f1, model);
              if (!plane)
                continue;

              if (plane->dhkl > min)
                list1 = g_slist_prepend(list1, plane);
              else
                break;
            }
          }
        }
      }
    }
  }

  /* sort via Dhkl */
  list1 = g_slist_sort(list1, (GCompareFunc) dhkl_compare);
  /* remove faces with same Dhkl (NB: assumes they are sorted!) */
  list1 = diff_get_unique_faces(list1, model);

  return (list1);
}
#define DEBUG_DIFF_CALC 0
extern "C" gint diff_calc_from_model(struct model_pak *model)
{
  gdouble dhkl_min, tmp, sfc, sol;
  gdouble vec[3], rdv[3];
  GSList *item, *list, *clist;
  struct plane_pak *plane;
  struct core_pak *core;

  if (!model)
    return (1);
  if (model->periodic != 3)
  {
    gui_text_show(WARNING, "Your model is not 3D periodic.\n");
    return (1);
  }

  /* checks */
  if (model->diffract.theta[0] >= model->diffract.theta[1])
  {
    gui_text_show(ERROR, "Invalid 2theta range.\n");
    return (2);
  }

  /* get min Dhkl for (specified) 2theta max */
  dhkl_min = 0.5 * model->diffract.wavelength / tbl_sin(0.5 * D2R * model->diffract.theta[1]);
  model->diffract.dhkl_min = dhkl_min;

  /* deal with the sin/lambda < 2.0 scattering restriction */
  if (model->diffract.wavelength < 0.5)
  {
    tmp = R2D * asin(2.0 * model->diffract.wavelength);
    if (model->diffract.theta[1] > tmp)
    {
      model->diffract.theta[1] = tmp;
      gui_text_show(WARNING, "Your maximum theta value is too large.\n");
    }
    if (model->diffract.theta[0] > tmp)
    {
      model->diffract.theta[1] = tmp;
      gui_text_show(WARNING, "Your minimum theta value is too large.\n");
    }
  }

#if DEBUG_DIFF_CALC
  printf(" radiation = %d\n", model->diffract.radiation);
  printf("wavelength = %f\n", model->diffract.wavelength);
  printf("     theta = %f %f %f\n", model->diffract.theta[0], model->diffract.theta[1], model->diffract.theta[2]);
  printf("      fwhm = %f %f %f\n", model->diffract.u, model->diffract.v, model->diffract.w);
  printf("  Dhkl min = %f\n", dhkl_min);
#endif

  /* NEW - setup for multiframe diffraction */
  if (diff_all_frames)
  {
    if (model->animating)
    {
      gui_text_show(ERROR, "Can't do multiple animations.\n");
      return (1);
    }
    if (!model->transform_list)
    {
      model->afp = fopen(model->filename, "r");
      if (!model->afp)
      {
        gui_text_show(ERROR, "Failed to open animation stream.\n");
        return (1);
      }
    }
    model->animating = TRUE;
  }

  /* NEW - handle multiple frames */
  for (;;)
  {
    list = diff_get_ranked_faces(dhkl_min, model);

    for (item = list; item; item = g_slist_next(item))
    {
      plane = (struct plane_pak *) item->data;
      sol = 0.5 / plane->dhkl;

      plane->f[0] = 0.0;
      plane->f[1] = 0.0;

      for (clist = model->cores; clist; clist = g_slist_next(clist))
      {
        core = (struct core_pak *) clist->data;
        ARR3SET(rdv, plane->index);
        vecmat(model->rlatmat, rdv);
        ARR3SET(vec, core->x);
        vecmat(model->latmat, vec);
        ARR3MUL(rdv, vec);
        tmp = 2.0 * G_PI * (rdv[0] + rdv[1] + rdv[2]);
        sfc = diff_get_sfc(model->diffract.radiation, sol, core);
        plane->f[0] += sfc * tbl_cos(tmp);
        plane->f[1] += sfc * tbl_sin(tmp);
      }
    }

    diff_calc_spectrum(list, model);
    free_slist(list);

    if (diff_all_frames)
    {
      model->cur_frame++;
      if (model->cur_frame == model->num_frames)
        break;
      read_frame(model->afp, model->cur_frame, model);
    } else
      break;
  }
  return (0);
}

/* Qt bridge functions — merged from diffraction_bridge.cpp */

/* Global instance pointer for Qt bridge */
DiffractionDialog *qt_diffraction_dialog_instance = nullptr;

extern "C" void qt_show_diffraction_dialog(struct model_pak *model, QWidget *parent)
{
  if (!model)
  {
    gui_text_show(GDIS_ERROR, "No active model. Please load a molecule first.");
    return;
  }
  if (model->periodic != 3)
  {
    gui_text_show(GDIS_ERROR, "Your model is not 3D periodic.");
    return;
  }

  qt_diffraction_dialog_instance = new DiffractionDialog(model, parent);
  qt_diffraction_dialog_instance->show();
}

extern "C" void qt_diffraction_calc(struct model_pak *model)
{
  gint ret = diff_calc_from_model(model);
  if (ret == 1)
    gui_text_show(GDIS_ERROR, "No active model for diffraction.");
  else if (ret == 2)
    gui_text_show(GDIS_ERROR, "Invalid 2theta range.");
  else
    gui_text_show(GDIS_INFO, "Diffraction calculation complete.");
}
