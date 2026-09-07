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
 * Qt6 main entry point for GDIS
 */

/* Auto-detect and set QPA platform before QApplication is created */
#include <QCoreApplication>
#include <QProcessEnvironment>
#include <QLoggingCategory>

#include <QApplication>
#include <QCommandLineParser>
#include <QLocale>
#include <QTranslator>
#include <QDir>
#include <QFileInfo>

#include "mainwindow.h"
#include "gdis_api.h"

#include "gdis.h"

extern "C" void task_queue_init(void);

/* Kill confirmation callback type and setter */
extern "C" {
typedef gboolean (*confirm_kill_func)(gint *pids, gint count);
void task_set_confirm_callback(confirm_kill_func cb);
}

extern struct sysenv_pak sysenv;

/* Forward declarations for C core init */
extern void sys_init(int argc, char *argv[]);
extern "C" {
extern void sys_free(void);
extern gint read_gdisrc(void);
extern void module_setup(void);
}
extern void command_main_loop(int argc, char *argv[]);

/* CLI-only mode: no GUI */
static void run_cli(int argc, char *argv[]) { command_main_loop(argc, argv); }

int main(int argc, char *argv[])
{
  /* Auto-detect QPA platform: native on macOS (cocoa), xcb for X11, offscreen otherwise */
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  QString qpa = env.value("QT_QPA_PLATFORM");
  if (qpa.isEmpty())
  {
#if defined(Q_OS_MAC)
    /* macOS native platform — cocoa is the default and best option */
    qputenv("QT_QPA_PLATFORM", "cocoa");
#elif defined(Q_OS_WIN)
    /* Windows — use the default (windows) platform plugin */
#else
    /* Linux/Unix: check for X11 display, prefer xcb for OpenGL support */
    QString display = env.value("DISPLAY");
    if (!display.isEmpty())
    {
      qputenv("QT_QPA_PLATFORM", "xcb");
    } else
    {
      /* No display — use offscreen (headless/server mode) */
      qputenv("QT_QPA_PLATFORM", "offscreen");
    }
#endif
  } else
  {
    /* User already set it — respect their choice */
    qputenv("QT_QPA_PLATFORM", qpa.toUtf8().constData());
  }

  QApplication app(argc, argv);
  app.setApplicationName("GDIS");
  app.setApplicationVersion("1.0");
  app.setOrganizationName("GDIS");

  /* Parse command line */
  QCommandLineParser parser;
  parser.addPositionalArgument("file", "File to load");
  parser.addOption(QCommandLineOption("cli", "Run in CLI mode (no GUI)"));
  parser.process(app);

  /* Initialize system */
  sys_init(argc, argv);

  /* Set init_file path so read_gdisrc() can find ~/.gdisrc */
  extern struct sysenv_pak sysenv;
  const gchar *ctemp = g_get_home_dir();
  if (ctemp)
    sysenv.init_file = g_build_filename(ctemp, INIT_FILE, NULL);
  else if (sysenv.gdis_path)
    sysenv.init_file = g_build_filename(sysenv.gdis_path, INIT_FILE, NULL);
  else if (sysenv.cwd)
    sysenv.init_file = g_build_filename(sysenv.cwd, INIT_FILE, NULL);

  /* Read config */
  read_gdisrc();

  /* Setup modules */
  module_setup();

  /* Initialize task queue (thread pool) — must be before any task_new() calls */
  task_queue_init();

  /* Register kill confirmation callback */
  extern void task_set_confirm_callback(confirm_kill_func cb);
  extern gboolean qt_confirm_kill(gint * pids, gint count);
  task_set_confirm_callback(qt_confirm_kill);

  /* Create main window FIRST so output callback is available during file loading */
  MainWindow window;

  /* Show greeting messages */
  extern void gui_text_show(gint type, const gchar *msg);
  gui_text_show(INFO, "This is free software, distributed under the terms of the GNU public license (GPL).\nFor more "
                      "information visit http://www.gnu.org/\n");
  gui_text_show(
      STANDARD,
      g_strdup_printf(
          "Welcome to GDIS version %4.2f.%d (%d), brought to you by Sean Fleming, Okadome Valencia, and Andrew Rohl\n",
          VERSION, PATCH, YEAR));

  /* Load files from command line */
  for (int i = 1; i < argc; i++)
  {
    QFileInfo fi(argv[i]);
    if (fi.exists())
    {
      file_load(argv[i], NULL);
    }
  }

  /* Refresh content table after loading */
  extern void qt_update_content_table(void);
  qt_update_content_table();

  /* Check if CLI mode */
  if (parser.isSet("cli"))
  {
    run_cli(argc, argv);
    sys_free();
    return 0;
  }

  window.show();

  /* Refresh tree after all models are loaded */
  extern void qt_tree_refresh(void);
  qt_tree_refresh();

  /* Refresh the Qt tree view */
  window.refresh_qt_tree();

  return app.exec();
}
