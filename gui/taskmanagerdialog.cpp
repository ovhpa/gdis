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

#include "taskmanagerdialog.h"
#include "gdis_api.h"

#include "gdis.h"
#include "task.h"
#include "interface.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QMessageBox>
#include <QDialog>
#include <QCoreApplication>
#include <QApplication>

extern struct sysenv_pak sysenv;

/* Forward declarations from qt_api.c */
extern "C" void qt_task_kill_running(struct task_pak *);

TaskManagerDialog::TaskManagerDialog(QWidget *parent)
    : QDialog(parent), m_table(nullptr), m_statusText(nullptr), m_timer(nullptr), m_lastSelectedTask(nullptr),
      m_lastLineCount(0)
{
  setWindowTitle(tr("Task Manager"));
  resize(700, 500);

  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(8, 6, 8, 6);
  mainLayout->setSpacing(6);

  /* Task table */
  m_table = new QTableWidget();
  m_table->setColumnCount(6);
  m_table->setHorizontalHeaderLabels({"PID", "Job", "Status", "% CPU", "% Mem", "Time"});
  m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
  m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
  m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
  m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
  m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
  m_table->setColumnWidth(0, 60);
  m_table->setColumnWidth(2, 80);
  m_table->setColumnWidth(3, 70);
  m_table->setColumnWidth(4, 70);
  m_table->setColumnWidth(5, 80);
  m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_table->setSelectionMode(QAbstractItemView::SingleSelection);
  m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  mainLayout->addWidget(m_table);

  /* Status text — use QPlainTextEdit for better large-text performance */
  auto *statusLabel = new QLabel(tr("Status Output"));
  mainLayout->addWidget(statusLabel);
  m_statusText = new QPlainTextEdit();
  m_statusText->setReadOnly(true);
  m_statusText->setLineWrapMode(QPlainTextEdit::NoWrap);
  mainLayout->addWidget(m_statusText);

  /* Buttons */
  auto *btnLayout = new QHBoxLayout();
  btnLayout->addStretch();
  auto *killBtn = new QPushButton(tr("Kill Selected"));
  auto *killAllBtn = new QPushButton(tr("Kill All"));
  auto *removeBtn = new QPushButton(tr("Remove Completed"));
  auto *closeBtn = new QPushButton(tr("Close"));
  btnLayout->addWidget(killBtn);
  btnLayout->addWidget(killAllBtn);
  btnLayout->addWidget(removeBtn);
  btnLayout->addWidget(closeBtn);
  mainLayout->addLayout(btnLayout);

  connect(killBtn, &QPushButton::clicked, this, &TaskManagerDialog::on_kill_selected);
  connect(killAllBtn, &QPushButton::clicked, this, &TaskManagerDialog::on_kill_all);
  connect(removeBtn, &QPushButton::clicked, this, &TaskManagerDialog::on_remove_completed);
  connect(closeBtn, &QPushButton::clicked, this, &TaskManagerDialog::on_close);

  /* Periodic refresh */
  m_timer = new QTimer(this);
  m_timer->setInterval(1000);
  connect(m_timer, &QTimer::timeout, this, &TaskManagerDialog::on_refresh);
  m_timer->start();

  /* Initial populate */
  on_refresh();
}

TaskManagerDialog::~TaskManagerDialog() {}

void TaskManagerDialog::on_refresh()
{
  /* Save selected row */
  int savedRow = -1;
  if (m_table->selectionModel()->hasSelection())
    savedRow = m_table->selectionModel()->selectedIndexes().first().row();

  populateTable();

  /* Restore selection */
  if (savedRow >= 0 && savedRow < m_table->rowCount())
    m_table->selectRow(savedRow);

  updateStatusText();
}

void TaskManagerDialog::populateTable()
{
  m_table->setSortingEnabled(false);

  /* Collect task pointers by status */
  struct task_pak **tasks[3] = {nullptr, nullptr, nullptr};
  int counts[3] = {0, 0, 0};

  for (GSList *tlist = sysenv.task_list; tlist; tlist = g_slist_next(tlist))
  {
    struct task_pak *task = (struct task_pak *) tlist->data;
    if (task->status == REMOVED)
      continue;
    if (task->status == QUEUED)
      continue;

    int idx = -1;
    if (task->status == RUNNING)
      idx = 0;
    else if (task->status == KILLED)
      idx = 1;
    else if (task->status == COMPLETED)
      idx = 2;

    if (idx >= 0)
    {
      tasks[idx] = (struct task_pak **) g_realloc(tasks[idx], (counts[idx] + 1) * sizeof(struct task_pak *));
      tasks[idx][counts[idx]++] = task;
    }
  }

  /* Build table */
  m_table->setRowCount(0);
  int row = 0;
  for (int s = 0; s < 3; s++)
  {
    for (int i = 0; i < counts[s]; i++)
    {
      struct task_pak *task = tasks[s][i];
      m_table->insertRow(row);

      /* PID */
      auto *pidItem = new QTableWidgetItem();
      pidItem->setText(QString::number(task->pid));
      pidItem->setTextAlignment(Qt::AlignCenter);
      m_table->setItem(row, 0, pidItem);

      /* Job */
      auto *jobItem = new QTableWidgetItem(QString::fromUtf8(task->label ? task->label : "Unknown"));
      m_table->setItem(row, 1, jobItem);

      /* Status + calc info for running tasks */
      auto *statusItem = new QTableWidgetItem();
      statusItem->setTextAlignment(Qt::AlignCenter);
      if (task->status == RUNNING)
      {
        /* Call calc_task_info to update pcpu/pmem from ps */
        extern void qt_task_calc_info(struct task_pak *);
        qt_task_calc_info(task);

        char prog[16];
        if (task->progress > 0.0)
          snprintf(prog, sizeof(prog), "%5.1f%%", task->progress);
        else
          strncpy(prog, "Running", sizeof(prog));
        statusItem->setText(QString::fromUtf8(prog));
      } else if (task->status == KILLED)
      {
        statusItem->setText(tr("Killed"));
        /* Reset CPU/Mem for killed/completed tasks */
        task->pcpu = 0.0;
        task->pmem = 0.0;
      } else if (task->status == COMPLETED)
      {
        statusItem->setText(tr("Completed"));
        /* Reset CPU/Mem for killed/completed tasks */
        task->pcpu = 0.0;
        task->pmem = 0.0;
      }
      m_table->setItem(row, 2, statusItem);

      /* % CPU */
      auto *cpuItem = new QTableWidgetItem();
      cpuItem->setText(QString::number(task->pcpu, 'f', 1));
      cpuItem->setTextAlignment(Qt::AlignCenter);
      m_table->setItem(row, 3, cpuItem);

      /* % Mem */
      auto *memItem = new QTableWidgetItem();
      memItem->setText(QString::number(task->pmem, 'f', 1));
      memItem->setTextAlignment(Qt::AlignCenter);
      m_table->setItem(row, 4, memItem);

      /* Time */
      auto *timeItem = new QTableWidgetItem(QString::fromUtf8(task->time ? task->time : ""));
      timeItem->setTextAlignment(Qt::AlignLeft);
      m_table->setItem(row, 5, timeItem);

      /* Store task pointer in item for selection */
      m_table->item(row, 0)->setData(Qt::UserRole, reinterpret_cast<qlonglong>(task));

      row++;
    }
  }

  m_table->setSortingEnabled(true);

  /* Free temp pointer arrays */
  for (int s = 0; s < 3; s++)
    g_free(tasks[s]);
}

void TaskManagerDialog::updateStatusText()
{
  /* Get selected task */
  QModelIndexList selected = m_table->selectionModel()->selectedIndexes();
  if (selected.isEmpty())
  {
    m_statusText->clear();
    m_lastSelectedTask = nullptr;
    m_lastLineCount = 0;
    return;
  }

  int row = selected.first().row();
  struct task_pak *task = reinterpret_cast<struct task_pak *>(m_table->item(row, 0)->data(Qt::UserRole).toLongLong());

  if (!task)
    return;

  /* Always update status from file — even for completed/killed tasks,
   * the file may have grown in a final burst when the process exited */
  extern void qt_task_status_update(struct task_pak *);
  qt_task_status_update(task);

  if (task->status_text && task->status_text->len > 0)
  {
    /* Always show last N lines of output to avoid Qt text layout crashes */
    const int MAX_DISPLAY_LINES = 1000;

    QString fullText = QString::fromUtf8(task->status_text->str);

    /* Split into lines and keep only the last MAX_DISPLAY_LINES */
    QStringList lines = fullText.split('\n', Qt::SkipEmptyParts);
    if (lines.size() > MAX_DISPLAY_LINES)
      lines = lines.mid(lines.size() - MAX_DISPLAY_LINES);

    QString displayText = lines.join('\n');

    /* For completed/killed tasks, add a header with total line count */
    if (task->status == COMPLETED || task->status == KILLED)
    {
      int totalLines = fullText.split('\n', Qt::SkipEmptyParts).size();
      displayText =
          QString("[Completed — %1 total lines, showing last %2]\n").arg(totalLines).arg(lines.size()) + displayText;
    }

    m_statusText->setPlainText(displayText);
    m_statusText->verticalScrollBar()->setValue(m_statusText->verticalScrollBar()->maximum());
  } else
  {
    m_statusText->clear();
    m_lastLineCount = 0;
  }

  m_lastSelectedTask = task;
  m_lastLineCount = task->status_text ? task->status_text->len : 0;
}

void TaskManagerDialog::on_kill_selected()
{
  QModelIndexList selected = m_table->selectionModel()->selectedIndexes();
  if (selected.isEmpty())
    return;

  int row = selected.first().row();
  struct task_pak *task = reinterpret_cast<struct task_pak *>(m_table->item(row, 0)->data(Qt::UserRole).toLongLong());

  if (!task)
    return;

  if (task->status == RUNNING)
    qt_task_kill_running(task);
  else if (task->status == QUEUED)
    task->status = KILLED;

  on_refresh();
}

void TaskManagerDialog::on_kill_all()
{
  /* Kill all queued first */
  for (GSList *tlist = sysenv.task_list; tlist; tlist = g_slist_next(tlist))
  {
    struct task_pak *task = (struct task_pak *) tlist->data;
    if (task->status == QUEUED)
      task->status = KILLED;
  }

  /* Then kill all running */
  for (GSList *tlist = sysenv.task_list; tlist; tlist = g_slist_next(tlist))
  {
    struct task_pak *task = (struct task_pak *) tlist->data;
    if (task->status == RUNNING)
      qt_task_kill_running(task);
  }

  on_refresh();
}

void TaskManagerDialog::on_remove_completed()
{
  bool removed = false;
  for (GSList *tlist = sysenv.task_list; tlist;)
  {
    struct task_pak *task = (struct task_pak *) tlist->data;
    tlist = g_slist_next(tlist);
    if (task->status == COMPLETED || task->status == KILLED)
    {
      sysenv.task_list = g_slist_remove(sysenv.task_list, task);
      task_free(task);
      removed = true;
    }
  }

  if (removed)
    on_refresh();
}

void TaskManagerDialog::on_close()
{
  if (m_timer)
    m_timer->stop();
  close();
}

/* Bridge function */
extern "C" void qt_show_task_manager_dialog(void)
{
  extern QWidget *get_main_window_widget();
  TaskManagerDialog *dlg = new TaskManagerDialog(get_main_window_widget());
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->show();
  dlg->raise();
  dlg->activateWindow();
}

/* Kill confirmation dialog — called from C core via task_confirm_callback */
extern "C" gboolean qt_confirm_kill(gint *pids, gint count)
{
  if (!pids || count <= 0)
    return TRUE;

  /* Build PID list string */
  GString *pid_str = g_string_new("PIDs: ");
  for (int i = 0; i < count; i++)
  {
    if (i > 0)
      g_string_append(pid_str, ", ");
    g_string_append_printf(pid_str, "%d", pids[i]);
  }

  /* Show confirmation dialog */
  QMessageBox msgBox(
      QMessageBox::Warning, QCoreApplication::translate("TaskManager", "Confirm Kill"),
      QString(QCoreApplication::translate(
                  "TaskManager",
                  "Confirm the killing of %1 process(es)\n\n%2\n\nThis will send SIGTERM followed by SIGKILL."))
          .arg(count)
          .arg(pid_str->str),
      QMessageBox::No | QMessageBox::Yes);
  msgBox.setButtonText(QMessageBox::Yes, QCoreApplication::translate("TaskManager", "Kill"));
  msgBox.setButtonText(QMessageBox::No, QCoreApplication::translate("TaskManager", "Cancel"));
  msgBox.setDefaultButton(QMessageBox::No);

  gboolean result = (msgBox.exec() == QMessageBox::Yes);

  g_string_free(pid_str, TRUE);
  return result;
}
