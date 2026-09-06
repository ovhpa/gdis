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
 * Executable Locations Dialog for GDIS Qt6 GUI
 */

#include "execpathsdialog.h"
#include "gdis_api.h"

#include <QApplication>

/* Path definitions — label, default executable, and accessor functions */
struct PathDef {
  const char *label;
  const char *default_exe;
  const char *(*getter)(void);
  void (*setter)(const char *);
};

static const PathDef PATH_DEFS[] = {
    {"Babel", "babel", qt_get_babel_path, qt_set_babel_path},
    {"GAMESS", "rungms", qt_get_gamess_path, qt_set_gamess_path},
    {"VASP", "vasp", qt_get_vasp_path, qt_set_vasp_path},
    {"USPEX", "uspex", qt_get_uspex_path, qt_set_uspex_path},
    {"MPIRUN", "mpirun", qt_get_mpirun_path, qt_set_mpirun_path},
    {"GULP", "gulp", qt_get_gulp_path, qt_set_gulp_path},
    {"Monty", "monty", qt_get_monty_path, qt_set_monty_path},
    {"POVRay", "povray", qt_get_povray_path, qt_set_povray_path},
    {"Image viewer", "display", qt_get_viewer_path, qt_set_viewer_path},
};
static const int PATH_COUNT = sizeof(PATH_DEFS) / sizeof(PATH_DEFS[0]);

ExecPathsDialog::ExecPathsDialog(QWidget *parent) : QDialog(parent)
{
  setWindowTitle(tr("Executable locations"));
  resize(640, 400);

  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(8, 6, 8, 6);
  mainLayout->setSpacing(6);

  /* Group box for all path entries */
  auto *groupBox = new QGroupBox(this);
  auto *formLayout = new QFormLayout(groupBox);
  formLayout->setSpacing(8);

  for (int i = 0; i < PATH_COUNT; i++)
  {
    auto *hLayout = new QHBoxLayout();
    auto *edit = new QLineEdit();
    edit->setText(QString::fromUtf8(PATH_DEFS[i].getter()));
    hLayout->addWidget(edit);

    auto *browseBtn = new QPushButton(tr("Browse"), groupBox);
    auto *clearBtn = new QPushButton(tr("Clear"), groupBox);

    hLayout->addWidget(browseBtn);
    hLayout->addWidget(clearBtn);

    formLayout->addRow(PATH_DEFS[i].label, hLayout);

    /* Connect browse button */
    connect(browseBtn, &QPushButton::clicked, this, [this, i, edit]() {
      QString fileName = QFileDialog::getOpenFileName(this, tr("Select %1 executable").arg(PATH_DEFS[i].label),
                                                      edit->text(), tr("Executable (*)"));

      if (fileName.isEmpty())
        return;

      PATH_DEFS[i].setter(fileName.toUtf8().constData());
      edit->setText(fileName);
    });

    /* Connect clear button */
    connect(clearBtn, &QPushButton::clicked, this, [this, i, edit]() {
      PATH_DEFS[i].setter(nullptr);
      edit->setText(QString::fromUtf8(PATH_DEFS[i].default_exe));
    });
  }

  mainLayout->addWidget(groupBox);

  /* Close button */
  auto *btnLayout = new QHBoxLayout();
  btnLayout->addStretch();
  auto *closeBtn = new QPushButton(tr("Close"), this);
  btnLayout->addWidget(closeBtn);
  mainLayout->addLayout(btnLayout);

  connect(closeBtn, &QPushButton::clicked, this, &ExecPathsDialog::on_close);
}

void ExecPathsDialog::on_browse(int index)
{
  Q_UNUSED(index);
  /* Not used — inline lambdas handle browsing */
}

void ExecPathsDialog::on_clear(int index)
{
  Q_UNUSED(index);
  /* Not used — inline lambdas handle clearing */
}

void ExecPathsDialog::on_close() { close(); }

/* Bridge function — called from mainwindow.cpp */
extern "C" void qt_show_exec_paths_dialog(void)
{
  extern QWidget *get_main_window_widget();
  ExecPathsDialog *dlg = new ExecPathsDialog(get_main_window_widget());
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->show();
  dlg->raise();
  dlg->activateWindow();
}
