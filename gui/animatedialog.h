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
 * Animation Dialog for GDIS Qt6 GUI
 * Layout: 3 tabs (Control/Processing/Rendering) on top,
 *         frame slider + playback buttons below
 */

#ifndef ANIMATEDIALOG_H
#define ANIMATEDIALOG_H

#include <QDialog>
#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QLineEdit>
#include <QSlider>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>

class GLCanvas;
struct model_pak;

class AnimationDialog : public QDialog
{
  Q_OBJECT

public:
  explicit AnimationDialog(struct model_pak *model, QWidget *parent = nullptr);
  ~AnimationDialog() override;

  void refresh();

private slots:
  void on_frame_changed(int frame);
  void on_rewind();
  void on_step_backward();
  void on_play();
  void on_stop();
  void on_step_forward();
  void on_fast_forward();
  void on_render_toggled(bool checked);
  void on_movie_type_changed(int index);
  void on_filename_changed();

  void on_anim_speed_changed(double v);
  void on_anim_step_changed(int v);
  void on_anim_loop_toggled(bool checked);
  void on_anim_fix_toggled(bool checked);
  void on_anim_noscale_toggled(bool checked);
  void on_confine_atoms();
  void on_confine_mols();
  void on_no_pbc();

  void on_mpeg_quality_changed(int v);
  void on_delay_changed(int v);

  void stop_animation();

private:
  void setupControlPage();
  void setupProcessingPage();
  void setupRenderingPage();
  void setupPlaybackControls();

  struct model_pak *m_model;
  GLCanvas *m_canvas;

  /* Tab widgets */
  QTabWidget *m_tabWidget = nullptr;

  /* Control page widgets */
  QLabel *m_numFramesLabel = nullptr;
  QDoubleSpinBox *m_animSpeedSpin = nullptr;
  QSpinBox *m_animStepSpin = nullptr;
  QCheckBox *m_animFixCheck = nullptr;
  QCheckBox *m_animNoscaleCheck = nullptr;
  QCheckBox *m_animLoopCheck = nullptr;

  /* Processing page widgets */
  QRadioButton *m_confineAtomsRadio = nullptr;
  QRadioButton *m_confineMolsRadio = nullptr;
  QRadioButton *m_noPbcRadio = nullptr;

  /* Rendering page widgets */
  QCheckBox *m_renderMovieCheck = nullptr;
  QComboBox *m_movieTypeCombo = nullptr;
  QSpinBox *m_mpegQualitySpin = nullptr;
  QSpinBox *m_delaySpin = nullptr;
  QLineEdit *m_filenameEdit = nullptr;
  QCheckBox *m_keepImagesCheck = nullptr;

  /* Playback controls */
  QSlider *m_frameSlider = nullptr;
  QSpinBox *m_frameSpin = nullptr;
  QPushButton *m_rewindBtn = nullptr;
  QPushButton *m_stepBackBtn = nullptr;
  QPushButton *m_playBtn = nullptr;
  QPushButton *m_stopBtn = nullptr;
  QPushButton *m_stepFwdBtn = nullptr;
  QPushButton *m_ffBtn = nullptr;
};

#endif /* ANIMATEDIALOG_H */
