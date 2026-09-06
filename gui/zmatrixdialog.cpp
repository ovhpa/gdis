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
 * Z-matrix Editor Dialog for GDIS Qt6 GUI
 */

#include "zmatrixdialog.h"
#include "gdis_api.h"

#ifndef G_DISABLE_SINGLE_INCLUDES
#define G_DISABLE_SINGLE_INCLUDES
#endif
#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "gdis.h"
#include "zmatrix.h"
#include "zmatrix_pak.h"
#include "parse.h"
#include "coords.h"
#include "zone.h"
#include "edit.h"
#ifdef __cplusplus
}
#endif
#include "matrix.h"


#include "renderdialog.h"

ZMatrixDialog::ZMatrixDialog(struct model_pak *model, QWidget *parent)
  : QDialog(parent), m_model(model), m_zmat((struct zmat_pak *) model->zmatrix)
{
  setWindowTitle("Z-matrix Editor");
  setMinimumWidth(600);
  setMinimumHeight(500);

  auto *mainLayout = new QVBoxLayout(this);

  /* === Units information === */
  auto *unitsGroup = new QGroupBox(this);
  auto *unitsLayout = new QHBoxLayout(unitsGroup);

  QLabel *distLabel = new QLabel("Distance units: ", unitsGroup);
  unitsLayout->addWidget(distLabel);
  QLabel *distVal = new QLabel(QString::fromUtf8(m_zmat->distance_units), unitsGroup);
  unitsLayout->addWidget(distVal);

  QLabel *angleLabel = new QLabel("Angle units: ", unitsGroup);
  unitsLayout->addWidget(angleLabel);
  QLabel *angleVal = new QLabel(QString::fromUtf8(m_zmat->angle_units), unitsGroup);
  unitsLayout->addWidget(angleVal);

  mainLayout->addWidget(unitsGroup);

  /* === Z-matrix text display === */
  auto *textGroup = new QGroupBox("Z-matrix", this);
  auto *textLayout = new QVBoxLayout(textGroup);

  m_textEdit = new QTextEdit(this);
  m_textEdit->setReadOnly(true);
  textLayout->addWidget(m_textEdit);

  mainLayout->addWidget(textGroup, 1); /* stretch */

  /* === Line selector and value editors === */
  auto *editGroup = new QGroupBox("Edit z-matrix values", this);
  auto *editLayout = new QHBoxLayout(editGroup);

  QLabel *lineLabel = new QLabel("Line:", editGroup);
  editLayout->addWidget(lineLabel);

  m_lineSpin = new QSpinBox(this);
  m_lineSpin->setRange(1, 999);
  m_lineSpin->setValue(1);
  editLayout->addWidget(m_lineSpin);

  for (int i = 0; i < 3; i++)
  {
    QLabel *valLabel = new QLabel(QString("V%1:").arg(i + 1), editGroup);
    editLayout->addWidget(valLabel);

    m_valueEntry[i] = new QLineEdit(this);
    editLayout->addWidget(m_valueEntry[i]);

    QObject::connect(m_valueEntry[i], &QLineEdit::textChanged, [this, i]() { on_value_entry_changed(i); });
  }

  QObject::connect(m_lineSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ZMatrixDialog::on_line_changed);

  mainLayout->addWidget(editGroup);

  /* === Variable editing section === */
  auto *varGroup = new QGroupBox("Variables", this);
  varGroup->setVisible(false); /* hidden until we know there are variables */
  auto *varLayout = new QHBoxLayout(varGroup);

  QLabel *varNameLabel = new QLabel("Variable:", varGroup);
  varLayout->addWidget(varNameLabel);

  m_variableCombo = new QComboBox(this);
  varLayout->addWidget(m_variableCombo);

  QObject::connect(m_variableCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                   &ZMatrixDialog::on_variable_name_changed);

  QLabel *varValLabel = new QLabel("Value:", varGroup);
  varLayout->addWidget(varValLabel);

  m_variableValueEntry = new QLineEdit(this);
  varLayout->addWidget(m_variableValueEntry);

  QObject::connect(m_variableValueEntry, &QLineEdit::textChanged, this, &ZMatrixDialog::on_variable_value_changed);

  mainLayout->addWidget(varGroup);

  /* === Buttons === */
  auto *btnLayout = new QHBoxLayout();
  m_recomputeBtn = new QPushButton("Recompute geometry", this);
  QObject::connect(m_recomputeBtn, &QPushButton::clicked, this, &ZMatrixDialog::on_recompute_geometry);
  btnLayout->addWidget(m_recomputeBtn);

  m_buildBtn = new QPushButton("Build z-matrix from selection", this);
  QObject::connect(m_buildBtn, &QPushButton::clicked, this, &ZMatrixDialog::on_build_from_selection);
  btnLayout->addWidget(m_buildBtn);

  mainLayout->addLayout(btnLayout);

  /* Populate initial data */
  populate_text_edit();
  update_line_entries();
  update_variable_combo();
}

void ZMatrixDialog::populate_text_edit()
{
  /* Re-fetch zmatrix pointer in case model was rebuilt */
  m_zmat = (struct zmat_pak *) m_model->zmatrix;

  if (!m_zmat || !m_zmat->zlines)
  {
    m_textEdit->setPlainText("(empty z-matrix)");
    return;
  }

  GString *buff = g_string_new(NULL);
  gint n = 1;

  for (GSList *list = m_zmat->zlines; list; list = g_slist_next(list))
  {
    struct zval_pak *zval = (struct zval_pak *) list->data;

    g_string_append_printf(buff, "[%d]  %s  %d %d %d", n++, zval->elem, zval->connect[0], zval->connect[1],
                           zval->connect[2]);
    for (int i = 0; i < 3; i++)
    {
      if (zval->name[i])
        g_string_append_printf(buff, "  %-9s", (gchar *) zval->name[i]);
      else
        g_string_append_printf(buff, "  %-9.4f", zval->value[i]);
    }
    g_string_append(buff, "\n");
  }

  m_textEdit->setPlainText(QString(buff->str));
  g_string_free(buff, TRUE);
}

void ZMatrixDialog::update_line_entries()
{
  /* Re-fetch zmatrix pointer in case model was rebuilt */
  m_zmat = (struct zmat_pak *) m_model->zmatrix;

  if (!m_zmat || !m_zmat->zlines)
    return;

  gint n = m_lineSpin->value();
  struct zval_pak *zval = (struct zval_pak *) g_slist_nth_data(m_zmat->zlines, n - 1);

  if (!zval)
    return;

  /* Block signals while setting entry values to avoid recursive changes */
  for (int i = 0; i < 3; i++)
  {
    m_valueEntry[i]->blockSignals(true);
    if (zval->name[i])
      /* Variable name — show the variable name, disable editing */
      m_valueEntry[i]->setText(QString((gchar *) zval->name[i]));
    else
      /* Numeric value */
      m_valueEntry[i]->setText(QString::number(zval->value[i], 'f', 6));

    /* Check if this entry holds a variable name (non-numeric) */
    bool is_variable = str_is_float(m_valueEntry[i]->text().toUtf8().constData()) == 0;
    m_valueEntry[i]->setReadOnly(is_variable);
    m_valueEntry[i]->blockSignals(false);
  }
}

void ZMatrixDialog::sync_entry_to_zval(int idx)
{
  /* Re-fetch zmatrix pointer in case model was rebuilt */
  m_zmat = (struct zmat_pak *) m_model->zmatrix;

  if (!m_zmat || !m_zmat->zlines)
    return;

  gint n = m_lineSpin->value();
  struct zval_pak *zval = (struct zval_pak *) g_slist_nth_data(m_zmat->zlines, n - 1);

  if (!zval || idx < 0 || idx > 2)
    return;

  const char *text = m_valueEntry[idx]->text().toUtf8().constData();
  if (str_is_float(text))
    zval->value[idx] = str_to_float(text);
}

void ZMatrixDialog::apply_entry_changes()
{
  /* Rebuild the text display with updated values */
  populate_text_edit();
}

void ZMatrixDialog::update_variable_combo()
{
  /* Re-fetch zmatrix pointer in case model was rebuilt */
  m_zmat = (struct zmat_pak *) m_model->zmatrix;

  /* Collect variable names from zmatrix hash table */
  m_variableCombo->clear();
  if (!m_zmat || !m_zmat->vars)
  {
    GList *keys = NULL;
    g_hash_table_foreach(m_zmat->vars,
                         [](gpointer key, gpointer val, gpointer user_data) {
                           GList **list = (GList **) user_data;
                           *list = g_list_append(*list, key);
                         },
                         &keys);

    for (GList *l = keys; l; l = g_list_next(l))
      m_variableCombo->addItem(QString((gchar *) l->data));

    g_list_free(keys);
  }

  /* Show/hide variable editing section */
  auto *varGroup = this->findChild<QGroupBox *>("Variables");
  if (varGroup)
    varGroup->setVisible(m_variableCombo->count() > 0);
}

void ZMatrixDialog::on_line_changed() { update_line_entries(); }

void ZMatrixDialog::on_value_entry_changed(int idx)
{
  sync_entry_to_zval(idx);
  /* Update the text display to reflect the change */
  populate_text_edit();
}

void ZMatrixDialog::on_variable_name_changed()
{
  /* Re-fetch zmatrix pointer in case model was rebuilt */
  m_zmat = (struct zmat_pak *) m_model->zmatrix;

  const char *name = m_variableCombo->currentText().toUtf8().constData();
  if (!m_zmat || !m_zmat->vars) return;
  {
    const char *val = (const char *) g_hash_table_lookup(m_zmat->vars, name);
    if (val)
      m_variableValueEntry->setText(QString(val));
  }
}

void ZMatrixDialog::on_variable_value_changed()
{
  /* Re-fetch zmatrix pointer in case model was rebuilt */
  m_zmat = (struct zmat_pak *) m_model->zmatrix;

  const char *name = m_variableCombo->currentText().toUtf8().constData();
  const char *val = m_variableValueEntry->text().toUtf8().constData();
  if (m_zmat && m_zmat->vars && name[0] != '\0')
    g_hash_table_insert(m_zmat->vars, g_strdup(name), g_strdup(val));
}

void ZMatrixDialog::on_recompute_geometry()
{
  /* Re-fetch zmatrix pointer in case model was rebuilt */
  m_zmat = (struct zmat_pak *) m_model->zmatrix;

  if (!m_zmat || !m_zmat->zcores)
    return;

  /* Delete old zmatrix cores and recompute from scratch */
  for (GSList *list = m_zmat->zcores; list; list = g_slist_next(list))
  {
    struct core_pak *core = (struct core_pak *) list->data;
    if (!core) continue;
    delete_core(core);
  }
  delete_commit(m_model);
  g_slist_free(m_zmat->zcores);
  m_zmat->zcores = NULL;

  /* Recompute zmatrix core list */
  zmat_process(m_zmat, m_model);

  /* If model is periodic but not fractional, convert to lattice coordinates */
  if (!m_model->fractional && m_model->periodic)
  {
    for (GSList *list = m_zmat->zcores; list; list = g_slist_next(list))
    {
      struct core_pak *core = (struct core_pak *) list->data;
      vecmat(m_model->ilatmat, core->x);
    }
  }

  /* Assign element data to cores */
  for (GSList *list = m_zmat->zcores; list; list = g_slist_next(list))
    elem_init((struct core_pak *) list->data, m_model);

  /* Refresh coords/connectivity/region */
  zone_init(m_model);
  coords_compute(m_model);
  connect_refresh(m_model);

  /* Force canvas redraw */
  m_model->need_clear = TRUE;
  redraw_canvas(ALL);

  /* Update the text display */
  populate_text_edit();
}

void ZMatrixDialog::on_build_from_selection()
{
  zmat_build();
  populate_text_edit();
  update_line_entries();
  update_variable_combo();
}
