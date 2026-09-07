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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <glib.h>
#include <QDoubleSpinBox>
#include <QTextEdit>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QComboBox>
#include <QStackedWidget>
#include <QDockWidget>

class GLCanvas;
class TextOverlayWidget;
class QSplitter;
class QTreeView;
class QStandardItemModel;
class QPlainTextEdit;
class QTableWidget;
class EditDialog;

/* Forward declaration for spatial refresh */
class EditDialog;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

protected:
  bool event(QEvent *event) override;
  bool eventFilter(QObject *obj, QEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void closeEvent(QCloseEvent *event) override;

private slots:
  /* File */
  void on_file_new();
  void on_file_open();
  void on_file_save();
  void on_file_close();
  void on_import_geomview();
  void on_import_project();
  void on_import_graph();
  void on_export_canvas();
  void on_export_graph_data();
  /*
    void on_export_graph_data();
  */

  /* Edit */
  void on_edit_undo();
  void on_edit_copy();
  void on_edit_paste();
  void on_edit_colour();
  void on_edit_delete();
  void on_edit_select_all();
  void on_edit_invert();
  void on_edit_hide();
  void on_edit_hide_unsel();
  void on_edit_unhide();

  /* Tools */
  void on_tools_animation();
  void on_tools_isosurfaces();
  void on_tools_periodic_table();
  void on_tools_editing();
  void on_tools_dislocations();
  void on_tools_docking();
  void on_tools_dynamics();
  void on_tools_surfaces();
  void on_tools_zmatrix();
  void on_tools_diffract();
  void on_tools_gulp();
  void on_tools_gamess();
  void on_tools_monty();
  void on_tools_siesta();
  void on_tools_vasp();
  void on_tools_uspex();
  void on_tools_analysis();
  void on_tools_measure();
  void on_tools_plots();

  /* View */
  void on_view_render_props();
  void on_view_reset_images();
  void on_view_normal_mode();
  void on_view_record_mode();
  void on_view_task_manager();
  void on_view_exec_paths();
  void on_view_default();
  void on_view_x();
  void on_view_y();
  void on_view_z();
  void on_view_a();
  void on_view_b();
  void on_view_c();
  void on_view_track_output();

  /* Rotation */
  void on_rotate_x();
  void on_rotate_y();
  void on_rotate_z();

  /* Help */
  void on_help_about();
  void on_help_manual();

  /* Existing */
  void on_about();
  void on_quit();
  void on_refresh();
  void on_single_view();
  void on_create_canvas();
  void on_delete_canvas();

  /* Tree */
  void on_tree_selection_changed(const QModelIndex &current, const QModelIndex &previous);

  /* Model mode combobox */
  void on_model_mode_changed(int index);

  /* Selection mode combobox */
  void on_select_mode_changed(int index);

  /* Update text overlay geometry to match canvas */

public:
  void refresh_qt_tree();
  void refresh_edit_panel();
  const gint *get_edit_type_map() const { return m_edit_type_map; }
  void sync_edit_panel_if_visible();

  /* Update text overlay geometry to match canvas. */
  void updateOverlayGeometry();
  void pushTextOverlays();
  void setOverlayWidget(TextOverlayWidget *w);
  TextOverlayWidget *getOverlayWidget() { return m_textOverlay; }
  int get_selected_tree_index();

private:
  void setupMenuBar();
  void setupToolBar();
  void setupDockWidgets();
  void setupCentralWidget();

  /* Force canvas resize/redraw — used after dock layout changes */
  void forceCanvasResize();

  GLCanvas *m_canvas;
  TextOverlayWidget *m_textOverlay;
  QDoubleSpinBox *m_angleSpin;

  QSplitter *m_splitter;
  QTreeView *m_modelTree;
  QStandardItemModel *m_treeModel;

  QTextEdit *m_textOutput;

  /* Mode comboboxes */
  QComboBox *m_modelModeCombo;
  QComboBox *m_selectModeCombo;

  /* Model content table */
  QTableWidget *m_contentTable;

  /* Pointer to the edit panel table for event filter. */
  QTableWidget *m_editPanelTable;

  /* Pointer to the open EditDialog (for spatial list refresh). */
  EditDialog *m_editDialog = nullptr;

  /* Panel widgets for each model mode — added to treeLayout */
  QStackedWidget *m_panelStack;

  /* Edit type mapping: Qt panel row -> C enum type */
  gint m_edit_type_map[12];

public:
  EditDialog *getEditDialog() const { return m_editDialog; }
};

#endif /* MAINWINDOW_H */
