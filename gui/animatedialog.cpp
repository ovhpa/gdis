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
 */

#include <QTimer>
#include <QFileDialog>
#include <stdio.h>

#include "animatedialog.h"
#include "glcanvas.h"
#include "gdis_api.h"

/* C headers — glib.h before Qt to avoid C++ exception specifier conflicts */
#include <glib.h>
#include "pak.h"
#include "file.h"

/* sysenv global — defined in main.c */
extern struct sysenv_pak sysenv;

/* C bridge declarations */
extern void qt_select_frame_model(struct model_pak *model);
extern void qt_refresh_content(void);
extern void qt_update_content_table(void);
extern void redraw_canvas(gint);
extern void qt_set_render_filename(const char *filename);
extern void qt_init_camera(struct model_pak *model);
extern void qt_povray_exec(const char *filename);
extern void *qt_get_gl_canvas(void);
extern void gui_text_show(gint type, const char *msg);

/* Movie assembly */
static void assemble_movie();

#include <QTimer>

static QTimer *g_animTimer = nullptr;

/* Helper: load a frame and update everything */
static void load_and_update(struct model_pak *model, GLCanvas *canvas)
{
  if (!model || !canvas)
    return;
  qt_init_camera(model);
  qt_select_frame_model(model);
  qt_refresh_content();
  qt_update_content_table();
  canvas->update();
}

/* Constructor */
AnimationDialog::AnimationDialog(struct model_pak *model, QWidget *parent) : QDialog(parent), m_model(model)
{
  setWindowTitle(QString("Animation: %1").arg("model"));
  setMinimumSize(500, 450);

  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(4, 4, 4, 4);
  mainLayout->setSpacing(4);

  /* === TABS (top) === */
  m_tabWidget = new QTabWidget(this);
  setupControlPage();
  setupProcessingPage();
  setupRenderingPage();
  mainLayout->addWidget(m_tabWidget);

  /* === FRAME SLIDER (below tabs) === */
  auto *sliderLayout = new QHBoxLayout();
  auto *frameLabel = new QLabel("Frame:", this);
  m_frameSpin = new QSpinBox(this);
  m_frameSpin->setRange(0, qt_model_get_num_frames() - 1);
  m_frameSpin->setValue(qt_model_get_cur_frame());
  m_frameSpin->setMinimumWidth(80);
  connect(m_frameSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AnimationDialog::on_frame_changed);

  m_frameSlider = new QSlider(Qt::Horizontal, this);
  m_frameSlider->setRange(0, qt_model_get_num_frames() - 1);
  m_frameSlider->setValue(qt_model_get_cur_frame());
  m_frameSlider->setMinimumWidth(200);
  connect(m_frameSlider, &QSlider::valueChanged, this, &AnimationDialog::on_frame_changed);
  connect(m_frameSlider, &QSlider::valueChanged, m_frameSpin, &QSpinBox::setValue);

  sliderLayout->addWidget(frameLabel);
  sliderLayout->addWidget(m_frameSlider);
  sliderLayout->addWidget(m_frameSpin);
  mainLayout->addLayout(sliderLayout);

  /* === PLAYBACK BUTTONS (below slider) === */
  auto *btnLayout = new QHBoxLayout();
  btnLayout->setSpacing(2);

  m_rewindBtn = new QPushButton(this);
  m_rewindBtn->setToolTip("Rewind");
  m_rewindBtn->setIcon(QIcon::fromTheme("media-seek-backward"));
  m_rewindBtn->setFixedSize(32, 32);
  m_rewindBtn->setStyleSheet("QPushButton { border: none; padding: 0; }");
  connect(m_rewindBtn, &QPushButton::clicked, this, &AnimationDialog::on_rewind);
  btnLayout->addWidget(m_rewindBtn);

  m_stepBackBtn = new QPushButton(this);
  m_stepBackBtn->setToolTip("Step Back");
  m_stepBackBtn->setIcon(QIcon::fromTheme("media-seek-backward"));
  m_stepBackBtn->setFixedSize(32, 32);
  m_stepBackBtn->setStyleSheet("QPushButton { border: none; padding: 0; }");
  connect(m_stepBackBtn, &QPushButton::clicked, this, &AnimationDialog::on_step_backward);
  btnLayout->addWidget(m_stepBackBtn);

  m_playBtn = new QPushButton(this);
  m_playBtn->setToolTip("Play");
  m_playBtn->setIcon(QIcon::fromTheme("media-playback-start"));
  m_playBtn->setFixedSize(32, 32);
  m_playBtn->setStyleSheet("QPushButton { border: none; padding: 0; }");
  connect(m_playBtn, &QPushButton::clicked, this, &AnimationDialog::on_play);
  btnLayout->addWidget(m_playBtn);

  m_stopBtn = new QPushButton(this);
  m_stopBtn->setToolTip("Stop");
  m_stopBtn->setIcon(QIcon::fromTheme("media-playback-stop"));
  m_stopBtn->setFixedSize(32, 32);
  m_stopBtn->setStyleSheet("QPushButton { border: none; padding: 0; }");
  connect(m_stopBtn, &QPushButton::clicked, this, &AnimationDialog::on_stop);
  btnLayout->addWidget(m_stopBtn);

  m_stepFwdBtn = new QPushButton(this);
  m_stepFwdBtn->setToolTip("Step Forward");
  m_stepFwdBtn->setIcon(QIcon::fromTheme("media-seek-forward"));
  m_stepFwdBtn->setFixedSize(32, 32);
  m_stepFwdBtn->setStyleSheet("QPushButton { border: none; padding: 0; }");
  connect(m_stepFwdBtn, &QPushButton::clicked, this, &AnimationDialog::on_step_forward);
  btnLayout->addWidget(m_stepFwdBtn);

  m_ffBtn = new QPushButton(this);
  m_ffBtn->setToolTip("Fast Forward");
  m_ffBtn->setIcon(QIcon::fromTheme("media-seek-forward"));
  m_ffBtn->setFixedSize(32, 32);
  m_ffBtn->setStyleSheet("QPushButton { border: none; padding: 0; }");
  connect(m_ffBtn, &QPushButton::clicked, this, &AnimationDialog::on_fast_forward);
  btnLayout->addWidget(m_ffBtn);

  mainLayout->addLayout(btnLayout);
}

AnimationDialog::~AnimationDialog() { stop_animation(); }

void AnimationDialog::refresh()
{
  int num = qt_model_get_num_frames();
  int cur = qt_model_get_cur_frame();
  m_frameSpin->setRange(0, num - 1);
  m_frameSlider->setRange(0, num - 1);
  m_frameSpin->setValue(cur);
  m_frameSlider->setValue(cur);

  /* Sync keep-images toggle */
  if (m_keepImagesCheck)
    m_keepImagesCheck->setChecked(!qt_get_render_no_keep_tempfiles());
}

/* === Control page === */
void AnimationDialog::setupControlPage()
{
  auto *page = new QWidget();
  auto *layout = new QVBoxLayout(page);
  layout->setContentsMargins(8, 8, 8, 8);

  /* Animation info */
  auto *infoGroup = new QGroupBox("Animation Info", page);
  auto *infoLayout = new QFormLayout(infoGroup);
  m_numFramesLabel = new QLabel(page);
  m_numFramesLabel->setText(QString("Number of frames: %1").arg(qt_model_get_num_frames()));
  infoLayout->addRow("", m_numFramesLabel);
  layout->addWidget(infoGroup);

  /* Parameters */
  auto *paramGroup = new QGroupBox("Parameters", page);
  auto *paramLayout = new QFormLayout(paramGroup);

  m_animSpeedSpin = new QDoubleSpinBox(paramGroup);
  m_animSpeedSpin->setRange(1.0, 40.0);
  m_animSpeedSpin->setSingleStep(1.0);
  m_animSpeedSpin->setValue(qt_model_get_anim_speed());
  connect(m_animSpeedSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &AnimationDialog::on_anim_speed_changed);
  paramLayout->addRow("Speed (x):", m_animSpeedSpin);

  m_animStepSpin = new QSpinBox(paramGroup);
  m_animStepSpin->setRange(1, qt_model_get_num_frames());
  m_animStepSpin->setSingleStep(1);
  m_animStepSpin->setValue(qt_model_get_anim_step());
  connect(m_animStepSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AnimationDialog::on_anim_step_changed);
  paramLayout->addRow("Step size:", m_animStepSpin);

  layout->addWidget(paramGroup);

  /* Options */
  auto *optGroup = new QGroupBox("Options", page);
  auto *optLayout = new QVBoxLayout(optGroup);

  m_animFixCheck = new QCheckBox("Don't recalculate connectivity", optGroup);
  m_animFixCheck->setChecked(qt_model_get_anim_fix());
  connect(m_animFixCheck, &QCheckBox::toggled, this, &AnimationDialog::on_anim_fix_toggled);
  optLayout->addWidget(m_animFixCheck);

  m_animNoscaleCheck = new QCheckBox("Don't recalculate scale", optGroup);
  m_animNoscaleCheck->setChecked(qt_model_get_anim_noscale());
  connect(m_animNoscaleCheck, &QCheckBox::toggled, this, &AnimationDialog::on_anim_noscale_toggled);
  optLayout->addWidget(m_animNoscaleCheck);

  m_animLoopCheck = new QCheckBox("Loop", optGroup);
  m_animLoopCheck->setChecked(qt_model_get_anim_loop());
  connect(m_animLoopCheck, &QCheckBox::toggled, this, &AnimationDialog::on_anim_loop_toggled);
  optLayout->addWidget(m_animLoopCheck);

  layout->addWidget(optGroup);
  layout->addStretch();

  m_tabWidget->addTab(page, "Control");
}

/* === Processing page === */
void AnimationDialog::setupProcessingPage()
{
  auto *page = new QWidget();
  auto *layout = new QVBoxLayout(page);
  layout->setContentsMargins(8, 8, 8, 8);

  auto *group = new QGroupBox("PBC Confinement", page);
  auto *groupLayout = new QVBoxLayout(group);

  m_confineAtomsRadio = new QRadioButton("Confine atoms to PBC", group);
  m_confineMolsRadio = new QRadioButton("Confine mols to PBC", group);
  m_noPbcRadio = new QRadioButton("Cell confinement off", group);

  groupLayout->addWidget(m_confineAtomsRadio);
  groupLayout->addWidget(m_confineMolsRadio);
  groupLayout->addWidget(m_noPbcRadio);

  int confine = qt_model_get_confine();
  if (confine == 1)
    m_confineAtomsRadio->setChecked(true);
  else if (confine == 2)
    m_confineMolsRadio->setChecked(true);
  else
    m_noPbcRadio->setChecked(true);

  connect(m_confineAtomsRadio, &QRadioButton::toggled, this, &AnimationDialog::on_confine_atoms);
  connect(m_confineMolsRadio, &QRadioButton::toggled, this, &AnimationDialog::on_confine_mols);
  connect(m_noPbcRadio, &QRadioButton::toggled, this, &AnimationDialog::on_no_pbc);

  layout->addWidget(group);
  layout->addStretch();

  m_tabWidget->addTab(page, "Processing");
}

/* === Rendering page === */
void AnimationDialog::setupRenderingPage()
{
  auto *page = new QWidget();
  auto *layout = new QVBoxLayout(page);
  layout->setContentsMargins(8, 8, 8, 8);

  auto *renderGroup = new QGroupBox("Movie Rendering", page);
  auto *renderLayout = new QFormLayout(renderGroup);

  gint anim = 0, atype = 0, mpeg_q = 0, delay = 0;
  qt_get_render_settings(&anim, &atype, &mpeg_q, &delay);

  m_renderMovieCheck = new QCheckBox("Create movie", renderGroup);
  m_renderMovieCheck->setChecked(anim);
  connect(m_renderMovieCheck, &QCheckBox::toggled, this, &AnimationDialog::on_render_toggled);
  renderLayout->addRow("", m_renderMovieCheck);

  m_movieTypeCombo = new QComboBox(renderGroup);
  m_movieTypeCombo->addItems({"Animated GIF", "MPEG"});
  m_movieTypeCombo->setCurrentIndex(atype);
  m_movieTypeCombo->setEnabled(false);
  connect(m_movieTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &AnimationDialog::on_movie_type_changed);
  renderLayout->addRow("Type:", m_movieTypeCombo);

  m_mpegQualitySpin = new QSpinBox(renderGroup);
  m_mpegQualitySpin->setRange(1, 100);
  m_mpegQualitySpin->setValue(mpeg_q);
  m_mpegQualitySpin->setEnabled(false);
  connect(m_mpegQualitySpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &AnimationDialog::on_mpeg_quality_changed);
  renderLayout->addRow("MPEG quality:", m_mpegQualitySpin);

  m_delaySpin = new QSpinBox(renderGroup);
  m_delaySpin->setRange(0, 100);
  m_delaySpin->setValue(delay);
  m_delaySpin->setEnabled(false);
  connect(m_delaySpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AnimationDialog::on_delay_changed);
  renderLayout->addRow("Delay (ms):", m_delaySpin);

  auto *fileLayout = new QHBoxLayout();
  auto *fileLabel = new QLabel("Filename:", renderGroup);
  m_filenameEdit = new QLineEdit(renderGroup);
  m_filenameEdit->setText(QString::fromUtf8(qt_get_render_filename()));
  m_filenameEdit->setEnabled(false);
  connect(m_filenameEdit, &QLineEdit::textChanged, this, &AnimationDialog::on_filename_changed);

  auto *browseBtn = new QPushButton("...", renderGroup);
  browseBtn->setEnabled(false);
  connect(browseBtn, &QPushButton::clicked, [this]() {
    QString fileName =
        QFileDialog::getSaveFileName(this, "Save Movie", ".", "GIF (*.gif);;MPEG (*.mpg);;All Files (*)");
    if (!fileName.isEmpty())
    {
      m_filenameEdit->setText(fileName);
      qt_set_render_filename(fileName.toUtf8().constData());
    }
  });

  fileLayout->addWidget(fileLabel);
  fileLayout->addWidget(m_filenameEdit);
  fileLayout->addWidget(browseBtn);
  renderLayout->addRow("", fileLayout);

  /* Keep intermediate images toggle */
  m_keepImagesCheck = new QCheckBox("Keep intermediate images", renderGroup);
  m_keepImagesCheck->setChecked(!qt_get_render_no_keep_tempfiles());
  connect(m_keepImagesCheck, &QCheckBox::toggled, [this](bool checked) { qt_set_render_no_keep_tempfiles(!checked); });
  renderLayout->addRow("", m_keepImagesCheck);

  layout->addWidget(renderGroup);
  layout->addStretch();

  m_tabWidget->addTab(page, "Rendering");
}

/* === Playback controls (already in constructor) === */
void AnimationDialog::setupPlaybackControls() { /* Already set up in constructor */ }

/* === Slot implementations === */
void AnimationDialog::on_frame_changed(int frame)
{
  qt_model_set_cur_frame(frame);
  load_and_update(m_model, static_cast<GLCanvas *>(qt_get_gl_canvas()));
}

void AnimationDialog::on_rewind()
{
  qt_model_set_cur_frame(0);
  m_frameSpin->setValue(0);
  m_frameSlider->setValue(0);
  load_and_update(m_model, static_cast<GLCanvas *>(qt_get_gl_canvas()));
}

void AnimationDialog::on_step_backward()
{
  int cur = qt_model_get_cur_frame();
  if (cur <= 0)
    return;
  qt_model_set_cur_frame(cur - 1);
  m_frameSpin->setValue(cur - 1);
  m_frameSlider->setValue(cur - 1);
  load_and_update(m_model, static_cast<GLCanvas *>(qt_get_gl_canvas()));
}

void AnimationDialog::on_play()
{
  int speed = (int) qt_model_get_anim_speed();
  if (speed < 1)
    speed = 1;

  qt_open_animation_file(m_model);
  qt_model_set_animating(1);

  if (g_animTimer)
    g_animTimer->stop();
  delete g_animTimer;

  g_animTimer = new QTimer();
  int freq = 25 * speed;
  if (freq < 25)
    freq = 25;
  g_animTimer->setInterval(freq);
  g_animTimer->setSingleShot(false);

  connect(g_animTimer, &QTimer::timeout, [this]() {
    if (!qt_model_get_animating())
      return;

    int cur = qt_model_get_cur_frame();
    int num = qt_model_get_num_frames();
    int step = qt_model_get_anim_step();
    int loop = qt_model_get_anim_loop();

    cur += step;
    if (cur >= num)
    {
      if (loop)
        cur = 0;
      else
      {
        qt_model_set_animating(0);
        return;
      }
    }

    qt_model_set_cur_frame(cur);
    m_frameSpin->setValue(cur);
    m_frameSlider->setValue(cur);
    load_and_update(m_model, static_cast<GLCanvas *>(qt_get_gl_canvas()));

    /* Render POV file for movie creation if enabled */
    gint anim = 0;
    qt_get_render_settings(&anim, NULL, NULL, NULL);
    if (anim)
    {
      QString povName =
          QString("%1_%2.pov").arg(QString::fromUtf8(qt_get_render_filename())).arg(cur, 6, 10, QChar('0'));
      QString povPath = QString("%1/%2").arg(qt_get_cwd()).arg(povName);

      qt_write_povray(povPath.toUtf8().constData(), m_model);

      if (!qt_get_render_no_povray_exec())
        qt_povray_exec(povPath.toUtf8().constData());
    }
  });

  g_animTimer->start();
}

void AnimationDialog::on_stop()
{
  qt_model_set_animating(0);
  qt_close_animation_file(m_model);
  stop_animation();

  /* Assemble movie if render was enabled */
  assemble_movie();
}

void AnimationDialog::on_step_forward()
{
  int cur = qt_model_get_cur_frame();
  int num = qt_model_get_num_frames();
  if (cur >= num - 1)
    return;
  qt_model_set_cur_frame(cur + 1);
  m_frameSpin->setValue(cur + 1);
  m_frameSlider->setValue(cur + 1);
  load_and_update(m_model, static_cast<GLCanvas *>(qt_get_gl_canvas()));
}

void AnimationDialog::on_fast_forward()
{
  int num = qt_model_get_num_frames();
  qt_model_set_cur_frame(num - 1);
  m_frameSpin->setValue(num - 1);
  m_frameSlider->setValue(num - 1);
  load_and_update(m_model, static_cast<GLCanvas *>(qt_get_gl_canvas()));
}

void AnimationDialog::on_render_toggled(bool checked)
{
  gint atype = 0, mpeg_q = 0, delay = 0;
  qt_get_render_settings(NULL, &atype, &mpeg_q, &delay);
  qt_set_render_settings(checked ? 1 : 0, atype, mpeg_q, delay);
  m_movieTypeCombo->setEnabled(checked);
  m_mpegQualitySpin->setEnabled(checked);
  m_delaySpin->setEnabled(checked);
  m_filenameEdit->setEnabled(checked);
}

void AnimationDialog::on_movie_type_changed(int index)
{
  gint anim = 0, mpeg_q = 0, delay = 0;
  qt_get_render_settings(&anim, NULL, &mpeg_q, &delay);
  qt_set_render_settings(anim, index, mpeg_q, delay);
}

void AnimationDialog::on_filename_changed() { qt_set_render_filename(m_filenameEdit->text().toUtf8().constData()); }

void AnimationDialog::on_anim_speed_changed(double v) { qt_model_set_anim_speed(v); }

void AnimationDialog::on_anim_step_changed(int v) { qt_model_set_anim_step(v); }

void AnimationDialog::on_anim_loop_toggled(bool checked) { qt_model_set_anim_loop(checked ? 1 : 0); }

void AnimationDialog::on_anim_fix_toggled(bool checked) { qt_model_set_anim_fix(checked ? 1 : 0); }

void AnimationDialog::on_anim_noscale_toggled(bool checked) { qt_model_set_anim_noscale(checked ? 1 : 0); }

void AnimationDialog::on_confine_atoms() { qt_model_set_confine(1); }
void AnimationDialog::on_confine_mols() { qt_model_set_confine(2); }
void AnimationDialog::on_no_pbc() { qt_model_set_confine(0); }

void AnimationDialog::on_mpeg_quality_changed(int v)
{
  gint anim = 0, atype = 0, delay = 0;
  qt_get_render_settings(&anim, &atype, NULL, &delay);
  qt_set_render_settings(anim, atype, v, delay);
}

void AnimationDialog::on_delay_changed(int v)
{
  gint anim = 0, atype = 0, mpeg_q = 0;
  qt_get_render_settings(&anim, &atype, &mpeg_q, NULL);
  qt_set_render_settings(anim, atype, mpeg_q, v);
}

void AnimationDialog::stop_animation()
{
  if (g_animTimer)
  {
    g_animTimer->stop();
    delete g_animTimer;
    g_animTimer = nullptr;
  }
}

/* Shell-quote a string for safe use in system() commands */
static QString shell_quote(const QString &s)
{
  QString q = s;
  q.replace("'", "'\\''");
  return "'" + q + "'";
}

/* Assemble GIF/MPEG movie from rendered POV frames */
static void assemble_movie()
{
  gint anim = 0, atype = 0, mpeg_q = 0, delay = 0;
  qt_get_render_settings(&anim, &atype, &mpeg_q, &delay);
  if (!anim)
    return;

  const char *base = qt_get_render_filename();
  if (!base || !*base)
    return;

  /* Build the full base path: cwd/base */
  QString base_path = QString("%1/%2").arg(qt_get_cwd()).arg(base);
  QString quoted_base = shell_quote(base_path);

  /* Build command: convert runs from cwd, files are in cwd */
  QString cmd;
  if (atype == 1)
  {
    /* MPEG */
    cmd = QString("%1 -quality %2 -delay %3 '%4'_*.tga %5.mpg")
              .arg(qt_get_convert_path())
              .arg(mpeg_q)
              .arg(delay)
              .arg(base_path)
              .arg(base_path);
  } else
  {
    /* GIF */
    cmd = QString("%1 -delay %2 '%3'_*.tga %4.gif").arg(qt_get_convert_path()).arg(delay).arg(base_path).arg(base_path);
  }

  int ret = system(cmd.toUtf8().constData());
  if (ret == 0)
    gui_text_show(5, "Completed movie creation.\n"); /* ITALIC */
  else
    gui_text_show(3, "Movie creation failed.\n"); /* ERROR */

  /* Cleanup temp files if user doesn't want to keep them */
  gint keep_temp = qt_get_render_no_keep_tempfiles();
  if (keep_temp)
  {
    QString rm_pov = QString("rm -rf '%1'_*.pov").arg(base_path);
    QString rm_tga = QString("rm -rf '%1'_*.tga").arg(base_path);
    system(rm_pov.toUtf8().constData());
    system(rm_tga.toUtf8().constData());
  }
}

/* Animation bridge functions — moved from gui_still_valid.c */
/* These access sysenv.render and sysenv.active_model */

extern "C" {

void qt_set_render_filename(const char *filename)
{
  g_free(sysenv.render.animate_file);
  sysenv.render.animate_file = g_strdup(filename);
}

const gchar *qt_get_render_filename(void) { return sysenv.render.animate_file; }

void qt_get_render_settings(gint *animate, gint *animate_type, gint *mpeg_quality, gint *delay)
{
  if (animate)
    *animate = sysenv.render.animate;
  if (animate_type)
    *animate_type = sysenv.render.animate_type;
  if (mpeg_quality)
    *mpeg_quality = sysenv.render.mpeg_quality;
  if (delay)
    *delay = sysenv.render.delay;
}

void qt_set_render_settings(gint animate, gint animate_type, gint mpeg_quality, gint delay)
{
  sysenv.render.animate = animate;
  sysenv.render.animate_type = animate_type;
  sysenv.render.mpeg_quality = mpeg_quality;
  sysenv.render.delay = delay;
}

void qt_model_set_animating(gint val) { ((struct model_pak *) sysenv.active_model)->animating = val; }
gint qt_model_get_animating(void) { return ((struct model_pak *) sysenv.active_model)->animating; }
void qt_model_set_confine(gint val) { ((struct model_pak *) sysenv.active_model)->anim_confine = val; }
gint qt_model_get_confine(void) { return ((struct model_pak *) sysenv.active_model)->anim_confine; }
void qt_model_set_cur_frame(gint val) { ((struct model_pak *) sysenv.active_model)->cur_frame = val; }
gint qt_model_get_cur_frame(void) { return ((struct model_pak *) sysenv.active_model)->cur_frame; }
gint qt_model_get_num_frames(void) { return ((struct model_pak *) sysenv.active_model)->num_frames; }
void qt_model_set_anim_speed(gdouble val) { ((struct model_pak *) sysenv.active_model)->anim_speed = val; }
gdouble qt_model_get_anim_speed(void) { return ((struct model_pak *) sysenv.active_model)->anim_speed; }
void qt_model_set_anim_step(gint val) { ((struct model_pak *) sysenv.active_model)->anim_step = val; }
gint qt_model_get_anim_step(void) { return ((struct model_pak *) sysenv.active_model)->anim_step; }
void qt_model_set_anim_fix(gint val) { ((struct model_pak *) sysenv.active_model)->anim_fix = val; }
gint qt_model_get_anim_fix(void) { return ((struct model_pak *) sysenv.active_model)->anim_fix; }
void qt_model_set_anim_noscale(gint val) { ((struct model_pak *) sysenv.active_model)->anim_noscale = val; }
gint qt_model_get_anim_noscale(void) { return ((struct model_pak *) sysenv.active_model)->anim_noscale; }
void qt_model_set_anim_loop(gint val) { ((struct model_pak *) sysenv.active_model)->anim_loop = val; }
gint qt_model_get_anim_loop(void) { return ((struct model_pak *) sysenv.active_model)->anim_loop; }

gint qt_get_render_no_keep_tempfiles(void) { return sysenv.render.no_keep_tempfiles; }

void qt_set_render_no_keep_tempfiles(gint val) { sysenv.render.no_keep_tempfiles = val; }

gint qt_write_povray(const char *filename, struct model_pak *model) { return write_povray((char *) filename, model); }

gint qt_get_render_no_povray_exec(void) { return sysenv.render.no_povray_exec; }

const char *qt_get_cwd(void) { return sysenv.cwd; }

void qt_open_animation_file(struct model_pak *model)
{
  if (!model->afp && !model->transform_list)
  {
    model->afp = fopen(model->filename, "r");
  }
}

void qt_close_animation_file(struct model_pak *model)
{
  if (model && model->afp && !model->transform_list)
  {
    fclose(model->afp);
    model->afp = NULL;
  }
}

} /* extern "C" */
