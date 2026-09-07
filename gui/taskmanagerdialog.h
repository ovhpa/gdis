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

#ifndef TASKMANAGERDIALOG_H
#define TASKMANAGERDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QHeaderView>
#include <QLabel>
#include <QScrollBar>

struct task_pak;

class TaskManagerDialog : public QDialog
{
  Q_OBJECT

public:
  explicit TaskManagerDialog(QWidget *parent = nullptr);
  ~TaskManagerDialog();

private:
  Q_SLOT void on_refresh();
  Q_SLOT void on_kill_selected();
  Q_SLOT void on_kill_all();
  Q_SLOT void on_remove_completed();
  Q_SLOT void on_close();

private:
  void populateTable();
  void updateStatusText();

  QTableWidget *m_table;
  QPlainTextEdit *m_statusText;
  QTimer *m_timer;
  struct task_pak *m_lastSelectedTask;
  int m_lastLineCount;
};

#endif // TASKMANAGERDIALOG_H
