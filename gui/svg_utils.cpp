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

#include "svg_utils.h"
#include <QSvgRenderer>
#include <QByteArray>
#include <QPainter>
#include <QImage>
#include <QPushButton>

/* all svg data here (see script in `./res`) */
#include "all_icons.h"

/* Icon lookup table - all_icons */
struct icon_entry {
  const char *const data;
  const char *name;
};

static const icon_entry embedded_icons[] = {
    {.data = DATA_ARROW, .name = "ARROW"},
    {.data = DATA_AVIEW, .name = "AVIEW"},
    {.data = DATA_AXES, .name = "AXES"},
    {.data = DATA_BOX, .name = "BOX"},
    {.data = DATA_BVIEW, .name = "BVIEW"},
    {.data = DATA_CAMERA, .name = "CAMERA"},
    {.data = DATA_CANVAS_CREATE, .name = "CANVAS_CREATE"},
    {.data = DATA_CANVAS_DELETE, .name = "CANVAS_DELETE"},
    {.data = DATA_CANVAS_SINGLE, .name = "CANVAS_SINGLE"},
    {.data = DATA_CELL, .name = "CELL"},
    {.data = DATA_CROSS, .name = "CROSS"},
    {.data = DATA_CVIEW, .name = "CVIEW"},
    {.data = DATA_DIAMOND2, .name = "DIAMOND2"},
    {.data = DATA_DISK, .name = "DISK"},
    {.data = DATA_ELEMENT, .name = "ELEMENT"},
    {.data = DATA_FASTFORWARD, .name = "FASTFORWARD"},
    {.data = DATA_FOLDER, .name = "FOLDER"},
    {.data = DATA_GEOM, .name = "GEOM"},
    {.data = DATA_GO, .name = "GO"},
    {.data = DATA_GRAPH, .name = "GRAPH"},
    {.data = DATA_LEFT_ARROW1, .name = "LEFT_ARROW1"},
    {.data = DATA_LOGO_LEFT, .name = "LOGO_LEFT"},
    {.data = DATA_LOGO_RIGHT, .name = "LOGO_RIGHT"},
    {.data = DATA_LOGO_WIDE, .name = "LOGO_WIDE"},
    {.data = DATA_MATRIX, .name = "MATRIX"},
    {.data = DATA_METHANE, .name = "METHANE"},
    {.data = DATA_PALETTE, .name = "PALETTE"},
    {.data = DATA_PAUSE, .name = "PAUSE"},
    {.data = DATA_PLAY, .name = "PLAY"},
    {.data = DATA_PLOTS, .name = "PLOTS"},
    {.data = DATA_PLUS, .name = "PLUS"},
    {.data = DATA_POLYMER, .name = "POLYMER"},
    {.data = DATA_RENDER_SETUP, .name = "RENDER_SETUP"},
    {.data = DATA_REWIND, .name = "REWIND"},
    {.data = DATA_RIGHT_ARROW1, .name = "RIGHT_ARROW1"},
    {.data = DATA_ROTATE1, .name = "ROTATE1"},
    {.data = DATA_ROTATE2, .name = "ROTATE2"},
    {.data = DATA_ROTATE3, .name = "ROTATE3"},
    {.data = DATA_SELECT_ALL, .name = "SELECT_ALL"},
    {.data = DATA_SPLIT_BOTH, .name = "SPLIT_BOTH"},
    {.data = DATA_SPLIT_HORZ, .name = "SPLIT_HORZ"},
    {.data = DATA_SPLIT_NONE, .name = "SPLIT_NONE"},
    {.data = DATA_SPLIT_VERT, .name = "SPLIT_VERT"},
    {.data = DATA_STEP_BACKWARD, .name = "STEP_BACKWARD"},
    {.data = DATA_STEP_FORWARD, .name = "STEP_FORWARD"},
    {.data = DATA_STOP, .name = "STOP"},
    {.data = DATA_SURFACE, .name = "SURFACE"},
    {.data = DATA_T1, .name = "T1"},
    {.data = DATA_T2, .name = "T2"},
    {.data = DATA_T3, .name = "T3"},
    {.data = DATA_TB_ANIMATE, .name = "TB_ANIMATE"},
    {.data = DATA_TB_DIFFRACTION, .name = "TB_DIFFRACTION"},
    {.data = DATA_TB_ISOSURFACE, .name = "TB_ISOSURFACE"},
    {.data = DATA_TB_SURFACE, .name = "TB_SURFACE"},
    {.data = DATA_TO_EPS, .name = "TO_EPS"},
    {.data = DATA_TO_PNG, .name = "TO_PNG"},
    {.data = DATA_TOOLS, .name = "TOOLS"},
    {.data = DATA_TRACK, .name = "TRACK"},
    {.data = DATA_XVIEW, .name = "XVIEW"},
    {.data = DATA_YVIEW, .name = "YVIEW"},
    {.data = DATA_ZVIEW, .name = "ZVIEW"},
};

static const int NUM_EMBEDDED = sizeof(embedded_icons) / sizeof(embedded_icons[0]);

QIcon LoadIconSVG(const char *svgData, const QSize &targetSize)
{
  QByteArray byteArray(svgData);
  QSvgRenderer renderer(byteArray);
  if (!renderer.isValid())
  {
    /* did not read properly, return an empty icon */
    return QIcon();
  }

  QImage image(targetSize, QImage::Format_ARGB32);
  image.fill(Qt::transparent);

  QPainter painter(&image);
  renderer.render(&painter);
  painter.end();

  return QIcon(QPixmap::fromImage(image));
}

QIcon loadGdisIcon(const char *embedded_name)
{
  for (int i = 0; i < NUM_EMBEDDED; i++)
  {
    if (strcmp(embedded_icons[i].name, embedded_name) == 0)
    {
      /* Return the icon at its original size; let the button handle scaling */
      return LoadIconSVG(embedded_icons[i].data);
    }
  }
  return QIcon();
}

void add_action_button(QVBoxLayout *layout, const QString &label_text, std::function<void()> callback)
{
  auto *hbox = new QHBoxLayout();
  hbox->setContentsMargins(0, 2, 0, 2);
  auto *label = new QLabel(label_text, nullptr);
  label->setStyleSheet("QLabel { font-weight: normal; }");
  hbox->addWidget(label);
  hbox->addStretch();
  auto *btn = new QPushButton(loadGdisIcon("GO"), "", nullptr);
  btn->setFixedSize(28, 28);
  btn->setStyleSheet("QPushButton { border: none; padding: 0; } "
                     "QPushButton:hover { background-color: rgba(180,180,255,0.3); }");
  QObject::connect(btn, &QPushButton::clicked, [callback]() { callback(); });
  hbox->addWidget(btn);
  layout->addLayout(hbox);
}

/* ---- Consistent |label ... widget| row helpers ---- */

QHBoxLayout *add_label_spinner(QVBoxLayout *parent, const QString &label, QSpinBox *spin, int labelWidth)
{
  auto *hbox = new QHBoxLayout();
  hbox->setContentsMargins(0, 1, 0, 1);
  auto *lbl = new QLabel(label);
  lbl->setMinimumWidth(labelWidth);
  lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  hbox->addWidget(lbl);
  spin->setMinimumWidth(80);
  hbox->addWidget(spin);
  parent->addLayout(hbox);
  return hbox;
}

QHBoxLayout *add_label_spinner(QVBoxLayout *parent, const QString &label, QDoubleSpinBox *spin, int labelWidth)
{
  auto *hbox = new QHBoxLayout();
  hbox->setContentsMargins(0, 1, 0, 1);
  auto *lbl = new QLabel(label);
  lbl->setMinimumWidth(labelWidth);
  lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  hbox->addWidget(lbl);
  spin->setMinimumWidth(80);
  hbox->addWidget(spin);
  parent->addLayout(hbox);
  return hbox;
}

QHBoxLayout *add_label_spinner(QVBoxLayout *parent, const QString &label, QComboBox *combo, int labelWidth)
{
  auto *hbox = new QHBoxLayout();
  hbox->setContentsMargins(0, 1, 0, 1);
  auto *lbl = new QLabel(label);
  lbl->setMinimumWidth(labelWidth);
  lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  hbox->addWidget(lbl);
  combo->setMinimumWidth(80);
  hbox->addWidget(combo);
  parent->addLayout(hbox);
  return hbox;
}

QHBoxLayout *add_label_edit(QVBoxLayout *parent, const QString &label, QLineEdit *edit, int labelWidth)
{
  auto *hbox = new QHBoxLayout();
  hbox->setContentsMargins(0, 1, 0, 1);
  auto *lbl = new QLabel(label);
  lbl->setMinimumWidth(labelWidth);
  lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  hbox->addWidget(lbl);
  edit->setMinimumWidth(80);
  hbox->addWidget(edit);
  parent->addLayout(hbox);
  return hbox;
}

QHBoxLayout *add_label_check(QVBoxLayout *parent, const QString &label, QCheckBox *check, int labelWidth)
{
  auto *hbox = new QHBoxLayout();
  hbox->setContentsMargins(0, 1, 0, 1);
  auto *lbl = new QLabel(label);
  lbl->setMinimumWidth(labelWidth);
  lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  hbox->addWidget(lbl);
  hbox->addWidget(check);
  parent->addLayout(hbox);
  return hbox;
}

QHBoxLayout *add_stretch_widget(QVBoxLayout *parent, QWidget *widget)
{
  auto *hbox = new QHBoxLayout();
  hbox->setContentsMargins(0, 1, 0, 1);
  widget->setMinimumWidth(80);
  hbox->addWidget(widget);
  parent->addLayout(hbox);
  return hbox;
}

void set_widget_tooltip(QWidget *widget, const char *tooltip)
{
  if (widget && tooltip)
    widget->setToolTip(QString::fromUtf8(tooltip));
}
