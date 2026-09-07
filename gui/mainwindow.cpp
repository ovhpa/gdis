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
 * Main Window for GDIS Qt6 GUI
 */

#include "mainwindow.h"
#include <QTimer>
#include "glcanvas.h"
#include "textoverlay.h"
#include "gdis_api.h"
#include "renderdialog.h"
#include "animatedialog.h"
#include "editdialog.h"
#include "zmatrixdialog.h"

#include "gdis.h"
#include "interface.h"
#include "coords.h"
#include "model.h"
// #include "matrix.h"
// #include "quaternion.h"
#include "render.h"

#include <QColorDialog>
#include <QHash>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QItemDelegate>
#include <QKeyEvent>
#include <QStackedWidget>
#include <glib.h>

/* Global pointer to GL canvas and main window for C-side access */
static GLCanvas *g_gl_canvas = nullptr;

/* sysenv.active_model is a gpointer */
extern struct sysenv_pak sysenv;
static MainWindow *g_main_window = nullptr;

/* Forward declarations */
extern "C" void qt_update_edit_panel_qt();
extern "C" void qt_show_surface_dialog();
extern "C" void qt_mdi_dialog(void);
extern void camera_init(struct model_pak *model);

MainWindow *get_main_window() { return g_main_window; }

extern "C" QWidget *get_main_window_widget() { return g_main_window; }

/* Image panel spinners — accessible from qt_api.c */
QSpinBox *image_spinners[6];

/* Symmetry panel table — accessible from qt_api.c */
QTableWidget *g_symmetry_table = nullptr;

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QSplitter>
#include <QTreeView>
#include <QStandardItemModel>
#include <QPlainTextEdit>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QIcon>
#include <QDir>
#include <QSpinBox>
#include <QAtomicInt>
#include <QTimer>
#include <QCoreApplication>

/* Global state for edit panel cellChanged handler. */
static QTableWidget *s_edit_table = nullptr;
static QAtomicInt s_applying_edit_local;
static QAtomicInt *s_applying_edit_ptr = &s_applying_edit_local;

extern "C" void sys_free(void);

/* Import functions from C core — need C linkage for C++ compilation. */
extern "C" void import_off(gchar *filename);
extern "C" void import_pcf(gchar *filename);

#include "svg_utils.h"

/* Icon cache — avoids re-parsing XPM data on every tree rebuild */
static QHash<QString, QIcon> s_icon_cache;

/* Wrapper for shared icon loading with caching */
static QIcon loadIcon(const QString &name, QWidget *widget)
{
  if (s_icon_cache.contains(name))
    return s_icon_cache[name];
  QIcon icon = loadGdisIcon(name.toUtf8().constData());
  /*
  QPixmap pixmap = icon.pixmap(QSize());// or force 16x16?
  pixmap.setDevicePixelRatio(widget->devicePixelRatioF());
  */
  if (!icon.isNull())
    s_icon_cache[name] = icon;
  return icon;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_canvas(nullptr), m_angleSpin(nullptr), m_splitter(nullptr), m_modelTree(nullptr),
      m_textOutput(nullptr), m_panelStack(nullptr)
{
  setWindowTitle("GDIS - Graphical Display");
  /* Use size from gdisrc if available */
  extern struct sysenv_pak sysenv;
  /* Debug: check if gdisrc values are valid */
  if (sysenv.width > 100 && sysenv.height > 100)
    resize((int) sysenv.width, (int) sysenv.height);
  else
    resize(800, 600);

  setupMenuBar();
  setupToolBar();
  setupCentralWidget();
  setupDockWidgets();
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent *event)
{
  /* Save window size to gdisrc */
  extern struct sysenv_pak sysenv;
  extern gint write_gdisrc(void);
  sysenv.width = (gdouble) width();
  sysenv.height = (gdouble) height();
  sysenv.write_gdisrc = TRUE;
  write_gdisrc();

  extern void sys_free(void);
  sys_free();
  event->accept();
}

void MainWindow::setupMenuBar()
{
  QMenuBar *menuBar = this->menuBar();

  /* ===== File menu ===== */
  QMenu *fileMenu = menuBar->addMenu(tr("&File"));

  QAction *newAction = new QAction(tr("&New"), this);
  newAction->setShortcut(QKeySequence::New);
  connect(newAction, &QAction::triggered, this, &MainWindow::on_file_new);
  fileMenu->addAction(newAction);

  QAction *openAction = new QAction(tr("&Open..."), this);
  openAction->setShortcut(QKeySequence::Open);
  connect(openAction, &QAction::triggered, this, &MainWindow::on_file_open);
  fileMenu->addAction(openAction);

  QAction *saveAction = new QAction(tr("&Save..."), this);
  saveAction->setShortcut(QKeySequence::Save);
  connect(saveAction, &QAction::triggered, this, &MainWindow::on_file_save);
  fileMenu->addAction(saveAction);

  QAction *closeAction = new QAction(tr("&Close"), this);
  closeAction->setShortcut(QKeySequence::Close);
  connect(closeAction, &QAction::triggered, this, &MainWindow::on_file_close);
  fileMenu->addAction(closeAction);

  fileMenu->addSeparator();

  /* Import submenu */
  QMenu *importMenu = fileMenu->addMenu(tr("&Import"));
  QAction *importGeomview = new QAction(tr("Geomview..."), this);
  connect(importGeomview, &QAction::triggered, this, &MainWindow::on_import_geomview);
  importMenu->addAction(importGeomview);
  QAction *importProject = new QAction(tr("Project..."), this);
  connect(importProject, &QAction::triggered, this, &MainWindow::on_import_project);
  importMenu->addAction(importProject);
  QAction *importGraph = new QAction(tr("Graph..."), this);
  connect(importGraph, &QAction::triggered, this, &MainWindow::on_import_graph);
  importMenu->addAction(importGraph);
  /* Export submenu */
  QMenu *exportMenu = fileMenu->addMenu(tr("&Export"));
  QAction *exportCanvas = new QAction(tr("Canvas snapshot..."), this);
  connect(exportCanvas, &QAction::triggered, this, &MainWindow::on_export_canvas);
  exportMenu->addAction(exportCanvas);
  QAction *exportGraphData = new QAction(tr("Graph data..."), this);
  connect(exportGraphData, &QAction::triggered, this, &MainWindow::on_export_graph_data);
  exportMenu->addAction(exportGraphData);

  fileMenu->addSeparator();

  QAction *quitAction = new QAction(tr("E&xit"), this);
  quitAction->setShortcut(QKeySequence::Quit);
  connect(quitAction, &QAction::triggered, this, &MainWindow::on_quit);
  fileMenu->addAction(quitAction);

  /* ===== Edit menu ===== */
  QMenu *editMenu = menuBar->addMenu(tr("&Edit"));

  QAction *undoAction = new QAction(tr("Undo"), this);
  undoAction->setShortcut(QKeySequence::Undo);
  connect(undoAction, &QAction::triggered, this, &MainWindow::on_edit_undo);
  editMenu->addAction(undoAction);

  QAction *copyAction = new QAction(tr("&Copy"), this);
  copyAction->setShortcut(QKeySequence::Copy);
  connect(copyAction, &QAction::triggered, this, &MainWindow::on_edit_copy);
  editMenu->addAction(copyAction);

  QAction *pasteAction = new QAction(tr("&Paste"), this);
  pasteAction->setShortcut(QKeySequence::Paste);
  connect(pasteAction, &QAction::triggered, this, &MainWindow::on_edit_paste);
  editMenu->addAction(pasteAction);

  editMenu->addSeparator();

  QAction *colourAction = new QAction(tr("Colour..."), this);
  connect(colourAction, &QAction::triggered, this, &MainWindow::on_edit_colour);
  editMenu->addAction(colourAction);

  editMenu->addSeparator();

  QAction *deleteAction = new QAction(tr("Delete selected"), this);
  connect(deleteAction, &QAction::triggered, this, &MainWindow::on_edit_delete);
  editMenu->addAction(deleteAction);

  QAction *selectAllAction = new QAction(tr("Select all"), this);
  selectAllAction->setShortcut(QKeySequence::SelectAll);
  connect(selectAllAction, &QAction::triggered, this, &MainWindow::on_edit_select_all);
  editMenu->addAction(selectAllAction);

  QAction *invertSelAction = new QAction(tr("Invert selection"), this);
  invertSelAction->setShortcut(Qt::CTRL | Qt::Key_I);
  connect(invertSelAction, &QAction::triggered, this, &MainWindow::on_edit_invert);
  editMenu->addAction(invertSelAction);

  QAction *hideSelAction = new QAction(tr("Hide selected"), this);
  hideSelAction->setShortcut(Qt::CTRL | Qt::Key_H);
  connect(hideSelAction, &QAction::triggered, this, &MainWindow::on_edit_hide);
  editMenu->addAction(hideSelAction);

  QAction *hideUnselAction = new QAction(tr("Hide unselected"), this);
  hideUnselAction->setShortcut(Qt::CTRL | Qt::Key_U);
  connect(hideUnselAction, &QAction::triggered, this, &MainWindow::on_edit_hide_unsel);
  editMenu->addAction(hideUnselAction);

  QAction *unhideAction = new QAction(tr("Unhide all"), this);
  unhideAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_U);
  connect(unhideAction, &QAction::triggered, this, &MainWindow::on_edit_unhide);
  editMenu->addAction(unhideAction);

  /* ===== Tools menu ===== */
  QMenu *toolsMenu = menuBar->addMenu(tr("&Tools"));

  /* Visualization submenu */
  QMenu *visMenu = toolsMenu->addMenu(tr("Visualization"));
  QAction *animAction = new QAction(tr("Animation..."), this);
  connect(animAction, &QAction::triggered, this, &MainWindow::on_tools_animation);
  visMenu->addAction(animAction);
  QAction *isoAction = new QAction(tr("Iso-surfaces..."), this);
  connect(isoAction, &QAction::triggered, this, &MainWindow::on_tools_isosurfaces);
  visMenu->addAction(isoAction);
  QAction *periodicAction = new QAction(tr("Periodic table..."), this);
  connect(periodicAction, &QAction::triggered, this, &MainWindow::on_tools_periodic_table);
  visMenu->addAction(periodicAction);

  /* Building submenu */
  QMenu *buildMenu = toolsMenu->addMenu(tr("Building"));
  QAction *editAction = new QAction(tr("Editing..."), this);
  editAction->setShortcut(Qt::CTRL | Qt::Key_E);
  connect(editAction, &QAction::triggered, this, &MainWindow::on_tools_editing);
  buildMenu->addAction(editAction);
  QAction *dislocAction = new QAction(tr("Dislocations..."), this);
  connect(dislocAction, &QAction::triggered, this, &MainWindow::on_tools_dislocations);
  buildMenu->addAction(dislocAction);
  QAction *dockAction = new QAction(tr("Docking..."), this);
  connect(dockAction, &QAction::triggered, this, &MainWindow::on_tools_docking);
  buildMenu->addAction(dockAction);
  QAction *dynamicsAction = new QAction(tr("Dynamics..."), this);
  connect(dynamicsAction, &QAction::triggered, this, &MainWindow::on_tools_dynamics);
  buildMenu->addAction(dynamicsAction);
  QAction *surfacesAction = new QAction(tr("Surfaces..."), this);
  connect(surfacesAction, &QAction::triggered, this, &MainWindow::on_tools_surfaces);
  buildMenu->addAction(surfacesAction);
  QAction *zmatAction = new QAction(tr("Zmatrix..."), this);
  connect(zmatAction, &QAction::triggered, this, &MainWindow::on_tools_zmatrix);
  buildMenu->addAction(zmatAction);

  /* Computation submenu */
  QMenu *compMenu = toolsMenu->addMenu(tr("Computation"));
  QAction *diffractAction = new QAction(tr("Diffraction..."), this);
  connect(diffractAction, &QAction::triggered, this, &MainWindow::on_tools_diffract);
  compMenu->addAction(diffractAction);
  QAction *gulpAction = new QAction(tr("GULP..."), this);
  connect(gulpAction, &QAction::triggered, this, &MainWindow::on_tools_gulp);
  compMenu->addAction(gulpAction);
  QAction *gamessAction = new QAction(tr("GAMESS..."), this);
  connect(gamessAction, &QAction::triggered, this, &MainWindow::on_tools_gamess);
  compMenu->addAction(gamessAction);
  QAction *montyAction = new QAction(tr("Monty..."), this);
  connect(montyAction, &QAction::triggered, this, &MainWindow::on_tools_monty);
  compMenu->addAction(montyAction);
  QAction *siestaAction = new QAction(tr("SIESTA..."), this);
  connect(siestaAction, &QAction::triggered, this, &MainWindow::on_tools_siesta);
  compMenu->addAction(siestaAction);
  QAction *vaspAction = new QAction(tr("VASP..."), this);
  connect(vaspAction, &QAction::triggered, this, &MainWindow::on_tools_vasp);
  compMenu->addAction(vaspAction);
  QAction *uspexAction = new QAction(tr("USPEX..."), this);
  connect(uspexAction, &QAction::triggered, this, &MainWindow::on_tools_uspex);
  compMenu->addAction(uspexAction);

  /* Analysis submenu */
  QMenu *analMenu = toolsMenu->addMenu(tr("Analysis"));
  QAction *dynamicsAnal = new QAction(tr("Dynamics..."), this);
  connect(dynamicsAnal, &QAction::triggered, this, &MainWindow::on_tools_analysis);
  analMenu->addAction(dynamicsAnal);
  QAction *measureAction = new QAction(tr("Measurements..."), this);
  connect(measureAction, &QAction::triggered, this, &MainWindow::on_tools_measure);
  analMenu->addAction(measureAction);
  QAction *plotsAction = new QAction(tr("Plots..."), this);
  connect(plotsAction, &QAction::triggered, this, &MainWindow::on_tools_plots);
  analMenu->addAction(plotsAction);

  /* ===== View menu ===== */
  QMenu *viewMenu = menuBar->addMenu(tr("&View"));

  QAction *renderProps = new QAction(tr("Display properties..."), this);
  renderProps->setShortcut(Qt::CTRL | Qt::Key_D);
  connect(renderProps, &QAction::triggered, this, &MainWindow::on_view_render_props);
  viewMenu->addAction(renderProps);

  viewMenu->addSeparator();

  QAction *resetImages = new QAction(tr("Reset model images"), this);
  resetImages->setShortcut(Qt::CTRL | Qt::Key_R);
  connect(resetImages, &QAction::triggered, this, &MainWindow::on_view_reset_images);
  viewMenu->addAction(resetImages);

  viewMenu->addSeparator();

  QAction *normalMode = new QAction(tr("Normal mode"), this);
  connect(normalMode, &QAction::triggered, this, &MainWindow::on_view_normal_mode);
  viewMenu->addAction(normalMode);

  QAction *recordMode = new QAction(tr("Recording mode"), this);
  connect(recordMode, &QAction::triggered, this, &MainWindow::on_view_record_mode);
  viewMenu->addAction(recordMode);

  viewMenu->addSeparator();

  QAction *taskMgr = new QAction(tr("Task manager..."), this);
  connect(taskMgr, &QAction::triggered, this, &MainWindow::on_view_task_manager);
  viewMenu->addAction(taskMgr);

  QAction *execPaths = new QAction(tr("Executable paths..."), this);
  connect(execPaths, &QAction::triggered, this, &MainWindow::on_view_exec_paths);
  viewMenu->addAction(execPaths);

  /* ===== Help menu ===== */
  QMenu *helpMenu = menuBar->addMenu(tr("&Help"));

  QAction *aboutAction = new QAction(tr("About..."), this);
  connect(aboutAction, &QAction::triggered, this, &MainWindow::on_help_about);
  helpMenu->addAction(aboutAction);

  QAction *manualAction = new QAction(tr("Manual..."), this);
  connect(manualAction, &QAction::triggered, this, &MainWindow::on_help_manual);
  helpMenu->addAction(manualAction);
}

void MainWindow::setupToolBar()
{
  QToolBar *toolBar = addToolBar(tr("&Toolbar"));
  toolBar->setMovable(false);
  toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
  toolBar->setIconSize(QSize(16, 16));

  /* === File operations === */
  QAction *openAction = toolBar->addAction(loadIcon("FOLDER", toolBar), tr("Open"));
  connect(openAction, &QAction::triggered, this, &MainWindow::on_file_open);

  QAction *saveAction = toolBar->addAction(loadIcon("DISK", toolBar), tr("Save"));
  connect(saveAction, &QAction::triggered, this, &MainWindow::on_file_save);

  QAction *newAction = toolBar->addAction(loadIcon("PLUS", toolBar), tr("New model"));
  connect(newAction, &QAction::triggered, this, &MainWindow::on_file_new);

  QAction *closeAction = toolBar->addAction(loadIcon("CROSS", toolBar), tr("Close"));
  connect(closeAction, &QAction::triggered, this, &MainWindow::on_file_close);

  toolBar->addSeparator();

  /* === Model editing === */
  QAction *editAction = toolBar->addAction(loadIcon("TOOLS", toolBar), tr("Editing"));
  connect(editAction, &QAction::triggered, this, &MainWindow::on_tools_editing);

  /* === Display properties === */
  QAction *renderAction = toolBar->addAction(loadIcon("PALETTE", toolBar), tr("Display properties"));
  connect(renderAction, &QAction::triggered, this, &MainWindow::on_view_render_props);

  /* === Periodic table === */
  QAction *periodicAction = toolBar->addAction(loadIcon("ELEMENT", toolBar), tr("Periodic table"));
  connect(periodicAction, &QAction::triggered, this, &MainWindow::on_tools_periodic_table);

  toolBar->addSeparator();

  /* === Selection === */
  QAction *selectAllAction = toolBar->addAction(loadIcon("SELECT_ALL", toolBar), tr("Select all"));
  connect(selectAllAction, &QAction::triggered, this, &MainWindow::on_edit_select_all);

  /* === Measurements === */
  QAction *measureAction = toolBar->addAction(loadIcon("GEOM", toolBar), tr("Measurements"));
  connect(measureAction, &QAction::triggered, this, &MainWindow::on_tools_measure);

  /* === Iso-surfaces === */
  QAction *isoAction = toolBar->addAction(loadIcon("TB_ISOSURFACE", toolBar), tr("Iso-surfaces"));
  connect(isoAction, &QAction::triggered, this, &MainWindow::on_tools_isosurfaces);

  /* === Diffraction === */
  QAction *diffractAction = toolBar->addAction(loadIcon("TB_DIFFRACTION", toolBar), tr("Diffraction"));
  connect(diffractAction, &QAction::triggered, this, &MainWindow::on_tools_diffract);

  /* === Surface === */
  QAction *surfaceAction = toolBar->addAction(loadIcon("TB_SURFACE", toolBar), tr("Surfaces"));
  connect(surfaceAction, &QAction::triggered, this, &MainWindow::on_tools_surfaces);

  /* === Model images === */
  QAction *resetImages = toolBar->addAction(loadIcon("CELL", toolBar), tr("Reset model images"));
  connect(resetImages, &QAction::triggered, this, &MainWindow::on_view_reset_images);

  toolBar->addSeparator();

  /* === Canvas management === */
  QAction *singleView = toolBar->addAction(loadIcon("CANVAS_SINGLE", toolBar), tr("Single canvas"));
  connect(singleView, &QAction::triggered, this, &MainWindow::on_single_view);

  QAction *createCanvas = toolBar->addAction(loadIcon("CANVAS_CREATE", toolBar), tr("Create canvas"));
  connect(createCanvas, &QAction::triggered, this, &MainWindow::on_create_canvas);

  QAction *deleteCanvas = toolBar->addAction(loadIcon("CANVAS_DELETE", toolBar), tr("Delete canvas"));
  connect(deleteCanvas, &QAction::triggered, this, &MainWindow::on_delete_canvas);

  toolBar->addSeparator();

  /* === View presets === */
  QAction *viewDefault = toolBar->addAction(loadIcon("AXES", toolBar), tr("Reset view"));
  connect(viewDefault, &QAction::triggered, this, &MainWindow::on_view_default);

  QAction *viewX = toolBar->addAction(loadIcon("XVIEW", toolBar), tr("View X"));
  connect(viewX, &QAction::triggered, this, &MainWindow::on_view_x);

  QAction *viewY = toolBar->addAction(loadIcon("YVIEW", toolBar), tr("View Y"));
  connect(viewY, &QAction::triggered, this, &MainWindow::on_view_y);

  QAction *viewZ = toolBar->addAction(loadIcon("ZVIEW", toolBar), tr("View Z"));
  connect(viewZ, &QAction::triggered, this, &MainWindow::on_view_z);

  QAction *viewA = toolBar->addAction(loadIcon("AVIEW", toolBar), tr("View A"));
  connect(viewA, &QAction::triggered, this, &MainWindow::on_view_a);

  QAction *viewB = toolBar->addAction(loadIcon("BVIEW", toolBar), tr("View B"));
  connect(viewB, &QAction::triggered, this, &MainWindow::on_view_b);

  QAction *viewC = toolBar->addAction(loadIcon("CVIEW", toolBar), tr("View C"));
  connect(viewC, &QAction::triggered, this, &MainWindow::on_view_c);

  toolBar->addSeparator();

  /* === Rotation === */
  QAction *rotX = toolBar->addAction(loadIcon("ROTATE1", toolBar), tr("Rotate X"));
  connect(rotX, &QAction::triggered, this, &MainWindow::on_rotate_x);

  QAction *rotY = toolBar->addAction(loadIcon("ROTATE2", toolBar), tr("Rotate Y"));
  connect(rotY, &QAction::triggered, this, &MainWindow::on_rotate_y);

  QAction *rotZ = toolBar->addAction(loadIcon("ROTATE3", toolBar), tr("Rotate Z"));
  connect(rotZ, &QAction::triggered, this, &MainWindow::on_rotate_z);

  /* Rotation angle spinner */
  m_angleSpin = new QDoubleSpinBox(toolBar);
  m_angleSpin->setRange(-360.0, 360.0);
  m_angleSpin->setSingleStep(0.1);
  m_angleSpin->setValue(90.0);
  m_angleSpin->setFixedWidth(80);
  toolBar->addWidget(m_angleSpin);
  sysenv.rotate_angle = 90.0;
  connect(m_angleSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          [this](double val) { sysenv.rotate_angle = val; });

  toolBar->addSeparator();

  /* === Animation/Record === */
  QAction *animAction = toolBar->addAction(loadIcon("TB_ANIMATE", toolBar), tr("Animation"));
  connect(animAction, &QAction::triggered, this, &MainWindow::on_tools_animation);

  QAction *recordAction = toolBar->addAction(loadIcon("CAMERA", toolBar), tr("Record mode"));
  connect(recordAction, &QAction::triggered, this, &MainWindow::on_view_record_mode);

  QAction *normalAction = toolBar->addAction(loadIcon("ARROW", toolBar), tr("Normal mode"));
  connect(normalAction, &QAction::triggered, this, &MainWindow::on_view_normal_mode);

  toolBar->addSeparator();

  /* === Plots/Export === */
  QAction *plotsAction = toolBar->addAction(loadIcon("PLOTS", toolBar), tr("Plots"));
  connect(plotsAction, &QAction::triggered, this, [this]() {
    extern void qt_show_graph_controls_dialog(void);
    qt_show_graph_controls_dialog();
  });

  QAction *exportAction = toolBar->addAction(loadIcon("TO_PNG", toolBar), tr("Export PNG"));
  connect(exportAction, &QAction::triggered, this, &MainWindow::on_export_canvas);

  QAction *trackAction = toolBar->addAction(loadIcon("TRACK", toolBar), tr("Track output"));
  connect(trackAction, &QAction::triggered, this, &MainWindow::on_view_track_output);
}

void MainWindow::setupCentralWidget()
{
  /* Create a container widget that holds the canvas.
   * The text overlay widget is positioned absolutely on top of the canvas. */
  QWidget *container = new QWidget(this);
  QVBoxLayout *layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  m_canvas = new GLCanvas(container);
  m_canvas->setMinimumSize(100, 100);

  /* Install event filter on the central widget to catch resize events.
   * QMainWindow may not propagate layout changes to QOpenGLWidget when
   * docks are toggled while maximized. This filter ensures we always
   * get notified and can force a redraw. */
  container->installEventFilter(this);
  g_gl_canvas = m_canvas;
  g_main_window = this;

  layout->addWidget(m_canvas);
  setCentralWidget(container);

  /* Text overlay: positioned absolutely over the canvas, not in the layout */
  m_textOverlay = new TextOverlayWidget(container);
  m_textOverlay->raise(); /* Ensure overlay is on top */
  m_textOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);

  /* Install event filter on canvas to track resize events */
  m_canvas->installEventFilter(this);

  /* Initial geometry sync */
  updateOverlayGeometry();
}

/* Forward declarations for panel creation */
static QWidget *create_content_panel(QWidget *parent, QWidget *Table);
static QWidget *create_edit_panel(QWidget *parent);
static QWidget *create_display_panel(QWidget *parent);
static QWidget *create_images_panel(QWidget *parent);
static QWidget *create_symmetry_panel(QWidget *parent);
static QWidget *create_view_panel(QWidget *parent);
static void update_edit_panel(QWidget *panel);

void MainWindow::setupDockWidgets()
{
  /* Model tree dock — tree + mode comboboxes */
  QDockWidget *treeDock = new QDockWidget(tr("Model Tree"), this);
  QWidget *treeWidget = new QWidget(treeDock);
  QVBoxLayout *treeLayout = new QVBoxLayout(treeWidget);
  treeLayout->setContentsMargins(4, 4, 4, 4);

  /* Tree view */
  m_modelTree = new QTreeView(treeWidget);

  /* Increase row height so icons render larger and clearer. */
  m_modelTree->setMinimumHeight(180);

  /* TODO: set treeModel row height */

  /* Create model with columns: icon, name */
  m_treeModel = new QStandardItemModel(this);
  m_treeModel->setHorizontalHeaderLabels({"", "Model"});
  m_modelTree->setModel(m_treeModel);

  /* Non-editable */
  /* Enable Enter key to activate tree items (equivalent to clicking). */
  m_modelTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_modelTree->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_modelTree->setSelectionMode(QAbstractItemView::SingleSelection);

  /* Connect selection and Enter key to activate items. */
  connect(m_modelTree->selectionModel(), &QItemSelectionModel::currentChanged, this,
          &MainWindow::on_tree_selection_changed);
  connect(m_modelTree, &QTreeView::activated, this, [this](const QModelIndex &idx) {
    m_modelTree->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect);
    on_tree_selection_changed(idx, QModelIndex());
  });

  m_modelTree->expandAll(); /*set always expand all*/
  /* Treeview: stretch to fill available space, max 120px */
  m_modelTree->setMaximumHeight(120);
  treeLayout->addWidget(m_modelTree, 1);

  /* Model mode combobox */
  m_modelModeCombo = new QComboBox(treeWidget);
  m_modelModeCombo->addItems({"Content", "Editing", "Display", "Images", "Symmetry", "Viewing"});
  connect(m_modelModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &MainWindow::on_model_mode_changed);
  treeLayout->addWidget(m_modelModeCombo);

  /* Panel widgets — all added to layout, shown/hidden by mode */
  m_contentTable = new QTableWidget();
  m_contentTable->setColumnCount(2);
  m_contentTable->setHorizontalHeaderLabels({"Property", "Value"});
  m_contentTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  m_contentTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  m_contentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_contentTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_contentTable->setRowCount(0);
  m_contentTable->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  m_contentTable->resizeColumnsToContents();
  m_contentTable->resizeRowsToContents();
  /* Content table: limit to ~2/3 of its natural height so the panel
   * doesn't take up too much vertical space. */
  m_contentTable->setMaximumHeight(180);

  m_panelStack = new QStackedWidget(treeWidget);
  //    m_panelStack->addWidget(m_contentTable); // index 0 = Content
  m_panelStack->addWidget(create_content_panel(m_panelStack, m_contentTable));
  m_panelStack->addWidget(create_edit_panel(m_panelStack));
  /* Populate edit type map for use in cellChanged handler. */
  extern gint qt_edit_type_element(void);
  extern gint qt_edit_type_name(void);
  extern gint qt_edit_type_core_ff(void);
  extern gint qt_edit_type_coord_x(void);
  extern gint qt_edit_type_coord_y(void);
  extern gint qt_edit_type_coord_z(void);
  extern gint qt_edit_type_charge(void);
  extern gint qt_edit_type_weight(void);
  extern gint qt_edit_type_sof(void);
  extern gint qt_edit_type_growth(void);
  extern gint qt_edit_type_region(void);
  extern gint qt_edit_type_translate(void);
  m_edit_type_map[0] = qt_edit_type_element();
  m_edit_type_map[1] = qt_edit_type_name();
  m_edit_type_map[2] = qt_edit_type_core_ff();
  m_edit_type_map[3] = qt_edit_type_coord_x();
  m_edit_type_map[4] = qt_edit_type_coord_y();
  m_edit_type_map[5] = qt_edit_type_coord_z();
  m_edit_type_map[6] = qt_edit_type_charge();
  m_edit_type_map[7] = qt_edit_type_weight();
  m_edit_type_map[8] = qt_edit_type_sof();
  m_edit_type_map[9] = qt_edit_type_growth();
  m_edit_type_map[10] = qt_edit_type_region();
  m_edit_type_map[11] = qt_edit_type_translate();
  m_panelStack->addWidget(create_display_panel(m_panelStack));
  m_panelStack->addWidget(create_images_panel(m_panelStack));
  m_panelStack->addWidget(create_symmetry_panel(m_panelStack));
  m_panelStack->addWidget(create_view_panel(m_panelStack));
  // m_panelStack->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  // m_panelStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  /* Register table pointer for C-side updates */
  extern void qt_set_content_table(gpointer);
  qt_set_content_table(static_cast<gpointer>(m_contentTable));

  /* Add stacked widget to layout */
  treeLayout->addWidget(m_panelStack);

  /* separator */
  QFrame *separator = new QFrame();
  separator->setFrameShape(QFrame::HLine);                              // Horizontal line
  separator->setFrameShadow(QFrame::Sunken);                            // Optional: adds depth
  separator->setContentsMargins(0, 0, 0, 0);                            // Remove padding
  separator->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); // QSizePolicy::Expanding, QSizePolicy::Fixed
  treeLayout->addWidget(separator);

  /* Model mode combobox */
  /*
      m_modelModeCombo = new QComboBox(treeWidget);
      m_modelModeCombo->addItems({"Content", "Editing", "Display", "Images", "Symmetry", "Viewing"});
      m_modelModeCombo->setMinimumHeight(28);
      connect(m_modelModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
              this, &MainWindow::on_model_mode_changed);
      treeLayout->addWidget(m_modelModeCombo);
  */

  /* Selection mode combobox */
  m_selectModeCombo = new QComboBox(treeWidget);
  m_selectModeCombo->addItems({"Atoms", "Atom Label", "Atom FF Type", "Elements", "Elements in Molecule", "Molecules",
                               "Molecule Fragments", "Regions"});
  m_selectModeCombo->setMinimumHeight(28);
  connect(m_selectModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &MainWindow::on_select_mode_changed);
  treeLayout->addWidget(m_selectModeCombo);

  /* Logo bar at bottom of tree dock */
  QWidget *logoBar = new QWidget(treeWidget);
  logoBar->setMinimumHeight(70);
  logoBar->setMaximumHeight(70);
  logoBar->setStyleSheet("background-color: #000000;");
  QHBoxLayout *logoLayout = new QHBoxLayout(logoBar);
  logoLayout->setContentsMargins(4, 4, 4, 4);
  logoLayout->setSpacing(8);
  QLabel *leftLogo = new QLabel(logoBar);
  QIcon leftIcon = loadGdisIcon("LOGO_LEFT");
  if (!leftIcon.isNull())
    leftLogo->setPixmap(leftIcon.pixmap(56, 70).scaled(56, 70, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  leftLogo->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  QLabel *rightLogo = new QLabel(logoBar);
  QIcon rightIcon = loadGdisIcon("LOGO_RIGHT");
  if (!rightIcon.isNull())
    rightLogo->setPixmap(rightIcon.pixmap(74, 70).scaled(74, 70, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  rightLogo->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  logoLayout->addWidget(leftLogo);
  logoLayout->addStretch();
  logoLayout->addWidget(rightLogo);
  treeLayout->addWidget(logoBar);

  treeDock->setWidget(treeWidget);
  treeDock->setMinimumSize(0, 600);

  addDockWidget(Qt::LeftDockWidgetArea, treeDock);

  /* Give tree focus so clicks register */
  m_modelTree->setFocus();

  /* Text output dock */
  QDockWidget *textDock = new QDockWidget(tr("Output"), this);
  m_textOutput = new QTextEdit(textDock);
  m_textOutput->setReadOnly(true);
  m_textOutput->setMinimumHeight(80);
  m_textOutput->setStyleSheet("QTextEdit { background-color: #1e1e1e; color: #d4d4d4; font-family: monospace; }");
  textDock->setWidget(m_textOutput);
  textDock->setMinimumHeight(80);
  textDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
  /* place the output at the bottom right */
  addDockWidget(Qt::BottomDockWidgetArea, textDock);

  /* Install event filters on dock widgets to catch hide/show events.
   * When a dock is hidden while the window is maximized, QMainWindow may not
   * resize the central widget. We force a canvas redraw here. */
  treeDock->installEventFilter(this);
  textDock->installEventFilter(this);
  /* Set output callback for gui_text_show */
  extern void qt_set_output_callback(void (*)(const gchar *, gint));
  static MainWindow *s_mw = this;
  qt_set_output_callback([](const gchar *text, gint type) {
    QString msg = QString::fromUtf8(text).trimmed();
    if (msg.isEmpty())
      return;
    QString html;
    switch (type)
    {
    case ERROR:
      html = QString("<span style='color:#ff4444;'>%1</span>").arg(msg.toHtmlEscaped());
      break;
    case INFO:
      html = QString("<span style='color:#44aaff;'>%1</span>").arg(msg.toHtmlEscaped());
      break;
    case WARNING:
      html = QString("<span style='color:#ffaa44;'>%1</span>").arg(msg.toHtmlEscaped());
      break;
    case ITALIC:
      html = QString("<span style='font-style:italic;color:#aaaaaa;'>%1</span>").arg(msg.toHtmlEscaped());
      break;
    default:
      html = QString("%1").arg(msg.toHtmlEscaped());
      break;
    }
    QMetaObject::invokeMethod(s_mw->m_textOutput, [html]() { s_mw->m_textOutput->append(html); }, Qt::QueuedConnection);
  });
  /* Set refresh callback for redraw_canvas */
  extern void qt_set_refresh_callback(void (*)(void));
  static MainWindow *s_mw2 = this;
  qt_set_refresh_callback([]() {
    /* Always refresh edit panel on redraw when it's the active panel. */
    if (s_mw2 && s_mw2->m_panelStack)
    {
      int idx = s_mw2->m_panelStack->currentIndex();
      if (idx == 1) /* Editing panel is visible */
        update_edit_panel(s_mw2->m_panelStack->widget(1));
    }
    QMetaObject::invokeMethod(s_mw2->m_canvas, [s_mw2]() { s_mw2->m_canvas->update(); });
  });
  /* ascribe left bottom corner to bottomDockWidget */
  setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
}

/* Slot implementations */
void MainWindow::on_file_open()
{
  QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"));
  if (!fileName.isEmpty())
  {
    /* Clear tree before loading new model */
    m_treeModel->clear();
    m_treeModel->setHorizontalHeaderLabels({"", "Model"});
    file_load(const_cast<gchar *>(fileName.toUtf8().constData()), NULL);
    /* Refresh tree after load */
    refresh_qt_tree();
    /* Refresh content table */
    extern void qt_update_content_table(void);
    qt_update_content_table();
    /* Sync image spinners */
    extern void qt_image_spinner_sync(void);
    qt_image_spinner_sync();
    /* Refresh symmetry table */
    extern void qt_gui_symmetry_refresh(gpointer);
    qt_gui_symmetry_refresh(NULL);
  }
}

void MainWindow::on_file_save()
{
  /*
    extern void file_save_dialog(void);
    file_save_dialog();
  */
}

extern "C" void qt_refresh_qt_tree(void);
extern "C" gboolean sysenv_ignore_tree_select(void);

extern "C" {
#include "graph_internal.h"
}

static void qt_tree_select_delete(void)
{
  extern int qt_tree_get_count(void);
  extern void qt_tree_model_select(int);
  extern void qt_tree_refresh(void);
  extern void qt_refresh_qt_tree(void);
  extern int qt_tree_get_selected_index(void);
  extern void graph_free(gpointer, struct model_pak *);
  extern void qt_refresh_all(void);

  int count = qt_tree_get_count();
  if (count <= 0)
    return;

  int selected = qt_tree_get_selected_index();
  if (selected < 0 || selected >= count)
    return;

  int depth = qt_tree_node_depth(selected);

  if (depth == 0)
  {
    /* Delete model */
    struct model_pak *model = qt_tree_get_model(selected);
    if (model)
      model_delete(model);
  } else if (depth == 1)
  {
    /* Delete graph */
    struct model_pak *model = qt_tree_get_model(selected);
    gpointer graph = qt_tree_get_graph(selected);
    if (model && graph)
      graph_free(graph, model);
  }

  /* Refresh tree data and Qt model */
  qt_refresh_all();

  /* Force Qt canvas repaint */
  extern void qt_force_canvas_refresh(void);
  qt_force_canvas_refresh();

  /* Select the next available item, or clear active model if none left. */
  int newCount = qt_tree_get_count();
  if (newCount > 0)
  {
    int next = selected < newCount ? selected : newCount - 1;
    qt_tree_model_select(next);
  } else
  {
    /* No models left — clear active model and force canvas redraw. */
    extern void tree_select_active(void);
    sysenv.active_model = NULL;
    extern void redraw_canvas(gint);
    redraw_canvas(ALL);
  }
}

void MainWindow::on_file_close() { qt_tree_select_delete(); }

void MainWindow::on_import_geomview()
{
  /* Import Geomview OFF file. */
  QString filePath = QFileDialog::getOpenFileName(this, tr("Import Geomview"), "", "Geomview OFF (*.off)");
  if (filePath.isEmpty())
    return;

  import_off(g_strdup(filePath.toUtf8().constData()));
}

void MainWindow::on_import_project()
{
  /* Import GDIS project file. */
  QString filePath = QFileDialog::getOpenFileName(this, tr("Import Project"), "", "GDIS Project (*.pcf)");
  if (filePath.isEmpty())
    return;

  import_pcf(g_strdup(filePath.toUtf8().constData()));
}

void MainWindow::on_import_graph()
{
  /* Import graph data file. */
  QString filePath = QFileDialog::getOpenFileName(this, tr("Import Graph"), "", "CSV Files (*.csv);;All Files (*)");
  if (filePath.isEmpty())
    return;

  extern void graph_read(gchar * filename);
  graph_read(g_strdup(filePath.toUtf8().constData()));
}

void MainWindow::on_export_canvas()
{
  if (!g_gl_canvas)
    return;

  /* Get the raw RGBA buffer from sysenv.cairo_surface. */
  extern unsigned char *qt_cairo_surface(void);
  extern gint qt_cairo_width(void);
  extern gint qt_cairo_height(void);
  unsigned char *pixels = qt_cairo_surface();
  gint w = qt_cairo_width();
  gint h = qt_cairo_height();
  if (!pixels || w <= 0 || h <= 0)
    return;

  /* Build default filename. */
  QString baseName = "gdis_snapshot";
  struct model_pak *model = qt_get_active_model();
  if (model && model->basename)
    baseName = QString(model->basename);

  /* Ask user for output format and file path. */
  QString defaultFilter = "PNG Image (*.png)";
  QString filePath = QFileDialog::getSaveFileName(this, tr("Export Canvas Snapshot"), baseName + ".png",
                                                  "PNG Image (*.png);;JPEG Image (*.jpg *.jpeg)", &defaultFilter);
  if (filePath.isEmpty())
    return;

  /* Cairo surface is ARGB32 format (A,R,G,B on little-endian). */
  QImage img(pixels, w, h, w * 4, QImage::Format_ARGB32);

  /* Save with appropriate format. */
  QString ext = QFileInfo(filePath).suffix().toLower();
  if (ext == "jpg" || ext == "jpeg")
    img.save(filePath, "JPEG", 90);
  else
    img.save(filePath, "PNG", -1); /* highest quality */
}

/* Edit */
void MainWindow::on_export_graph_data()
{
  struct model_pak *model = qt_get_active_model();
  if (!model || !model->graph_active)
  {
    QMessageBox::warning(this, tr("Export Graph Data"), tr("No active graph to export."));
    return;
  }

  QString defaultName = "graph_data.csv";
  if (model && model->basename)
    defaultName = QString(model->basename) + ".csv";

  QString filePath =
      QFileDialog::getSaveFileName(this, tr("Export Graph Data"), defaultName, "CSV Files (*.csv);;All Files (*)");
  if (filePath.isEmpty())
    return;

  /* graph_write expects a basename in sysenv.cwd — extract just the filename. */
  extern void graph_write(gchar * name, gpointer ptr_graph);
  QString baseName = QFileInfo(filePath).fileName();
  graph_write(g_strdup(baseName.toUtf8().constData()), model->graph_active);
}

void MainWindow::on_edit_undo()
{
  extern void undo_active(void);
  undo_active();
}
void MainWindow::on_edit_copy()
{
  extern void select_copy(void);
  select_copy();
}
void MainWindow::on_edit_paste()
{
  extern void select_paste(void);
  select_paste();
}
void MainWindow::on_edit_colour()
{
  extern struct sysenv_pak sysenv;
  extern void redraw_canvas(gint);

  /* Current halo colour from C side */
  QColor current(sysenv.render.halo_colour[0] * 255, sysenv.render.halo_colour[1] * 255,
                 sysenv.render.halo_colour[2] * 255);

  auto color = QColorDialog::getColor(current, this, tr("Selection halo colour"));
  if (color.isValid())
  {
    /* Update the C-side rendering structure */
    sysenv.render.halo_colour[0] = color.redF();
    sysenv.render.halo_colour[1] = color.greenF();
    sysenv.render.halo_colour[2] = color.blueF();

    /* Trigger redraw so the new colour takes effect */
    if (m_canvas)
      m_canvas->update();
  }
}
void MainWindow::on_edit_delete()
{
  extern void select_delete(void);
  extern void redraw_canvas(gint);
  select_delete();
  redraw_canvas(1); /* ALL */
  if (m_canvas)
    m_canvas->update();
}

bool MainWindow::event(QEvent *event)
{
  /* Intercept Delete key at the window level, before any child widget */
  if (event->type() == QEvent::KeyPress)
  {
    QKeyEvent *ke = static_cast<QKeyEvent *>(event);
    if (ke->key() == Qt::Key_Delete)
    {
      on_edit_delete();
      return true;
    }
  }

  /* Catch window state changes — triggered when the window is maximized.
   * Forces canvas redraw to pick up new dimensions. */
  if (event->type() == QEvent::WindowStateChange)
  {
    forceCanvasResize();
  }

  return QMainWindow::event(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
  /* Force the central widget to fill available space after any resize.
   * QMainWindow's layout system doesn't properly resize the central widget
   * when dock visibility changes on a maximized window. We fix this by
   * manually setting the container geometry to match the window size minus
   * menu bar and toolbar space. */
  if (centralWidget())
  {
    /* Calculate available space: window rect minus non-client area.
     * QMainWindow already accounts for menu/toolbar in its layout, but we
     * need to ensure the central widget fills whatever space is available. */
    QRect avail = contentsRect();
    if (!avail.isEmpty() && centralWidget()->geometry() != avail)
    {
      centralWidget()->setGeometry(avail);
    }
  }

  QMainWindow::resizeEvent(event);
  /* Ensure canvas redraws with correct viewport. */
  forceCanvasResize();
}

void MainWindow::on_edit_select_all()
{
  extern void select_all(void);
  select_all();
  if (g_gl_canvas)
    g_gl_canvas->update();
}
void MainWindow::on_edit_invert()
{
  extern void select_invert(void);
  extern void redraw_canvas(gint);
  select_invert();
  /* Force a full repaint — selection changes are not geometry changes,
   * so need_clear is NOT set, but we still need to re-draw the scene
   * so that any visual feedback (selection highlight, etc.) is shown. */
  redraw_canvas(ALL);
  QCoreApplication::processEvents();
}
void MainWindow::on_edit_hide()
{
  extern void select_hide(void);
  select_hide();
}
void MainWindow::on_edit_hide_unsel()
{
  extern void unselect_hide(void);
  unselect_hide();
}
void MainWindow::on_edit_unhide()
{
  extern void unhide_atoms(void);
  unhide_atoms();
}

/* Tools - Visualization */
void MainWindow::on_tools_animation()
{
  extern void qt_show_animate_dialog(struct model_pak *);
  extern struct model_pak *qt_get_active_model(void);
  struct model_pak *model = qt_get_active_model();
  if (model && model->animation)
    qt_show_animate_dialog(model);
}
void MainWindow::on_tools_isosurfaces()
{
  extern void qt_show_isosurfaces_dialog(struct model_pak *);
  struct model_pak *model = qt_get_active_model();
  if (model)
    qt_show_isosurfaces_dialog(model);
}
void MainWindow::on_tools_periodic_table()
{
  extern void qt_show_periodic_table_dialog(void);
  qt_show_periodic_table_dialog();
}

/* Tools - Building */
void MainWindow::on_tools_editing()
{
  m_editDialog = new EditDialog(this);
  m_editDialog->show();
}
void MainWindow::on_tools_dislocations() { qt_defect_dialog(); }
void MainWindow::on_tools_docking()
{
  struct model_pak *model = qt_get_active_model();
  if (!model || model->periodic != 2)
  {
    QMessageBox::warning(this, "Docking",
                         "Docking requires a surface model (periodic=2). Please load a surface first.");
    return;
  }
  qt_dock_dialog();
}
void MainWindow::on_tools_dynamics() { qt_mdi_dialog(); }
void MainWindow::on_tools_surfaces() { qt_show_surface_dialog(); }
void MainWindow::on_tools_zmatrix()
{
  struct model_pak *model = qt_get_active_model();
  if (!model)
    return;

  struct zmat_pak *zmat = (struct zmat_pak *) model->zmatrix;
  if (!zmat)
  {
    QMessageBox::warning(this, "Z-matrix Editor", "No z-matrix defined for the active model.");
    return;
  }

  auto *d = new ZMatrixDialog(model, this);
  d->show();
}

/* Tools - Computation */
void MainWindow::on_tools_diffract()
{
  extern void qt_show_diffraction_dialog(struct model_pak * model, QWidget * parent);
  extern struct model_pak *qt_get_active_model(void);
  struct model_pak *model = qt_get_active_model();
  qt_show_diffraction_dialog(model, this);
}
void MainWindow::on_tools_gulp()
{
  extern void qt_show_gulp_dialog(void);
  qt_show_gulp_dialog();
}
void MainWindow::on_tools_gamess()
{
  extern void qt_show_gamess_dialog(struct model_pak *);
  qt_show_gamess_dialog((struct model_pak *) qt_get_active_model());
}
void MainWindow::on_tools_monty()
{
  extern void qt_show_monty_dialog(struct model_pak *);
  qt_show_monty_dialog((struct model_pak *) qt_get_active_model());
}
void MainWindow::on_tools_siesta()
{
  extern void qt_show_siesta_dialog(struct model_pak *);
  qt_show_siesta_dialog((struct model_pak *) qt_get_active_model());
}
void MainWindow::on_tools_vasp()
{
  extern void qt_show_vasp_dialog(void);
  qt_show_vasp_dialog();
}
void MainWindow::on_tools_uspex()
{
  extern void qt_show_uspex_dialog(void);
  qt_show_uspex_dialog();
}

/* Tools - Analysis */
void MainWindow::on_tools_analysis()
{
  extern void qt_show_md_analysis_dialog(void);
  qt_show_md_analysis_dialog();
}
void MainWindow::on_tools_measure()
{
  extern void qt_show_measurements_dialog(void);
  qt_show_measurements_dialog();
}
void MainWindow::on_tools_plots()
{
  extern void qt_show_plots_dialog(void);
  qt_show_plots_dialog();
}

/* View */
extern "C" {
#include "matrix.h"
#include "quaternion.h"
void gui_view_x(void)
{
  struct camera_pak *camera;
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (model)
  {
    camera_reset(model);
    camera = (struct camera_pak *) model->camera;
    quat_concat_euler(camera->q, YAW, 0.5 * G_PI);
    gui_model_select(model);
  }
}
void gui_view_y(void)
{
  struct camera_pak *camera;
  struct model_pak *model = (struct model_pak *) sysenv.active_model;

  if (model)
  {
    camera_reset(model);
    camera = (struct camera_pak *) model->camera;
    quat_concat_euler(camera->q, YAW, G_PI);
    gui_model_select(model);
  }
}
void gui_view_z(void)
{
  struct camera_pak *camera;
  struct model_pak *model = (struct model_pak *) sysenv.active_model;

  if (model)
  {
    camera_reset(model);
    camera = (struct camera_pak *) model->camera;
    quat_concat_euler(camera->q, PITCH, -0.5 * G_PI);
    gui_model_select(model);
  }
}
void gui_view_a(void)
{
  gdouble angle, a[3], v[3];
  struct camera_pak *camera;
  struct model_pak *model = (struct model_pak *) sysenv.active_model;

  if (model && (model->periodic > 1 || model->id == MORPH))
  {
    camera_reset(model);
    camera = (struct camera_pak *) model->camera;

    /* a axis vector */
    VEC3SET(a, -1.0, 0.0, 0.0);
    vecmat(model->latmat, a);

    /* angle */
    angle = via(camera->v, a, 3);

    /* rotation axis */
    crossprod(v, camera->v, a);
    normalize(v, 3);

    if (v[0] < 1e-6 && v[1] < 1e-6 && v[2] < 1e-6)
      VEC3SET(v, 1.0, 0.0, 0.0);

    /* align */
    if (fabs(v[0]) < 1e-6 && fabs(v[1]) < 1e-6 && fabs(v[2]) < 1e-6)
      quat_concat_euler(camera->q, YAW, 0.5 * G_PI);
    else
      quat_concat(camera->q, v, angle);

    gui_model_select(model);
  } else
    gui_view_x();
}
void gui_view_b(void)
{
  gdouble angle, b[3], v[3];
  struct camera_pak *camera;
  struct model_pak *model = (struct model_pak *) sysenv.active_model;

  if (model && (model->periodic > 1 || model->id == MORPH))
  {
    camera_reset(model);
    camera = (struct camera_pak *) model->camera;

    /* b axis vector */
    VEC3SET(b, 0.0, -1.0, 0.0);
    vecmat(model->latmat, b);

    /* angle */
    angle = via(camera->v, b, 3);

    /* rotation axis */
    crossprod(v, camera->v, b);
    normalize(v, 3);

    /* align */
    if (fabs(v[0]) < 1e-6 && fabs(v[1]) < 1e-6 && fabs(v[2]) < 1e-6)
      quat_concat_euler(camera->q, YAW, G_PI);
    else
      quat_concat(camera->q, v, angle);

    gui_model_select(model);
  } else
    gui_view_y();
}
void gui_view_c(void)
{
  gdouble a, c[3], v[3];
  struct camera_pak *camera;
  struct model_pak *model = (struct model_pak *) sysenv.active_model;

  if (model && (model->periodic > 2 || model->id == MORPH))
  {
    camera_reset(model);
    camera = (struct camera_pak *) model->camera;

    /* c axis vector */
    VEC3SET(c, 0.0, 0.0, -1.0);
    vecmat(model->latmat, c);

    /* angle */
    a = via(camera->v, c, 3);

    /* rotation axis */
    crossprod(v, camera->v, c);
    normalize(v, 3);

    /* align */
    quat_concat(camera->q, v, a);

    gui_model_select(model);
  } else
    gui_view_z();
}
void gui_rotate_x(void)
{
  gdouble angle;
  struct camera_pak *camera;
  struct model_pak *model = (struct model_pak *) sysenv.active_model;

  angle = sysenv.rotate_angle;

  angle *= D2R;
  if (model)
  /* if (model && (model->periodic > 1 || model->id == MORPH)) */
  {
    camera = (struct camera_pak *) model->camera;
    quat_concat_euler(camera->q, PITCH, angle);

    gui_model_select(model);
  }
}
void gui_rotate_y(void)
{
  gdouble angle;
  struct camera_pak *camera;
  struct model_pak *model = (struct model_pak *) sysenv.active_model;

  angle = sysenv.rotate_angle;

  angle *= D2R;

  if (model)
  /* if (model && (model->periodic > 1 || model->id == MORPH)) */
  {
    camera = (struct camera_pak *) model->camera;

    quat_concat_euler(camera->q, ROLL, angle);
    gui_model_select(model);
  }
}
void gui_rotate_z(void)
{
  gdouble angle;
  struct camera_pak *camera;
  struct model_pak *model = (struct model_pak *) sysenv.active_model;

  angle = sysenv.rotate_angle;

  angle *= D2R;

  if (model)
  /* if (model && (model->periodic > 1 || model->id == MORPH)) */
  {
    camera = (struct camera_pak *) model->camera;
    quat_concat_euler(camera->q, YAW, angle);

    gui_model_select(model);
  }
}
}
void MainWindow::on_view_render_props()
{
  RenderDialog *dlg = new RenderDialog(this);
  dlg->setCanvas(nullptr);
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->show();
}
void MainWindow::on_view_reset_images()
{
  extern void space_image_widget_reset(void);
  space_image_widget_reset();
}
void MainWindow::on_view_normal_mode()
{
  extern void gui_mode_default(void);
  gui_mode_switch(FREE);
}
void MainWindow::on_view_record_mode()
{
  extern void gui_mode_record(void);
  gui_mode_switch(RECORD);
}
void MainWindow::on_view_task_manager()
{
  extern void qt_show_task_manager_dialog(void);
  qt_show_task_manager_dialog();
}
void MainWindow::on_view_exec_paths() { qt_show_exec_paths_dialog(); }
void MainWindow::on_view_default()
{
  gui_view_default();
  if (g_gl_canvas)
    g_gl_canvas->update();
}
void MainWindow::on_view_x()
{
  gui_view_x();
  if (g_gl_canvas)
    g_gl_canvas->update();
}
void MainWindow::on_view_y()
{
  gui_view_y();
  if (g_gl_canvas)
    g_gl_canvas->update();
}
void MainWindow::on_view_z()
{
  gui_view_z();
  if (g_gl_canvas)
    g_gl_canvas->update();
}
void MainWindow::on_view_a()
{
  gui_view_a();
  if (g_gl_canvas)
    g_gl_canvas->update();
}
void MainWindow::on_view_b()
{
  gui_view_b();
  if (g_gl_canvas)
    g_gl_canvas->update();
}
void MainWindow::on_view_c()
{
  gui_view_c();
  if (g_gl_canvas)
    g_gl_canvas->update();
}
void MainWindow::on_view_track_output() { gui_track_output(); }

/* Rotation */
void MainWindow::on_rotate_x()
{
  gui_rotate_x();
  if (m_canvas)
    m_canvas->update();
}
void MainWindow::on_rotate_y()
{
  gui_rotate_y();
  if (m_canvas)
    m_canvas->update();
}
void MainWindow::on_rotate_z()
{
  gui_rotate_z();
  if (m_canvas)
    m_canvas->update();
}
/* Help */
void MainWindow::on_help_about()
{
  QMessageBox::about(this, tr("About GDIS"),
                     tr("--- GDIS ---\n"
                        "QT6 version.\n"
                        "\n"
                        "Lic.:  GPLv2"));
}
void MainWindow::on_help_manual() { /* TODO: open manual */ }
void MainWindow::on_about() { on_help_about(); }

/* File */
void MainWindow::on_file_new()
{
  extern void edit_model_create(void);
  edit_model_create();
}

/* Other */
void MainWindow::on_quit() { close(); }
void MainWindow::on_refresh()
{
  extern void redraw_canvas(gint);
  redraw_canvas(1);
}
void MainWindow::on_single_view()
{
  extern void canvas_single(void);
  canvas_single();
  m_canvas->update();
}
void MainWindow::on_create_canvas()
{
  extern void canvas_create(void);
  canvas_create();
  m_canvas->update();
}
void MainWindow::on_delete_canvas()
{
  extern void canvas_delete(void);
  canvas_delete();
  m_canvas->update();
}

void MainWindow::on_tree_selection_changed(const QModelIndex &current, const QModelIndex &previous)
{
  Q_UNUSED(previous);
  if (!current.isValid())
    return;
  /* Ignore during batch loading — Qt focus events can overwrite active_model */
  if (sysenv_ignore_tree_select())
    return;
  /* The model stores the global index in Qt::UserRole */
  int globalIndex = current.data(Qt::UserRole).toInt();
  if (globalIndex < 0)
    return;
  extern void qt_tree_model_select(int);
  qt_tree_model_select(globalIndex);
  /* Update editing panel with new selection. */
  if (m_panelStack->currentIndex() == 1)
    update_edit_panel(m_panelStack->widget(1));
  /* Sync image spinners if on Images panel */
  if (m_panelStack->currentIndex() == 3)
  {
    extern void qt_image_spinner_sync(void);
    qt_image_spinner_sync();
  }
  /* Refresh symmetry table on model change */
  extern void qt_gui_symmetry_refresh(gpointer);
  qt_gui_symmetry_refresh(NULL);
  /* Force immediate redraw after model selection */
  m_canvas->update();
}

/* ============================================================
   Panel creation helpers
   ============================================================ */
/* Create the Content panel */
static QWidget *create_content_panel(QWidget *parent, QWidget *m_contentTable)
{
  auto *w = new QWidget(parent);
  w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  auto *layout = new QVBoxLayout(w);

  layout->addWidget(m_contentTable, 1); /* stretch factor 1: take available space */

  layout->addStretch(0);

  return w;
}
/* Create the Editing panel (atom properties) */
static QWidget *create_edit_panel(QWidget *parent)
{
  auto *w = new QWidget(parent);
  w->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  auto *layout = new QVBoxLayout(w);

  /* Two-column table: Property | Value — like the Content Properties panel */
  const char *labels_arr[] = {"Element", "Label", "FF Type", "x",      "y",      "z",
                              "Charge",  "Mass",  "SOF",     "Growth", "Region", "Translate"};

  auto *table = new QTableWidget(12, 2);
  table->setHorizontalHeaderLabels({"Property", "Value"});
  table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  table->verticalHeader()->setVisible(false);
  /* Allow editing only in the Value column (column 1), not Property column */
  /* Editing handled by cellChanged handler; no triggers needed. */
  table->setItemDelegateForColumn(0, new QItemDelegate(table)); // non-editable property labels
  table->setSelectionBehavior(QAbstractItemView::SelectRows);
  table->setShowGrid(false);

  for (int i = 0; i < 12; i++)
  {
    auto *propLabel = new QTableWidgetItem(labels_arr[i]);
    propLabel->setFlags(Qt::NoItemFlags);
    propLabel->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    table->setItem(i, 0, propLabel);

    auto *valueEdit = new QTableWidgetItem("");
    valueEdit->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    table->setItem(i, 1, valueEdit);
  }

  /* Set row heights for readability */
  for (int i = 0; i < 12; i++)
    table->setRowHeight(i, 30);

  layout->addWidget(table, 1); /* stretch factor 1: take available space */

  /* Connect table cell edits to apply changes and refresh */
  extern void qt_atom_properties_change(gint);
  extern void qt_set_edit_field_text(gint, const gchar *);
  extern void redraw_canvas(gint);
  extern gint qt_edit_type_element(void);
  extern gint qt_edit_type_name(void);
  extern gint qt_edit_type_core_ff(void);
  extern gint qt_edit_type_coord_x(void);
  extern gint qt_edit_type_coord_y(void);
  extern gint qt_edit_type_coord_z(void);
  extern gint qt_edit_type_charge(void);
  extern gint qt_edit_type_weight(void);
  extern gint qt_edit_type_sof(void);
  extern gint qt_edit_type_growth(void);
  extern gint qt_edit_type_region(void);
  extern gint qt_edit_type_translate(void);
  /* Map Qt panel indices (0-11) to C enum types for atom_properties_change. */
  gint local_edit_type_map[12];
  local_edit_type_map[0] = ELEMENT;
  local_edit_type_map[1] = NAME;
  local_edit_type_map[2] = CORE_FF;
  local_edit_type_map[3] = COORD_X;
  local_edit_type_map[4] = COORD_Y;
  local_edit_type_map[5] = COORD_Z;
  local_edit_type_map[6] = CHARGE;
  local_edit_type_map[7] = WEIGHT;
  local_edit_type_map[8] = SOF;
  local_edit_type_map[9] = CORE_GROWTH_SLICE;
  local_edit_type_map[10] = CORE_REGION;
  local_edit_type_map[11] = CORE_TRANSLATE;

  /* Store pointers for the cellChanged handler and refresh_guard. */
  s_edit_table = table;
  QObject::connect(table, &QTableWidget::cellChanged, [table]() {
    if (!s_edit_table)
      return;
    int row = table->currentRow();
    if (row < 0 || row >= 12)
      return;
    QTableWidgetItem *item = table->item(row, 1);
    if (!item)
      return;

    /* Guard against re-entrant calls from update_edit_panel. */
    if (s_applying_edit_ptr && s_applying_edit_ptr->loadAcquire())
      return;

    gint local_map[12];
    memcpy(local_map, g_main_window->get_edit_type_map(), sizeof(local_map));

    if (s_applying_edit_ptr)
      s_applying_edit_ptr->storeRelease(1);
    qt_set_edit_field_text(local_map[row], item->text().toUtf8().constData());
    qt_atom_properties_change(local_map[row]);
    if (s_applying_edit_ptr)
      s_applying_edit_ptr->storeRelease(0);

    /* Mark model as needing clear so redraw actually repaints */
    {
      struct model_pak *model = qt_get_active_model();
      if (model)
        model->need_clear = TRUE;
    }

    /* Force immediate GL refresh with full clear. */
    extern void redraw_canvas(gint);
    redraw_canvas(ALL);
  });

  /* Buttons: Add atoms, Add bonds, Delete bonds, Normal mode */
  auto *frame1 = new QFrame();
  frame1->setFrameStyle(QFrame::StyledPanel);
  auto *btns1 = new QVBoxLayout(frame1);
  btns1->setContentsMargins(4, 4, 4, 4);
  btns1->setSpacing(2);

  extern void qt_gui_mode_switch(gpointer);
  extern void select_flag_ghost(void);
  extern void select_flag_normal(void);
  extern void redraw_canvas(gint);
  extern gint qt_mode_free(void);
  extern gint qt_mode_atom_add(void);
  extern gint qt_mode_bond_single(void);
  extern gint qt_mode_bond_delete(void);

  auto *btnAddAtoms = new QPushButton("Add atoms");
  QObject::connect(btnAddAtoms, &QPushButton::clicked, []() { gui_mode_switch(qt_mode_atom_add()); });
  btns1->addWidget(btnAddAtoms);

  auto *btnAddBonds = new QPushButton("Add bonds");
  QObject::connect(btnAddBonds, &QPushButton::clicked, []() { gui_mode_switch(qt_mode_bond_single()); });
  btns1->addWidget(btnAddBonds);

  auto *btnDelBonds = new QPushButton("Delete bonds");
  QObject::connect(btnDelBonds, &QPushButton::clicked, []() { gui_mode_switch(qt_mode_bond_delete()); });
  btns1->addWidget(btnDelBonds);

  auto *btnNormal = new QPushButton("Normal mode");
  QObject::connect(btnNormal, &QPushButton::clicked, []() { gui_mode_switch(qt_mode_free()); });
  btns1->addWidget(btnNormal);
  layout->addWidget(frame1);

  /* Ghost/Normal buttons */
  auto *frame2 = new QFrame();
  frame2->setFrameStyle(QFrame::StyledPanel);
  auto *btns2 = new QVBoxLayout(frame2);
  btns2->setContentsMargins(4, 4, 4, 4);
  btns2->setSpacing(2);

  auto *btnGhost = new QPushButton("Mark as ghost");
  QObject::connect(btnGhost, &QPushButton::clicked, []() {
    select_flag_ghost();
    redraw_canvas(1); /* ALL */
    if (g_gl_canvas)
      g_gl_canvas->update();
  });
  btns2->addWidget(btnGhost);

  auto *btnNormal2 = new QPushButton("Mark as normal");
  QObject::connect(btnNormal2, &QPushButton::clicked, []() {
    select_flag_normal();
    redraw_canvas(1); /* ALL */
    if (g_gl_canvas)
      g_gl_canvas->update();
  });
  btns2->addWidget(btnNormal2);
  layout->addWidget(frame2);

  layout->addStretch(); /* finally add space */

  return w;
}

/* Create the Display panel (rendering mode buttons) */
static QWidget *create_display_panel(QWidget *parent)
{
  auto *w = new QWidget(parent);
  w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  auto *layout = new QVBoxLayout(w);

  extern void qt_render_mode_set(gint);
  extern void qt_render_mode_polyhedral(void);
  extern void qt_render_mode_zone(void);
  extern void qt_render_wire_atoms(void);
  extern void qt_render_solid_atoms(void);
  extern gint qt_render_stick(void);
  extern gint qt_render_ball_stick(void);
  extern gint qt_render_cpk(void);
  extern gint qt_render_liquorice(void);
  extern gint qt_render_polyhedral(void);
  extern gint qt_render_zone(void);
  extern void redraw_canvas(gint);

  auto add_mode_btn = [&](const QString &label, std::function<void()> fn) {
    /* Create a compact row widget: label + icon button */
    auto *row = new QWidget();
    row->setFixedHeight(28);
    auto *hbox = new QHBoxLayout(row);
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(2);
    auto *lbl = new QLabel(label, row);
    lbl->setStyleSheet("QLabel { font-weight: normal; } ");
    hbox->addWidget(lbl);
    auto *btn = new QPushButton(loadIcon("GO", row), "", row);
    btn->setFixedSize(28, 28);
    btn->setStyleSheet("QPushButton { border: none; padding: 0; } "
                       "QPushButton:hover { background-color: rgba(180,180,255,0.3); }");
    QObject::connect(btn, &QPushButton::clicked, [fn]() {
      fn();
      if (g_gl_canvas)
        g_gl_canvas->update();
    });
    hbox->addWidget(btn);
    layout->addWidget(row);
  };

  add_mode_btn("Ball & Stick", []() {
    qt_render_mode_set(qt_render_ball_stick());
    redraw_canvas(1);
  });
  add_mode_btn("CPK", []() {
    qt_render_mode_set(qt_render_cpk());
    redraw_canvas(1);
  });
  add_mode_btn("Liquorice", []() {
    qt_render_mode_set(qt_render_liquorice());
    redraw_canvas(1);
  });
  add_mode_btn("Polyhedral", []() {
    qt_render_mode_polyhedral();
    redraw_canvas(1);
  });
  add_mode_btn("Stick", []() {
    qt_render_mode_set(qt_render_stick());
    redraw_canvas(1);
  });
  add_mode_btn("Zone based", []() {
    qt_render_mode_zone();
    redraw_canvas(1);
  });
  add_mode_btn("Wire frame", []() {
    qt_render_wire_atoms();
    redraw_canvas(1);
  });
  add_mode_btn("Solid", []() {
    qt_render_solid_atoms();
    redraw_canvas(1);
  });

  layout->addStretch(); /* finally add space */

  return w;
}

/* Create the Images panel (periodic image spinners) */
static QWidget *create_images_panel(QWidget *parent)
{
  auto *w = new QWidget(parent);
  w->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  auto *layout = new QVBoxLayout(w);
  layout->setContentsMargins(4, 4, 4, 4);
  layout->setSpacing(4);

  extern void redraw_canvas(gint);

  /* Grid: rows=x,y,z; cols=label(negative), negative spinner, label(positive), positive spinner */
  auto *grid = new QGridLayout();
  grid->setContentsMargins(0, 0, 0, 0);
  grid->setSpacing(4);

  /* Direction labels */
  auto *negLabel = new QLabel("−", w);
  negLabel->setAlignment(Qt::AlignCenter);
  auto *posLabel = new QLabel("+", w);
  posLabel->setAlignment(Qt::AlignCenter);
  grid->addWidget(negLabel, 0, 1);
  grid->addWidget(posLabel, 0, 3);

  /* Axis labels and spinners */
  static const char *axes[] = {"x", "y", "z"};
  static QSpinBox *spinners[6];
  for (int i = 0; i < 3; i++)
  {
    auto *axisLabel = new QLabel(axes[i], w);
    axisLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(axisLabel, i + 1, 0);

    spinners[2 * i] = new QSpinBox(w);
    spinners[2 * i]->setRange(0, 10);
    spinners[2 * i]->setValue(0);
    QObject::connect(spinners[2 * i], QOverload<int>::of(&QSpinBox::valueChanged), [i](int val) {
      extern void qt_image_spinner_set(gint, gint, gint);
      qt_image_spinner_set(i, 2 * i, val);
      if (g_gl_canvas)
        g_gl_canvas->update();
    });
    grid->addWidget(spinners[2 * i], i + 1, 1);

    spinners[2 * i + 1] = new QSpinBox(w);
    spinners[2 * i + 1]->setRange(0, 10);
    spinners[2 * i + 1]->setValue(1);
    QObject::connect(spinners[2 * i + 1], QOverload<int>::of(&QSpinBox::valueChanged), [i](int val) {
      extern void qt_image_spinner_set(gint, gint, gint);
      qt_image_spinner_set(i, 2 * i + 1, val);
      if (g_gl_canvas)
        g_gl_canvas->update();
    });
    grid->addWidget(spinners[2 * i + 1], i + 1, 3);
  }

  /* Column stretch: all columns share space equally for centered layout */
  grid->setColumnStretch(0, 1);
  grid->setColumnStretch(1, 1);
  grid->setColumnStretch(2, 1);
  grid->setColumnStretch(3, 1);

  layout->addLayout(grid);
  layout->addStretch();

  return w;
}

/* Update spinner widgets from stored values */
extern gint qt_image_spinner_values[6];

void qt_image_spinner_update_widgets(void)
{
  extern QSpinBox *image_spinners[6];
  if (!image_spinners[0])
    return;

  for (int i = 0; i < 6; i++)
  {
    image_spinners[i]->blockSignals(true);
    image_spinners[i]->setValue(qt_image_spinner_values[i]);
    image_spinners[i]->blockSignals(false);
  }
}

/* Set symmetry table data */
void qt_set_symmetry_table(gpointer, const gchar **labels, const gchar **values, gint count)
{
  extern QTableWidget *symmetry_table_ptr(void);
  auto *table = symmetry_table_ptr();
  if (!table)
    return;

  table->setRowCount(count);
  for (int i = 0; i < count; i++)
  {
    auto *labelItem = new QTableWidgetItem(QString::fromUtf8(labels[i]));
    labelItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    table->setItem(i, 0, labelItem);
    auto *valueItem = new QTableWidgetItem(QString::fromUtf8(values[i]));
    valueItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);
    table->setItem(i, 1, valueItem);
  }
}

/* Create the Symmetry panel (space group info table + buttons) */
static QWidget *create_symmetry_panel(QWidget *parent)
{
  auto *w = new QWidget(parent);
  w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  auto *layout = new QVBoxLayout(w);
  layout->setContentsMargins(4, 4, 4, 4);
  layout->setSpacing(4);

  extern void qt_gui_symmetry_refresh(gpointer);
  extern void gui_symmetry_analyse(void);
  extern void gui_symmetry_toggle(void);
  extern void redraw_canvas(gint);

  /* Table: rows=properties, cols=label, value */
  g_symmetry_table = new QTableWidget();
  g_symmetry_table->setColumnCount(2);
  g_symmetry_table->setHorizontalHeaderLabels({"Property", "Value"});
  g_symmetry_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  g_symmetry_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  g_symmetry_table->verticalHeader()->setVisible(false);
  g_symmetry_table->setSelectionBehavior(QAbstractItemView::SelectRows);
  g_symmetry_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  layout->addWidget(g_symmetry_table);

  /* Buttons */
  auto *frame = new QFrame();
  frame->setFrameStyle(QFrame::StyledPanel);
  auto *btns = new QVBoxLayout(frame);
  btns->setContentsMargins(4, 4, 4, 4);
  btns->setSpacing(2);

  auto *btnGuess = new QPushButton("Guess Pointgroup");
  QObject::connect(btnGuess, &QPushButton::clicked, []() {
    extern void gui_symmetry_analyse_periodic(void);
    gui_symmetry_analyse_periodic();
    qt_gui_symmetry_refresh(NULL);
    redraw_canvas(1);
  });
  btns->addWidget(btnGuess);

  auto *btnAsym = new QPushButton("Asymmetric unit");
  QObject::connect(btnAsym, &QPushButton::clicked, []() {
    gui_symmetry_toggle();
    redraw_canvas(1);
  });
  btns->addWidget(btnAsym);
  layout->addWidget(frame);

  layout->addStretch();

  return w;
}

/* Accessor for symmetry table */
extern QTableWidget *symmetry_table_ptr(void) { return g_symmetry_table; }

/* Create the Viewing panel (view axis buttons) */
static QWidget *create_view_panel(QWidget *parent)
{
  auto *w = new QWidget(parent);
  w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  auto *layout = new QVBoxLayout(w);

  extern void gui_view_x(void);
  extern void gui_view_y(void);
  extern void gui_view_z(void);
  extern void gui_view_a(void);
  extern void gui_view_b(void);
  extern void gui_view_c(void);
  extern void redraw_canvas(gint);

  /* Row 1: x, y, z */
  auto *row1 = new QHBoxLayout();
  const char *row1_axes[] = {" x ", " y ", " z "};
  for (int i = 0; i < 3; i++)
  {
    auto *btn = new QPushButton(row1_axes[i]);
    btn->setFixedWidth(40);
    int axis = i;
    QObject::connect(btn, &QPushButton::clicked, [axis]() {
      switch (axis)
      {
      case 0:
        gui_view_x();
        break;
      case 1:
        gui_view_y();
        break;
      case 2:
        gui_view_z();
        break;
      }
      if (g_gl_canvas)
        g_gl_canvas->update();
    });
    row1->addWidget(btn);
  }
  layout->addLayout(row1);

  /* Row 2: a, b, c */
  auto *row2 = new QHBoxLayout();
  const char *row2_axes[] = {" a ", " b ", " c "};
  for (int i = 0; i < 3; i++)
  {
    auto *btn = new QPushButton(row2_axes[i]);
    btn->setFixedWidth(40);
    int axis = i;
    QObject::connect(btn, &QPushButton::clicked, [axis]() {
      switch (axis)
      {
      case 0:
        gui_view_a();
        break;
      case 1:
        gui_view_b();
        break;
      case 2:
        gui_view_c();
        break;
      }
      if (g_gl_canvas)
        g_gl_canvas->update();
    });
    row2->addWidget(btn);
  }
  layout->addLayout(row2);

  layout->addStretch(); /* finally add space */

  return w;
}

/* Update the Editing panel with current selection data */
extern "C" void qt_update_edit_panel_qt()
{
  if (g_main_window)
    g_main_window->refresh_edit_panel();
}

void MainWindow::sync_edit_panel_if_visible()
{
  if (m_panelStack && m_panelStack->currentIndex() == 1)
    update_edit_panel(m_panelStack->widget(1));
}

void MainWindow::refresh_edit_panel()
{
  /* Block cellChanged during panel refresh to prevent cascade. */
  if (s_applying_edit_ptr)
    s_applying_edit_ptr->storeRelease(1);
  if (m_panelStack)
  {
    QWidget *panel = m_panelStack->widget(1);
    if (panel)
      update_edit_panel(panel);
  }
  if (s_applying_edit_ptr)
    s_applying_edit_ptr->storeRelease(0);
}

/* Refresh the spatial list in an open EditDialog. */
extern "C" void qt_refresh_edit_dialog_spatial_list(void)
{
  if (g_main_window && g_main_window->getEditDialog())
  {
    g_main_window->getEditDialog()->populate_spatial_list();
  }
}

static void update_edit_panel(QWidget *panel)
{
  if (!panel)
    return;

  /* Find the QTableWidget in the panel */
  auto *table = panel->findChild<QTableWidget *>();
  if (!table)
    return;

  struct model_pak *model = qt_get_active_model();
  if (!model)
    return;

  gint n = qt_model_selection_count(model);
  gdouble q = 0, m = 0, s = 0, centroid[3] = {0, 0, 0};
  struct core_pak *core = NULL;

  if (n > 0)
  {
    if (n == 1)
      core = (struct core_pak *) qt_model_selection_head(model);
    else
    {
      for (gint i = 0; i < n; i++)
      {
        core = (struct core_pak *) qt_selection_nth(model, i);
        centroid[0] += qt_core_x(core, 0);
        centroid[1] += qt_core_x(core, 1);
        centroid[2] += qt_core_x(core, 2);
        q += qt_core_charge(core);
        m += qt_core_mass(core);
        s += qt_core_has_sof(core) ? qt_core_sof(core) : 1.0;
      }
      centroid[0] /= (gdouble) n;
      centroid[1] /= (gdouble) n;
      centroid[2] /= (gdouble) n;
      s /= (gdouble) n;
      core = NULL;
    }
  }

  if (core)
  {
    table->item(0, 1)->setText(QString(qt_elem_symbol(qt_core_code(core))));
    table->item(1, 1)->setText(QString(qt_core_label(core)));
    table->item(2, 1)->setText(QString(qt_core_type(core)));
    table->item(3, 1)->setText(QString::number(qt_core_x(core, 0), 'f', 4));
    table->item(4, 1)->setText(QString::number(qt_core_x(core, 1), 'f', 4));
    table->item(5, 1)->setText(QString::number(qt_core_x(core, 2), 'f', 4));
    table->item(6, 1)->setText(QString::number(q, 'f', 4));
    table->item(7, 1)->setText(QString::number(m, 'f', 4));
    table->item(8, 1)->setText(qt_core_has_sof(core) ? QString::number(qt_core_sof(core), 'f', 4) : QString("1.0000"));
    table->item(9, 1)->setText(qt_core_growth(core) ? "Yes" : "No");
    table->item(10, 1)->setText(QString::number(qt_core_region(core) + 1));
    table->item(11, 1)->setText(qt_core_translate(core) ? "Yes" : "No");
  } else if (n > 1)
  {
    table->item(0, 1)->setText("");
    table->item(1, 1)->setText("centroid");
    table->item(2, 1)->setText("");
    table->item(3, 1)->setText(QString::number(centroid[0], 'f', 4));
    table->item(4, 1)->setText(QString::number(centroid[1], 'f', 4));
    table->item(5, 1)->setText(QString::number(centroid[2], 'f', 4));
    table->item(6, 1)->setText(QString::number(q, 'f', 4));
    table->item(7, 1)->setText(QString::number(m, 'f', 4));
    table->item(8, 1)->setText(QString::number(s, 'f', 4));
    table->item(9, 1)->setText("");
    table->item(10, 1)->setText("");
    table->item(11, 1)->setText("");
  } else
  {
    for (int i = 0; i < 12; i++)
      table->item(i, 1)->setText("");
  }
}

/* Update the Symmetry panel with current model symmetry info */
static void update_symmetry_panel(QWidget *panel)
{
  if (!panel)
    return;

  struct model_pak *model = qt_get_active_model();
  if (!model)
    return;

  auto *table = panel->findChild<QTableWidget *>();
  if (!table)
    return;

  table->setRowCount(0);

  gint periodic = qt_model_periodic(model);
  if (!periodic)
    return;

  int start_col = (periodic == 3) ? 0 : 3;
  const char *labels[] = {"space group", "system", "atoms",        "a",      "b", "c", "alpha",
                          "beta",        "gamma",  "surface area", "volume", NULL};

  for (int i = start_col; labels[i]; i++)
  {
    QString value;
    bool has_value = false;

    switch (i)
    {
    case 0:
      value = qt_model_sgname(model);
      has_value = true;
      break;
    case 1:
      value = qt_model_latticename(model);
      has_value = true;
      break;
    case 3:
      value = QString::number(qt_model_pbc(model, 0), 'f', 4);
      has_value = true;
      break;
    case 4:
      if (periodic > 1)
      {
        value = QString::number(qt_model_pbc(model, 1), 'f', 4);
        has_value = true;
      }
      break;
    case 5:
      if (periodic > 2)
      {
        value = QString::number(qt_model_pbc(model, 2), 'f', 4);
        has_value = true;
      }
      break;
    case 6:
      if (periodic > 2)
      {
        value = QString::number(57.295779513 * qt_model_pbc(model, 3), 'f', 2);
        has_value = true;
      }
      break;
    case 7:
      if (periodic > 2)
      {
        value = QString::number(57.295779513 * qt_model_pbc(model, 4), 'f', 2);
        has_value = true;
      }
      break;
    case 8:
      if (periodic > 1)
      {
        value = QString::number(57.295779513 * qt_model_pbc(model, 5), 'f', 2);
        has_value = true;
      }
      break;
    case 9:
      if (periodic == 2)
      {
        value = QString::number(qt_model_area(model), 'f', 4);
        has_value = true;
      }
      break;
    case 10:
      if (periodic == 3)
      {
        value = QString::number(qt_model_volume(model), 'f', 2);
        has_value = true;
      }
      break;
    }

    if (has_value)
    {
      table->insertRow(table->rowCount());
      table->setItem(table->rowCount() - 1, 0, new QTableWidgetItem(value));
    }
  }
}

/* Model mode combobox: switches between active model panels */
void MainWindow::on_model_mode_changed(int index)
{
  extern void qt_update_content_table();

  /* Switch panel via stacked widget */
  m_panelStack->setCurrentIndex(index);

  switch (index)
  {
  case 0: /* Content */
  {
    struct model_pak *model = qt_get_active_model();
    if (!model)
      break;
    /* Verify model is still in the active model list (not freed) */
    if (qt_model_valid(model))
      qt_gui_content_refresh(NULL);
    break;
  }
  case 1: /* Editing */
    update_edit_panel(m_panelStack->widget(1));
    break;
  case 4: /* Symmetry */
    update_symmetry_panel(m_panelStack->widget(4));
    extern void qt_gui_symmetry_refresh(gpointer);
    qt_gui_symmetry_refresh(NULL);
    break;
  }

  /* Force redraw */
  m_canvas->update();

  /* Sync image spinners when switching to Images panel */
  if (index == 3)
  {
    extern void qt_image_spinner_sync(void);
    qt_image_spinner_sync();
  }
}

/* Selection mode combobox: sets the selection mode */
void MainWindow::updateOverlayGeometry()
{
  if (!m_canvas || !m_textOverlay)
    return;
  m_textOverlay->setGeometry(m_canvas->geometry());
}

void MainWindow::forceCanvasResize()
{
  if (!m_canvas)
    return;

  /* Force QOpenGLWidget to recompute its viewport by triggering a redraw.
   * This is called from resizeEvent and WindowStateChange handlers as a
   * safety net. The real fix is the central widget event filter below, which
   * catches layout-driven resizes that QMainWindow skips for QOpenGLWidget. */
  m_canvas->update();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
  if (obj == m_canvas && event->type() == QEvent::Resize)
  {
    updateOverlayGeometry();
  }

  /* Catch dock widget hide/show events — forces canvas redraw when docks toggle. */
  if ((event->type() == QEvent::Hide || event->type() == QEvent::Show) && qobject_cast<QDockWidget *>(obj))
  {
    forceCanvasResize();
  }

  /* Catch central widget resize events — catches layout-driven resizes.
   * We force the canvas to recompute its viewport. */
  if (event->type() == QEvent::Resize && obj == centralWidget())
  {
    if (m_canvas)
      m_canvas->update();
  }

  /* Handle Enter/Return key on the edit table for inline editing. */
  if (event->type() == QEvent::KeyPress && obj == s_edit_table)
  {
    auto *ke = static_cast<QKeyEvent *>(event);
    if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter)
    {
      s_edit_table->edit(s_edit_table->currentIndex());
      return true;
    }
  }
  return QMainWindow::eventFilter(obj, event);
}

void MainWindow::on_select_mode_changed(int index)
{
  extern void gui_mode_switch(gint);
  extern void redraw_canvas(gint);

  /* Map combobox index to select_mode enum values (gdis.h) */
  /* CORE=1, ATOM_LABEL=8, ATOM_TYPE=9, ELEM=7, ELEM_MOL=10, MOL=5, FRAGMENT=11, REGION=12 */
  static const gint modes[] = {1, 8, 9, 7, 10, 5, 11, 12};

  if (index < 0 || index >= 8)
    return;

  /* Set the selection mode via bridge function */
  qt_set_select_mode(modes[index]);

  /* For fragment mode, also call gui_mode_switch(SELECT_FRAGMENT) */
  if (index == 6) /* FRAGMENT */
  {
    extern void gui_mode_switch(gint);
    gui_mode_switch(50); /* SELECT_FRAGMENT = 50 from interface.h */
  }

  /* Force redraw */
  m_canvas->update();
}

/* Qt bridge: show animation dialog */
extern "C" void qt_show_animate_dialog(struct model_pak *model)
{
  AnimationDialog *dlg = new AnimationDialog(model, g_main_window);
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->show();
}

/* Qt bridge: get GL canvas pointer */
extern "C" void *qt_get_gl_canvas() { return static_cast<void *>(g_gl_canvas); }

extern "C" void qt_repaint_canvas()
{
  if (g_gl_canvas)
    g_gl_canvas->update();
}

/* Push text overlays from C to Qt widget — C linkage.
 * Reads directly from C-side overlay storage to avoid
 * passing raw pointers across the C/C++ boundary. */
extern "C" void qt_text_overlay_widget_push(void)
{
  if (!g_main_window)
    return;

  /* Forward declaration — defined in gl_main.c */
  extern int qt_text_overlay_count_raw(void);
  extern void qt_text_overlay_get_raw(gint idx, gint * out_x, gint * out_y, gint * out_w, gint * out_h);
  extern unsigned char *qt_text_overlay_get_buf_raw(gint idx);

  /* Use a temporary to hold the widget pointer (avoids private access) */
  TextOverlayWidget *widget = g_main_window->getOverlayWidget();
  if (!widget)
    return;

  gint count = qt_text_overlay_count_raw();
  if (count <= 0)
  {
    widget->clearOverlays();
    return;
  }

  QVector<TextOverlay> overlays;
  for (gint i = 0; i < count; i++)
  {
    gint x, y, w, h;
    qt_text_overlay_get_raw(i, &x, &y, &w, &h);
    unsigned char *buf = qt_text_overlay_get_buf_raw(i);
    if (!buf || w <= 0 || h <= 0)
      continue;

    /* Deep copy the pixel data into a QImage */
    QImage img(buf, w, h, w * 4, QImage::Format_ARGB32);
    img = img.copy(); /* Make an owned copy */
    if (!img.isNull())
    {
      TextOverlay ov;
      ov.x = x;
      ov.y = y;
      ov.image = img;
      overlays.append(ov);
    }
  }
  widget->addOverlays(overlays);
}

extern "C" void qt_force_overlay_repaint(void)
{
  if (g_main_window)
    g_main_window->getOverlayWidget()->update();
}

/* C bridge: populate the content table with model properties */
extern "C" void qt_content_table_populate(gpointer table_ptr)
{
  QTableWidget *table = static_cast<QTableWidget *>(table_ptr);
  if (!table)
    return;

  /* Get property count and data from C side */
  extern int qt_get_property_count(void);
  extern const char *qt_get_property_label(int index);
  extern const char *qt_get_property_value(int index);

  int count = qt_get_property_count();
  table->setRowCount(0);

  /* Count non-empty, non-dummy properties first */
  int visible = 0;
  for (int i = 0; i < count; i++)
  {
    const char *label = qt_get_property_label(i);
    const char *value = qt_get_property_value(i);
    if (label && label[0] && value && value[0] && strcmp(value, "dummy") != 0)
      visible++;
  }

  table->setRowCount(visible);

  /* Populate only non-empty, non-dummy rows */
  int row = 0;
  for (int i = 0; i < count; i++)
  {
    const char *label = qt_get_property_label(i);
    const char *value = qt_get_property_value(i);
    if (label && label[0] && value && value[0] && strcmp(value, "dummy") != 0)
    {
      table->setItem(row, 0, new QTableWidgetItem(QString::fromUtf8(label)));
      table->setItem(row, 1, new QTableWidgetItem(QString::fromUtf8(value)));
      row++;
    }
  }
}

extern "C" void qt_refresh_qt_tree()
{
  if (g_main_window)
    g_main_window->refresh_qt_tree();
}

void MainWindow::refresh_qt_tree()
{
  extern void qt_tree_refresh(void);
  extern int qt_tree_get_count(void);
  extern const char *qt_tree_model_name(int);
  extern int qt_tree_model_type(int);
  extern int qt_tree_node_depth(int);
  extern const char *qt_tree_node_icon(int);

  qt_tree_refresh();

  /* Suspend signals during bulk rebuild to avoid UI freezes */
  m_modelTree->blockSignals(true);
  m_treeModel->clear();
  m_treeModel->setHorizontalHeaderLabels({"", "Model"});

  /* Build hierarchical tree: models at depth 0, graphs as children */
  int count = qt_tree_get_count();
  QStandardItem *currentModelItem = nullptr;

  for (int i = 0; i < count; i++)
  {
    int depth = qt_tree_node_depth(i);
    const char *icon_name = qt_tree_node_icon(i);
    QStandardItem *iconItem = new QStandardItem();
    if (icon_name)
    {
      iconItem->setIcon(loadIcon(icon_name, m_modelTree));
    }
    QStandardItem *nameItem = new QStandardItem(QString(qt_tree_model_name(i)));
    nameItem->setData(i, Qt::UserRole);
    nameItem->setEditable(false);

    if (depth == 0)
    {
      /* Model node: add as top-level row */
      m_treeModel->setItem(m_treeModel->rowCount(), 0, iconItem);
      m_treeModel->setItem(m_treeModel->rowCount() - 1, 1, nameItem);
      currentModelItem = m_treeModel->item(m_treeModel->rowCount() - 1);
    } else
    {
      /* Graph node: add as child of current model */
      if (currentModelItem)
      {
        currentModelItem->setChild(currentModelItem->rowCount(), 0, iconItem);
        currentModelItem->setChild(currentModelItem->rowCount() - 1, 1, nameItem);
      }
    }
  }

  m_modelTree->blockSignals(false);
  m_modelTree->update();
}

int MainWindow::get_selected_tree_index()
{
  QModelIndex current = m_modelTree->selectionModel()->currentIndex();
  if (!current.isValid())
    return -1;
  int idx = current.data(Qt::UserRole).toInt();
  return idx;
}

/* C bridge: get selected tree index from Qt */
extern "C" int MainWindow_get_selected_tree_index(void)
{
  if (!g_main_window)
    return -1;
  return g_main_window->get_selected_tree_index();
}

/* C bridge: signal that canvas needs redraw.
 * The render loop timer will pick up the dirty flag and re-render at ~60fps.
 * This provides smooth, consistent frame pacing during drag rotation. */
extern "C" void qt_force_canvas_refresh(void)
{
  /* Refresh edit panel whenever canvas needs repainting and edit panel is visible. */
  if (g_main_window)
    g_main_window->sync_edit_panel_if_visible();
  /* Trigger repaint in Qt mode when redraw_canvas() is called from C code. */
  if (g_gl_canvas)
    g_gl_canvas->update();
}

/* View and track functions — moved from gui_still_valid.c */
extern "C" void gui_view_default(void)
{
  struct model_pak *model;

  /* do we have a loaded model? */
  model = (struct model_pak *) sysenv.active_model;
  if (!model)
    return;

  /* NEW - recentre (eg if atoms added -> centroid change) */
  coords_init(CENT_COORDS, model);

  /* make the changes */
  VEC3SET(model->offset, 0.0, 0.0, 0.0);

  /* update */
  camera_init(model);

  /* trigger any dialog updates */
  gui_model_select(model);
}

extern "C" void gui_track_output(void)
{
  struct model_pak *model;
  gchar *ptr;
  /**/
  model = (struct model_pak *) sysenv.active_model;
  if (!model)
    return;
  /*invert current tracking**/
  model->track_me = (model->track_me == FALSE);
  switch (model->id)
  {
  case USPEX:
    if (model->track_me)
    {
      extern void gui_text_show(gint type, const gchar *msg);
      gui_text_show(1, "Tracking USPEX output...\n");
    }
    break;
  case GULP:
    if (model->track_me)
    {
      extern void gui_text_show(gint type, const gchar *msg);
      gui_text_show(1, "Tracking GULP output...\n");
    }
    break;
  case VASP:
    if (model->track_me)
    {
      extern void gui_text_show(gint type, const gchar *msg);
      gui_text_show(1, "Tracking VASP output...\n");
    }
    break;
  default:
    if (model->track_me)
    {
      extern void gui_text_show(gint type, const gchar *msg);
      gui_text_show(1, "Tracking output...\n");
    }
    break;
  }
}

extern "C" void qt_gui_content_refresh(gpointer dummy)
{
  struct model_pak *model = qt_get_active_model();
  if (!model)
    return;

  qt_update_content_table();
}

/* EOF: catastophic edit */

/* Qt bridge: set nanotube chirality values. */
// extern "C" void qt_set_edit_chirality(int idx, gdouble val)
//{
//   /* Chirality is stored in the edit dialog's state. This is a no-op stub.
//    * The actual chirality values are applied when the nanotube is created. */
// }
// extern "C" gboolean sysenv_ignore_tree_select(void) { return FALSE; }
//
///* Qt bridge: add model to tree and redraw canvas. */
// extern "C" void tree_model_add(struct model_pak *model)
//{
//   /* Stub — model already in sysenv.mal, just trigger refresh */
//   if (g_main_window)
//     qt_refresh_qt_tree();
// }
//
// extern "C" void redraw_canvas(gint region)
//{
//   /* Trigger repaint in Qt mode. */
//   if (g_gl_canvas)
//     g_gl_canvas->update();
// }
// extern "C" void qt_update_content_table(void) { /* stub */ }
// extern "C" void qt_tree_refresh(void) { /* stub */ }
// extern "C" void qt_set_content_table(gpointer table_ptr) { /* stub */ }
// extern "C" void qt_gui_symmetry_refresh(void) { /* stub */ }
