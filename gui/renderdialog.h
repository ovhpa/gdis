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
 * Render Properties Dialog for GDIS Qt6 GUI
 */

#ifndef RENDERDIALOG_H
#define RENDERDIALOG_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QTabWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QListWidget>
#include <QColor>
#include <QColorDialog>

class RenderDialog : public QDialog
{
  Q_OBJECT

public:
  explicit RenderDialog(QWidget *parent = nullptr);
  ~RenderDialog() override;

  void refresh();
  void setCanvas(QWidget *canvas);

private:
  Q_SLOT void on_render_mode_changed(int index);
  void on_colouring_scheme_changed(int index);
  void on_camera_projection_changed(int index);
  void on_light_add();
  void on_light_remove();
  void on_light_modify();
  void on_bg_colour_clicked();
  void on_rsurf_colour_clicked();
  void on_ribbon_colour_clicked();
  void on_sphere_quality_changed(int);
  void on_cylinder_quality_changed(int);
  void on_fog_density_changed(double);
  void on_ball_radius_changed(double);
  void on_stick_radius_changed(double);
  void on_stick_thickness_changed(double);
  void on_frame_thickness_changed(double);
  void on_cpk_scale_changed(double);
  void on_ahl_strength_changed(double);
  void on_ahl_size_changed(int);
  void on_shl_strength_changed(double);
  void on_shl_size_changed(int);
  void on_ribbon_curvature_changed(double);
  void on_ribbon_thickness_changed(double);
  void on_ribbon_quality_changed(int);
  void on_ghost_opacity_changed(double);
  void on_surface_opacity_changed(double);
  void on_zone_size_changed(int);
  void on_geom_line_width(double);
  void on_povray_width_changed(int);
  void on_povray_height_changed(int);
  void on_render_setup();
  void on_camera_create_waypoint();
  void on_stereo_windowed();
  void on_stereo_fullscreen();
  void on_stereo_off();

private:
  void setupMainPage();
  void setupColoursPage();
  void setupCameraPage();
  void setupLightsPage();
  void setupOpenGLPage();
  void setupPOVRayPage();
  void setupStereoPage();
  void setupTogglesPage();

  QWidget *m_mainPage = nullptr;
  QWidget *m_coloursPage = nullptr;
  QWidget *m_cameraPage = nullptr;
  QWidget *m_lightsPage = nullptr;
  QWidget *m_openGLPage = nullptr;
  QWidget *m_povrayPage = nullptr;
  QWidget *m_stereoPage = nullptr;
  QWidget *m_togglesPage = nullptr;

  void updateLightList();
  void loadLightFromWidget();
  void saveLightToWidget(int row);

  /* Camera page widgets */
  QComboBox *m_cameraProjCombo = nullptr;
  QComboBox *m_cameraModeCombo = nullptr;
  QSpinBox *m_cameraFOVSpin = nullptr;
  QLineEdit *m_cameraZoomEdit = nullptr;
  QSpinBox *m_cameraWaypointFramesSpin = nullptr;
  QComboBox *m_cameraAxisCombo = nullptr;
  QSpinBox *m_cameraRotStartSpin = nullptr;
  QSpinBox *m_cameraRotStopSpin = nullptr;
  QSpinBox *m_cameraRotIncSpin = nullptr;
  QCheckBox *m_cameraOverwriteCheck = nullptr;
  QListWidget *m_cameraList = nullptr;

  /* Main page widgets */
  QComboBox *m_renderModeCombo = nullptr;
  QCheckBox *m_antialiasCheck = nullptr;
  QCheckBox *m_cpkScaleCheck = nullptr;
  QCheckBox *m_wireSurfaceCheck = nullptr;
  QCheckBox *m_showHiddenCheck = nullptr;
  QDoubleSpinBox *m_ballRadiusSpin = nullptr;
  QDoubleSpinBox *m_stickRadiusSpin = nullptr;
  QDoubleSpinBox *m_stickThicknessSpin = nullptr;
  QDoubleSpinBox *m_frameThicknessSpin = nullptr;
  QDoubleSpinBox *m_cpkScaleSpin = nullptr;
  QDoubleSpinBox *m_ahlStrengthSpin = nullptr;
  QSpinBox *m_ahlSizeSpin = nullptr;
  QDoubleSpinBox *m_shlStrengthSpin = nullptr;
  QSpinBox *m_shlSizeSpin = nullptr;
  QDoubleSpinBox *m_ribbonCurvatureSpin = nullptr;
  QDoubleSpinBox *m_ribbonThicknessSpin = nullptr;
  QSpinBox *m_ribbonQualitySpin = nullptr;
  QDoubleSpinBox *m_ghostOpacitySpin = nullptr;
  QDoubleSpinBox *m_surfaceOpacitySpin = nullptr;
  QSpinBox *m_zoneSizeSpin = nullptr;

  /* Colours page */
  QPushButton *m_bgColourBtn = nullptr;
  QPushButton *m_rsurfColourBtn = nullptr;
  QPushButton *m_ribbonColourBtn = nullptr;
  QComboBox *m_colouringSchemeCombo = nullptr;

  /* Lights page */
  QListWidget *m_lightList = nullptr;
  QDoubleSpinBox *m_ambientSpin = nullptr;
  QDoubleSpinBox *m_diffuseSpin = nullptr;
  QDoubleSpinBox *m_specularSpin = nullptr;
  QComboBox *m_lightTypeCombo = nullptr;
  QDoubleSpinBox *m_lightXSpin = nullptr;
  QDoubleSpinBox *m_lightYSpin = nullptr;
  QDoubleSpinBox *m_lightZSpin = nullptr;

  /* OpenGL page */
  QSpinBox *m_sphereQualitySpin = nullptr;
  QSpinBox *m_cylinderQualitySpin = nullptr;
  QCheckBox *m_autoQualityCheck = nullptr;
  QCheckBox *m_fastRotationCheck = nullptr;
  QCheckBox *m_halosCheck = nullptr;
  QCheckBox *m_fogCheck = nullptr;
  QDoubleSpinBox *m_fogDensitySpin = nullptr;
  QDoubleSpinBox *m_geomLineWidthSpin = nullptr;

  /* POVRay page */
  QSpinBox *m_povrayWidthSpin = nullptr;
  QSpinBox *m_povrayHeightSpin = nullptr;
  QCheckBox *m_shadowlessCheck = nullptr;
  QCheckBox *m_noPovrayExecCheck = nullptr;
  QCheckBox *m_noKeepTempfilesCheck = nullptr;
  QPushButton *m_renderBtn = nullptr;

  /* Stereo page */
  QCheckBox *m_stereoAnaglyphCheck = nullptr;
  QCheckBox *m_stereoAnaglyphBlueRedCheck = nullptr;
  QPushButton *m_stereoWindowedBtn = nullptr;
  QPushButton *m_stereoFullscreenBtn = nullptr;
  QPushButton *m_stereoOffBtn = nullptr;
  QDoubleSpinBox *m_stereoEyeOffsetSpin = nullptr;
  QDoubleSpinBox *m_stereoParallaxSpin = nullptr;
  QCheckBox *m_stereoLeftCheck = nullptr;
  QCheckBox *m_stereoRightCheck = nullptr;

  /* Toggles page */
  QCheckBox *m_showCellCheck = nullptr;
  QCheckBox *m_showCellImagesCheck = nullptr;
  QCheckBox *m_showCellLengthsCheck = nullptr;
  QCheckBox *m_showCoresCheck = nullptr;
  QCheckBox *m_showShellsCheck = nullptr;
  QCheckBox *m_showLinksCheck = nullptr;
  QCheckBox *m_showAtomIndexCheck = nullptr;
  QCheckBox *m_showAtomLabelsCheck = nullptr;
  QCheckBox *m_showAtomTypesCheck = nullptr;
  QCheckBox *m_showCoreChargesCheck = nullptr;
  QCheckBox *m_showShellChargesCheck = nullptr;
  QCheckBox *m_showAtomChargesCheck = nullptr;
  QCheckBox *m_showNmrShiftsCheck = nullptr;
  QCheckBox *m_showNmrCsaCheck = nullptr;
  QCheckBox *m_showNmrEfgCheck = nullptr;
  QCheckBox *m_showHydrogenCheck = nullptr;
  QCheckBox *m_showZeoliteCheck = nullptr;
  QCheckBox *m_showSelectionLabelsCheck = nullptr;
  QCheckBox *m_showAxesCheck = nullptr;
  QCheckBox *m_showEnergyCheck = nullptr;
  QCheckBox *m_showSpatialTextCheck = nullptr;
  QCheckBox *m_showWaypointsCheck = nullptr;
  QCheckBox *m_showRegion1ACheck = nullptr;
  QCheckBox *m_showRegion2ACheck = nullptr;
  QCheckBox *m_showRegion1BCheck = nullptr;
  QCheckBox *m_showRegion2BCheck = nullptr;

  /* Current light data */
  struct {
    double ambient = 0.2;
    double diffuse = 0.8;
    double specular = 0.5;
    double x[3] = {0.0, 0.0, 1.0};
    int type = 0; // 0=directional, 1=positional
    QColor colour = QColor(255, 255, 255);
  } m_currentLight;

  /* Background colour storage */
  QColor m_bgColour;
  QColor m_rsurfColour;
  QColor m_ribbonColour;

  /* Canvas widget for immediate redraw */
  QWidget *m_canvas = nullptr;
  void triggerRedraw();
  void populateCameraList();

  /* Helper: connect a checkbox to an int field with refresh */
  void connectToggle(QCheckBox *check, int *field);
  /* Helper: connect a checkbox to a render int field with refresh */
  void connectRenderToggle(QCheckBox *check, int *field);
};

#endif /* RENDERDIALOG_H */
