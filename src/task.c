/*
Copyright (C) 2000 by Sean David Fleming

sean@ivec.org

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

/* irix / BSD: enable POSIX signal semantics on legacy systems. Not needed on macOS (Darwin) or modern Linux. */
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sgi__)
#define _BSD_SIGNALS 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>

#include "gdis.h"
#include "file.h"
#include "task.h"
#include "job.h"
#include "grid.h"
#include "interface.h"

/* top level data structure */
extern struct sysenv_pak sysenv;

/* Global mutex for thread-safe GUI updates (replaces gdk_threads_enter/leave) */
GMutex *gdis_threads_mutex = NULL;

/* Callback for confirming process kills (set by Qt GUI layer). */
confirm_kill_func task_confirm_callback = NULL;
void task_set_confirm_callback(confirm_kill_func cb) { task_confirm_callback = cb; }

void ensure_threads_mutex(void)
{
  if (!gdis_threads_mutex)
  {
    static GMutex *init_lock = NULL;
    if (!init_lock)
    {
      init_lock = g_mutex_new();
    }
    g_mutex_lock(init_lock);
    if (!gdis_threads_mutex)
      gdis_threads_mutex = g_mutex_new();
    g_mutex_unlock(init_lock);
  }
}

#ifndef __WIN32
#include <sys/wait.h>
#endif

/**********************************************/
/* execute task in thread created by the pool */
/**********************************************/
#define DEBUG_TASK_PROCESS 0
void task_process(struct task_pak *task, gpointer data)
{
  /* checks */
  if (!task)
    return;
  if (task->status != QUEUED)
    return;

  /* setup for current task */
  if (!task->is_async)
    task->pid = getpid();
  /* TODO - should be mutex locking this */
  task->status = RUNNING;

  /* NB: the primary task needs to not do anything that could */
  /* cause problems eg update GUI elements since this can cause conflicts */

  /* execute the primary task */
  task->primary(task->ptr1, task);
  if (task->is_async)
  {
    pid_t w;
    gint status = 0;
    fprintf(stderr, "[task_process] waiting for async pid=%d\n", task->pid);
    /*here we NEED to wait for async task to die*/
    do
    {
      w = waitpid(task->pid, &status, WUNTRACED | WCONTINUED);
      if (w == -1)
      {
        /* ECHILD: child already reaped (shouldn't happen),
         * EINTR: interrupted by signal, just retry.
         * Any other error: treat as fatal. */
        if (errno == EINTR)
          continue;
        if (errno == ECHILD)
        {
          fprintf(stdout, "TASK %i: no child process (already reaped)\n", task->pid);
          break;
        }
        fprintf(stdout, "TASK %i: waitpid failed, errno=%d (%s)\n", task->pid, errno, strerror(errno));
        task->status = KILLED;
        break;
      }
      if (WIFSIGNALED(status))
        task->status = KILLED; /*<-necessary?*/
#if DEBUG_TASK_PROCESS
      /*do we have checkpoint/restart?*/
      if (WIFSTOPPED(status))
        fprintf(stdout, "TASK %i stopped!\n", task->pid);
      else if (WIFCONTINUED(status))
        fprintf(stdout, "TASK %i continued!\n", task->pid);
#endif
    } while ((!WIFEXITED(status)) && (!WIFSIGNALED(status)));
  }

  /* NB: GUI updates should ALL be done in the cleanup task, since we */
  /* do the threads enter to avoid problems - this locks the GUI */
  /* until we call the threads leave function at the end */

  ensure_threads_mutex();
  g_mutex_lock(gdis_threads_mutex);

  fprintf(stderr, "[task_process] pre-cleanup: status=%d (QUEUED=%d RUNNING=%d KILLED=%d COMPLETED=%d)\n", task->status,
          0, 1, 2, 3);
  if (task->status != KILLED)
  {
    /* execute the cleanup task */
    if (task->cleanup)
    {
      fprintf(stderr, "[task_process] running cleanup for task: %s\n", task->label);
      task->cleanup(task->ptr2);
    }
    task->status = COMPLETED;
  } else
  {
    fprintf(stderr, "[task_process] SKIPPING cleanup: status is KILLED\n");
  }

  g_mutex_unlock(gdis_threads_mutex);

  /* job completion notification */
  /* Qt: gdk_beep() has no equivalent in Qt without a GdkDisplay;
   * use a harmless no-op to avoid GDK assertion failures. */
  /* gdk_beep(); */
}

/***************************************************/
/* set up the thread pool to process task requests */
/***************************************************/
void task_queue_init(void)
{
#ifdef G_THREADS_ENABLED
  // g_thread_supported is now depreacted and replaced by a define:
  // #define g_thread_supported()     (1)
  // if (!g_thread_supported())
  //   {
  /* TODO - disallow queueing of background tasks if this happens */
  //  gui_text_show(ERROR, "Task queue initialization failed.\n");
  //  }
  // else
  sysenv.thread_pool = g_thread_pool_new((GFunc) task_process, NULL, sysenv.max_threads, FALSE, NULL);
#endif
}

/*****************************/
/* terminate the thread pool */
/*****************************/
void task_queue_free(void) { g_thread_pool_free(sysenv.thread_pool, TRUE, FALSE); }

/*************************/
/* free a task structure */
/*************************/
void task_free(gpointer data)
{
  struct task_pak *task = data;

  g_assert(task != NULL);

  g_free(task->label);
  g_free(task->time);
  g_free(task->message);
  g_free(task->status_file);

  if (task->status_fp)
    fclose(task->status_fp);
  task->status_fp = NULL;

  g_string_free(task->status_text, TRUE);

  g_free(task);
}

/****************************/
/* submit a background task */
/****************************/
/* TODO - only show certain tasks in the manager, since this */
/* could be used to do any tasks in the background - some of */
/* which may be slow GUI tasks we dont want to be cancellable */
void task_new(const gchar *label, gpointer func1, gpointer arg1, gpointer func2, gpointer arg2, gpointer model)
{
  struct task_pak *task;

  /* duplicate the task data */
  task = g_malloc(sizeof(struct task_pak));
  sysenv.task_list = g_slist_prepend(sysenv.task_list, task);
  task->is_async = FALSE;
  task->pid = -1;
  task->status = QUEUED;
  task->time = NULL;
  task->message = NULL;
  task->pcpu = 0.0;
  task->pmem = 0.0;
  task->progress = 0.0;
  task->locked_model = model;

  task->status_file = NULL;
  task->status_fp = NULL;
  task->status_fp_pos = 0L;
  task->status_index = -1;
  task->status_text = g_string_new(NULL);

  task->label = g_strdup(label);
  task->primary = func1;
  task->cleanup = func2;
  task->ptr1 = arg1;
  task->ptr2 = arg2;
  /*
  if (model)
    ((struct model_pak *) model)->locked = TRUE;
  */

  /* queue the task */
  g_thread_pool_push(sysenv.thread_pool, task, NULL);
}

/**************************************/
/* submit a system task:              */
/* tentative to get some tasks hidden */
/* and executed immediately.. (OVHPA) */
/**************************************/
void task_system_new(const gchar *label, gpointer func1, gpointer arg1, gpointer func2, gpointer arg2, gpointer model)
{
  struct task_pak *task;

  /* duplicate the task data */
  task = g_malloc(sizeof(struct task_pak));
  sysenv.task_list = g_slist_prepend(sysenv.task_list, task);
  task->is_async = FALSE;
  task->pid = -1;
  task->status = QUEUED;
  task->time = NULL;
  task->message = NULL;
  task->pcpu = 0.0;
  task->pmem = 0.0;
  task->progress = 0.0;
  task->locked_model = model;

  task->status_file = NULL;
  task->status_fp = NULL;
  task->status_fp_pos = 0L;
  task->status_index = -1;
  task->status_text = g_string_new(NULL);

  task->label = g_strdup_printf("SYS: %s", label);
  task->primary = func1;
  task->cleanup = func2;
  task->ptr1 = arg1;
  task->ptr2 = arg2;

  /* queue the task */
  g_thread_pool_push(sysenv.thread_pool, task, NULL);
  /*push it to front*/
  if (g_thread_pool_move_to_front(sysenv.thread_pool, task))
  {
    /*task is selected and front*/
    if (task->status == QUEUED)
    {
      /*temporarily increase the pool size to immediately get the system task to start*/
      gint max = g_thread_pool_get_max_threads(sysenv.thread_pool);
      g_thread_pool_set_max_threads(sysenv.thread_pool, max + 2, NULL);
      while (task->status == QUEUED)
        usleep(50 * 1000);                                          /*sleep 50ms until sys task is started*/
      g_thread_pool_set_max_threads(sysenv.thread_pool, max, NULL); /*go back to former thread pool size*/
    }
  }
}
/*********************************************/
/* NEW - async call gives correct thread PID */
/*********************************************/
#define DEBUG_TASK_ASYNC 0
gint task_async(const gchar *command, pid_t *pid)
{
  gint status;
  gchar **argv;
  GError *error = NULL;

  /* checks */
  if (!command)
    return (1);

#if _WIN32
  chdir(sysenv.cwd);
  system(command);
#else
  /* setup the command vector */
  argv = g_malloc(4 * sizeof(gchar *));
  *(argv) = g_strdup("/bin/sh");
  *(argv + 1) = g_strdup("-c");
  *(argv + 2) = g_strdup(command);
  *(argv + 3) = NULL;

  status = g_spawn_async(sysenv.cwd, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, pid, &error);

  g_strfreev(argv);
#endif

  /* g_spawn_async returns TRUE on success, FALSE on failure.
   * Return 0 on success, non-zero on failure (standard convention),
   * so callers can use: if (task_async(...)) task->status = KILLED; */
  if (!status)
  {
    fprintf(stderr, "task_async() FAILED: %s\n", error->message);
    g_error_free(error);
  } else
  {
    fprintf(stderr, "task_async() OK: spawned pid=%d\n", *pid);
  }

  return (!status); /* invert: 0 = success, 1 = failure */
}
/***********************************************************************/
/* Create a simple async task and wait for it replace direct execution */
/***********************************************************************/
gint task_sync_now(const gchar *command)
{
  pid_t pid, w;
  gint status = 0;
  gchar **argv;
  GError *error = NULL;
  gboolean spawn_ok;

  /* checks */
  if (!command)
    return (1);

#if _WIN32
  /*not sure waitpid would work here*/
  chdir(sysenv.cwd);
  system(command);
  return (0);
#else
  /* setup the command vector */
  argv = g_malloc(4 * sizeof(gchar *));
  *(argv) = g_strdup("/bin/sh");
  *(argv + 1) = g_strdup("-c");
  *(argv + 2) = g_strdup(command);
  *(argv + 3) = NULL;

  spawn_ok = g_spawn_async(sysenv.cwd, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &pid, &error);

  g_strfreev(argv);
#endif

  if (!spawn_ok)
  {
    printf("task_sync_now() launch error: %s\n", error->message);
    if (error)
      g_error_free(error);
    return (1);
  }

  do
  {
    w = waitpid(pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
    if (w == -1)
    {
      fprintf(stdout, "process %i incorrect termination!\n", pid);
      break;
    }
    if (w == 0)
    {
      /* Child not ready yet, retry */
      usleep(500 * 1000);
      continue;
    }
  } while ((!WIFEXITED(status)) && (!WIFSIGNALED(status)));

  return (0); /* success */
}

/**************************************/
/* platform independant task spawning */
/**************************************/
#define DEBUG_TASK_SYNC 0
gint task_sync(const gchar *command)
{
  gint status;
  gchar **argv;
  GError *error = NULL;

  /* checks */
  if (!command)
    return (1);

#if _WIN32
  chdir(sysenv.cwd);
  system(command);
#else
  /* setup the command vector */
  argv = g_malloc(4 * sizeof(gchar *));
  *(argv) = g_strdup("/bin/sh");
  *(argv + 1) = g_strdup("-c");
  *(argv + 2) = g_strdup(command);
  *(argv + 3) = NULL;
  status = g_spawn_sync(sysenv.cwd, argv, NULL, 0, NULL, NULL, NULL, NULL, NULL, &error);
  g_strfreev(argv);
#endif

  if (!status)
    printf("task_sync() error: %s\n", error->message);

  return (status);
}

/********************************************/
/* filter out unwanted lines in status file */
/********************************************/
gint task_status_keep(gint type, const gchar *line)
{
  switch (type)
  {
  case GULP:
    if (strstr(line, "CPU"))
      return (1);
    if (strstr(line, " **"))
      return (1);
    /*
        if (strstr(line, "="))
          if (strstr(line, "energy"))
            return(1);
    */
    break;

  default:
    return (1);
  }
  return (0);
}

/**************************************************/
/* create descriptive string from the status file */
/**************************************************/
void task_status_update(struct task_pak *task)
{
  /*gint filter;*/
  gchar *line;

  g_assert(task != NULL);

  /* read in the status file */
  if (task->status_file)
  {
    if (!task->status_fp)
    {
      task->status_index = 0;
      if ((!task->is_async) && (strlen((task->status_text)->str)))
        return;
      task->status_fp = fopen(task->status_file, "rt");
      if (task->status_fp == NULL)
        return;
    }

    /* Check if file has grown since last read */
    struct stat st;
    long cur_size = -1;
    if (stat(task->status_file, &st) == 0)
      cur_size = st.st_size;

    if (cur_size > task->status_fp_pos)
    {
      fseek(task->status_fp, task->status_fp_pos, SEEK_SET);
      clearerr(task->status_fp);
      line = file_read_line(task->status_fp);
      while (line)
      {
        g_string_append(task->status_text, line);
        g_free(line);
        task->status_fp_pos = ftell(task->status_fp);
        line = file_read_line(task->status_fp);
      }
    }

    /* On completion/kill, do one final read before closing to catch
     * any last-burst data that was written when the process exited */
    if (task->status == COMPLETED || task->status == KILLED)
    {
      fseek(task->status_fp, task->status_fp_pos, SEEK_SET);
      clearerr(task->status_fp);
      line = file_read_line(task->status_fp);
      while (line)
      {
        g_string_append(task->status_text, line);
        g_free(line);
        task->status_fp_pos = ftell(task->status_fp);
        line = file_read_line(task->status_fp);
      }
      fclose(task->status_fp);
      task->status_fp = NULL;
    }
  }
}
