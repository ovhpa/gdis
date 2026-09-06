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

#ifndef  SVG_UTILS_H
#define  SVG_UTILS_H

#include <QPixmap>
#include <QIcon>
#include <QSize>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QString>
#include <QWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <functional>

QIcon LoadIconSVG(const char* svgData, const QSize& targetSize = QSize(16, 16));
QIcon loadGdisIcon(const char *embedded_name);

/* Create a label + icon button row for QVBoxLayout */
void add_action_button(QVBoxLayout *layout, const QString &label_text, std::function<void()> callback);

/*
 * Create a |label ... widget| row with consistent formatting:
 *   - Label is left-aligned, fixed-width (default 120px)
 *   - Widget is left-aligned, takes remaining space
 *   - Returns the QHBoxLayout* so caller can add more widgets
 */
QHBoxLayout *add_label_spinner(QVBoxLayout *parent, const QString &label, QSpinBox *spin, int labelWidth = 120);
QHBoxLayout *add_label_spinner(QVBoxLayout *parent, const QString &label, QDoubleSpinBox *spin, int labelWidth = 120);
QHBoxLayout *add_label_spinner(QVBoxLayout *parent, const QString &label, QComboBox *combo, int labelWidth = 120);

/*
 * Create a |label ... text_field| row.
 * Returns the QHBoxLayout*.
 */
QHBoxLayout *add_label_edit(QVBoxLayout *parent, const QString &label, QLineEdit *edit, int labelWidth = 120);

/*
 * Create a |label ... checkbox| row.
 * Returns the QHBoxLayout*.
 */
QHBoxLayout *add_label_check(QVBoxLayout *parent, const QString &label, QCheckBox *check, int labelWidth = 120);

/*
 * Create a standalone row with just a widget stretched across (no label).
 */
QHBoxLayout *add_stretch_widget(QVBoxLayout *parent, QWidget *widget);

/*
 * Set a tooltip on any QWidget. Converts UTF-8 C string to QString.
 */
void set_widget_tooltip(QWidget *widget, const char *tooltip);


#endif //SVG_UTILS_H
