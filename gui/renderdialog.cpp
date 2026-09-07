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
 * Render Properties Dialog Implementation for GDIS Qt6 GUI
 */

#include "renderdialog.h"
#include "gdis_api.h"
#include "svg_utils.h"
#include "gl_text.h"
#include <QFontDialog>
#include <QFont>

#define LINELEN 300

/* Extern declarations for C functions used in Qt mode */
extern "C" {
void geom_label_toggle(void);
void gui_create_waypoint(void);
void gui_text_show(gint, const gchar *);
void camera_waypoint_animate(gint, gint, struct model_pak *);
void camera_rotate_animate(gint, gdouble *, gint, struct model_pak *);
}
extern gint gl_fontsize;

/* Animation globals from gui_render.c */
extern gdouble render_animate_frames;
extern gint render_animate_overwrite;
extern gdouble render_animate_angle[3];

/* Camera mode enums (from gdis.h) */
enum { FREE = 0, LOCKED = 5 };

/* Define constants that pak.h and model colour schemes need */
#define FILELEN 512
#define LINELEN 300
#define MAX_DISPLAYED 9

/* Model colour scheme enums — must match gdis.h values exactly */
enum { MOL = 5, ELEM = 7, REGION = 12, GROWTH_SLICE = 13, TRANSLATE = 14, OCCUPANCY = 15, VELOCITY = 16 };

/* Include glib for gint, gdouble, GSList, etc. */
#include <glib.h>

/* Include C core headers */
extern "C" {
#include "pak.h"
#include "render.h"
#include "coords.h"
#include "select.h"
#include "spatial.h"
}

/* Extern for sysenv */
extern struct sysenv_pak sysenv;

/* Render mode mapping: 0=Ball&Stick, 1=CPK, 2=Liquorice, 3=Stick */
static const int render_modes[] = {BALL_STICK, CPK, LIQUORICE, STICK};

/* Colouring scheme mapping */
static const int colouring_scheme_values[] = {ELEM, GROWTH_SLICE, MOL, OCCUPANCY, REGION, VELOCITY, TRANSLATE};

/* Helper: update button colour */
static void updateButtonColour(QPushButton *btn, const QColor &colour)
{
  QString style =
      QString("QPushButton { background-color: %1; border: 1px solid gray; padding: 4px; }").arg(colour.name());
  btn->setStyleSheet(style);
}

RenderDialog::RenderDialog(QWidget *parent)
    : QDialog(parent), m_bgColour(0, 0, 0), m_rsurfColour(255, 255, 255), m_ribbonColour(200, 200, 200)
{
  setWindowTitle("Display Properties");
  setMinimumSize(700, 500);

  auto *mainLayout = new QVBoxLayout(this);
  auto *tabWidget = new QTabWidget(this);

  setupMainPage();
  setupColoursPage();
  setupCameraPage();
  setupLightsPage();
  setupOpenGLPage();
  setupPOVRayPage();
  setupStereoPage();
  setupTogglesPage();

  tabWidget->addTab(m_mainPage, "Main");
  tabWidget->addTab(m_coloursPage, "Colours");
  tabWidget->addTab(m_cameraPage, "Camera");
  tabWidget->addTab(m_lightsPage, "Lights");
  tabWidget->addTab(m_openGLPage, "OpenGL");
  tabWidget->addTab(m_povrayPage, "POVRay");
  tabWidget->addTab(m_stereoPage, "Stereo");
  tabWidget->addTab(m_togglesPage, "Toggles");

  mainLayout->addWidget(tabWidget);

  auto *btnBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
  connect(btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  mainLayout->addWidget(btnBox);

  refresh();
}

RenderDialog::~RenderDialog() = default;

void RenderDialog::setCanvas(QWidget *canvas) { m_canvas = canvas; }

void RenderDialog::triggerRedraw()
{
  /* Set need_clear so the framebuffer is properly cleared before re-drawing.
   * This ensures toggle changes (show/hide atoms, shells, bonds, etc.) take
   * effect immediately without requiring a click or resize event. */
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (model)
    model->need_clear = TRUE;
  extern void redraw_canvas(gint);
  redraw_canvas(1); /* SINGLE */
}

void RenderDialog::refresh()
{
  /* Main page */
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (model)
  {
    int mode = model->default_render_mode;
    for (int i = 0; i < 4; i++)
    {
      if (render_modes[i] == mode)
      {
        m_renderModeCombo->setCurrentIndex(i);
        break;
      }
    }
  }
  if (m_antialiasCheck)
    m_antialiasCheck->setChecked(sysenv.render.antialias);
  if (m_cpkScaleCheck)
    m_cpkScaleCheck->setChecked(sysenv.render.scale_ball_size);
  if (m_wireSurfaceCheck)
    m_wireSurfaceCheck->setChecked(sysenv.render.wire_surface);
  if (m_showHiddenCheck)
    m_showHiddenCheck->setChecked(sysenv.render.wire_show_hidden);

  /* Colours */
  m_bgColour = QColor((int) (sysenv.render.bg_colour[0] * 255), (int) (sysenv.render.bg_colour[1] * 255),
                      (int) (sysenv.render.bg_colour[2] * 255));
  m_rsurfColour = QColor((int) (sysenv.render.rsurf_colour[0] * 255), (int) (sysenv.render.rsurf_colour[1] * 255),
                         (int) (sysenv.render.rsurf_colour[2] * 255));
  m_ribbonColour = QColor((int) (sysenv.render.ribbon_colour[0] * 255), (int) (sysenv.render.ribbon_colour[1] * 255),
                          (int) (sysenv.render.ribbon_colour[2] * 255));
  if (m_bgColourBtn)
    updateButtonColour(m_bgColourBtn, m_bgColour);
  if (m_rsurfColourBtn)
    updateButtonColour(m_rsurfColourBtn, m_rsurfColour);
  if (m_ribbonColourBtn)
    updateButtonColour(m_ribbonColourBtn, m_ribbonColour);

  /* OpenGL */
  if (m_sphereQualitySpin)
    m_sphereQualitySpin->setValue((int) sysenv.render.sphere_quality);
  if (m_cylinderQualitySpin)
    m_cylinderQualitySpin->setValue((int) sysenv.render.cylinder_quality);
  if (m_autoQualityCheck)
    m_autoQualityCheck->setChecked(sysenv.render.auto_quality);
  if (m_fastRotationCheck)
    m_fastRotationCheck->setChecked(sysenv.render.fast_rotation);
  if (m_halosCheck)
    m_halosCheck->setChecked(sysenv.render.halos);
  if (m_fogCheck)
    m_fogCheck->setChecked(sysenv.render.fog);
  if (m_fogDensitySpin)
    m_fogDensitySpin->setValue(sysenv.render.fog_density);
  if (m_geomLineWidthSpin)
    m_geomLineWidthSpin->setValue(sysenv.render.geom_line_width);

  /* POVRay */
  if (m_povrayWidthSpin)
    m_povrayWidthSpin->setValue((int) sysenv.render.width);
  if (m_povrayHeightSpin)
    m_povrayHeightSpin->setValue((int) sysenv.render.height);
  if (m_shadowlessCheck)
    m_shadowlessCheck->setChecked(sysenv.render.shadowless);
  if (m_noPovrayExecCheck)
    m_noPovrayExecCheck->setChecked(sysenv.render.no_povray_exec);
  if (m_noKeepTempfilesCheck)
    m_noKeepTempfilesCheck->setChecked(sysenv.render.no_keep_tempfiles);

  /* Camera */
  if (model && model->camera)
  {
    struct camera_pak *cam = (struct camera_pak *) model->camera;
    if (m_cameraProjCombo)
    {
      m_cameraProjCombo->setCurrentIndex(cam->perspective ? 0 : 1);
    }
    if (m_cameraModeCombo)
    {
      m_cameraModeCombo->setCurrentIndex(cam->mode == LOCKED ? 1 : 0);
    }
    if (m_cameraFOVSpin)
    {
      m_cameraFOVSpin->setValue(cam->perspective ? (int) cam->fov : 180);
      m_cameraFOVSpin->setEnabled(cam->perspective);
    }
    if (m_cameraZoomEdit)
    {
      if (cam->perspective)
      {
        m_cameraZoomEdit->setText("not applicable");
        m_cameraZoomEdit->setEnabled(false);
      } else
      {
        double zoom = 1000.0 / (model->rmax * cam->zoom);
        m_cameraZoomEdit->setText(QString::number(zoom, 'f', 1));
        m_cameraZoomEdit->setEnabled(true);
      }
    }
  }

  /* Animation globals */
  if (m_cameraWaypointFramesSpin)
    m_cameraWaypointFramesSpin->setValue((int) render_animate_frames);
  if (m_cameraRotStartSpin)
    m_cameraRotStartSpin->setValue((int) render_animate_angle[0]);
  if (m_cameraRotStopSpin)
    m_cameraRotStopSpin->setValue((int) render_animate_angle[1]);
  if (m_cameraRotIncSpin)
    m_cameraRotIncSpin->setValue((int) render_animate_angle[2]);

  /* Stereo */
  if (m_stereoAnaglyphCheck)
    m_stereoAnaglyphCheck->setChecked(sysenv.render.stereo_anaglyph);
  if (m_stereoAnaglyphBlueRedCheck)
    m_stereoAnaglyphBlueRedCheck->setChecked(sysenv.render.stereo_anaglyph_blue_red);
  if (m_stereoLeftCheck)
    m_stereoLeftCheck->setChecked(sysenv.render.stereo_left);
  if (m_stereoRightCheck)
    m_stereoRightCheck->setChecked(sysenv.render.stereo_right);
  if (m_stereoEyeOffsetSpin)
    m_stereoEyeOffsetSpin->setValue(sysenv.render.stereo_eye_offset);
  if (m_stereoParallaxSpin)
    m_stereoParallaxSpin->setValue(sysenv.render.stereo_parallax);

  /* Toggles */
  if (model)
  {
    if (m_showCellCheck)
      m_showCellCheck->setChecked(model->show_cell);
    if (m_showCellImagesCheck)
      m_showCellImagesCheck->setChecked(model->show_cell_images);
    if (m_showCellLengthsCheck)
      m_showCellLengthsCheck->setChecked(model->show_cell_lengths);
    if (m_showCoresCheck)
      m_showCoresCheck->setChecked(model->show_cores);
    if (m_showShellsCheck)
      m_showShellsCheck->setChecked(model->show_shells);
    if (m_showLinksCheck)
      m_showLinksCheck->setChecked(model->show_links);
    if (m_showAtomIndexCheck)
      m_showAtomIndexCheck->setChecked(model->show_atom_index);
    if (m_showAtomLabelsCheck)
      m_showAtomLabelsCheck->setChecked(model->show_atom_labels);
    if (m_showAtomTypesCheck)
      m_showAtomTypesCheck->setChecked(model->show_atom_types);
    if (m_showCoreChargesCheck)
      m_showCoreChargesCheck->setChecked(model->show_core_charges);
    if (m_showShellChargesCheck)
      m_showShellChargesCheck->setChecked(model->show_shell_charges);
    if (m_showAtomChargesCheck)
      m_showAtomChargesCheck->setChecked(model->show_atom_charges);
    if (m_showNmrShiftsCheck)
      m_showNmrShiftsCheck->setChecked(model->show_nmr_shifts);
    if (m_showNmrCsaCheck)
      m_showNmrCsaCheck->setChecked(model->show_nmr_csa);
    if (m_showNmrEfgCheck)
      m_showNmrEfgCheck->setChecked(model->show_nmr_efg);
    if (m_showHydrogenCheck)
      m_showHydrogenCheck->setChecked(model->build_hydrogen);
    if (m_showZeoliteCheck)
      m_showZeoliteCheck->setChecked(model->build_zeolite);
    if (m_showSelectionLabelsCheck)
      m_showSelectionLabelsCheck->setChecked(model->show_selection_labels);
    if (m_showAxesCheck)
      m_showAxesCheck->setChecked(model->show_axes);
    if (m_showEnergyCheck)
      m_showEnergyCheck->setChecked(sysenv.render.show_energy);
    if (m_showSpatialTextCheck)
      m_showSpatialTextCheck->setChecked(model->morph_label);
    if (m_showWaypointsCheck)
      m_showWaypointsCheck->setChecked(model->show_waypoints);
    if (m_showRegion1ACheck)
      m_showRegion1ACheck->setChecked(model->show_region1A);
    if (m_showRegion2ACheck)
      m_showRegion2ACheck->setChecked(model->show_region2A);
    if (m_showRegion1BCheck)
      m_showRegion1BCheck->setChecked(model->show_region1B);
    if (m_showRegion2BCheck)
      m_showRegion2BCheck->setChecked(model->show_region2B);
  }

  updateLightList();
  populateCameraList();
}

void RenderDialog::setupMainPage()
{
  m_mainPage = new QWidget();
  auto *page = m_mainPage;
  page->setObjectName("mainPage");

  /* Two-column layout for compact display */
  auto *mainLayout = new QVBoxLayout(page);
  auto *twoColLayout = new QHBoxLayout();
  auto *leftCol = new QVBoxLayout();
  auto *rightCol = new QVBoxLayout();

  /* === LEFT COLUMN === */

  /* Render mode */
  auto *modeGroup = new QGroupBox("Render Mode");
  auto *modeLayout = new QVBoxLayout(modeGroup);
  m_renderModeCombo = new QComboBox();
  m_renderModeCombo->addItems({"Ball & Stick", "CPK", "Liquorice", "Stick"});
  modeLayout->addWidget(m_renderModeCombo);
  leftCol->addWidget(modeGroup);

  /* Toggles */
  auto *toggleGroup = new QGroupBox("Rendering Options");
  auto *toggleLayout = new QVBoxLayout(toggleGroup);
  m_antialiasCheck = new QCheckBox("Antialias");
  m_cpkScaleCheck = new QCheckBox("CPK scaling");
  m_wireSurfaceCheck = new QCheckBox("Wire frame surfaces");
  m_showHiddenCheck = new QCheckBox("Show hidden surfaces");
  toggleLayout->addWidget(m_antialiasCheck);
  toggleLayout->addWidget(m_cpkScaleCheck);
  toggleLayout->addWidget(m_wireSurfaceCheck);
  toggleLayout->addWidget(m_showHiddenCheck);
  toggleLayout->addStretch();
  toggleGroup->setMinimumHeight(100);
  leftCol->addWidget(toggleGroup);

  /* Radii — using consistent |label ... spinner| rows */
  auto *radiiGroup = new QGroupBox("Radii");
  auto *radiiLayout = new QVBoxLayout(radiiGroup);
  m_ballRadiusSpin = new QDoubleSpinBox();
  m_ballRadiusSpin->setRange(0.1, 0.5);
  m_ballRadiusSpin->setSingleStep(0.02);
  add_label_spinner(radiiLayout, "Ball radius:", m_ballRadiusSpin);

  m_stickRadiusSpin = new QDoubleSpinBox();
  m_stickRadiusSpin->setRange(0.02, 0.5);
  m_stickRadiusSpin->setSingleStep(0.01);
  add_label_spinner(radiiLayout, "Cylinder radius:", m_stickRadiusSpin);

  m_stickThicknessSpin = new QDoubleSpinBox();
  m_stickThicknessSpin->setRange(0.1, 9.0);
  m_stickThicknessSpin->setSingleStep(0.05);
  add_label_spinner(radiiLayout, "Stick thickness:", m_stickThicknessSpin);

  m_frameThicknessSpin = new QDoubleSpinBox();
  m_frameThicknessSpin->setRange(0.1, 9.0);
  m_frameThicknessSpin->setSingleStep(0.05);
  add_label_spinner(radiiLayout, "Line width:", m_frameThicknessSpin);

  m_cpkScaleSpin = new QDoubleSpinBox();
  m_cpkScaleSpin->setRange(0.1, 3.0);
  m_cpkScaleSpin->setSingleStep(0.02);
  add_label_spinner(radiiLayout, "CPK scaling:", m_cpkScaleSpin);

  radiiGroup->setMinimumHeight(160);
  leftCol->addWidget(radiiGroup);

  /* Highlighting — moved to right column */

  /* === RIGHT COLUMN === */

  /* Ribbon */
  auto *ribbonGroup = new QGroupBox("Ribbon");
  auto *ribbonLayout = new QVBoxLayout(ribbonGroup);
  m_ribbonCurvatureSpin = new QDoubleSpinBox();
  m_ribbonCurvatureSpin->setRange(0.0, 1.0);
  m_ribbonCurvatureSpin->setSingleStep(0.1);
  add_label_spinner(ribbonLayout, "Curvature:", m_ribbonCurvatureSpin);

  m_ribbonThicknessSpin = new QDoubleSpinBox();
  m_ribbonThicknessSpin->setRange(0.4, 6.0);
  m_ribbonThicknessSpin->setSingleStep(0.2);
  add_label_spinner(ribbonLayout, "Thickness:", m_ribbonThicknessSpin);

  m_ribbonQualitySpin = new QSpinBox();
  m_ribbonQualitySpin->setRange(1, 30);
  m_ribbonQualitySpin->setSingleStep(1);
  add_label_spinner(ribbonLayout, "Quality:", m_ribbonQualitySpin);

  rightCol->addWidget(ribbonGroup);

  /* Ghost/Surface opacity */
  auto *opacityGroup = new QGroupBox("Opacity");
  auto *opacityLayout = new QVBoxLayout(opacityGroup);
  m_ghostOpacitySpin = new QDoubleSpinBox();
  m_ghostOpacitySpin->setRange(0.0, 1.0);
  m_ghostOpacitySpin->setSingleStep(0.1);
  add_label_spinner(opacityLayout, "Ghost atom:", m_ghostOpacitySpin);

  m_surfaceOpacitySpin = new QDoubleSpinBox();
  m_surfaceOpacitySpin->setRange(0.0, 1.0);
  m_surfaceOpacitySpin->setSingleStep(0.1);
  add_label_spinner(opacityLayout, "Surface:", m_surfaceOpacitySpin);

  rightCol->addWidget(opacityGroup);

  /* Highlighting — moved here from left column */
  auto *highlightGroup = new QGroupBox("Highlighting");
  auto *highlightLayout = new QVBoxLayout(highlightGroup);
  m_ahlStrengthSpin = new QDoubleSpinBox();
  m_ahlStrengthSpin->setRange(0.0, 1.0);
  m_ahlStrengthSpin->setSingleStep(0.05);
  add_label_spinner(highlightLayout, "Atom power:", m_ahlStrengthSpin);

  m_ahlSizeSpin = new QSpinBox();
  m_ahlSizeSpin->setRange(0, 128);
  m_ahlSizeSpin->setSingleStep(5);
  add_label_spinner(highlightLayout, "Atom focus:", m_ahlSizeSpin);

  m_shlStrengthSpin = new QDoubleSpinBox();
  m_shlStrengthSpin->setRange(0.0, 1.0);
  m_shlStrengthSpin->setSingleStep(0.05);
  add_label_spinner(highlightLayout, "Surface power:", m_shlStrengthSpin);

  m_shlSizeSpin = new QSpinBox();
  m_shlSizeSpin->setRange(0, 128);
  m_shlSizeSpin->setSingleStep(5);
  add_label_spinner(highlightLayout, "Surface focus:", m_shlSizeSpin);

  highlightGroup->setMinimumHeight(120);
  rightCol->addWidget(highlightGroup);

  /* Zone size */
  auto *zoneGroup = new QGroupBox("Zone Grid");
  auto *zoneLayout = new QVBoxLayout(zoneGroup);
  m_zoneSizeSpin = new QSpinBox();
  m_zoneSizeSpin->setRange(1, 1000);
  m_zoneSizeSpin->setSingleStep(1);
  add_label_spinner(zoneLayout, "Grid size:", m_zoneSizeSpin);
  rightCol->addWidget(zoneGroup);

  /* Add columns to two-col layout */
  twoColLayout->addLayout(leftCol, 1);
  twoColLayout->addLayout(rightCol, 1);

  mainLayout->addLayout(twoColLayout);
  mainLayout->addStretch();

  /* Connect signals */
  connect(m_renderModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &RenderDialog::on_render_mode_changed);
  connect(m_antialiasCheck, &QCheckBox::toggled, this, [this](bool v) {
    sysenv.render.antialias = v;
    triggerRedraw();
  });
  connect(m_cpkScaleCheck, &QCheckBox::toggled, this, [this](bool v) {
    sysenv.render.scale_ball_size = v;
    triggerRedraw();
  });
  connect(m_wireSurfaceCheck, &QCheckBox::toggled, this, [this](bool v) {
    sysenv.render.wire_surface = v;
    triggerRedraw();
  });
  connect(m_showHiddenCheck, &QCheckBox::toggled, this, [this](bool v) {
    sysenv.render.wire_show_hidden = v;
    triggerRedraw();
  });
  connect(m_ballRadiusSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &RenderDialog::on_ball_radius_changed);
  connect(m_stickRadiusSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &RenderDialog::on_stick_radius_changed);
  connect(m_stickThicknessSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &RenderDialog::on_stick_thickness_changed);
  connect(m_frameThicknessSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &RenderDialog::on_frame_thickness_changed);
  connect(m_cpkScaleSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &RenderDialog::on_cpk_scale_changed);
  connect(m_ahlStrengthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &RenderDialog::on_ahl_strength_changed);
  connect(m_ahlSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &RenderDialog::on_ahl_size_changed);
  connect(m_shlStrengthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &RenderDialog::on_shl_strength_changed);
  connect(m_shlSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &RenderDialog::on_shl_size_changed);
  connect(m_ribbonCurvatureSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &RenderDialog::on_ribbon_curvature_changed);
  connect(m_ribbonThicknessSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &RenderDialog::on_ribbon_thickness_changed);
  connect(m_ribbonQualitySpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &RenderDialog::on_ribbon_quality_changed);
  connect(m_ghostOpacitySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &RenderDialog::on_ghost_opacity_changed);
  connect(m_surfaceOpacitySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &RenderDialog::on_surface_opacity_changed);
  connect(m_zoneSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &RenderDialog::on_zone_size_changed);
}

void RenderDialog::setupColoursPage()
{
  m_coloursPage = new QWidget();
  auto *page = m_coloursPage;
  page->setObjectName("coloursPage");
  auto *layout = new QVBoxLayout(page);

  auto *colourGroup = new QGroupBox("Colours");
  auto *colourLayout = new QVBoxLayout(colourGroup);

  auto *bgLayout = new QHBoxLayout();
  bgLayout->addWidget(new QLabel("Background:"));
  m_bgColourBtn = new QPushButton("Background");
  m_bgColourBtn->setMaximumWidth(150);
  bgLayout->addWidget(m_bgColourBtn);
  colourLayout->addLayout(bgLayout);

  auto *rsurfLayout = new QHBoxLayout();
  rsurfLayout->addWidget(new QLabel("Re-entrant:"));
  m_rsurfColourBtn = new QPushButton("Re-entrant");
  m_rsurfColourBtn->setMaximumWidth(150);
  rsurfLayout->addWidget(m_rsurfColourBtn);
  colourLayout->addLayout(rsurfLayout);

  auto *ribbonLayout = new QHBoxLayout();
  ribbonLayout->addWidget(new QLabel("Ribbon:"));
  m_ribbonColourBtn = new QPushButton("Ribbon");
  m_ribbonColourBtn->setMaximumWidth(150);
  ribbonLayout->addWidget(m_ribbonColourBtn);
  colourLayout->addLayout(ribbonLayout);

  layout->addWidget(colourGroup);

  auto *schemeGroup = new QGroupBox("Colouring Scheme");
  auto *schemeLayout = new QHBoxLayout(schemeGroup);
  schemeLayout->addWidget(new QLabel("Scheme:"));
  m_colouringSchemeCombo = new QComboBox();
  m_colouringSchemeCombo->addItems(
      {"element", "growth slice", "molecule", "occupancy", "region", "temperature", "translation"});
  schemeLayout->addWidget(m_colouringSchemeCombo);
  auto *applySchemeBtn = new QPushButton("Apply");
  schemeLayout->addWidget(applySchemeBtn);
  layout->addWidget(schemeGroup);

  connect(m_bgColourBtn, &QPushButton::clicked, this, &RenderDialog::on_bg_colour_clicked);
  connect(m_rsurfColourBtn, &QPushButton::clicked, this, &RenderDialog::on_rsurf_colour_clicked);
  connect(m_ribbonColourBtn, &QPushButton::clicked, this, &RenderDialog::on_ribbon_colour_clicked);
  connect(applySchemeBtn, &QPushButton::clicked, this, [this]() {
    struct model_pak *model = (struct model_pak *) sysenv.active_model;
    if (!model)
      return;
    int idx = m_colouringSchemeCombo->currentIndex();
    if (idx >= 0 && idx < 7)
      model_colour_scheme(colouring_scheme_values[idx], model);
    extern void qt_force_canvas_refresh(void);
    qt_force_canvas_refresh();
  });
  connect(m_colouringSchemeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &RenderDialog::on_colouring_scheme_changed);

  layout->addStretch();
}

void RenderDialog::setupCameraPage()
{
  m_cameraPage = new QWidget();
  auto *page = m_cameraPage;
  page->setObjectName("cameraPage");
  auto *mainLayout = new QVBoxLayout(page);

  /* Two-column split */
  auto *twoColLayout = new QHBoxLayout();
  auto *leftCol = new QVBoxLayout();
  auto *rightCol = new QVBoxLayout();

  /* === LEFT COLUMN: Camera info + animations === */

  /* Camera info */
  auto *camGroup = new QGroupBox("Camera Settings");
  auto *camLayout = new QVBoxLayout(camGroup);

  m_cameraProjCombo = new QComboBox();
  m_cameraProjCombo->addItems({"perspective", "orthographic"});
  add_label_spinner(camLayout, "Projection:", m_cameraProjCombo);

  m_cameraModeCombo = new QComboBox();
  m_cameraModeCombo->addItems({"variable", "origin"});
  add_label_spinner(camLayout, "Focal point:", m_cameraModeCombo);

  m_cameraFOVSpin = new QSpinBox();
  m_cameraFOVSpin->setRange(1, 180);
  m_cameraFOVSpin->setValue(70);
  add_label_spinner(camLayout, "Field of view:", m_cameraFOVSpin);

  m_cameraZoomEdit = new QLineEdit();
  add_label_edit(camLayout, "Zoom factor:", m_cameraZoomEdit);

  leftCol->addWidget(camGroup);

  /* Waypoints */
  auto *wpGroup = new QGroupBox("Waypoints");
  auto *wpLayout = new QVBoxLayout(wpGroup);
  auto *wpBtnLayout = new QHBoxLayout();
  auto *wpAddBtn = new QPushButton("Add waypoint");
  auto *wpDelBtn = new QPushButton("Delete waypoint");
  wpBtnLayout->addWidget(wpAddBtn);
  wpBtnLayout->addWidget(wpDelBtn);
  wpLayout->addLayout(wpBtnLayout);

  auto *wpAnimateBtn = new QPushButton("Create animation from waypoints");
  wpLayout->addWidget(wpAnimateBtn);

  m_cameraWaypointFramesSpin = new QSpinBox();
  m_cameraWaypointFramesSpin->setRange(1, 1000);
  add_label_spinner(wpLayout, "Frames per traversal:", m_cameraWaypointFramesSpin);

  leftCol->addWidget(wpGroup);

  /* Animated rotation */
  auto *rotGroup = new QGroupBox("Animated Rotation");
  auto *rotLayout = new QVBoxLayout(rotGroup);

  m_cameraAxisCombo = new QComboBox();
  m_cameraAxisCombo->addItems({"x axis", "y axis", "z axis"});
  add_label_spinner(rotLayout, "Rotation vector:", m_cameraAxisCombo);

  m_cameraRotStartSpin = new QSpinBox();
  m_cameraRotStartSpin->setRange(0, 359);
  add_label_spinner(rotLayout, "Start:", m_cameraRotStartSpin);

  m_cameraRotStopSpin = new QSpinBox();
  m_cameraRotStopSpin->setRange(1, 360);
  add_label_spinner(rotLayout, "Stop:", m_cameraRotStopSpin);

  m_cameraRotIncSpin = new QSpinBox();
  m_cameraRotIncSpin->setRange(1, 10);
  add_label_spinner(rotLayout, "Increment:", m_cameraRotIncSpin);

  auto *rotAnimateBtn = new QPushButton("Create animated rotation");
  rotLayout->addWidget(rotAnimateBtn);

  m_cameraOverwriteCheck = new QCheckBox("Overwrite old frames");
  m_cameraOverwriteCheck->setChecked(render_animate_overwrite);
  connect(m_cameraOverwriteCheck, &QCheckBox::toggled, this, [](bool v) { render_animate_overwrite = v; });
  rotLayout->addWidget(m_cameraOverwriteCheck);

  leftCol->addWidget(rotGroup);

  /* === RIGHT COLUMN: Camera list === */
  auto *listGroup = new QGroupBox("Cameras");
  auto *listLayout = new QVBoxLayout(listGroup);
  m_cameraList = new QListWidget();
  listLayout->addWidget(m_cameraList);
  rightCol->addWidget(listGroup);

  twoColLayout->addLayout(leftCol, 1);
  twoColLayout->addLayout(rightCol, 1);
  mainLayout->addLayout(twoColLayout);

  /* Connections */
  connect(m_cameraProjCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &RenderDialog::on_camera_projection_changed);
  connect(m_cameraModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
    struct model_pak *model = (struct model_pak *) sysenv.active_model;
    if (!model || !model->camera)
      return;
    struct camera_pak *cam = (struct camera_pak *) model->camera;
    cam->mode = (idx == 0) ? FREE : LOCKED;
    extern void qt_force_canvas_refresh(void);
    qt_force_canvas_refresh();
  });
  connect(m_cameraFOVSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) {
    struct model_pak *model = (struct model_pak *) sysenv.active_model;
    if (!model || !model->camera)
      return;
    struct camera_pak *cam = (struct camera_pak *) model->camera;
    cam->fov = v;
    extern void qt_force_canvas_refresh(void);
    qt_force_canvas_refresh();
  });
  connect(m_cameraZoomEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
    struct model_pak *model = (struct model_pak *) sysenv.active_model;
    if (!model || !model->camera)
      return;
    struct camera_pak *cam = (struct camera_pak *) model->camera;
    bool ok;
    double z = text.toDouble(&ok);
    if (ok && model->rmax > 0)
    {
      cam->zoom = 1000.0 / (model->rmax * z);
      extern void qt_force_canvas_refresh(void);
      qt_force_canvas_refresh();
    }
  });
  connect(wpAddBtn, &QPushButton::clicked, this, &RenderDialog::on_camera_create_waypoint);
  connect(wpDelBtn, &QPushButton::clicked, this, [this]() {
    int row = m_cameraList->currentRow();
    if (row < 0)
      return;
    struct model_pak *model = (struct model_pak *) sysenv.active_model;
    if (!model)
      return;

    /* Row 0 is default, can't delete */
    if (row == 0)
    {
      extern void gui_text_show(gint, const gchar *);
      gui_text_show(GDIS_ERROR, g_strdup("You cannot delete the default camera."));
      return;
    }

    /* Get camera from the list widget */
    QListWidgetItem *item = m_cameraList->item(row);
    if (!item)
      return;
    struct camera_pak *cam = (struct camera_pak *) (void *) (quintptr) item->data(Qt::UserRole).toULongLong();
    if (!cam)
      return;

    if (cam == model->camera_default)
    {
      extern void gui_text_show(gint, const gchar *);
      gui_text_show(GDIS_ERROR, g_strdup("You cannot delete the default camera."));
      return;
    }

    if (cam == model->camera)
      model->camera = model->camera_default;
    model->waypoint_list = g_slist_remove(model->waypoint_list, cam);

    populateCameraList();
    extern void qt_force_canvas_refresh(void);
    qt_force_canvas_refresh();
  });
  connect(wpAnimateBtn, &QPushButton::clicked, this, [this]() {
    struct model_pak *model = (struct model_pak *) sysenv.active_model;
    if (!model)
      return;
    int frames = m_cameraWaypointFramesSpin->value();
    int overwrite = m_cameraOverwriteCheck->isChecked() ? 1 : 0;
    extern void camera_waypoint_animate(gint, gint, struct model_pak *);
    camera_waypoint_animate(frames, overwrite, model);
  });
  connect(rotAnimateBtn, &QPushButton::clicked, this, [this]() {
    struct model_pak *model = (struct model_pak *) sysenv.active_model;
    if (!model)
      return;
    /* Axis mapping: x=0, y=1, z=2 */
    int axis = m_cameraAxisCombo->currentIndex();
    gdouble angle[3] = {m_cameraRotStartSpin->value(), m_cameraRotStopSpin->value(), m_cameraRotIncSpin->value()};
    int overwrite = m_cameraOverwriteCheck->isChecked() ? 1 : 0;
    extern void camera_rotate_animate(gint, gdouble *, gint, struct model_pak *);
    camera_rotate_animate(axis, angle, overwrite, model);
  });

  mainLayout->addStretch();
}

void RenderDialog::setupLightsPage()
{
  m_lightsPage = new QWidget();
  auto *page = m_lightsPage;
  page->setObjectName("lightsPage");
  auto *layout = new QHBoxLayout(page);

  /* Left panel - light editing */
  auto *editGroup = new QGroupBox("Current Light");
  auto *editLayout = new QVBoxLayout(editGroup);

  /* Colour */
  auto *colourRow = new QHBoxLayout();
  colourRow->addWidget(new QLabel("Colour:"));
  auto *lightColourBtn = new QPushButton("Light Colour");
  lightColourBtn->setMaximumWidth(150);
  colourRow->addWidget(lightColourBtn);
  editLayout->addLayout(colourRow);

  /* Ambient / Diffuse / Specular */
  m_ambientSpin = new QDoubleSpinBox();
  m_ambientSpin->setRange(0.0, 1.0);
  m_ambientSpin->setSingleStep(0.1);
  add_label_spinner(editLayout, "Ambient:", m_ambientSpin);

  m_diffuseSpin = new QDoubleSpinBox();
  m_diffuseSpin->setRange(0.0, 1.0);
  m_diffuseSpin->setSingleStep(0.1);
  add_label_spinner(editLayout, "Diffuse:", m_diffuseSpin);

  m_specularSpin = new QDoubleSpinBox();
  m_specularSpin->setRange(0.0, 1.0);
  m_specularSpin->setSingleStep(0.1);
  add_label_spinner(editLayout, "Specular:", m_specularSpin);

  /* Type — no indentation, on its own row */
  auto *typeRow = new QHBoxLayout();
  typeRow->addWidget(new QLabel("Type:"));
  m_lightTypeCombo = new QComboBox();
  m_lightTypeCombo->addItems({"Directional", "Positional"});
  typeRow->addWidget(m_lightTypeCombo);
  editLayout->addLayout(typeRow);

  /* X / Y / Z on same line */
  auto *xyzLayout = new QHBoxLayout();
  auto *xLayout = new QHBoxLayout();
  xLayout->addWidget(new QLabel("X:"));
  m_lightXSpin = new QDoubleSpinBox();
  m_lightXSpin->setRange(-1000.0, 1000.0);
  m_lightXSpin->setSingleStep(0.1);
  xLayout->addWidget(m_lightXSpin);
  xyzLayout->addLayout(xLayout);

  auto *yLayout = new QHBoxLayout();
  yLayout->addWidget(new QLabel("Y:"));
  m_lightYSpin = new QDoubleSpinBox();
  m_lightYSpin->setRange(-1000.0, 1000.0);
  m_lightYSpin->setSingleStep(0.1);
  yLayout->addWidget(m_lightYSpin);
  xyzLayout->addLayout(yLayout);

  auto *zLayout = new QHBoxLayout();
  zLayout->addWidget(new QLabel("Z:"));
  m_lightZSpin = new QDoubleSpinBox();
  m_lightZSpin->setRange(-1000.0, 1000.0);
  m_lightZSpin->setSingleStep(0.1);
  zLayout->addWidget(m_lightZSpin);
  xyzLayout->addLayout(zLayout);
  editLayout->addLayout(xyzLayout);

  /* Action buttons */
  auto *actionLayout = new QHBoxLayout();
  auto *addBtn = new QPushButton("Add");
  auto *modBtn = new QPushButton("Modify");
  auto *delBtn = new QPushButton("Delete");
  actionLayout->addWidget(addBtn);
  actionLayout->addWidget(modBtn);
  actionLayout->addWidget(delBtn);
  editLayout->addLayout(actionLayout);

  connect(lightColourBtn, &QPushButton::clicked, this, [this, lightColourBtn]() {
    QColor c = QColorDialog::getColor(m_currentLight.colour, this);
    if (c.isValid())
    {
      m_currentLight.colour = c;
      updateButtonColour(lightColourBtn, c);
    }
  });
  connect(addBtn, &QPushButton::clicked, this, &RenderDialog::on_light_add);
  connect(modBtn, &QPushButton::clicked, this, &RenderDialog::on_light_modify);
  connect(delBtn, &QPushButton::clicked, this, &RenderDialog::on_light_remove);

  layout->addWidget(editGroup);

  /* Right panel - light list */
  auto *listGroup = new QGroupBox("Light Sources");
  auto *listLayout = new QVBoxLayout(listGroup);
  m_lightList = new QListWidget();
  m_lightList->setMinimumWidth(250);
  listLayout->addWidget(m_lightList);

  connect(m_lightList, &QListWidget::currentRowChanged, this, [this](int row) {
    if (row >= 0)
      saveLightToWidget(row);
  });

  layout->addWidget(listGroup);
}

void RenderDialog::setupOpenGLPage()
{
  m_openGLPage = new QWidget();
  auto *page = m_openGLPage;
  page->setObjectName("openGLPage");
  auto *mainLayout = new QVBoxLayout(page);

  /* Two-column split */
  auto *twoColLayout = new QHBoxLayout();
  auto *leftCol = new QVBoxLayout();
  auto *rightCol = new QVBoxLayout();

  /* === LEFT: Quality + Performance + Fog === */

  auto *qualityGroup = new QGroupBox("Quality");
  auto *qualityLayout = new QVBoxLayout(qualityGroup);
  m_sphereQualitySpin = new QSpinBox();
  m_sphereQualitySpin->setRange(0, 8);
  add_label_spinner(qualityLayout, "Sphere quality:", m_sphereQualitySpin);
  m_cylinderQualitySpin = new QSpinBox();
  m_cylinderQualitySpin->setRange(4, 20);
  add_label_spinner(qualityLayout, "Cylinder quality:", m_cylinderQualitySpin);
  leftCol->addWidget(qualityGroup);

  auto *perfGroup = new QGroupBox("Performance");
  auto *perfLayout = new QVBoxLayout(perfGroup);
  m_autoQualityCheck = new QCheckBox("Automatic quality adjustment");
  m_fastRotationCheck = new QCheckBox("Fast rotation");
  m_halosCheck = new QCheckBox("Selection halos");
  perfLayout->addWidget(m_autoQualityCheck);
  perfLayout->addWidget(m_fastRotationCheck);
  perfLayout->addWidget(m_halosCheck);
  leftCol->addWidget(perfGroup);

  auto *fogGroup = new QGroupBox("Fog / Depth");
  auto *fogLayout = new QVBoxLayout(fogGroup);
  m_fogCheck = new QCheckBox("Depth queueing (fog)");
  fogLayout->addWidget(m_fogCheck);
  m_fogDensitySpin = new QDoubleSpinBox();
  m_fogDensitySpin->setRange(0.0, 1.0);
  m_fogDensitySpin->setSingleStep(0.1);
  add_label_spinner(fogLayout, "Strength:", m_fogDensitySpin);
  leftCol->addWidget(fogGroup);

  /* === RIGHT: Geometry + buttons === */

  auto *geomGroup = new QGroupBox("Geometry");
  auto *geomLayout = new QVBoxLayout(geomGroup);
  auto *geomBtnRow = new QHBoxLayout();
  auto *geomLabelBtn = new QPushButton("Label geometric measurements");
  geomBtnRow->addWidget(geomLabelBtn);
  geomLayout->addLayout(geomBtnRow);

  m_geomLineWidthSpin = new QDoubleSpinBox();
  m_geomLineWidthSpin->setRange(0.5, 9.0);
  m_geomLineWidthSpin->setSingleStep(0.5);
  add_label_spinner(geomLayout, "Line width:", m_geomLineWidthSpin);
  rightCol->addWidget(geomGroup);

  /* Change font button */
  auto *fontBtn = new QPushButton("Change graphics font");
  rightCol->addWidget(fontBtn);

  twoColLayout->addLayout(leftCol, 1);
  twoColLayout->addLayout(rightCol, 1);
  mainLayout->addLayout(twoColLayout);

  connect(m_sphereQualitySpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &RenderDialog::on_sphere_quality_changed);
  connect(m_cylinderQualitySpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &RenderDialog::on_cylinder_quality_changed);
  connect(m_autoQualityCheck, &QCheckBox::toggled, this, [](bool v) { sysenv.render.auto_quality = v; });
  connect(m_fastRotationCheck, &QCheckBox::toggled, this, [](bool v) { sysenv.render.fast_rotation = v; });
  connect(m_halosCheck, &QCheckBox::toggled, this, [this](bool v) {
    sysenv.render.halos = v;
    triggerRedraw();
  });
  connect(m_fogCheck, &QCheckBox::toggled, this, [this](bool v) {
    sysenv.render.fog = v;
    triggerRedraw();
  });
  connect(m_fogDensitySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &RenderDialog::on_fog_density_changed);
  connect(m_geomLineWidthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double v) {
    sysenv.render.geom_line_width = v;
    triggerRedraw();
  });
  connect(geomLabelBtn, &QPushButton::clicked, this, []() {
    geom_label_toggle();
    extern void qt_force_canvas_refresh(void);
    qt_force_canvas_refresh();
  });
  connect(fontBtn, &QPushButton::clicked, this, [this]() {
    bool ok;
    QFont currentFont(gl_get_fontname(), gl_fontsize);
    QFont f = QFontDialog::getFont(&ok, currentFont, this);
    if (ok)
    {
      gl_fontsize = f.pointSize();
      strncpy(sysenv.gl_fontname, f.family().toUtf8().constData(), LINELEN - 1);
      sysenv.gl_fontname[LINELEN - 1] = '\0';
      /* Update the actual Qt font used for rendering */
      gl_text_init_font(f.family().toUtf8().constData());
      extern void qt_force_canvas_refresh(void);
      qt_force_canvas_refresh();
    }
  });

  mainLayout->addStretch();
}

void RenderDialog::setupPOVRayPage()
{
  m_povrayPage = new QWidget();
  auto *page = m_povrayPage;
  page->setObjectName("povrayPage");
  auto *layout = new QVBoxLayout(page);

  auto *sizeGroup = new QGroupBox("Image Size");
  auto *sizeLayout = new QVBoxLayout(sizeGroup);
  m_povrayWidthSpin = new QSpinBox();
  m_povrayWidthSpin->setRange(100, 2000);
  m_povrayWidthSpin->setSingleStep(100);
  add_label_spinner(sizeLayout, "Width:", m_povrayWidthSpin);
  m_povrayHeightSpin = new QSpinBox();
  m_povrayHeightSpin->setRange(100, 2000);
  m_povrayHeightSpin->setSingleStep(100);
  add_label_spinner(sizeLayout, "Height:", m_povrayHeightSpin);
  layout->addWidget(sizeGroup);

  auto *optionsGroup = new QGroupBox("Options");
  auto *optionsLayout = new QVBoxLayout(optionsGroup);
  m_shadowlessCheck = new QCheckBox("Shadowless");
  m_noPovrayExecCheck = new QCheckBox("Create povray input files then stop");
  m_noKeepTempfilesCheck = new QCheckBox("Delete intermediate input/image files");
  optionsLayout->addWidget(m_shadowlessCheck);
  optionsLayout->addWidget(m_noPovrayExecCheck);
  optionsLayout->addWidget(m_noKeepTempfilesCheck);
  layout->addWidget(optionsGroup);

  m_renderBtn = new QPushButton("    Render    ");
  m_renderBtn->setMinimumWidth(200);
  layout->addWidget(m_renderBtn, 0, Qt::AlignHCenter);
  connect(m_renderBtn, &QPushButton::clicked, this, &RenderDialog::on_render_setup);

  connect(m_povrayWidthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &RenderDialog::on_povray_width_changed);
  connect(m_povrayHeightSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &RenderDialog::on_povray_height_changed);
  connect(m_shadowlessCheck, &QCheckBox::toggled, this, [](bool v) { sysenv.render.shadowless = v; });
  connect(m_noPovrayExecCheck, &QCheckBox::toggled, this, [](bool v) { sysenv.render.no_povray_exec = v; });
  connect(m_noKeepTempfilesCheck, &QCheckBox::toggled, this, [](bool v) { sysenv.render.no_keep_tempfiles = v; });

  layout->addStretch();
}

/* ===== Helper functions ===== */

void RenderDialog::updateLightList()
{
  m_lightList->clear();
  gpointer list_ptr = sysenv.render.light_list;
  for (int i = 0; list_ptr && i < 100; i++, list_ptr = g_slist_next((GSList *) list_ptr))
  {
    struct light_pak *light = (struct light_pak *) g_slist_nth_data((GSList *) sysenv.render.light_list, i);
    QString text = QString("%1 (%2, %3, %4)")
                       .arg(light->type == DIRECTIONAL ? "Dir" : "Pos")
                       .arg(light->x[0], 0, 'f', 1)
                       .arg(light->x[1], 0, 'f', 1)
                       .arg(light->x[2], 0, 'f', 1);
    m_lightList->addItem(text);
  }
}

void RenderDialog::loadLightFromWidget()
{
  m_currentLight.ambient = m_ambientSpin->value();
  m_currentLight.diffuse = m_diffuseSpin->value();
  m_currentLight.specular = m_specularSpin->value();
  m_currentLight.x[0] = m_lightXSpin->value();
  m_currentLight.x[1] = m_lightYSpin->value();
  m_currentLight.x[2] = m_lightZSpin->value();
  /* Store combo box index directly (0=Directional, 1=Positional) to match
   * the type field initialization in the header. Avoids enum value mismatch */
  m_currentLight.type = m_lightTypeCombo->currentIndex();
}

void RenderDialog::saveLightToWidget(int row)
{
  struct light_pak *light = (struct light_pak *) g_slist_nth_data((GSList *) sysenv.render.light_list, row);
  if (!light)
    return;
  m_ambientSpin->setValue(light->ambient);
  m_diffuseSpin->setValue(light->diffuse);
  m_specularSpin->setValue(light->specular);
  m_lightXSpin->setValue(light->x[0]);
  m_lightYSpin->setValue(light->x[1]);
  m_lightZSpin->setValue(light->x[2]);
  /* Convert enum value back to combo box index (7=DIRECTIONAL→0, 8=POSITIONAL→1) */
  m_lightTypeCombo->setCurrentIndex(light->type == DIRECTIONAL ? 0 : 1);
}

/* ===== Slot implementations ===== */

void RenderDialog::on_render_mode_changed(int index)
{
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (!model)
    return;

  int mode = render_modes[index];
  if (mode < 0)
    return;

  spatial_destroy_by_label("polyhedra", model);
  spatial_destroy_by_label("zones", model);
  model->default_render_mode = mode;

  if (model->selection)
    core_render_mode_set(mode, model->selection);
  else
    core_render_mode_set(mode, model->cores);

  triggerRedraw();
}

void RenderDialog::on_colouring_scheme_changed(int index)
{
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (!model)
    return;

  if (index >= 0 && index < 7)
    model_colour_scheme(colouring_scheme_values[index], model);

  triggerRedraw();
}

void RenderDialog::on_camera_projection_changed(int index)
{
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (!model || !model->camera)
    return;

  struct camera_pak *camera = (struct camera_pak *) model->camera;
  camera->perspective = (index == 0);

  /* Toggle FOV/Zoom sensitivity */
  if (m_cameraFOVSpin)
  {
    m_cameraFOVSpin->setEnabled(camera->perspective);
    if (camera->perspective)
    {
      camera->fov = 70;
      m_cameraFOVSpin->setValue(70);
    } else
    {
      m_cameraFOVSpin->setValue(180);
    }
  }
  if (m_cameraZoomEdit)
  {
    if (camera->perspective)
    {
      m_cameraZoomEdit->setText("not applicable");
      m_cameraZoomEdit->setEnabled(false);
    } else
    {
      double zoom = 1000.0 / (model->rmax * camera->zoom);
      m_cameraZoomEdit->setText(QString::number(zoom, 'f', 1));
      m_cameraZoomEdit->setEnabled(true);
    }
  }

  extern void qt_force_canvas_refresh(void);
  qt_force_canvas_refresh();
}

void RenderDialog::on_light_add()
{
  loadLightFromWidget();
  struct light_pak *light = (struct light_pak *) g_malloc(sizeof(struct light_pak));
  /* Copy fields individually - m_currentLight has different layout than struct light_pak */
  light->type = (m_currentLight.type == 1) ? POSITIONAL : DIRECTIONAL;
  light->x[0] = m_currentLight.x[0];
  light->x[1] = m_currentLight.x[1];
  light->x[2] = m_currentLight.x[2];
  light->colour[0] = m_currentLight.colour.redF();
  light->colour[1] = m_currentLight.colour.greenF();
  light->colour[2] = m_currentLight.colour.blueF();
  light->ambient = m_currentLight.ambient;
  light->diffuse = m_currentLight.diffuse;
  light->specular = m_currentLight.specular;
  sysenv.render.light_list = g_slist_append(sysenv.render.light_list, light);
  updateLightList();
  triggerRedraw();
}

void RenderDialog::on_light_remove()
{
  int row = m_lightList->currentRow();
  if (row < 0)
    return;
  struct light_pak *light = (struct light_pak *) g_slist_nth_data((GSList *) sysenv.render.light_list, row);
  if (!light)
    return;
  sysenv.render.light_list = g_slist_remove(sysenv.render.light_list, light);
  updateLightList();
  triggerRedraw();
}

void RenderDialog::on_light_modify()
{
  int row = m_lightList->currentRow();
  if (row < 0)
    return;
  loadLightFromWidget();
  struct light_pak *light = (struct light_pak *) g_slist_nth_data((GSList *) sysenv.render.light_list, row);
  if (!light)
    return;
  /* Copy fields individually - m_currentLight has different layout than struct light_pak */
  light->type = (m_currentLight.type == 1) ? POSITIONAL : DIRECTIONAL;
  light->x[0] = m_currentLight.x[0];
  light->x[1] = m_currentLight.x[1];
  light->x[2] = m_currentLight.x[2];
  light->colour[0] = m_currentLight.colour.redF();
  light->colour[1] = m_currentLight.colour.greenF();
  light->colour[2] = m_currentLight.colour.blueF();
  light->ambient = m_currentLight.ambient;
  light->diffuse = m_currentLight.diffuse;
  light->specular = m_currentLight.specular;
  updateLightList();
  triggerRedraw();
}

void RenderDialog::on_bg_colour_clicked()
{
  QColor c = QColorDialog::getColor(m_bgColour, this);
  if (c.isValid())
  {
    m_bgColour = c;
    sysenv.render.bg_colour[0] = c.redF();
    sysenv.render.bg_colour[1] = c.greenF();
    sysenv.render.bg_colour[2] = c.blueF();
    updateButtonColour(m_bgColourBtn, c);
  }
}

void RenderDialog::on_rsurf_colour_clicked()
{
  QColor c = QColorDialog::getColor(m_rsurfColour, this);
  if (c.isValid())
  {
    m_rsurfColour = c;
    sysenv.render.rsurf_colour[0] = c.redF();
    sysenv.render.rsurf_colour[1] = c.greenF();
    sysenv.render.rsurf_colour[2] = c.blueF();
    updateButtonColour(m_rsurfColourBtn, c);
  }
}

void RenderDialog::on_ribbon_colour_clicked()
{
  QColor c = QColorDialog::getColor(m_ribbonColour, this);
  if (c.isValid())
  {
    m_ribbonColour = c;
    sysenv.render.ribbon_colour[0] = c.redF();
    sysenv.render.ribbon_colour[1] = c.greenF();
    sysenv.render.ribbon_colour[2] = c.blueF();
    updateButtonColour(m_ribbonColourBtn, c);
  }
}

void RenderDialog::on_sphere_quality_changed(int v) { sysenv.render.sphere_quality = v; }
void RenderDialog::on_cylinder_quality_changed(int v) { sysenv.render.cylinder_quality = v; }
void RenderDialog::on_fog_density_changed(double v)
{
  sysenv.render.fog_density = v;
  triggerRedraw();
}
void RenderDialog::on_ball_radius_changed(double v)
{
  sysenv.render.ball_radius = v;
  triggerRedraw();
}
void RenderDialog::on_stick_radius_changed(double v)
{
  sysenv.render.stick_radius = v;
  triggerRedraw();
}
void RenderDialog::on_stick_thickness_changed(double v)
{
  sysenv.render.stick_thickness = v;
  triggerRedraw();
}
void RenderDialog::on_frame_thickness_changed(double v)
{
  sysenv.render.frame_thickness = v;
  triggerRedraw();
}
void RenderDialog::on_cpk_scale_changed(double v)
{
  sysenv.render.cpk_scale = v;
  triggerRedraw();
}
void RenderDialog::on_ahl_strength_changed(double v) { sysenv.render.ahl_strength = v; }
void RenderDialog::on_ahl_size_changed(int v) { sysenv.render.ahl_size = v; }
void RenderDialog::on_shl_strength_changed(double v) { sysenv.render.shl_strength = v; }
void RenderDialog::on_shl_size_changed(int v) { sysenv.render.shl_size = v; }
void RenderDialog::on_ribbon_curvature_changed(double v) { sysenv.render.ribbon_curvature = v; }
void RenderDialog::on_ribbon_thickness_changed(double v) { sysenv.render.ribbon_thickness = v; }
void RenderDialog::on_ribbon_quality_changed(int v) { sysenv.render.ribbon_quality = v; }
void RenderDialog::on_ghost_opacity_changed(double v)
{
  sysenv.render.ghost_opacity = v;
  triggerRedraw();
}
void RenderDialog::on_surface_opacity_changed(double v)
{
  sysenv.render.transmit = v;
  triggerRedraw();
}
void RenderDialog::on_zone_size_changed(int v)
{
  sysenv.render.zone_size = v;
  triggerRedraw();
}
void RenderDialog::on_geom_line_width(double v)
{
  sysenv.render.geom_line_width = v;
  triggerRedraw();
}
void RenderDialog::on_povray_width_changed(int v) { sysenv.render.width = v; }
void RenderDialog::on_povray_height_changed(int v) { sysenv.render.height = v; }
void RenderDialog::on_render_setup() { povray_task(); }
void RenderDialog::on_camera_create_waypoint()
{
  gui_create_waypoint();
  refresh();
  populateCameraList();
  extern void qt_force_canvas_refresh(void);
  qt_force_canvas_refresh();
}

void RenderDialog::populateCameraList()
{
  if (!m_cameraList)
    return;
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (!model)
    return;

  m_cameraList->clear();

  /* Default camera */
  struct camera_pak *defCam = (struct camera_pak *) model->camera_default;
  if (defCam)
  {
    m_cameraList->addItem("default");
    m_cameraList->item(0)->setData(Qt::UserRole, QVariant::fromValue((quintptr) defCam));
  }

  /* Waypoints */
  GSList *list = model->waypoint_list;
  int n = 1;
  while (list)
  {
    struct camera_pak *cam = (struct camera_pak *) list->data;
    QString text = QString("waypoint %1").arg(n++);
    m_cameraList->addItem(text);
    m_cameraList->item(n - 1)->setData(Qt::UserRole, QVariant::fromValue((quintptr) cam));
    list = g_slist_next(list);
  }
}

/* Helper: connect checkbox to model field */
void RenderDialog::connectToggle(QCheckBox *check, int *field)
{
  connect(check, &QCheckBox::toggled, this, [this, field](bool v) {
    *field = v;
    triggerRedraw();
  });
}

/* Helper: connect checkbox to render int field */
void RenderDialog::connectRenderToggle(QCheckBox *check, int *field)
{
  connect(check, &QCheckBox::toggled, this, [this, field](bool v) {
    *field = v;
    triggerRedraw();
  });
}

/* Stereo page */
void RenderDialog::setupStereoPage()
{
  m_stereoPage = new QWidget();
  auto *page = m_stereoPage;
  page->setObjectName("stereoPage");
  auto *layout = new QVBoxLayout(page);

  /* Anaglyph toggle */
  auto *anaglyphLayout = new QHBoxLayout();
  m_stereoAnaglyphCheck = new QCheckBox("Red/Cyan");
  m_stereoAnaglyphBlueRedCheck = new QCheckBox("Red/Blue");
  anaglyphLayout->addWidget(m_stereoAnaglyphCheck);
  anaglyphLayout->addWidget(m_stereoAnaglyphBlueRedCheck);
  layout->addLayout(anaglyphLayout);

  auto *modeGroup = new QGroupBox("Stereo Mode");
  auto *modeLayout = new QVBoxLayout(modeGroup);
  m_stereoWindowedBtn = new QPushButton("Windowed stereo", modeGroup);
  m_stereoFullscreenBtn = new QPushButton("Fullscreen stereo", modeGroup);
  m_stereoOffBtn = new QPushButton("Stereo off", modeGroup);
  modeLayout->addWidget(m_stereoWindowedBtn);
  modeLayout->addWidget(m_stereoFullscreenBtn);
  modeLayout->addWidget(m_stereoOffBtn);
  layout->addWidget(modeGroup);

  auto *paramsGroup = new QGroupBox("Stereo Parameters");
  auto *paramsLayout = new QVBoxLayout(paramsGroup);
  m_stereoEyeOffsetSpin = new QDoubleSpinBox();
  m_stereoEyeOffsetSpin->setRange(0.01, 4.0);
  m_stereoEyeOffsetSpin->setSingleStep(0.01);
  add_label_spinner(paramsLayout, "Eye separation:", m_stereoEyeOffsetSpin);
  m_stereoParallaxSpin = new QDoubleSpinBox();
  m_stereoParallaxSpin->setRange(0.01, 4.0);
  m_stereoParallaxSpin->setSingleStep(0.01);
  add_label_spinner(paramsLayout, "Frustum asymmetry:", m_stereoParallaxSpin);
  layout->addWidget(paramsGroup);

  auto *eyeGroup = new QGroupBox("Eye Selection");
  auto *eyeLayout = new QVBoxLayout(eyeGroup);
  m_stereoLeftCheck = new QCheckBox("Left eye active", eyeGroup);
  m_stereoRightCheck = new QCheckBox("Right eye active", eyeGroup);
  eyeLayout->addWidget(m_stereoLeftCheck);
  eyeLayout->addWidget(m_stereoRightCheck);
  layout->addWidget(eyeGroup);

  /* Connections */
  connect(m_stereoWindowedBtn, &QPushButton::clicked, this, &RenderDialog::on_stereo_windowed);
  connect(m_stereoFullscreenBtn, &QPushButton::clicked, this, &RenderDialog::on_stereo_fullscreen);
  connect(m_stereoOffBtn, &QPushButton::clicked, this, &RenderDialog::on_stereo_off);
  connect(m_stereoEyeOffsetSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double v) {
    sysenv.render.stereo_eye_offset = v;
    triggerRedraw();
  });
  connect(m_stereoParallaxSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double v) {
    sysenv.render.stereo_parallax = v;
    triggerRedraw();
  });
  connect(m_stereoLeftCheck, &QCheckBox::toggled, [this](bool v) {
    sysenv.render.stereo_left = v;
    triggerRedraw();
  });
  connect(m_stereoRightCheck, &QCheckBox::toggled, [this](bool v) {
    sysenv.render.stereo_right = v;
    triggerRedraw();
  });
  connect(m_stereoAnaglyphCheck, &QCheckBox::toggled, [this](bool v) {
    sysenv.render.stereo_anaglyph = v;
    triggerRedraw();
  });
  connect(m_stereoAnaglyphBlueRedCheck, &QCheckBox::toggled, [this](bool v) {
    sysenv.render.stereo_anaglyph_blue_red = v;
    triggerRedraw();
  });

  layout->addStretch();
}

void RenderDialog::on_stereo_windowed()
{
  sysenv.stereo = TRUE;
  sysenv.stereo_fullscreen = FALSE;
  sysenv.render.perspective = TRUE;

  /* Call the C-side bridge to create/open stereo window */
  extern void qt_stereo_open_window(void);
  qt_stereo_open_window();

  triggerRedraw();
}

void RenderDialog::on_stereo_fullscreen()
{
  sysenv.stereo = TRUE;
  sysenv.stereo_fullscreen = TRUE;
  sysenv.render.perspective = TRUE;

  /* Call the C-side bridge to create/open stereo window */
  extern void qt_stereo_open_window(void);
  qt_stereo_open_window();

  triggerRedraw();
}
void RenderDialog::on_stereo_off()
{
  sysenv.stereo = FALSE;
  sysenv.render.perspective = FALSE;
  triggerRedraw();
}

/* Toggles page */
void RenderDialog::setupTogglesPage()
{
  m_togglesPage = new QWidget();
  auto *page = m_togglesPage;
  page->setObjectName("togglesPage");
  auto *mainLayout = new QVBoxLayout(page);

  /* Two-column layout */
  auto *twoColLayout = new QHBoxLayout();

  /* === LEFT COLUMN: Display Elements === */
  auto *leftGroup = new QGroupBox("Display Elements");
  auto *leftLayout = new QVBoxLayout(leftGroup);
  leftGroup->setMinimumHeight(320);
  leftGroup->setMaximumHeight(420);

  m_showCellCheck = new QCheckBox("Show cell");
  m_showCellImagesCheck = new QCheckBox("Show cell images");
  m_showCellLengthsCheck = new QCheckBox("Show cell lengths");
  m_showCoresCheck = new QCheckBox("Show cores");
  m_showShellsCheck = new QCheckBox("Show shells");
  m_showLinksCheck = new QCheckBox("Show core-shell links");
  m_showAtomIndexCheck = new QCheckBox("Show core indices");
  m_showAtomLabelsCheck = new QCheckBox("Show core labels");
  m_showAtomTypesCheck = new QCheckBox("Show core types");
  m_showCoreChargesCheck = new QCheckBox("Show core charges");
  m_showShellChargesCheck = new QCheckBox("Show shell charges");
  m_showAtomChargesCheck = new QCheckBox("Show atom charges");
  m_showNmrShiftsCheck = new QCheckBox("Show NMR shielding");
  m_showNmrCsaCheck = new QCheckBox("Show NMR CSA");
  m_showNmrEfgCheck = new QCheckBox("Show NMR EFG");
  m_showHydrogenCheck = new QCheckBox("Show hydrogen bonds");
  m_showZeoliteCheck = new QCheckBox("Show zeolite bonds");
  m_showSelectionLabelsCheck = new QCheckBox("Show labels on selection");

  leftLayout->addWidget(m_showCellCheck);
  leftLayout->addWidget(m_showCellImagesCheck);
  leftLayout->addWidget(m_showCellLengthsCheck);
  leftLayout->addWidget(m_showCoresCheck);
  leftLayout->addWidget(m_showShellsCheck);
  leftLayout->addWidget(m_showLinksCheck);
  leftLayout->addWidget(m_showAtomIndexCheck);
  leftLayout->addWidget(m_showAtomLabelsCheck);
  leftLayout->addWidget(m_showAtomTypesCheck);
  leftLayout->addWidget(m_showCoreChargesCheck);
  leftLayout->addWidget(m_showShellChargesCheck);
  leftLayout->addWidget(m_showAtomChargesCheck);
  leftLayout->addWidget(m_showNmrShiftsCheck);
  leftLayout->addWidget(m_showNmrCsaCheck);
  leftLayout->addWidget(m_showNmrEfgCheck);
  leftLayout->addWidget(m_showHydrogenCheck);
  leftLayout->addWidget(m_showZeoliteCheck);
  leftLayout->addWidget(m_showSelectionLabelsCheck);

  twoColLayout->addWidget(leftGroup);

  /* === RIGHT COLUMN: Display Options + Regions === */
  auto *rightColLayout = new QVBoxLayout();

  auto *rightGroup = new QGroupBox("Display Options");
  auto *rightLayout = new QVBoxLayout(rightGroup);
  rightGroup->setMinimumHeight(120);
  rightGroup->setMaximumHeight(180);

  m_showAxesCheck = new QCheckBox("Show axes");
  m_showEnergyCheck = new QCheckBox("Show energy");
  m_showSpatialTextCheck = new QCheckBox("Show spatial text");
  m_showWaypointsCheck = new QCheckBox("Show camera waypoints");

  rightLayout->addWidget(m_showAxesCheck);
  rightLayout->addWidget(m_showEnergyCheck);
  rightLayout->addWidget(m_showSpatialTextCheck);
  rightLayout->addWidget(m_showWaypointsCheck);

  rightColLayout->addWidget(rightGroup);

  auto *regionGroup = new QGroupBox("Regions");
  auto *regionLayout = new QVBoxLayout(regionGroup);
  regionGroup->setMinimumHeight(100);
  regionGroup->setMaximumHeight(150);

  m_showRegion1ACheck = new QCheckBox("Show region 1");
  m_showRegion2ACheck = new QCheckBox("Show region 2");
  m_showRegion1BCheck = new QCheckBox("Show region 3");
  m_showRegion2BCheck = new QCheckBox("Show region 4");

  regionLayout->addWidget(m_showRegion1ACheck);
  regionLayout->addWidget(m_showRegion2ACheck);
  regionLayout->addWidget(m_showRegion1BCheck);
  regionLayout->addWidget(m_showRegion2BCheck);

  rightColLayout->addWidget(regionGroup);
  rightColLayout->addStretch();

  twoColLayout->addLayout(rightColLayout);

  mainLayout->addLayout(twoColLayout);

  /* Connections — all toggles refresh the canvas */
  auto *model = (struct model_pak *) sysenv.active_model;
  auto connectModelToggle = [this, model](QCheckBox *check, int *field) {
    connect(check, &QCheckBox::toggled, this, [this, model, field](bool v) {
      *field = v;
      triggerRedraw();
    });
  };
  connectModelToggle(m_showCellCheck, &model->show_cell);
  connectModelToggle(m_showCellImagesCheck, &model->show_cell_images);
  connectModelToggle(m_showCellLengthsCheck, &model->show_cell_lengths);
  connectModelToggle(m_showCoresCheck, &model->show_cores);
  connectModelToggle(m_showShellsCheck, &model->show_shells);
  connectModelToggle(m_showLinksCheck, &model->show_links);
  connectModelToggle(m_showAtomIndexCheck, &model->show_atom_index);
  connectModelToggle(m_showAtomLabelsCheck, &model->show_atom_labels);
  connectModelToggle(m_showAtomTypesCheck, &model->show_atom_types);
  connectModelToggle(m_showCoreChargesCheck, &model->show_core_charges);
  connectModelToggle(m_showShellChargesCheck, &model->show_shell_charges);
  connectModelToggle(m_showAtomChargesCheck, &model->show_atom_charges);
  connectModelToggle(m_showNmrShiftsCheck, &model->show_nmr_shifts);
  connectModelToggle(m_showNmrCsaCheck, &model->show_nmr_csa);
  connectModelToggle(m_showNmrEfgCheck, &model->show_nmr_efg);
  connectModelToggle(m_showHydrogenCheck, &model->build_hydrogen);
  connectModelToggle(m_showZeoliteCheck, &model->build_zeolite);
  connectModelToggle(m_showSelectionLabelsCheck, &model->show_selection_labels);
  connectModelToggle(m_showAxesCheck, &model->show_axes);
  connectModelToggle(m_showSpatialTextCheck, &model->morph_label);
  connectModelToggle(m_showWaypointsCheck, &model->show_waypoints);
  connectModelToggle(m_showRegion1ACheck, &model->show_region1A);
  connectModelToggle(m_showRegion2ACheck, &model->show_region2A);
  connectModelToggle(m_showRegion1BCheck, &model->show_region1B);
  connectModelToggle(m_showRegion2BCheck, &model->show_region2B);
  connectRenderToggle(m_showEnergyCheck, &sysenv.render.show_energy);

  mainLayout->addStretch();
}
