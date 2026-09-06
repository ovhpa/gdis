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

#include "graphcontrolsdialog.h"
#include <cmath>

#include "gdis.h"
#include "graph.h"
#include "model.h"
#include "interface.h"
#include "gui_shorts.h"

/* Qt bridge functions */
#ifdef __cplusplus
extern "C" {
#endif
void qt_force_canvas_refresh(void);
#ifdef __cplusplus
}
#endif

/* Forward declarations for tree/canvas refresh */
extern void redraw_canvas(gint);

extern struct sysenv_pak sysenv;

/* ---- enum-to-combo helpers ---- */
static int color_to_combo(graph_color c)
{
  switch (c)
  {
  case GRAPH_COLOR_BLACK:
    return 0;
  case GRAPH_COLOR_WHITE:
    return 1;
  case GRAPH_COLOR_BLUE:
    return 2;
  case GRAPH_COLOR_GREEN:
    return 3;
  case GRAPH_COLOR_RED:
    return 4;
  case GRAPH_COLOR_YELLOW:
    return 5;
  case GRAPH_COLOR_GRAY:
    return 6;
  case GRAPH_COLOR_NAVY:
    return 7;
  case GRAPH_COLOR_LIME:
    return 8;
  case GRAPH_COLOR_TEAL:
    return 9;
  case GRAPH_COLOR_AQUA:
    return 10;
  case GRAPH_COLOR_MAROON:
    return 11;
  case GRAPH_COLOR_PURPLE:
    return 12;
  case GRAPH_COLOR_OLIVE:
    return 13;
  case GRAPH_COLOR_SILVER:
    return 14;
  case GRAPH_COLOR_FUSHIA:
    return 15;
  case GRAPH_COLOR_DEFAULT:
  default:
    return 16;
  }
}
static graph_color combo_to_color(int idx)
{
  switch (idx)
  {
  case 0:
    return GRAPH_COLOR_BLACK;
  case 1:
    return GRAPH_COLOR_WHITE;
  case 2:
    return GRAPH_COLOR_BLUE;
  case 3:
    return GRAPH_COLOR_GREEN;
  case 4:
    return GRAPH_COLOR_RED;
  case 5:
    return GRAPH_COLOR_YELLOW;
  case 6:
    return GRAPH_COLOR_GRAY;
  case 7:
    return GRAPH_COLOR_NAVY;
  case 8:
    return GRAPH_COLOR_LIME;
  case 9:
    return GRAPH_COLOR_TEAL;
  case 10:
    return GRAPH_COLOR_AQUA;
  case 11:
    return GRAPH_COLOR_MAROON;
  case 12:
    return GRAPH_COLOR_PURPLE;
  case 13:
    return GRAPH_COLOR_OLIVE;
  case 14:
    return GRAPH_COLOR_SILVER;
  case 15:
    return GRAPH_COLOR_FUSHIA;
  case 16:
  default:
    return GRAPH_COLOR_DEFAULT;
  }
}
static int line_to_combo(graph_line l)
{
  switch (l)
  {
  case GRAPH_LINE_SINGLE:
    return 1;
  case GRAPH_LINE_DASH:
    return 2;
  case GRAPH_LINE_DOT:
    return 3;
  case GRAPH_LINE_THICK:
    return 4;
  case GRAPH_LINE_NONE:
  default:
    return 0;
  }
}
static graph_line combo_to_line(int idx)
{
  switch (idx)
  {
  case 1:
    return GRAPH_LINE_SINGLE;
  case 2:
    return GRAPH_LINE_DASH;
  case 3:
    return GRAPH_LINE_DOT;
  case 4:
    return GRAPH_LINE_THICK;
  case 0:
  default:
    return GRAPH_LINE_NONE;
  }
}
static int symbol_to_combo(graph_symbol s)
{
  switch (s)
  {
  case GRAPH_SYMB_CROSS:
    return 1;
  case GRAPH_SYMB_SQUARE:
    return 2;
  case GRAPH_SYMB_TRI_UP:
    return 3;
  case GRAPH_SYMB_TRI_DN:
    return 4;
  case GRAPH_SYMB_DIAM:
    return 5;
  case GRAPH_SYMB_NONE:
  default:
    return 0;
  }
}
static graph_symbol combo_to_symbol(int idx)
{
  switch (idx)
  {
  case 1:
    return GRAPH_SYMB_CROSS;
  case 2:
    return GRAPH_SYMB_SQUARE;
  case 3:
    return GRAPH_SYMB_TRI_UP;
  case 4:
    return GRAPH_SYMB_TRI_DN;
  case 5:
    return GRAPH_SYMB_DIAM;
  case 0:
  default:
    return GRAPH_SYMB_NONE;
  }
}
static int type_to_combo(graph_type t)
{
  switch (t)
  {
  case GRAPH_XY_TYPE:
  case GRAPH_IY_TYPE:
    return 2;
  case GRAPH_IX_TYPE:
  case GRAPH_XX_TYPE:
    return 1;
  case GRAPH_REGULAR:
  default:
    return 0;
  }
}
static graph_type combo_to_type(int idx)
{
  switch (idx)
  {
  case 2:
    return GRAPH_XY_TYPE;
  case 1:
    return GRAPH_IX_TYPE;
  case 0:
  default:
    return GRAPH_REGULAR;
  }
}

/* ---- Auto-range helpers (port from toggle_auto_x/y) ---- */
void GraphControlsDialog::auto_x_range()
{
  GSList *list = m_graph->set_list;
  if (!list)
    return;

  gpointer first = list->data;
  g_data_x *p_x = nullptr;
  if (first)
  {
    g_data_x *test_gx = (g_data_x *) first;
    if (test_gx && test_gx->x != NULL && test_gx->x_size > 0)
      p_x = test_gx;
  }

  switch (m_graph->type)
  {
  case GRAPH_IY_TYPE:
  case GRAPH_XY_TYPE:
  case GRAPH_IX_TYPE:
  case GRAPH_XX_TYPE:
    if (p_x && p_x->x_size > 0)
    {
      m_graph->xmin = p_x->x[0];
      m_graph->xmax = p_x->x[0];
      for (int i = 1; i < p_x->x_size; i++)
      {
        if (p_x->x[i] < m_graph->xmin)
          m_graph->xmin = p_x->x[i];
        if (p_x->x[i] > m_graph->xmax)
          m_graph->xmax = p_x->x[i];
      }
    }
    break;
  case GRAPH_REGULAR:
  default:
    m_graph->xmin = 0.;
    m_graph->xmax = (gdouble) m_graph->size;
    break;
  }
  m_xminSpin->setValue(m_graph->xmin);
  m_xmaxSpin->setValue(m_graph->xmax);
}

void GraphControlsDialog::auto_y_range()
{
  GSList *list = m_graph->set_list;
  if (!list)
    return;

  /* Skip X entry (first g_data_x) */
  gpointer first = list->data;
  g_data_x *test_gx = (g_data_x *) first;
  if (test_gx && test_gx->x != NULL && test_gx->x_size > 0)
    list = g_slist_next(list);

  if (!list)
    return;

  /* Find the first Y entry with valid data */
  gpointer p_y_item = nullptr;
  for (; list; list = g_slist_next(list))
  {
    if (!list->data) continue;
    g_data_y *dy = (g_data_y *) list->data;
    if (dy && dy->y != NULL && dy->y_size > 0)
    {
      p_y_item = list->data;
      break;
    }
  }

  if (!p_y_item)
    return;

  g_data_y *p_y = (g_data_y *) p_y_item;

  if (std::isnan(p_y->y[0]))
  {
    m_graph->ymin = p_y->y[1];
    m_graph->ymax = p_y->y[1];
  } else
  {
    m_graph->ymin = p_y->y[0];
    m_graph->ymax = p_y->y[0];
  }

  /* Scan all Y entries for min/max */
  list = m_graph->set_list;
  if (test_gx && test_gx->x != NULL && test_gx->x_size > 0)
    list = g_slist_next(list);

  for (; list; list = g_slist_next(list))
  {
    if (!list->data) continue;
    g_data_y *dy = (g_data_y *) list->data;
    if (dy && dy->y != NULL && dy->y_size > 0)
    {
      for (int i = 0; i < dy->y_size; i++)
      {
        if (std::isnan(dy->y[i]))
          continue;
        if (dy->y[i] < m_graph->ymin)
          m_graph->ymin = dy->y[i];
        if (dy->y[i] > m_graph->ymax)
          m_graph->ymax = dy->y[i];
      }
    }
  }

  double range = m_graph->ymax - m_graph->ymin;
  if (range != 0.0)
  {
    m_graph->ymin -= range * 0.05;
    m_graph->ymax += range * 0.05;
  }

  m_yminSpin->setValue(m_graph->ymin);
  m_ymaxSpin->setValue(m_graph->ymax);
}

/* ---- Dialog implementation ---- */

GraphControlsDialog::GraphControlsDialog(struct graph_pak *graph, struct model_pak *model, QWidget *parent)
    : QDialog(parent), m_graph(graph), m_model(model), m_byValueEnabled(false)
{
  setWindowTitle(tr("GRAPH CONTROLS"));
  setMinimumSize(520, 380);
  setupUI();
  syncFromGraph();
}

GraphControlsDialog::~GraphControlsDialog() {}

void GraphControlsDialog::setupUI()
{
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(8, 6, 8, 6);
  mainLayout->setSpacing(4);

  /* ===== TITLE & AXIS LABELS ===== */
  auto *titlesGroup = new QGroupBox(tr("Title & Axis Labels"));
  auto *titlesLayout = new QVBoxLayout(titlesGroup);
  titlesLayout->setSpacing(4);

  // Main title + font size
  auto *titleRow = new QHBoxLayout();
  titleRow->addWidget(new QLabel(tr("Main title:")));
  m_titleEdit = new QLineEdit();
  titleRow->addWidget(m_titleEdit);
  titleRow->addWidget(new QLabel(tr("Font:")));
  m_titleFontSize = new QSpinBox();
  m_titleFontSize->setRange(6, 48);
  m_titleFontSize->setValue(0); // 0 = default
  titleRow->addWidget(m_titleFontSize);
  titlesLayout->addLayout(titleRow);

  // Sub-title + font size
  auto *subTitleRow = new QHBoxLayout();
  subTitleRow->addWidget(new QLabel(tr("Sub-title:")));
  m_subTitleEdit = new QLineEdit();
  subTitleRow->addWidget(m_subTitleEdit);
  subTitleRow->addWidget(new QLabel(tr("Font:")));
  m_subTitleFontSize = new QSpinBox();
  m_subTitleFontSize->setRange(6, 48);
  m_subTitleFontSize->setValue(0);
  subTitleRow->addWidget(m_subTitleFontSize);
  titlesLayout->addLayout(subTitleRow);

  // X axis + font size
  auto *xTitleRow = new QHBoxLayout();
  xTitleRow->addWidget(new QLabel(tr("X axis:")));
  m_xTitleEdit = new QLineEdit();
  xTitleRow->addWidget(m_xTitleEdit);
  xTitleRow->addWidget(new QLabel(tr("Font:")));
  m_xTitleFontSize = new QSpinBox();
  m_xTitleFontSize->setRange(6, 48);
  m_xTitleFontSize->setValue(0);
  xTitleRow->addWidget(m_xTitleFontSize);
  titlesLayout->addLayout(xTitleRow);

  // Y axis + font size
  auto *yTitleRow = new QHBoxLayout();
  yTitleRow->addWidget(new QLabel(tr("Y axis:")));
  m_yTitleEdit = new QLineEdit();
  yTitleRow->addWidget(m_yTitleEdit);
  yTitleRow->addWidget(new QLabel(tr("Font:")));
  m_yTitleFontSize = new QSpinBox();
  m_yTitleFontSize->setRange(6, 48);
  m_yTitleFontSize->setValue(0);
  yTitleRow->addWidget(m_yTitleFontSize);
  titlesLayout->addLayout(yTitleRow);

  mainLayout->addWidget(titlesGroup);

  /* ===== DIMENSIONS ===== */
  auto *dimGroup = new QGroupBox(tr("Dimensions"));
  auto *dimLayout = new QVBoxLayout(dimGroup);
  dimLayout->setSpacing(4);

  auto *xDimRow = new QHBoxLayout();
  xDimRow->addWidget(new QLabel(tr("X_MIN:")));
  m_xminSpin = new QDoubleSpinBox();
  m_xminSpin->setRange(-1e10, 1e10);
  m_xminSpin->setDecimals(6);
  xDimRow->addWidget(m_xminSpin);
  xDimRow->addWidget(new QLabel(tr("X_MAX:")));
  m_xmaxSpin = new QDoubleSpinBox();
  m_xmaxSpin->setRange(-1e10, 1e10);
  m_xmaxSpin->setDecimals(6);
  xDimRow->addWidget(m_xmaxSpin);
  xDimRow->addWidget(new QLabel(tr("XTICS:")));
  m_xticsSpin = new QSpinBox();
  m_xticsSpin->setRange(1, 50);
  xDimRow->addWidget(m_xticsSpin);
  dimLayout->addLayout(xDimRow);

  auto *yDimRow = new QHBoxLayout();
  yDimRow->addWidget(new QLabel(tr("Y_MIN:")));
  m_yminSpin = new QDoubleSpinBox();
  m_yminSpin->setRange(-1e10, 1e10);
  m_yminSpin->setDecimals(6);
  yDimRow->addWidget(m_yminSpin);
  yDimRow->addWidget(new QLabel(tr("Y_MAX:")));
  m_ymaxSpin = new QDoubleSpinBox();
  m_ymaxSpin->setRange(-1e10, 1e10);
  m_ymaxSpin->setDecimals(6);
  yDimRow->addWidget(m_ymaxSpin);
  yDimRow->addWidget(new QLabel(tr("YTICS:")));
  m_yticsSpin = new QSpinBox();
  m_yticsSpin->setRange(1, 50);
  yDimRow->addWidget(m_yticsSpin);
  dimLayout->addLayout(yDimRow);

  auto *autoRow = new QHBoxLayout();
  m_autoXCheck = new QCheckBox(tr("Auto X"));
  m_autoXCheck->setChecked(true);
  connect(m_autoXCheck, &QCheckBox::toggled, this, &GraphControlsDialog::on_auto_x_toggled);
  autoRow->addWidget(m_autoXCheck);
  m_autoYCheck = new QCheckBox(tr("Auto Y"));
  m_autoYCheck->setChecked(true);
  connect(m_autoYCheck, &QCheckBox::toggled, this, &GraphControlsDialog::on_auto_y_toggled);
  autoRow->addWidget(m_autoYCheck);
  dimLayout->addLayout(autoRow);

  mainLayout->addWidget(dimGroup);

  /* ===== DATA SET / SPECIAL — DISABLED (TODO: re-enable later) ===== */
  auto *specialGroup = new QGroupBox(tr("Data Set Properties"));
  specialGroup->setEnabled(false);
  auto *specialLayout = new QVBoxLayout(specialGroup);
  specialLayout->setSpacing(4);

  // SET + TYPE
  auto *setRow = new QHBoxLayout();
  setRow->addWidget(new QLabel(tr("SET:")));
  m_setSpin = new QSpinBox();
  setRow->addWidget(m_setSpin);
  setRow->addWidget(new QLabel(tr("TYPE:")));
  m_typeCombo = new QComboBox();
  m_typeCombo->addItem("NORMAL");
  m_typeCombo->addItem("X_LINE");
  m_typeCombo->addItem("Y_LINE");
  setRow->addWidget(m_typeCombo);
  specialLayout->addLayout(setRow);

  // BY VALUE + NUM + SIZE
  auto *byValRow = new QHBoxLayout();
  m_byValueCheck = new QCheckBox(tr("BY VALUE"));
  byValRow->addWidget(m_byValueCheck);
  byValRow->addWidget(new QLabel(tr("NUM:")));
  m_numSpin = new QSpinBox();
  byValRow->addWidget(m_numSpin);
  m_sizeEdit = new QLineEdit();
  m_sizeEdit->setReadOnly(true);
  byValRow->addWidget(new QLabel(tr("SIZE:")));
  byValRow->addWidget(m_sizeEdit);
  specialLayout->addLayout(byValRow);

  // IDX + SYMBOL
  auto *idxRow = new QHBoxLayout();
  idxRow->addWidget(new QLabel(tr("IDX:")));
  m_idxEdit = new QLineEdit();
  idxRow->addWidget(m_idxEdit);
  idxRow->addWidget(new QLabel(tr("SYMBOL:")));
  m_symbolCombo = new QComboBox();
  m_symbolCombo->addItem("NONE");
  m_symbolCombo->addItem("CROSS");
  m_symbolCombo->addItem("SQUARE");
  m_symbolCombo->addItem("TRIANGLE (UP)");
  m_symbolCombo->addItem("TRIANGLE (DN)");
  m_symbolCombo->addItem("DIAMOND");
  idxRow->addWidget(m_symbolCombo);
  specialLayout->addLayout(idxRow);

  // X_VAL + Y_VAL
  auto *valRow = new QHBoxLayout();
  valRow->addWidget(new QLabel(tr("X_VAL:")));
  m_xValSpin = new QDoubleSpinBox();
  valRow->addWidget(m_xValSpin);
  valRow->addWidget(new QLabel(tr("Y_VAL:")));
  m_yValSpin = new QDoubleSpinBox();
  valRow->addWidget(m_yValSpin);
  specialLayout->addLayout(valRow);

  // LINE + COLOR
  auto *lineRow = new QHBoxLayout();
  lineRow->addWidget(new QLabel(tr("LINE:")));
  m_lineCombo = new QComboBox();
  m_lineCombo->addItem("NONE");
  m_lineCombo->addItem("SINGLE");
  m_lineCombo->addItem("DASH");
  m_lineCombo->addItem("DOT");
  m_lineCombo->addItem("THICK");
  lineRow->addWidget(m_lineCombo);
  lineRow->addWidget(new QLabel(tr("COLOR:")));
  m_colorCombo = new QComboBox();
  m_colorCombo->addItem("BLACK");
  m_colorCombo->addItem("WHITE");
  m_colorCombo->addItem("BLUE");
  m_colorCombo->addItem("GREEN");
  m_colorCombo->addItem("RED");
  m_colorCombo->addItem("YELLOW");
  m_colorCombo->addItem("GRAY");
  m_colorCombo->addItem("NAVY");
  m_colorCombo->addItem("LIME");
  m_colorCombo->addItem("TEAL");
  m_colorCombo->addItem("AQUA");
  m_colorCombo->addItem("MAROON");
  m_colorCombo->addItem("PURPLE");
  m_colorCombo->addItem("OLIVE");
  m_colorCombo->addItem("SILVER");
  m_colorCombo->addItem("FUSHIA");
  m_colorCombo->addItem("DEFAULT");
  lineRow->addWidget(m_colorCombo);
  specialLayout->addLayout(lineRow);

  mainLayout->addWidget(specialGroup);

  /* ===== ACTION BUTTONS ===== */
  auto *btnLayout = new QHBoxLayout();
  btnLayout->setSpacing(6);
  auto *applyBtn = new QPushButton(tr("Apply"));
  auto *closeBtn = new QPushButton(tr("Close"));
  btnLayout->addStretch();
  btnLayout->addWidget(applyBtn);
  btnLayout->addWidget(closeBtn);
  mainLayout->addLayout(btnLayout);

  /* Connections */
  connect(applyBtn, &QPushButton::clicked, this, &GraphControlsDialog::on_apply);
  connect(closeBtn, &QPushButton::clicked, this, &GraphControlsDialog::on_close);
  connect(m_setSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &GraphControlsDialog::on_set_changed);
  connect(m_byValueCheck, &QCheckBox::toggled, this, [this](bool checked) {
    m_byValueEnabled = checked;
    m_numSpin->setEnabled(checked);
  });

  /* Auto-range: apply on toggle */
  if (m_autoXCheck->isChecked())
    auto_x_range();
  if (m_autoYCheck->isChecked())
    auto_y_range();
}

void GraphControlsDialog::on_auto_x_toggled(bool checked)
{
  if (checked)
  {
    auto_x_range();
    m_xminSpin->setReadOnly(true);
    m_xmaxSpin->setReadOnly(true);
  } else
  {
    m_xminSpin->setReadOnly(false);
    m_xmaxSpin->setReadOnly(false);
  }
}

void GraphControlsDialog::on_auto_y_toggled(bool checked)
{
  if (checked)
  {
    auto_y_range();
    m_yminSpin->setReadOnly(true);
    m_ymaxSpin->setReadOnly(true);
  } else
  {
    m_yminSpin->setReadOnly(false);
    m_ymaxSpin->setReadOnly(false);
  }
}

/* Helper: count Y data sets in set_list, skipping the first X entry.
   Also handles old API graphs where there is no separate X entry. */
static gint graph_count_y_sets(struct graph_pak *graph);

void GraphControlsDialog::syncFromGraph()
{
  if (!m_graph)
    return;

  /* Titles */
  m_titleEdit->setText(m_graph->title ? QString::fromUtf8(m_graph->title) : QString());
  m_subTitleEdit->setText(m_graph->sub_title ? QString::fromUtf8(m_graph->sub_title) : QString());
  m_xTitleEdit->setText(m_graph->x_title ? QString::fromUtf8(m_graph->x_title) : QString());
  m_yTitleEdit->setText(m_graph->y_title ? QString::fromUtf8(m_graph->y_title) : QString());

  /* Font sizes */
  m_titleFontSize->setValue(m_graph->title_font);
  m_subTitleFontSize->setValue(m_graph->sub_title_font);
  m_xTitleFontSize->setValue(m_graph->x_title_font);
  m_yTitleFontSize->setValue(m_graph->y_title_font);

  /* Dimensions */
  m_xminSpin->setValue(m_graph->xmin);
  m_xmaxSpin->setValue(m_graph->xmax);
  m_yminSpin->setValue(m_graph->ymin);
  m_ymaxSpin->setValue(m_graph->ymax);
  m_xticsSpin->setValue(m_graph->xticks);
  m_yticsSpin->setValue(m_graph->yticks);

  /* Set spin range: count Y sets, skip X entry */
  gint y_count = graph_count_y_sets(m_graph);
  if (y_count < 1) y_count = 1;
  m_setSpin->setMaximum(y_count);
  m_setSpin->setValue(1);

  /* Size */
  m_sizeEdit->setText("1");

  /* Type */
  m_typeCombo->setCurrentIndex(type_to_combo(m_graph->type));

  /* By value */
  m_byValueCheck->setChecked(m_byValueEnabled);
  m_numSpin->setEnabled(m_byValueEnabled);
}

/* Helper: count Y data sets in set_list, skipping the first X entry.
   Also handles old API graphs where there is no separate X entry. */
static gint graph_count_y_sets(struct graph_pak *graph)
{
  if (!graph || !graph->set_list)
    return 0;

  GSList *list = graph->set_list;
  gint idx = 0;
  gpointer first = list->data;

  /* Check if the first entry is a proper g_data_x (new API). */
  gboolean has_x = FALSE;
  if (first)
  {
    g_data_x *test_gx = (g_data_x *) first;
    if (test_gx->x != NULL && test_gx->x_size > 0)
      has_x = TRUE;
  }

  gint count = 0;
  if (has_x)
  {
    /* Skip X entry */
    list = g_slist_next(list);
  }
  while (list)
  {
    gpointer item = list->data;
    if (!item)
    {
      list = g_slist_next(list);
      continue;
    }
    /* Try as g_data_y first */
    g_data_y *dy = (g_data_y *) item;
    if (dy && dy->y != NULL && dy->y_size > 0)
    {
      count++;
    }
    else
    {
      /* Old API: raw gdouble* — still counts as a Y set */
      count++;
    }
    list = g_slist_next(list);
  }
  return count;
}

void GraphControlsDialog::on_set_changed(int setNum)
{
  if (!m_graph || !m_graph->set_list)
    return;

  GSList *list = m_graph->set_list;
  gint y_count = graph_count_y_sets(m_graph);
  /* setNum is 1-based (spinbox default=1). Convert to 0-based index. */
  if (setNum < 1 || setNum > y_count)
    return;

  int idx = 0;
  g_data_y *p_y = nullptr;

  /* Skip past the first entry if it's X data */
  gpointer first = list->data;
  g_data_x *test_gx = (g_data_x *) first;
  if (test_gx && test_gx->x != NULL && test_gx->x_size > 0)
    list = g_slist_next(list);

  /* Advance to the Y set we want */
  while (list && idx < setNum - 1)
  {
    list = g_slist_next(list);
    idx++;
  }
  if (!list)
    return;

  p_y = (g_data_y *) list->data;

  if (p_y)
  {
    m_sizeEdit->setText(QString::number(p_y->y_size));
    m_setSpin->setMaximum(p_y->y_size);
  }

  if (p_y)
  {
    m_lineCombo->setCurrentIndex(line_to_combo(p_y->line));
    m_colorCombo->setCurrentIndex(color_to_combo(p_y->color));
  }

  if (p_y && p_y->symbol)
  {
    if (p_y->mixed_symbol && p_y->y_size > 0)
    {
      int symIdx = 0;
      for (int i = 0; i < p_y->y_size; i++)
      {
        if (p_y->symbol[i] != GRAPH_SYMB_NONE)
        {
          symIdx = symbol_to_combo(p_y->symbol[i]);
          break;
        }
      }
      m_symbolCombo->setCurrentIndex(symIdx);
    } else
    {
      m_symbolCombo->setCurrentIndex(symbol_to_combo(p_y->symbol[0]));
    }
  }
  else if (p_y)
  {
    /* symbol array not allocated — default to none */
    m_symbolCombo->setCurrentIndex(0);
  }
}

void GraphControlsDialog::applyToGraph()
{
  if (!m_graph)
    return;

  /* Titles */
  if (m_graph->title)
    g_free(m_graph->title);
  const char *title = m_titleEdit->text().toUtf8().constData();
  m_graph->title = title[0] ? g_strdup(title) : nullptr;

  if (m_graph->sub_title)
    g_free(m_graph->sub_title);
  const char *subTitle = m_subTitleEdit->text().toUtf8().constData();
  m_graph->sub_title = subTitle[0] ? g_strdup(subTitle) : nullptr;

  if (m_graph->x_title)
    g_free(m_graph->x_title);
  const char *xTitle = m_xTitleEdit->text().toUtf8().constData();
  m_graph->x_title = xTitle[0] ? g_strdup(xTitle) : nullptr;

  if (m_graph->y_title)
    g_free(m_graph->y_title);
  const char *yTitle = m_yTitleEdit->text().toUtf8().constData();
  m_graph->y_title = yTitle[0] ? g_strdup(yTitle) : nullptr;

  /* Font sizes (0 = use default gl_fontsize) */
  m_graph->title_font = m_titleFontSize->value();
  m_graph->sub_title_font = m_subTitleFontSize->value();
  m_graph->x_title_font = m_xTitleFontSize->value();
  m_graph->y_title_font = m_yTitleFontSize->value();

  /* Dimensions */
  m_graph->xmin = m_xminSpin->value();
  m_graph->xmax = m_xmaxSpin->value();
  m_graph->ymin = m_yminSpin->value();
  m_graph->ymax = m_ymaxSpin->value();
  m_graph->xticks = m_xticsSpin->value();
  m_graph->yticks = m_yticsSpin->value();

  /* Force canvas refresh + tree refresh */
  qt_force_canvas_refresh();
  if (m_model)
  {
    redraw_canvas(ALL);
  }
}

void GraphControlsDialog::on_apply() { applyToGraph(); }

void GraphControlsDialog::on_close()
{
  applyToGraph();
  close();
}

/* Bridge function */
extern "C" void qt_show_graph_controls_dialog(void)
{
  extern struct sysenv_pak sysenv;
  extern QWidget *get_main_window_widget();

  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (!model)
    return;
  if (!model->graph_active)
    return;

  struct graph_pak *graph = (struct graph_pak *) model->graph_active;

  GraphControlsDialog *dlg = new GraphControlsDialog(graph, model, get_main_window_widget());
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->show();
}
