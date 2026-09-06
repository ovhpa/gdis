/*
Copyright (C) 2003 by Sean David Fleming

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "gdis.h"
#include "model.h"
#include "graph.h"
#include "matrix.h"
#include "numeric.h"
#include "opengl.h"
#include "file.h"
#include "parse.h"
#include "dialog.h"
#include "interface.h"
#ifdef WITH_GUI
#include "file_vasp.h"
#endif
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
/*for uspex structure -> should be in interface?*/
#include "file_uspex.h"
/* externals */
extern struct sysenv_pak sysenv;
extern gint gl_fontsize;

/******************************************/
/* extract info from graph data structure */
/******************************************/
gchar *graph_treename(gpointer data)
{
  struct graph_pak *graph = data;
  return (graph->treename);
}

/***************************/
/* free a particular graph */
/***************************/
void graph_free(gpointer data, struct model_pak *model)
{
  struct graph_pak *graph = data;

  model->graph_list = g_slist_remove(model->graph_list, graph);

  if (model->graph_active == graph)
    model->graph_active = NULL;

    /* Clear graph pointer from VASP struct to prevent double-free
     * when model_free() later calls free_vasp_out(). */
#ifdef WITH_GUI
  if (model->vasp)
  {
    vasp_output_struct *vo = (vasp_output_struct *) model->vasp;
    if (vo->graph_energy == graph)
      vo->graph_energy = NULL;
    if (vo->graph_volume == graph)
      vo->graph_volume = NULL;
    if (vo->graph_forces == graph)
      vo->graph_forces = NULL;
    if (vo->graph_stress == graph)
      vo->graph_stress = NULL;
  }
#endif

  graph_reset(graph); // free_slist(graph->set_list);
  g_free(graph->treename);
  g_free(graph);
}

/******************************/
/* free all graphs in a model */
/******************************/
void graph_free_list(struct model_pak *model)
{
  GSList *list;
  struct graph_pak *graph;

  for (list = model->graph_list; list; list = g_slist_next(list))
  {
    graph = list->data;

    graph_reset(graph); // free_slist(graph->set_list);
    g_free(graph->treename);
    g_free(graph);
  }
  g_slist_free(model->graph_list);

  model->graph_list = NULL;
  model->graph_active = NULL;
}

/************************/
/* allocate a new graph */
/************************/
/* return an effective graph id */
gpointer graph_new(const gchar *name, struct model_pak *model)
{
  static gint n = 0;
  struct graph_pak *graph;

  g_assert(model != NULL);

  graph = g_malloc(sizeof(struct graph_pak));

  graph->treename = g_strdup_printf("%s", name);
  graph->treenumber = n;
  graph->wavelength = 0.0;
  graph->grafted = FALSE;
  graph->xlabel = TRUE;
  graph->ylabel = TRUE;
  graph->xmin = 0.0;
  graph->xmax = 0.0;
  graph->ymin = 0.0;
  graph->ymax = 0.0;
  graph->xticks = 5;
  graph->yticks = 5;
  graph->size = 0;
  graph->select = -1;
  graph->select_label = NULL;
  graph->set_list = NULL;
  graph->type = GRAPH_REGULAR;
  graph->require_xaxis = FALSE;
  graph->require_yaxis = FALSE;
  /*NEW: graph_controls*/
  graph->title = NULL;
  graph->sub_title = NULL;
  graph->x_title = NULL;
  graph->y_title = NULL;
  graph->title_font = 0;
  graph->sub_title_font = 0;
  graph->x_title_font = 0;
  graph->y_title_font = 0;

  /* append to preserve intuitive graph order on the model tree */
  model->graph_list = g_slist_append(model->graph_list, graph);

  model->graph_active = graph;

  n++;

  return (graph);
}

/**********************/
/* reset a graph data */
/**********************/
void graph_reset_data(struct graph_pak *graph)
{
  GSList *list; // set_list;
  g_data_x *px;
  g_data_y *py;
  gdouble *ptr;
  /**/
  g_assert(graph != NULL);
  switch (graph->type)
  {
  case GRAPH_YX_TYPE:
  case GRAPH_IY_TYPE:
  case GRAPH_XY_TYPE:
  case GRAPH_IX_TYPE:
  case GRAPH_XX_TYPE:
    /*the first data is a g_data_x*/
    list = graph->set_list;
    if (list == NULL)
      return;
    px = (g_data_x *) list->data;
    if (px != NULL)
    {
      if (px->x != NULL)
        g_free(px->x); /*remove x data*/
      px->x = NULL;
      g_free(px); /*necessary?*/
    }
    list->data = NULL;
    list = g_slist_next(list);
    while (list)
    {
      /*every list is a py*/
      py = (g_data_y *) list->data;
      if (py == NULL)
        continue; /*empty data list legal*/
      if (py->y != NULL)
        g_free(py->y);
      py->y = NULL;
      if (py->idx != NULL)
        g_free(py->idx);
      py->idx = NULL;
      if (py->symbol != NULL)
        g_free(py->symbol);
      py->symbol = NULL;
      g_free(py);
      list->data = NULL;
      list = g_slist_next(list);
    }
    g_slist_free(graph->set_list);
    graph->set_list = NULL;
    return;
  case GRAPH_REGULAR:
  default:
    list = graph->set_list;
    if (list == NULL)
      return;
    ptr = (gdouble *) list->data;
    if (ptr != NULL)
      g_free(ptr);
    list->data = NULL;
    list = g_slist_next(list);
    while (list)
    {
      ptr = (gdouble *) list->data;
      if (ptr != NULL)
        g_free(ptr);
      list->data = NULL;
      list = g_slist_next(list);
    }
    g_slist_free(graph->set_list);
    graph->set_list = NULL;
  }
  /**/
}

/*****************/
/* reset a graph */
/*****************/
void graph_reset(struct graph_pak *graph)
{
  g_assert(graph != NULL);
  /**/
  graph->wavelength = 0.0;
  graph->grafted = FALSE;
  graph->xlabel = TRUE;
  graph->ylabel = TRUE;
  graph->xmin = 0.0;
  graph->xmax = 0.0;
  graph->ymin = 0.0;
  graph->ymax = 0.0;
  graph->xticks = 5;
  graph->yticks = 5;
  graph->size = 0;
  graph->select = -1;
  if (graph->select_label != NULL)
    g_free(graph->select_label);
  graph->select_label = NULL;
  graph_reset_data(graph);
  // graph->set_list = NULL; <- set by previous
  graph->type = GRAPH_REGULAR;
  graph->require_xaxis = FALSE;
  graph->require_yaxis = FALSE;
  /*NEW: graph_controls*/
  if (graph->title != NULL)
    g_free(graph->title);
  graph->title = NULL;
  if (graph->sub_title != NULL)
    g_free(graph->sub_title);
  graph->sub_title = NULL;
  if (graph->x_title != NULL)
    g_free(graph->x_title);
  graph->x_title = NULL;
  if (graph->y_title != NULL)
    g_free(graph->y_title);
  graph->y_title = NULL;
  graph->title_font = 0;
  graph->sub_title_font = 0;
  graph->x_title_font = 0;
  graph->y_title_font = 0;
}

/**************/
/* axes setup */
/**************/
void graph_init_y(gdouble *x, struct graph_pak *graph)
{
  gdouble ymin, ymax;

  g_assert(graph != NULL);

  ymin = min(graph->size, x);
  ymax = max(graph->size, x);

  if (ymin < graph->ymin)
    graph->ymin = ymin;

  if (ymax > graph->ymax)
    graph->ymax = ymax;
}

/*******************/
/* tree graft flag */
/*******************/
void graph_set_grafted(gint value, gpointer data)
{
  struct graph_pak *graph = data;

  g_assert(graph != NULL);

  graph->grafted = value;
}

/**************************/
/* control label printing */
/**************************/
void graph_set_xticks(gint label, gint ticks, gpointer ptr_graph)
{
  struct graph_pak *graph = ptr_graph;

  g_assert(graph != NULL);
  if (label)
    g_assert(ticks > 1);

  graph->xlabel = label;
  graph->xticks = ticks;
}

/**************************/
/* control label printing */
/**************************/
void graph_set_yticks(gint label, gint ticks, gpointer ptr_graph)
{
  struct graph_pak *graph = ptr_graph;

  g_assert(graph != NULL);
  if (label)
    g_assert(ticks > 1);

  graph->ylabel = label;
  graph->yticks = ticks;
}

/**********************/
/* special graph data */
/**********************/
void graph_set_wavelength(gdouble wavelength, gpointer ptr_graph)
{
  struct graph_pak *graph = ptr_graph;

  g_assert(graph != NULL);

  graph->wavelength = wavelength;
}
/********************************/
/* regular selection of 1D data */
/********************************/
void graph_set_select(gdouble x, gchar *label, gpointer data)
{
  gdouble n;
  struct graph_pak *graph = data;

  g_assert(graph != NULL);

  /*
  printf("x = %f, min, max = %f, %f\n", x, graph->xmin, graph->xmax);
  */

  /* locate the value's position in the data point list */
  n = (x - graph->xmin) / (graph->xmax - graph->xmin);
  n *= graph->size;

  graph->select = (gint) n;
  g_free(graph->select_label);
  if (label)
    graph->select_label = g_strdup(label);
  else
    graph->select_label = NULL;

  /*
  printf("select -> %d : [0, %d]\n", graph->select, graph->size);
  */
}
/*****************************/
/* add dependent data set(s) */
/*****************************/
void graph_add_data(gint size, gdouble *x, gdouble x1, gdouble x2, gpointer data)
{
  gdouble *ptr;
  struct graph_pak *graph = data;

  g_assert(graph != NULL);

  /* try to prevent the user supplying different sized data */
  /* TODO - sample in some fashion if different? */
  if (graph->size)
    g_assert(graph->size == size);
  else
    graph->size = size;

  ptr = g_malloc(size * sizeof(gdouble));

  memcpy(ptr, x, size * sizeof(gdouble));

  graph->xmin = x1;
  graph->xmax = x2;
  graph_init_y(x, graph);

  graph->set_list = g_slist_append(graph->set_list, ptr);
}
/*********************************/
/* add borned (x,y) data (ovhpa) */
/*********************************/
void graph_add_borned_data(gint size, gdouble *x, gdouble x_min, gdouble x_max, gdouble y_min, gdouble y_max, gint type,
                           gpointer data)
{
  gdouble *ptr;
  struct graph_pak *graph = data;

  g_assert(graph != NULL);

  if (graph->size)
    g_assert(graph->size == size);
  else
    graph->size = size;

  ptr = g_malloc(size * sizeof(gdouble));
  memcpy(ptr, x, size * sizeof(gdouble));

  graph->xmin = x_min;
  graph->xmax = x_max;
  /* because graph_init_y destroy supplied graph limits */
  graph->ymin = y_min;
  graph->ymax = y_max;
  graph->type = type;

  graph->set_list = g_slist_append(graph->set_list, ptr);
}
/**********************/
/* NEW - toggle xaxis */
/**********************/
void dat_graph_toggle_xaxis(gpointer pgraph)
{
  struct graph_pak *graph = pgraph;
  g_assert(graph != NULL);
  graph->require_xaxis = !(graph->require_xaxis == TRUE);
}
/**********************/
/* NEW - toggle yaxis */
/**********************/
void dat_graph_toggle_yaxis(gpointer pgraph)
{
  struct graph_pak *graph = pgraph;
  g_assert(graph != NULL);
  graph->require_yaxis = !(graph->require_yaxis == TRUE);
}
/******************************/
/* NEW - dat_graph: set title */
/******************************/
void dat_graph_set_title(const gchar *title, gpointer pgraph)
{
  struct graph_pak *graph = pgraph;
  g_assert(graph != NULL);
  /*duplicate string*/
  graph->title = g_strdup(title);
}
/**********************************/
/* NEW - dat_graph: set sub-title */
/**********************************/
void dat_graph_set_sub_title(const gchar *title, gpointer pgraph)
{
  struct graph_pak *graph = pgraph;
  g_assert(graph != NULL);
  /*duplicate string*/
  graph->sub_title = g_strdup(title);
}
/********************************/
/* NEW - dat_graph: set x_title */
/********************************/
void dat_graph_set_x_title(const gchar *x_title, gpointer pgraph)
{
  struct graph_pak *graph = pgraph;
  g_assert(graph != NULL);
  /*duplicate string*/
  graph->x_title = g_strdup(x_title);
}
/********************************/
/* NEW - dat_graph: set y_title */
/********************************/
void dat_graph_set_y_title(const gchar *y_title, gpointer pgraph)
{
  struct graph_pak *graph = pgraph;
  g_assert(graph != NULL);
  /*duplicate string*/
  graph->y_title = g_strdup(y_title);
}
/*************************/
/* NEW - dat_graph: set x*/
/*************************/
void dat_graph_set_x(g_data_x dx, gpointer pgraph)
{
  struct graph_pak *graph = pgraph;
  int i;
  g_data_x *px;

  g_assert(graph != NULL);

  /*duplicate and register*/
  px = g_malloc(sizeof(g_data_x));
  g_assert(px != NULL);
  px->x_size = dx.x_size;
  px->x = g_malloc(dx.x_size * sizeof(gdouble));
  // memcpy(px->x,dx.x,dx.x_size*sizeof(gdouble));
  for (i = 0; i < dx.x_size; i++)
    px->x[i] = dx.x[i];

  graph->size = 0; /*graph size hold the number of y arrays!*/
  graph->set_list = g_slist_append(graph->set_list, px);
}
/**************************/
/* NEW - dat_graph: add y */
/**************************/
void dat_graph_add_y(g_data_y dy, gpointer pgraph)
{
  struct graph_pak *graph = pgraph;
  g_data_y *py;

  g_assert(graph != NULL);

  if (dy.y_size == 0)
  {
    /*empty size data set are OK*/
    py = g_malloc(sizeof(g_data_y));
    py->y_size = 0;
    py->y = NULL;
    py->idx = NULL;
    py->type = GRAPH_REGULAR;
    py->symbol = NULL;
    py->sym_color = NULL;
    py->mixed_symbol = FALSE;
    py->line = GRAPH_LINE_NONE;
    py->color = GRAPH_COLOR_DEFAULT;
    graph->size++; /*graph size hold the number of y arrays!*/
    graph->set_list = g_slist_append(graph->set_list, py);
    return;
  }
  /*duplicate and register*/
  py = g_malloc(sizeof(g_data_y));
  g_assert(py != NULL);
  py->y_size = dy.y_size;
  py->y = g_malloc(dy.y_size * sizeof(gdouble)); /*_BUG_ triggered #06/06/2019*/
  memcpy(py->y, dy.y, dy.y_size * sizeof(gdouble));
  if (dy.idx == NULL)
    py->idx = NULL;
  else
  {
    py->idx = g_malloc(dy.y_size * sizeof(gint32));
    memcpy(py->idx, dy.idx, dy.y_size * sizeof(gint32));
  }
  if (dy.symbol == NULL)
    py->symbol = NULL;
  else
  {
    py->symbol = g_malloc(dy.y_size * sizeof(graph_symbol));
    memcpy(py->symbol, dy.symbol, dy.y_size * sizeof(graph_symbol));
  }
  if (dy.sym_color == NULL)
    py->sym_color = NULL;
  else
  {
    py->sym_color = g_malloc(dy.y_size * sizeof(graph_color));
    memcpy(py->sym_color, dy.sym_color, dy.y_size * sizeof(graph_color));
  }
  py->mixed_symbol = dy.mixed_symbol;
  py->line = dy.line;
  py->color = dy.color;
  py->type = dy.type;

  graph->size++; /*graph size hold the number of y arrays!*/
  graph->set_list = g_slist_append(graph->set_list, py);
}
/*******************************/
/* NEW - dat_graph: set limits */
/*******************************/
void dat_graph_set_limits(gdouble x_min, gdouble x_max, gdouble y_min, gdouble y_max, gpointer pgraph)
{
  struct graph_pak *graph = pgraph;

  g_assert(graph != NULL);

  graph->xmin = x_min;
  graph->xmax = x_max;
  graph->ymin = y_min;
  graph->ymax = y_max;
}
/*******************************/
/* NEW - set global graph type */
/*******************************/
void dat_graph_set_type(graph_type type, gpointer pgraph)
{
  struct graph_pak *graph = pgraph;

  g_assert(graph != NULL);

  graph->type = type;
}
/***********************************/
/* NEW - dat_graph: select a value */
/***********************************/
/*NEW: selection sensivity*/
#define SEL_SENS 5
#define DEBUG_PEAK_SELECT 0
void dat_graph_select(gint x, gint y, struct model_pak *model)
{
  struct graph_pak *graph;
  struct canvas_pak *canvas;
  gdouble ox, dx, xf;
  gdouble oy, dy, yf;
  gint xx, yy;
  int i, j;
  GSList *list;
  FILE *vf;
  gint x_index;
  gint y_index;
  g_data_x *p_x;
  g_data_y *p_y;

  g_assert(model != NULL);
  graph = (struct graph_pak *) model->graph_active;
  g_assert(graph != NULL);
  canvas = g_slist_nth_data(sysenv.canvas_list, 0);
  if (!canvas)
    return;
  /*calculate x data*/
  ox = canvas->x + 4 * gl_fontsize;
  if (graph->ylabel)
    ox += 2 * gl_fontsize;
  oy = canvas->y + canvas->height - 2 * gl_fontsize;
  if (graph->xlabel)
    oy -= 2 * gl_fontsize;
  dy = (canvas->height - 8.0 * gl_fontsize);
  dx = (canvas->width - 2.0 * ox);

  if (graph->title)
    dy = (canvas->height - 10.0 * gl_fontsize);
  if (graph->x_title)
    oy = canvas->y + canvas->height - 5 * gl_fontsize;

  list = graph->set_list;
  p_x = (g_data_x *) list->data;

  /*NEW: process the diffraction case first*/
  if ((graph->type == GRAPH_XY_TYPE) && (graph->wavelength >= 0.1))
  {
    gint8 det, h, k, l;
    /*get the y_value first*/
    x_index = -1;
    y_index = -1;
    list = g_slist_next(list);
    p_y = (g_data_y *) list->data; /*there is only one set*/
    for (i = 0; i < p_y->y_size; i++)
    {
      yf = p_y->y[i];
      yf -= graph->ymin;
      yf /= (graph->ymax - graph->ymin);
      yf *= dy;
      yy = (gint) yf;
      yy *= -1;
      yy += oy;
      if ((y > (yy - SEL_SENS)) && (y < (yy + SEL_SENS)))
      {
        /*there can be several similar intensity*/
        /*check if we are in the good region*/
        xf = p_x->x[i];
        xf -= graph->xmin;
        xf /= (graph->xmax - graph->xmin);
        xx = ox + xf * dx;
        if ((x > (xx - SEL_SENS)) && (x < (xx + SEL_SENS)))
        {
          /*got it*/
          x_index = i;
          y_index = i;
          graph->select_2 = p_y->y[y_index];
          break;
        }
      }
    }
    if (y_index < 0)
    {
      /*not found*/
      if (graph->select_label != NULL)
        g_free(graph->select_label);
      graph->select_label = NULL;
      return;
    }
    if (p_y->y[y_index] == 0.0)
    {
      /*we are in the background, try x first*/
      x_index = -1;
      y_index = -1;
      for (i = 0; i < p_x->x_size; i++)
      {
        xf = p_x->x[i];
        xf -= graph->xmin;
        xf /= (graph->xmax - graph->xmin);
        xx = ox + xf * dx;
        if ((x > (xx - SEL_SENS)) && (x < (xx + SEL_SENS)))
        {
          /*in background, there can be multiple y*/
          /*... -> get the one which is not 0*/
          if (p_y->y[i] != 0.0)
          {
            /*got it*/
            x_index = i;
            y_index = i;
            graph->select_2 = p_y->y[y_index];
            break;
          }
        }
      }
    }
    if (y_index < 0)
    {
      /*not found*/
      if (graph->select_label != NULL)
        g_free(graph->select_label);
      graph->select_label = NULL;
      return;
    }
    if (p_y->y[y_index] == 0.0)
    {
      /*found background! Should NEVER happen, but just in case...*/
      if (graph->select_label != NULL)
        g_free(graph->select_label);
      graph->select_label = NULL;
      return;
    }
    graph->select = x_index;
    if (graph->select_label)
      g_free(graph->select_label);
    if (p_y->idx[y_index] == 0)
    {
      /*problem with label??*/
      graph->select_label = g_strdup_printf("#%X", p_y->idx[y_index]);
    } else
    {
      det = ((p_y->idx[y_index] >> 24) & 0xFF);
      h = p_y->idx[y_index] & 0xFF;
      k = (p_y->idx[y_index] >> 8) & 0xFF;
      l = (p_y->idx[y_index] >> 16) & 0xFF;
      if (det == 0x55)
        graph->select_label = g_strdup_printf("(%d %d %d)", h, k, l);
      else
        graph->select_label = g_strdup_printf("[%d %d %d]", h, k, l); /*something wrong*/
    }
    return;
  } /*end of diffraction*/
  /*get the corresponding x index*/
  x_index = -1;
  for (i = 0; i < p_x->x_size; i++)
  {
    xf = p_x->x[i];
    xf -= graph->xmin;
    xf /= (graph->xmax - graph->xmin);
    xx = ox + xf * dx;
    if ((x > (xx - SEL_SENS)) && (x < (xx + SEL_SENS)))
    {
      /*got it*/
      x_index = i;
      break;
    }
  }
  if (x_index < 0)
  {
    /*not found*/
    if (graph->select_label != NULL)
      g_free(graph->select_label);
    graph->select_label = NULL;
    return;
  }
  /*scan for the proper y value*/
  j = 0;
  y_index = -1;
  if ((graph->type == GRAPH_IY_TYPE) || (graph->type == GRAPH_XY_TYPE))
  {
    list = g_slist_next(list);
    for (; list; list = g_slist_next(list))
    {
      p_y = (g_data_y *) list->data;
      yf = p_y->y[x_index]; /*only need to look here*/
      yf -= graph->ymin;
      yf /= (graph->ymax - graph->ymin);
      yf *= dy;
      yy = (gint) yf;
      yy *= -1;
      yy += oy;
      if ((y > (yy - SEL_SENS)) && (y < (yy + SEL_SENS)))
      {
        /*got it*/
        y_index = 1;
        graph->select = x_index;
        graph->select_2 = p_y->y[x_index];
        if (graph->select_label)
          g_free(graph->select_label);
        if (graph->type == GRAPH_IY_TYPE)
          graph->select_label = g_strdup_printf("[%i,%f]", (gint) p_x->x[x_index], p_y->y[x_index]);
        else
          graph->select_label = g_strdup_printf("[%G,%G]", p_x->x[x_index], p_y->y[x_index]);
        if (p_y->idx != NULL)
        {
          if (p_y->idx[x_index] < 0)
          {
            //					update_frame_uspex(p_y->idx[x_index]-1,model);
            return; /*unavailable*/
          }
          /*load structure*/
          vf = fopen(model->filename, "r");
          if (!vf)
            return;
          model->cur_frame = p_y->idx[x_index] - 1;
          read_raw_frame(vf, p_y->idx[x_index] - 1, model);
          fclose(vf);
          model_prep(model);
        }
        break;
      }
      j++;
    }
    if (y_index < 0)
    {
      /*not found*/
      if (graph->select_label != NULL)
        g_free(graph->select_label);
      graph->select_label = NULL;
      return;
    }
  } else if ((graph->type == GRAPH_IX_TYPE) || (graph->type == GRAPH_XX_TYPE))
  {
    /*in this case, we know the data is on the x_index set*/
    list = g_slist_nth(graph->set_list, x_index + 1);
    if (!list)
      return; /*missing data?*/
    p_y = (g_data_y *) list->data;
    for (j = 0; j < p_y->y_size; j++)
    {
      yf = p_y->y[j];
      yf -= graph->ymin;
      yf /= (graph->ymax - graph->ymin);
      yf *= dy;
      yy = (gint) yf;
      yy *= -1;
      yy += oy;
      if ((y > yy - SEL_SENS) && (y < yy + SEL_SENS))
      {
        /*got it*/
        y_index = j;
        graph->select = x_index;
        graph->select_2 = p_y->y[y_index];
        if (graph->select_label)
          g_free(graph->select_label);
        if (graph->type == GRAPH_IX_TYPE)
          graph->select_label = g_strdup_printf("[%i,%f]", (gint) p_x->x[x_index], p_y->y[y_index]);
        else
          graph->select_label = g_strdup_printf("[%G,%G]", p_x->x[x_index], p_y->y[y_index]);
        if (p_y->idx != NULL)
        {
          if (p_y->idx[y_index] < 0)
          {
            //					update_frame_uspex(p_y->idx[y_index],model);
            return; /*unavailable*/
          }
          /*load structure*/
          vf = fopen(model->filename, "r");
          if (!vf)
            return;
          model->cur_frame = p_y->idx[y_index];
          read_raw_frame(vf, p_y->idx[y_index], model);
          fclose(vf);
          model_prep(model);
        }
        break;
      }
    }
    if (y_index < 0)
    {
      /*not found*/
      if (graph->select_label != NULL)
        g_free(graph->select_label);
      graph->select_label = NULL;
      return;
    }
  } else
  {
    /*unknown graph type*/
    if (graph->select_label != NULL)
      g_free(graph->select_label);
    graph->select_label = NULL;
    return;
  }

  /* Trigger redraw and content update after graph selection */
  extern void redraw_canvas(gint);
  redraw_canvas(1);

  /* Update content table with new selection info */
  extern void model_content_refresh(struct model_pak *);
  extern void qt_update_content_table(void);
  if (sysenv.active_model)
  {
    model_content_refresh(sysenv.active_model);
    qt_update_content_table();
  }
}
/************************************/
/* graph data extraction primitives */
/************************************/
gdouble graph_xmin(gpointer data)
{
  struct graph_pak *graph = data;
  return (graph->xmin);
}

gdouble graph_xmax(gpointer data)
{
  struct graph_pak *graph = data;
  return (graph->xmax);
}

gint graph_ylabel(gpointer data)
{
  struct graph_pak *graph = data;
  return (graph->ylabel);
}

gdouble graph_wavelength(gpointer data)
{
  struct graph_pak *graph = data;
  return (graph->wavelength);
}

gint graph_grafted(gpointer data)
{
  struct graph_pak *graph = data;
  return (graph->grafted);
}
/****************************************************/
/* draw a graph using the new, general graph system */
/****************************************************/
void graph_draw_new(struct canvas_pak *canvas, struct graph_pak *graph)
{
  /*draw a graph using the new, more general graph system*/
  gint i, j, x, y, oldx, oldy, ox, oy, sx, sy;
  gint flag;
  gchar *text;
  gdouble *ptr;
  gdouble xf, yf, dx, dy;
  GSList *list;
  /*specific*/
  gint size;
  gdouble *xval = NULL;
  /*NEW - dat_graph*/
  g_data_x *gx = NULL;
  g_data_y *gy = NULL;
  graph_type type = graph->type;
  graph_line line = GRAPH_LINE_NONE;
  /* compute origin */
  ox = canvas->x + 4 * gl_fontsize;
  if (graph->ylabel)
    ox += 2 * gl_fontsize;
  oy = canvas->y + canvas->height - 2 * gl_fontsize;
  if (graph->xlabel)
    oy -= 2 * gl_fontsize;
  /* increments for screen drawing */
  dy = (canvas->height - 8.0 * gl_fontsize);
  dx = (canvas->width - 2.0 * ox);
  if (graph->title)
  {
    //  oy += 2*gl_fontsize;
    dy = (canvas->height - 10.0 * gl_fontsize);
  }
  if (graph->x_title)
  {
    /*we have a x_title we need to offset y*/
    oy = canvas->y + canvas->height - 5 * gl_fontsize;
  }

  /* axes label colour */
  glColor3f(sysenv.render.fg_colour[0], sysenv.render.fg_colour[1], sysenv.render.fg_colour[2]);
  glLineWidth(2.0);
  /* x labels */
  { /*do the usual xmin,xmax ticks*/
    oldx = ox;
    for (i = 0; i < graph->xticks; i++)
    {
      /* get real index */
      xf = (gdouble) i / (gdouble) (graph->xticks - 1);
      x = ox + xf * dx;
      if (graph->xlabel)
      {
        /*only calculate real value when needed*/
        xf *= (graph->xmax - graph->xmin);
        xf += graph->xmin;
        if ((type == GRAPH_IY_TYPE) || (type == GRAPH_IX_TYPE))
        {
          text = g_strdup_printf("%i", (gint) xf);
          pango_print(text, x, oy + gl_fontsize, canvas, gl_fontsize - 2, 0);
        } else
        {
          text = g_strdup_printf("%.2f", xf);
          pango_print(text, x - 2 * gl_fontsize, oy + gl_fontsize, canvas, gl_fontsize - 2, 0);
        }
        g_free(text);
      }
      /* axis segment + tick */
      glBegin(GL_LINE_STRIP);
      gl_vertex_window(oldx, oy, canvas);
      gl_vertex_window(x, oy, canvas);
      gl_vertex_window(x, oy + 5, canvas);
      glEnd();
      if ((type == GRAPH_IY_TYPE) || (type == GRAPH_XY_TYPE) || (type == GRAPH_YX_TYPE) || (type == GRAPH_IX_TYPE) ||
          (type == GRAPH_XX_TYPE))
      {
        glBegin(GL_LINE_STRIP);
        gl_vertex_window(oldx, oy - dy - 1, canvas);
        gl_vertex_window(x, oy - dy - 1, canvas);
        gl_vertex_window(x, oy - dy - 5, canvas);
        glEnd();
      }
      oldx = x;
    }
  }
  /* y labels */
  { /*do the usual ymin,ymax ticks*/
    oldy = oy;
    for (i = 0; i < graph->yticks; i++)
    {
      /* get screen position */
      yf = (gdouble) i / (gdouble) (graph->yticks - 1);
      y = -yf * dy;
      y += oy;
      /* label */
      if (graph->ylabel)
      {
        /*only calculate real value when needed*/
        yf *= (graph->ymax - graph->ymin);
        yf += graph->ymin;
        if (graph->ymax > 999.999999)
          text = g_strdup_printf("%.2e", yf);
        else
          text = g_strdup_printf("%7.2f", yf);
        pango_print(text, 2 * gl_fontsize, y - 1, canvas, gl_fontsize - 2, 0);
        g_free(text);
      }
      /* axis segment + tick */
      glBegin(GL_LINE_STRIP);
      gl_vertex_window(ox, oldy, canvas);
      gl_vertex_window(ox, y - 1, canvas);
      gl_vertex_window(ox - 5, y - 1, canvas);
      glEnd();
      if ((type == GRAPH_IY_TYPE) || (type == GRAPH_XY_TYPE) || (type == GRAPH_YX_TYPE) || (type == GRAPH_IX_TYPE) ||
          (type == GRAPH_XX_TYPE))
      {
        glBegin(GL_LINE_STRIP);
        gl_vertex_window(ox + dx, oldy, canvas);
        gl_vertex_window(ox + dx, y - 1, canvas);
        gl_vertex_window(ox + dx + 4, y - 1, canvas);
        glEnd();
      }
      oldy = y;
    }
  }
  /* data drawing colour */
  glColor3f(sysenv.render.title_colour[0], sysenv.render.title_colour[1], sysenv.render.title_colour[2]);
  glLineWidth(1.0);
  flag = FALSE;
  sx = sy = 0;
  list = graph->set_list;
  size = graph->size;
  if ((type == GRAPH_IY_TYPE) || (type == GRAPH_XY_TYPE) || (type == GRAPH_YX_TYPE) || (type == GRAPH_IX_TYPE) ||
      (type == GRAPH_XX_TYPE))
  {
    /*we have dedicated x,y*/
    gx = (g_data_x *) list->data;
    xval = &(gx->x[0]);
    list = g_slist_next(list);
  }
  j = 0;
  for (; list; list = g_slist_next(list))
  {
    /*FIXME here with ALL graph*/
    if ((type == GRAPH_IY_TYPE) || (type == GRAPH_XY_TYPE) || (type == GRAPH_YX_TYPE) || (type == GRAPH_IX_TYPE) ||
        (type == GRAPH_XX_TYPE))
    {
      gy = (g_data_y *) list->data;
      ptr = &(gy->y[0]);
      size = gy->y_size;
      line = gy->line;
      if ((ptr == NULL) || (size == 0))
      {
        /*empty data set is OK*/
        j++;
        continue;
      }
      if (gy->type != GRAPH_REGULAR)
        type = gy->type; /*update type within type?*/
      if (gy->sym_color == NULL)
      { /*no indivudual coloring -> same color for symbol and line*/
        switch (gy->color)
        {
        case GRAPH_COLOR_WHITE: /*1.0,  1.0,    1.0*/
          glColor3f(1.0, 1.0, 1.0);
          break;
        case GRAPH_COLOR_BLUE: /*0.0,  0.0,    1.0*/
          glColor3f(0.0, 0.0, 1.0);
          break;
        case GRAPH_COLOR_GREEN: /*0.0,  0.5,    0.0*/
          glColor3f(0.0, 0.5, 0.0);
          break;
        case GRAPH_COLOR_RED: /*1.0,  0.0,    0.0*/
          glColor3f(1.0, 0.0, 0.0);
          break;
        case GRAPH_COLOR_YELLOW: /*1.0,  1.0,    0.0*/
          glColor3f(1.0, 1.0, 0.0);
          break;
        case GRAPH_COLOR_GRAY: /*0.5,  0.5,    0.5*/
          glColor3f(0.5, 0.5, 0.5);
          break;
        case GRAPH_COLOR_NAVY: /*0.0,  0.0,    0.5*/
          glColor3f(0.0, 0.0, 0.5);
          break;
        case GRAPH_COLOR_LIME: /*0.0,  1.0,    0.0*/
          glColor3f(0.0, 1.0, 0.0);
          break;
        case GRAPH_COLOR_TEAL: /*0.0,  0.5,    0.5*/
          glColor3f(0.0, 0.5, 0.5);
          break;
        case GRAPH_COLOR_AQUA: /*0.0,  1.0,    1.0*/
          glColor3f(0.0, 1.0, 1.0);
          break;
        case GRAPH_COLOR_MAROON: /*0.5,  0.0,    0.0*/
          glColor3f(0.5, 0.0, 0.0);
          break;
        case GRAPH_COLOR_PURPLE: /*0.5,  0.0,    0.5*/
          glColor3f(0.5, 0.0, 0.5);
          break;
        case GRAPH_COLOR_OLIVE: /*0.5,  0.5,    0.0*/
          glColor3f(0.5, 0.5, 0.0);
          break;
        case GRAPH_COLOR_SILVER: /*0.75, 0.75,   0.75*/
          glColor3f(0.75, 0.75, 0.75);
          break;
        case GRAPH_COLOR_FUSHIA: /*1.0,  0.0,    1.0*/
          glColor3f(1.0, 0.0, 1.0);
          break;
        case GRAPH_COLOR_BLACK: /*0.0,0.0,0.0*/
          glColor3f(0., 0., 0.);
        case GRAPH_COLOR_DEFAULT: /*whatever title color*/
        default:
          glColor3f(sysenv.render.title_colour[0], sysenv.render.title_colour[1], sysenv.render.title_colour[2]);
        }
      } /*NEW: sym_color*/
    } else
    {
      ptr = (gdouble *) list->data;
    }
    oldx = -1;
    oldy = -1;
    i = 0;
    while (i < size)
    {
      /*exclude non-value*/
      if (isnan(ptr[i]))
      {
        i++;
        oldx = -1;
        continue;
      }
      /*get real values*/
      switch (type)
      {
      case GRAPH_IY_TYPE:
      case GRAPH_XY_TYPE:
        xf = xval[i];
        yf = ptr[i];
        break;
      case GRAPH_YX_TYPE: /*interleaved data*/
        xf = ptr[i + 1];
        yf = ptr[i];
        break;
      case GRAPH_IX_TYPE:
      case GRAPH_XX_TYPE:
        xf = xval[j];
        yf = ptr[i];
        break;
        /*complete until full*/
      case GRAPH_REGULAR:
      default:
        xf = (gdouble) i;
        yf = ptr[i];
      }
      /*new: skip drawing for points outside of graph extrema (also interrupt line when line drawing)*/
      if ((xf < graph->xmin) || (xf > graph->xmax) || (yf < graph->ymin) || (yf > graph->ymax))
      {
        /*update index*/
        switch (type)
        {
        case GRAPH_YX_TYPE:
          i++; /* ie twice ++ */
        case GRAPH_IY_TYPE:
        case GRAPH_XY_TYPE:
        case GRAPH_IX_TYPE:
        case GRAPH_XX_TYPE:
        case GRAPH_REGULAR:
        default:
          i++;
        }
        oldx = -1;
        continue;
      }
      /*NEW: change color based on sym_color*/
      if (type != GRAPH_REGULAR)
      {
        if (gy->sym_color != NULL)
        {
          switch (gy->sym_color[i])
          {
          case GRAPH_COLOR_WHITE: /*1.0,  1.0,    1.0*/
            glColor3f(1.0, 1.0, 1.0);
            break;
          case GRAPH_COLOR_BLUE: /*0.0,  0.0,    1.0*/
            glColor3f(0.0, 0.0, 1.0);
            break;
          case GRAPH_COLOR_GREEN: /*0.0,  0.5,    0.0*/
            glColor3f(0.0, 0.5, 0.0);
            break;
          case GRAPH_COLOR_RED: /*1.0,  0.0,    0.0*/
            glColor3f(1.0, 0.0, 0.0);
            break;
          case GRAPH_COLOR_YELLOW: /*1.0,  1.0,    0.0*/
            glColor3f(1.0, 1.0, 0.0);
            break;
          case GRAPH_COLOR_GRAY: /*0.5,  0.5,    0.5*/
            glColor3f(0.5, 0.5, 0.5);
            break;
          case GRAPH_COLOR_NAVY: /*0.0,  0.0,    0.5*/
            glColor3f(0.0, 0.0, 0.5);
            break;
          case GRAPH_COLOR_LIME: /*0.0,  1.0,    0.0*/
            glColor3f(0.0, 1.0, 0.0);
            break;
          case GRAPH_COLOR_TEAL: /*0.0,  0.5,    0.5*/
            glColor3f(0.0, 0.5, 0.5);
            break;
          case GRAPH_COLOR_AQUA: /*0.0,  1.0,    1.0*/
            glColor3f(0.0, 1.0, 1.0);
            break;
          case GRAPH_COLOR_MAROON: /*0.5,  0.0,    0.0*/
            glColor3f(0.5, 0.0, 0.0);
            break;
          case GRAPH_COLOR_PURPLE: /*0.5,  0.0,    0.5*/
            glColor3f(0.5, 0.0, 0.5);
            break;
          case GRAPH_COLOR_OLIVE: /*0.5,  0.5,    0.0*/
            glColor3f(0.5, 0.5, 0.0);
            break;
          case GRAPH_COLOR_SILVER: /*0.75, 0.75,   0.75*/
            glColor3f(0.75, 0.75, 0.75);
            break;
          case GRAPH_COLOR_FUSHIA: /*1.0,  0.0,    1.0*/
            glColor3f(1.0, 0.0, 1.0);
            break;
          case GRAPH_COLOR_BLACK: /*0.0,0.0,0.0*/
            glColor3f(0., 0., 0.);
          case GRAPH_COLOR_DEFAULT: /*whatever title color*/
          default:
            glColor3f(sysenv.render.title_colour[0], sysenv.render.title_colour[1], sysenv.render.title_colour[2]);
          }
        }
      }
      /*calculate screen values, draw points*/
      switch (type)
      {
      case GRAPH_IY_TYPE:
      case GRAPH_XY_TYPE:
      case GRAPH_YX_TYPE:
      case GRAPH_IX_TYPE:
      case GRAPH_XX_TYPE:
        xf -= graph->xmin;
        xf /= (graph->xmax - graph->xmin);
        x = ox + xf * dx;
        yf -= graph->ymin;
        yf /= (graph->ymax - graph->ymin);
        yf *= dy;
        y = (gint) yf;
        y *= -1;
        y += oy;
        if (gy->symbol)
        {
          switch (gy->symbol[i])
          {
          case GRAPH_SYMB_SQUARE:
            glBegin(GL_LINE_STRIP);
            gl_vertex_window(x - 2, y - 2, canvas);
            gl_vertex_window(x + 2, y - 2, canvas);
            gl_vertex_window(x + 2, y + 2, canvas);
            gl_vertex_window(x - 2, y + 2, canvas);
            gl_vertex_window(x - 2, y - 2, canvas);
            glEnd();
            break;
          case GRAPH_SYMB_CROSS:
            glBegin(GL_LINE_STRIP);
            gl_vertex_window(x - 2, y - 2, canvas);
            gl_vertex_window(x + 2, y + 2, canvas);
            glEnd();
            glBegin(GL_LINE_STRIP);
            gl_vertex_window(x + 2, y - 2, canvas);
            gl_vertex_window(x - 2, y + 2, canvas);
            glEnd();
            break;
          case GRAPH_SYMB_TRI_DN:
            glBegin(GL_LINE_STRIP);
            gl_vertex_window(x, y - 3, canvas);
            gl_vertex_window(x - 3, y + 3, canvas);
            gl_vertex_window(x + 3, y + 3, canvas);
            gl_vertex_window(x, y - 3, canvas);
            glEnd();
            break;
          case GRAPH_SYMB_TRI_UP:
            glBegin(GL_LINE_STRIP);
            gl_vertex_window(x, y + 3, canvas);
            gl_vertex_window(x - 3, y - 3, canvas);
            gl_vertex_window(x + 3, y - 3, canvas);
            gl_vertex_window(x, y + 3, canvas);
            glEnd();
            break;
          case GRAPH_SYMB_DIAM:
            glBegin(GL_LINE_STRIP);
            gl_vertex_window(x, y + 5, canvas);
            gl_vertex_window(x - 5, y, canvas);
            gl_vertex_window(x, y - 5, canvas);
            gl_vertex_window(x + 5, y, canvas);
            gl_vertex_window(x, y + 5, canvas);
            glEnd();
            break;
          case GRAPH_SYMB_NONE:
          default:
            /*no symbol*/
            break;
          }
        } else
        {
          /*draw a default rectangle*/
          glBegin(GL_LINE_STRIP);
          gl_vertex_window(x - 2, y - 2, canvas);
          gl_vertex_window(x + 2, y - 2, canvas);
          gl_vertex_window(x + 2, y + 2, canvas);
          gl_vertex_window(x - 2, y + 2, canvas);
          gl_vertex_window(x - 2, y - 2, canvas);
          glEnd();
        }
        if ((i == graph->select) && (ptr[i] == graph->select_2))
        {
          sx = x;
          sy = y - 1;
          flag = TRUE;
        }
        break;
      case GRAPH_REGULAR:
      default:
        xf = (gdouble) i / (gdouble) (graph->size - 1);
        x = ox + xf * dx;
        yf -= graph->ymin;
        yf /= (graph->ymax - graph->ymin);
        yf *= dy;
        y = (gint) yf;
        y *= -1;
        y += oy;
        /*selector will be only within graph limits*/
        if (i == graph->select)
        {
          sx = x;
          sy = y - 1;
          flag = TRUE;
        }
      }
      if (oldx != -1)
      {
        /*plot LINE data*/
        /*set line color*/
        if (type != GRAPH_REGULAR)
        {
          switch (gy->color)
          {
          case GRAPH_COLOR_WHITE: /*1.0,  1.0,    1.0*/
            glColor3f(1.0, 1.0, 1.0);
            break;
          case GRAPH_COLOR_BLUE: /*0.0,  0.0,    1.0*/
            glColor3f(0.0, 0.0, 1.0);
            break;
          case GRAPH_COLOR_GREEN: /*0.0,  0.5,    0.0*/
            glColor3f(0.0, 0.5, 0.0);
            break;
          case GRAPH_COLOR_RED: /*1.0,  0.0,    0.0*/
            glColor3f(1.0, 0.0, 0.0);
            break;
          case GRAPH_COLOR_YELLOW: /*1.0,  1.0,    0.0*/
            glColor3f(1.0, 1.0, 0.0);
            break;
          case GRAPH_COLOR_GRAY: /*0.5,  0.5,    0.5*/
            glColor3f(0.5, 0.5, 0.5);
            break;
          case GRAPH_COLOR_NAVY: /*0.0,  0.0,    0.5*/
            glColor3f(0.0, 0.0, 0.5);
            break;
          case GRAPH_COLOR_LIME: /*0.0,  1.0,    0.0*/
            glColor3f(0.0, 1.0, 0.0);
            break;
          case GRAPH_COLOR_TEAL: /*0.0,  0.5,    0.5*/
            glColor3f(0.0, 0.5, 0.5);
            break;
          case GRAPH_COLOR_AQUA: /*0.0,  1.0,    1.0*/
            glColor3f(0.0, 1.0, 1.0);
            break;
          case GRAPH_COLOR_MAROON: /*0.5,  0.0,    0.0*/
            glColor3f(0.5, 0.0, 0.0);
            break;
          case GRAPH_COLOR_PURPLE: /*0.5,  0.0,    0.5*/
            glColor3f(0.5, 0.0, 0.5);
            break;
          case GRAPH_COLOR_OLIVE: /*0.5,  0.5,    0.0*/
            glColor3f(0.5, 0.5, 0.0);
            break;
          case GRAPH_COLOR_SILVER: /*0.75, 0.75,   0.75*/
            glColor3f(0.75, 0.75, 0.75);
            break;
          case GRAPH_COLOR_FUSHIA: /*1.0,  0.0,    1.0*/
            glColor3f(1.0, 0.0, 1.0);
            break;
          case GRAPH_COLOR_BLACK: /*0.0,0.0,0.0*/
            glColor3f(0., 0., 0.);
          case GRAPH_COLOR_DEFAULT: /*whatever title color*/
          default:
            glColor3f(sysenv.render.title_colour[0], sysenv.render.title_colour[1], sysenv.render.title_colour[2]);
          }
        }
        /*draw the LINE*/
        switch (type)
        {
        case GRAPH_IY_TYPE:
        case GRAPH_XY_TYPE:
        case GRAPH_YX_TYPE:
          /* NEW: use line*/
          switch (line)
          {
          case GRAPH_LINE_THICK:
            glLineWidth(2.0);
          case GRAPH_LINE_SINGLE:
            glBegin(GL_LINE_STRIP);
            gl_vertex_window(oldx, oldy - 1, canvas);
            gl_vertex_window(x, y - 1, canvas);
            glEnd();
            glLineWidth(1.0);
            break;
          case GRAPH_LINE_DASH:
            glEnable(GL_LINE_STIPPLE);
            glLineStipple(1, 0xCCCC);
            glBegin(GL_LINE_STRIP);
            gl_vertex_window(oldx, oldy - 1, canvas);
            gl_vertex_window(x, y - 1, canvas);
            glEnd();
            glDisable(GL_LINE_STIPPLE);
          case GRAPH_LINE_DOT:
            glEnable(GL_LINE_STIPPLE);
            glLineStipple(1, 0xAAAA);
            glBegin(GL_LINE_STRIP);
            gl_vertex_window(oldx, oldy - 1, canvas);
            gl_vertex_window(x, y - 1, canvas);
            glEnd();
            glDisable(GL_LINE_STIPPLE);
            break;
          case GRAPH_LINE_NONE:
          default:
            break;
          }
          break;
        case GRAPH_IX_TYPE:
        case GRAPH_XX_TYPE:
          /*not ready yet*/
          break;
        case GRAPH_REGULAR:
        default:
          /*draw a line*/
          glBegin(GL_LINE_STRIP);
          /* lift y axis 1 pixel up so y=0 won't overwrite the x axis */
          gl_vertex_window(oldx, oldy - 1, canvas);
          gl_vertex_window(x, y - 1, canvas);
          glEnd();
        }
      }
      oldx = x;
      oldy = y;
      /*update index*/
      switch (type)
      {
      case GRAPH_YX_TYPE:
        i++; /* ie twice ++ */
      case GRAPH_IY_TYPE:
      case GRAPH_XY_TYPE:
      case GRAPH_IX_TYPE:
      case GRAPH_XX_TYPE:
      case GRAPH_REGULAR:
      default:
        i++;
      }
    }
    j++;
  }
  /*outside of loop*/
  switch (type)
  {
  /*plot add-axis*/
  case GRAPH_IY_TYPE:
  case GRAPH_XY_TYPE:
  case GRAPH_YX_TYPE:
  case GRAPH_IX_TYPE:
  case GRAPH_XX_TYPE:
    flag = (graph->select_label != NULL);
    break;
  case GRAPH_REGULAR:
  default:
    break;
  }
  /* NEW - selector*/

  if ((flag) || (graph->select_label != NULL))
  {
    gint xoff;
    list = graph->set_list;
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0x0303);
    glColor3f(0.9, 0.7, 0.4);
    glLineWidth(2.0);
    /*depend on graph->type ; TODO: manage changing types*/
    switch (graph->type)
    {
    case GRAPH_IX_TYPE:
    case GRAPH_XX_TYPE:
      gx = (g_data_x *) list->data;
      xf = gx->x[graph->select];
      yf = graph->select_2;
      xf -= graph->xmin;
      xf /= (graph->xmax - graph->xmin);
      sx = ox + xf * dx;
      yf -= graph->ymin;
      yf /= (graph->ymax - graph->ymin);
      yf *= dy;
      sy = (gint) yf;
      sy *= -1;
      sy += oy;
      /*horizontal*/
      glBegin(GL_LINES);
      gl_vertex_window(ox, sy, canvas);
      gl_vertex_window(sx, sy, canvas);
      glEnd();
      /*vertical*/
      glBegin(GL_LINES);
      gl_vertex_window(sx, sy, canvas);
      gl_vertex_window(sx, -1 * (gint) (dy) + oy, canvas);
      glEnd();
      break;
    case GRAPH_IY_TYPE:
    case GRAPH_XY_TYPE:
      gx = (g_data_x *) list->data;
      xf = gx->x[graph->select];
      yf = graph->select_2;
      xf -= graph->xmin;
      xf /= (graph->xmax - graph->xmin);
      sx = ox + xf * dx;
      yf -= graph->ymin;
      yf /= (graph->ymax - graph->ymin);
      yf *= dy;
      sy = (gint) yf;
      sy *= -1;
      sy += oy;
      /*horizontal*/
      glBegin(GL_LINES);
      gl_vertex_window(ox, sy, canvas);
      gl_vertex_window(sx, sy, canvas);
      glEnd();
      /*vertical*/
      glBegin(GL_LINES);
      gl_vertex_window(sx, sy, canvas);
      gl_vertex_window(sx, -1 * (gint) (dy) + oy, canvas);
      glEnd();
      break;
    case GRAPH_YX_TYPE: /*this should not be selectable to begin with...*/
      gx = (g_data_x *) list->data;
      xf = graph->select_2;
      yf = gx->x[graph->select];
      xf -= graph->xmin;
      xf /= (graph->xmax - graph->xmin);
      sx = ox + xf * dx;
      yf -= graph->ymin;
      yf /= (graph->ymax - graph->ymin);
      yf *= dy;
      sy = (gint) yf;
      sy *= -1;
      sy += oy;
      /*horizontal*/
      glBegin(GL_LINES);
      gl_vertex_window(ox, sy, canvas);
      gl_vertex_window(sx, sy, canvas);
      glEnd();
      /*vertical*/
      glBegin(GL_LINES);
      gl_vertex_window(sx, sy, canvas);
      gl_vertex_window(sx, -1 * (gint) (dy) + oy, canvas);
      glEnd();
      break;
    default:
      glBegin(GL_LINES);
      gl_vertex_window(sx, sy - 10, canvas);
      gl_vertex_window(sx, 3 * gl_fontsize, canvas);
      glEnd();
    }
    /*label is always same (I think)*/
    xoff = (gint) strlen(graph->select_label);
    xoff *= gl_fontsize;
    xoff /= 4;
    pango_print(graph->select_label, sx - xoff, 0, canvas, gl_fontsize - 2, 0);
    glDisable(GL_LINE_STIPPLE);
  }
  /* NEW: set ondemand axis*/
  if (graph->require_xaxis)
  {
    /* add xaxis */
    glBegin(GL_LINE_STRIP);
    glColor3f(0.9, 0.7, 0.4); /*change?*/
    glLineWidth(2.0);
    x = ox;
    yf = (0.0 - 1.0 * graph->ymin) / (graph->ymax - graph->ymin);
    y = oy - dy * yf;
    gl_vertex_window(x, y - 1, canvas);
    x = ox + dx;
    gl_vertex_window(x, y - 1, canvas);
    glEnd();
  }
  if (graph->require_yaxis)
  {
    /* add yaxis */
    glBegin(GL_LINE_STRIP);
    glColor3f(0.9, 0.7, 0.4); /*change?*/
    glLineWidth(2.0);
    xf = -1.0 * graph->xmin / (graph->xmax - graph->xmin);
    x = ox + xf * dx;
    y = oy - dy;
    gl_vertex_window(x, y - 1, canvas);
    y = oy;
    gl_vertex_window(x, y - 1, canvas);
    glEnd();
  }
  /* NEW: set titles */
  if (graph->title)
  {
    pango_print_sz(graph->title, ox + 2, oy - (gint) dy - 4 * gl_fontsize, canvas, graph->title_font, 0);
  }
  if (graph->sub_title)
  {
    pango_print_sz(graph->sub_title, ox + 0.5 * dx + 2, oy - dy - 2 * gl_fontsize, canvas, graph->sub_title_font, 0);
  }
  if (graph->x_title)
  {
    if (graph->type == GRAPH_YX_TYPE)
    {
      gchar *ptr, *ptr2;
      ptr = g_strdup(graph->x_title);
      ptr2 = ptr;
      while ((*ptr2 != '\t') && (*ptr2 != '\0'))
        ptr2++;
      if (*ptr2 != '\0')
      {
        *ptr2 = '\0';
        ptr2++;
        pango_print_sz(ptr, ox + 2 + gl_fontsize, oy + 2 * gl_fontsize, canvas, graph->x_title_font, 0);
        pango_print_sz(ptr2, ox + 0.5 * dx + 2, oy + 2 * gl_fontsize, canvas, graph->x_title_font, 0);
      } else
      {
        pango_print_sz(ptr, ox + 2, oy + 2 * gl_fontsize, canvas, graph->x_title_font, 0);
      }
      g_free(ptr);
    } else
    {
      pango_print_sz(graph->x_title, ox + 0.33 * dx + 2, oy + 2 * gl_fontsize, canvas, graph->x_title_font, 0);
    }
  }
  if (graph->y_title)
  {
    pango_print_sz(graph->y_title, 0 + 0.5 * gl_fontsize, oy - 0.33 * dy, canvas, graph->y_title_font, 90);
  }
  /*axis?*/
}

/****************/
/* generic call */
/****************/
void graph_draw_1d(struct canvas_pak *canvas, struct graph_pak *graph)
{ /* NEW graph selector */
  switch (graph->type)
  {
  case GRAPH_REGULAR:
  case GRAPH_IY_TYPE:
  case GRAPH_XY_TYPE:
  case GRAPH_YX_TYPE:
  case GRAPH_IX_TYPE:
  case GRAPH_XX_TYPE:
    graph_draw_new(canvas, graph);
    break;
  default:
    graph_draw_new(canvas, graph);
    fprintf(stderr, "WARNING: graph type unknown!\n");
  }
}

/*********************************/
/* init for OpenGL graph drawing */
/*********************************/
void graph_draw(struct canvas_pak *canvas, struct model_pak *model)
{
  /* checks */
  g_assert(canvas != NULL);
  g_assert(model != NULL);
  if (!g_slist_find(model->graph_list, model->graph_active))
    return;

  /* init drawing model */
  glDisable(GL_LIGHTING);
  glDisable(GL_LINE_STIPPLE);
  glDisable(GL_DEPTH_TEST);

  glEnable(GL_BLEND);
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_POINT_SMOOTH);

  glDisable(GL_COLOR_LOGIC_OP);

  /* draw the appropriate type */
  graph_draw_1d(canvas, model->graph_active);
}

/*******************/
/* graph exporting */
/*******************/

/* Helper: check if a Y data set has mixed symbol/idx types (e.g. SCF vs ionic).
   Returns TRUE if the set uses idx < 0 to mark some entries as "not this type" */
static gboolean y_set_has_mixed_types(g_data_y *dy)
{
  if (!dy || !dy->y || dy->y_size <= 0) return FALSE;
  if (dy->symbol == NULL && dy->idx == NULL) return FALSE;

  gint has_cross = FALSE, has_diam = FALSE;
  for (gint i = 0; i < dy->y_size; i++)
  {
    if (dy->symbol)
    {
      if (dy->symbol[i] == GRAPH_SYMB_CROSS)   has_cross = TRUE;
      if (dy->symbol[i] == GRAPH_SYMB_DIAM)    has_diam = TRUE;
    }
    else if (dy->idx)
    {
      if (dy->idx[i] < 0)     has_cross = TRUE;  /* SCF-only marker */
      if (dy->idx[i] > 0)     has_diam = TRUE;   /* ionic step marker */
    }
  }
  return has_cross && has_diam;
}

void graph_write(gchar *name, gpointer ptr_graph)
{
  FILE *fp;
  struct graph_pak *graph = ptr_graph;
  GSList *list;

  /* checks */
  g_assert(graph != NULL);
  g_assert(name != NULL);

  gchar *filename = g_build_filename(sysenv.cwd, name, NULL);
  fp = fopen(filename, "wt");
  if (!fp)
  {
    g_free(filename);
    return;
  }

  /* --- CSV header: column labels ------------------------------------- */
  fprintf(fp, "# GDIS graph data (CSV)\n");

  /* Walk set_list to classify entries.
     New API (dat_graph): first = g_data_x*, rest = g_data_y*.
     Old API (graph_add_data): all entries are raw gdouble* (Y data only,
       X is implicit: indices 0..size-1 mapped to [xmin, xmax]). */

  GPtrArray *y_arrays = g_ptr_array_new(); /* array of gpointer (g_data_y* or gdouble*) */
  g_data_x *gx = NULL;
  gint idx = 0;

  for (list = graph->set_list; list; list = g_slist_next(list), idx++)
  {
    if (!list->data)
      continue;

    gpointer item = list->data;

    if (idx == 0)
    {
      /* First entry: try to interpret as X data */
      g_data_x *test_gx = (g_data_x *) item;
      if (test_gx && test_gx->x != NULL && test_gx->x_size > 0)
      {
        gx = test_gx;
      }
      else
      {
        /* First entry is not X data — treat as Y data */
        g_ptr_array_add(y_arrays, item);
      }
    }
    else
    {
      /* Remaining entries: always treat as Y data. */
      g_ptr_array_add(y_arrays, item);
    }
  }

  gint y_count = (gint) y_arrays->len;

  if (y_count == 0)
  {
    /* No Y data at all — just write X */
    fprintf(fp, "x\n");
    if (gx && gx->x_size > 0)
    {
      for (gint i = 0; i < gx->x_size; i++)
        fprintf(fp, "%.16g\n", gx->x[i]);
    }
  }
  else
  {
    /* Determine number of output columns.
       If the first Y set has mixed types (SCF vs ionic), split into y1+y2.
       Otherwise use one column per Y set. */
    gint n_out_cols = y_count;
    g_data_y *first_dy = NULL;
    if (y_arrays->len > 0)
    {
      gpointer first_item = g_ptr_array_index(y_arrays, 0);
      first_dy = (g_data_y *) first_item;
      if (first_dy && y_set_has_mixed_types(first_dy))
        n_out_cols++; /* extra column for the filtered subset */
    }

    fprintf(fp, "x");
    for (gint c = 0; c < n_out_cols; c++)
      fprintf(fp, ",y%d", c + 1);
    fprintf(fp, "\n");

    /* Determine the number of rows */
    gint n_rows = gx ? gx->x_size : 0;
    if (n_rows == 0 && y_arrays->len > 0)
    {
      gpointer first_y = g_ptr_array_index(y_arrays, 0);
      g_data_y *dy = (g_data_y *) first_y;
      if (dy && dy->y_size > 0)
        n_rows = dy->y_size;
      else
        n_rows = graph->size; /* old API: use stored size */
    }

    /* Write rows */
    for (gint i = 0; i < n_rows; i++)
    {
      if (gx && i < gx->x_size)
        fprintf(fp, "%.16g", gx->x[i]);
      else
      {
        /* No explicit X data — generate from indices mapped to [xmin, xmax] */
        gdouble xval;
        if (n_rows > 1)
          xval = graph->xmin + ((gdouble) i / (gdouble)(n_rows - 1)) * (graph->xmax - graph->xmin);
        else
          xval = graph->xmin;
        fprintf(fp, "%.16g", xval);
      }

      for (gint yi = 0; yi < y_arrays->len; yi++)
      {
        gpointer item = g_ptr_array_index(y_arrays, yi);
        g_data_y *dy = (g_data_y *) item;

        /* Check if this is a proper g_data_y or raw gdouble* */
        gboolean is_gy = (dy && dy->y != NULL && dy->y_size > 0);

        if (is_gy)
        {
          if (i < dy->y_size)
          {
            /* For the first Y set with mixed types, output two columns:
               y1 = all energies, y2 = only ionic step energies. */
            if (yi == 0 && y_set_has_mixed_types(dy))
            {
              /* Column y1: all energy values (NaN → ".") */
              if (isnan(dy->y[i]))
                fprintf(fp, ",.");
              else
                fprintf(fp, ",%.16g", dy->y[i]);

              /* Column y2: only ionic step entries (idx > 0) */
              if (dy->idx && dy->idx[i] > 0)
                fprintf(fp, ",%.16g", dy->y[i]);
              else
                fprintf(fp, ",.");
            }
            else
            {
              /* Normal: output value or "." for missing */
              if ((dy->idx && dy->idx[i] < 0) || isnan(dy->y[i]))
                fprintf(fp, ",.");
              else
                fprintf(fp, ",%.16g", dy->y[i]);
            }
          }
          else
            fprintf(fp, ",.");
        }
        else
        {
          /* Raw gdouble* from old graph_add_data API */
          gdouble *raw_y = (gdouble *) item;
          if (i < graph->size)
            fprintf(fp, ",%.16g", raw_y[i]);
          else
            fprintf(fp, ",.");
        }
      }
      fprintf(fp, "\n");
    }
  }

  g_ptr_array_free(y_arrays, FALSE);
  fclose(fp);
  g_free(filename);
}

/*******************/
/* graph importing */
/*******************/

/* Parse a single line: skip leading whitespace, return pointer to first non-space char. */
static inline gchar *skip_whitespace(gchar *s)
{
  while (*s && g_ascii_isspace(*s)) s++;
  return s;
}

/* Strip trailing newline/carriage-return from a string in-place; return the string. */
static gchar *strip_newline(gchar *s)
{
  gint len = (gint) strlen(s);
  while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
    s[--len] = '\0';
  return s;
}

/* Parse a keyword=value or keyword value line. Returns TRUE if 'key' matched.
   *out is set to the remainder after key (stripped of leading space). */
static gboolean parse_keyword(gchar *line, const gchar *key, gchar **out)
{
  line = skip_whitespace(line);
  if (!g_str_has_prefix(line, key))
    return FALSE;
  line += strlen(key);
  /* Allow optional '=' or space between keyword and value */
  line = skip_whitespace(line);
  if (*line == '=') line++;
  *out = (gchar *) line;
  return TRUE;
}

/* Helper: dynamically grow a double array. */
typedef struct {
  gdouble *data;
  gint     len;
  gint     cap;
} dyn_array_t;

static void dyn_init(dyn_array_t *a)
{
  a->data = NULL; a->len = 0; a->cap = 0;
}

static void dyn_append(dyn_array_t *a, gdouble v)
{
  if (a->len >= a->cap)
  {
    a->cap = a->cap ? a->cap * 2 : 1024;
    a->data = g_realloc(a->data, a->cap * sizeof(gdouble));
  }
  a->data[a->len++] = v;
}

static void dyn_append_nan(dyn_array_t *a)
{
  if (a->len >= a->cap)
  {
    a->cap = a->cap ? a->cap * 2 : 1024;
    a->data = g_realloc(a->data, a->cap * sizeof(gdouble));
  }
  a->data[a->len++] = NAN;
}

static void dyn_free(dyn_array_t *a)
{
  g_free(a->data);
}

/* Try to parse a line as a number. Returns TRUE and sets *val on success.
   Handles: "123", "." (missing), "nan", "NaN", empty lines. */
static gboolean try_parse_double(gchar *s, gdouble *val)
{
  while (*s && g_ascii_isspace(*s)) s++;
  if (*s == '\0') return FALSE;

  /* Missing-value markers: "." or "nan" (case-insensitive) */
  if ((strlen(s) == 1 && *s == '.') ||
      g_ascii_strncasecmp(s, "nan", 3) == 0)
    return FALSE;

  gchar *end;
  gdouble v = g_ascii_strtod(s, &end);
  if (end == s) return FALSE;
  *val = v;
  return TRUE;
}

/* Parse a CSV-style file with header row and data rows.
   First column is X; remaining columns are Y1, Y2, ... */
  static void add_y_blocks(GPtrArray *y_arrays, struct graph_pak *graph)
  {
    static const graph_color y_colors[] = {
      GRAPH_COLOR_BLUE,
      GRAPH_COLOR_RED,
      GRAPH_COLOR_GREEN,
      GRAPH_COLOR_PURPLE,
      GRAPH_COLOR_OLIVE,
      GRAPH_COLOR_TEAL,
      GRAPH_COLOR_NAVY,
      GRAPH_COLOR_MAROON,
    };
    static const graph_symbol y_symbols[] = {
      GRAPH_SYMB_CROSS,
      GRAPH_SYMB_SQUARE,
      GRAPH_SYMB_TRI_UP,
      GRAPH_SYMB_DIAM,
      GRAPH_SYMB_TRI_DN,
    };

    for (guint bi = 0; bi < y_arrays->len; bi++)
    {
      dyn_array_t *ya = (dyn_array_t *) g_ptr_array_index(y_arrays, bi);

      gint valid_count = 0;
      for (gint i = 0; i < ya->len; i++)
        if (!isnan(ya->data[i])) valid_count++;

      g_data_y gy;
      gy.y_size = ya->len;
      gy.y = g_new(gdouble, ya->len);
      memcpy(gy.y, ya->data, ya->len * sizeof(gdouble));
      gy.idx = NULL;
      gy.type = GRAPH_XY_TYPE;
      gy.line = GRAPH_LINE_SINGLE;
      gy.color = y_colors[bi % G_N_ELEMENTS(y_colors)];

      if (valid_count > 0)
      {
        gy.symbol = g_new(graph_symbol, ya->len);
        gy.sym_color = NULL;
        gy.mixed_symbol = FALSE;
        for (gint i = 0; i < ya->len; i++)
          gy.symbol[i] = isnan(ya->data[i]) ? GRAPH_SYMB_NONE : y_symbols[bi % G_N_ELEMENTS(y_symbols)];
      }
      else
      {
        gy.symbol = NULL;
        gy.sym_color = NULL;
        gy.mixed_symbol = FALSE;
      }
      dat_graph_add_y(gy, graph);
    }
  }

static gboolean graph_read_csv(gchar *filename, struct model_pak *model)
{
  FILE *fp;
  gchar *line;

  /* Build full path */
  gchar *fullpath;
  if (g_path_is_absolute(filename))
    fullpath = g_strdup(filename);
  else
    fullpath = g_build_filename(sysenv.cwd, filename, NULL);
  fp = fopen(fullpath, "rt");
  if (!fp)
  {
    g_free(fullpath);
    return FALSE;
  }
  g_free(fullpath);

  /* --- Phase 1: read all lines --------------------------------------- */
  dyn_array_t x_arr; dyn_init(&x_arr);
  GPtrArray *y_arrays = g_ptr_array_new_with_free_func(g_free); /* each: dyn_array_t* */

  gboolean header_seen = FALSE;
  gint expected_cols = 0; /* number of columns (including X) from header */

  line = file_read_line(fp);
  while (line)
  {
    /* Strip trailing whitespace/newlines */
    gchar *p = line + strlen(line);
    while (p > line && g_ascii_isspace(*(p - 1))) p--;
    *p = '\0';

    /* Skip blank lines and comment-only lines */
    if (*line == '\0' || *line == '#')
    {
      g_free(line); line = file_read_line(fp);
      continue;
    }

    /* --- Parse header row (first non-comment, non-empty line) -------- */
    if (!header_seen)
    {
      header_seen = TRUE;

      /* Count commas to determine column count */
      expected_cols = 1;
      for (gchar *c = line; *c; c++)
        if (*c == ',') expected_cols++;

      g_free(line); line = file_read_line(fp);
      continue;
    }

    /* --- Parse data row ---------------------------------------------- */
    /* Split on commas. We need to handle the case where a field is empty.
       Simple approach: manually walk through the string. */
    gchar *tok_start = line;
    gint col = 0;

    while (*tok_start)
    {
      if (col >= expected_cols) break; /* extra columns — ignore */

      gchar *comma = strchr(tok_start, ',');
      gchar *field;
      gint field_len;

      if (comma)
      {
        field = tok_start;
        field_len = (gint)(comma - tok_start);
        tok_start = comma + 1;
      }
      else
      {
        field = tok_start;
        field_len = (gint)strlen(tok_start);
        tok_start += field_len; /* advance past last token */
      }

      gdouble val;
      if (try_parse_double(field, &val))
      {
        if (col == 0)
          dyn_append(&x_arr, val);
        else
        {
          /* Ensure y_arrays has enough entries for this column */
          while ((gint)y_arrays->len < col)
          {
            dyn_array_t *ya = g_new(dyn_array_t, 1);
            dyn_init(ya);
            g_ptr_array_add(y_arrays, ya);
          }
          dyn_append((dyn_array_t *) g_ptr_array_index(y_arrays, col - 1), val);
        }
      }
      else
      {
        /* Missing value ("." or "nan") — store NaN to preserve row alignment */
        if (col == 0)
          dyn_append_nan(&x_arr);
        else
        {
          while ((gint)y_arrays->len < col)
          {
            dyn_array_t *ya = g_new(dyn_array_t, 1);
            dyn_init(ya);
            g_ptr_array_add(y_arrays, ya);
          }
          dyn_append_nan((dyn_array_t *) g_ptr_array_index(y_arrays, col - 1));
        }
      }

      if (!comma) break;
      col++;
    }

    g_free(line); line = file_read_line(fp);
  }
  fclose(fp);

  /* Helper: add Y data blocks with distinct colors/symbols per set. */

  /* --- Phase 2: build graph ------------------------------------------ */
  struct graph_pak *graph;

  if (x_arr.len == 0 || y_arrays->len == 0)
  {
    dyn_free(&x_arr);
    for (guint i = 0; i < y_arrays->len; i++)
      dyn_free((dyn_array_t *) g_ptr_array_index(y_arrays, i));
    g_ptr_array_free(y_arrays, TRUE);
    return FALSE;
  }

  /* Compute Y min/max from all blocks */
  gdouble ymin = G_MAXDOUBLE, ymax = -G_MAXDOUBLE;
  for (guint bi = 0; bi < y_arrays->len; bi++)
  {
    dyn_array_t *ya = (dyn_array_t *) g_ptr_array_index(y_arrays, bi);
    for (gint i = 0; i < ya->len; i++)
    {
      if (ya->data[i] < ymin) ymin = ya->data[i];
      if (ya->data[i] > ymax) ymax = ya->data[i];
    }
  }

  /* If no valid Y values found, use X range */
  if (ymin == G_MAXDOUBLE)
  {
    ymin = x_arr.data[0];
    ymax = x_arr.data[0];
    for (gint i = 1; i < x_arr.len; i++)
    {
      if (x_arr.data[i] < ymin) ymin = x_arr.data[i];
      if (x_arr.data[i] > ymax) ymax = x_arr.data[i];
    }
  }

  graph = graph_new("Graph", model);
  graph->xmin = x_arr.data[0];
  graph->xmax = x_arr.data[x_arr.len - 1];
  graph->ymin = ymin;
  graph->ymax = ymax;
  graph->type = GRAPH_XY_TYPE;

  /* X data */
  g_data_x gx;
  gx.x_size = x_arr.len;
  gx.x = g_new(gdouble, x_arr.len);
  memcpy(gx.x, x_arr.data, x_arr.len * sizeof(gdouble));
  dat_graph_set_x(gx, graph);

  /* Y data blocks with distinct colors/symbols */
  add_y_blocks(y_arrays, graph);

  /* Cleanup */
  dyn_free(&x_arr);
  for (guint i = 0; i < y_arrays->len; i++)
    dyn_free((dyn_array_t *) g_ptr_array_index(y_arrays, i));
  g_ptr_array_free(y_arrays, TRUE);

  /* Refresh tree and canvas to show the new graph. */
  extern void qt_tree_refresh(void);
  extern void qt_refresh_qt_tree(void);
  extern void redraw_canvas(gint);
  qt_tree_refresh();
  qt_refresh_qt_tree();
  redraw_canvas(1);

  return TRUE;
}

/* Parse the old structured text format (data_x / data_y blocks).
   This handles files like model_0.txt where X and Y are in separate
   single-column sections, with "." or "nan" as missing-value markers. */
static gboolean graph_read_structured(gchar *filename, struct model_pak *model)
{
  FILE *fp;
  gchar *line;

  /* Build full path */
  gchar *fullpath;
  if (g_path_is_absolute(filename))
    fullpath = g_strdup(filename);
  else
    fullpath = g_build_filename(sysenv.cwd, filename, NULL);
  fp = fopen(fullpath, "rt");
  if (!fp)
  {
    g_free(fullpath);
    return FALSE;
  }
  g_free(fullpath);

  /* Detect: does this file contain data_x / data_y section headers? */
  gboolean has_structured = FALSE;
  line = file_read_line(fp);
  while (line)
  {
    gchar *trimmed = line;
    while (*trimmed && g_ascii_isspace(*trimmed)) trimmed++;
    if (*trimmed == '\0' || *trimmed == '#')
    {
      g_free(line); line = file_read_line(fp);
      continue;
    }
    if (g_str_has_prefix(trimmed, "data_x") || g_str_has_prefix(trimmed, "data_y"))
    {
      has_structured = TRUE;
      break;
    }
    g_free(line); line = file_read_line(fp);
  }
  fclose(fp);

  if (!has_structured)
    return FALSE;

  /* --- Phase 2: parse data ------------------------------------------- */
  dyn_array_t x_arr; dyn_init(&x_arr);
  GPtrArray *y_arrays = g_ptr_array_new_with_free_func(g_free); /* each: dyn_array_t* */

  gboolean in_data_x = FALSE;
  gint y_block_target = -1;
  dyn_array_t cur_y_arr; dyn_init(&cur_y_arr);

  fp = fopen(fullpath, "rt");
  line = file_read_line(fp);
  while (line)
  {
    gchar *trimmed = strip_newline(line);
    trimmed = skip_whitespace(trimmed);

    /* Skip blank lines and comments */
    if (*trimmed == '\0' || *trimmed == '#')
    {
      g_free(line); line = file_read_line(fp);
      continue;
    }

    /* Section headers */
    if (g_str_has_prefix(trimmed, "data_x"))
    {
      in_data_x = TRUE;
      y_block_target = -1;
      g_free(line); line = file_read_line(fp);
      continue;
    }

    if (g_str_has_prefix(trimmed, "data_y"))
    {
      /* Finalize previous Y block */
      if (cur_y_arr.len > 0)
      {
        dyn_array_t *saved = g_new(dyn_array_t, 1);
        *saved = cur_y_arr;
        g_ptr_array_add(y_arrays, saved);
      }
      dyn_free(&cur_y_arr); dyn_init(&cur_y_arr);

      in_data_x = FALSE;
      y_block_target = -1;

      gchar *rest;
      if (parse_keyword(trimmed, "data_y", &rest))
        y_block_target = atoi(skip_whitespace(rest));

      g_free(line); line = file_read_line(fp);
      continue;
    }

    /* Data lines: try to parse as a number */
    gdouble dval;
    if (try_parse_double(trimmed, &dval))
    {
      if (in_data_x)
        dyn_append(&x_arr, dval);
      else if (y_block_target >= 0)
        dyn_append(&cur_y_arr, dval);
    }
    else
    {
      /* Missing value — store NaN to preserve row alignment */
      if (in_data_x)
        dyn_append_nan(&x_arr);
      else if (y_block_target >= 0)
        dyn_append_nan(&cur_y_arr);
    }

    g_free(line); line = file_read_line(fp);
  }
  fclose(fp);

  /* Finalize last Y block if any */
  if (cur_y_arr.len > 0)
  {
    dyn_array_t *saved = g_new(dyn_array_t, 1);
    *saved = cur_y_arr;
    g_ptr_array_add(y_arrays, saved);
  }

  /* --- Build graph --------------------------------------------------- */
  struct graph_pak *graph;

  if (x_arr.len == 0 || y_arrays->len == 0)
  {
    dyn_free(&x_arr);
    for (guint i = 0; i < y_arrays->len; i++)
      dyn_free((dyn_array_t *) g_ptr_array_index(y_arrays, i));
    g_ptr_array_free(y_arrays, TRUE);
    return FALSE;
  }

  /* Compute Y min/max */
  gdouble ymin = G_MAXDOUBLE, ymax = -G_MAXDOUBLE;
  for (guint bi = 0; bi < y_arrays->len; bi++)
  {
    dyn_array_t *ya = (dyn_array_t *) g_ptr_array_index(y_arrays, bi);
    for (gint i = 0; i < ya->len; i++)
    {
      if (ya->data[i] < ymin) ymin = ya->data[i];
      if (ya->data[i] > ymax) ymax = ya->data[i];
    }
  }

  graph = graph_new("Graph", model);
  graph->xmin = x_arr.data[0];
  graph->xmax = x_arr.data[x_arr.len - 1];
  graph->ymin = ymin;
  graph->ymax = ymax;
  graph->type = GRAPH_XY_TYPE;

  /* X data */
  g_data_x gx;
  gx.x_size = x_arr.len;
  gx.x = g_new(gdouble, x_arr.len);
  memcpy(gx.x, x_arr.data, x_arr.len * sizeof(gdouble));
  dat_graph_set_x(gx, graph);

  /* Y data blocks with distinct colors/symbols */
  add_y_blocks(y_arrays, graph);

  /* Cleanup */
  dyn_free(&x_arr);
  for (guint i = 0; i < y_arrays->len; i++)
    dyn_free((dyn_array_t *) g_ptr_array_index(y_arrays, i));
  g_ptr_array_free(y_arrays, TRUE);

  /* Refresh tree and canvas */
  extern void qt_tree_refresh(void);
  extern void qt_refresh_qt_tree(void);
  extern void redraw_canvas(gint);
  qt_tree_refresh();
  qt_refresh_qt_tree();
  redraw_canvas(1);

  return TRUE;
}

/* Legacy fallback parser for backward compatibility with old two-column format. */
static void graph_read_legacy(gchar *filename)
{
  gint i, j, n;
  gchar *line, **buff;
  gdouble xstart = 0.;
  gdouble xstop, x[4], *y;
  GArray *garray;
  struct graph_pak *graph;
  struct model_pak *model;
  FILE *fp;

  /* Use the active model; create one only as last resort. */
  model = sysenv.active_model;
  if (!model)
  {
    edit_model_create();
    model = sysenv.active_model;
  }

  garray = g_array_new(FALSE, FALSE, sizeof(gdouble));

  /* Build full path — don't prepend cwd if filename is already absolute. */
  gchar *fullpath;
  if (g_path_is_absolute(filename))
    fullpath = g_strdup(filename);
  else
    fullpath = g_build_filename(sysenv.cwd, filename, NULL);
  fp = fopen(fullpath, "rt");
  if (!fp)
  {
    g_free(fullpath);
    return;
  }
  line = file_read_line(fp);
  n = 0;
  x[0] = 0.;
  while (line)
  {
    gint token_count;
    buff = tokenize(line, &token_count);
    g_free(line);

    j = 0;
    for (i = 0; i < token_count && j < 4; i++)
    {
      if (str_is_float(*(buff + i)))
        x[j++] = str_to_float(*(buff + i));
    }

    if (j > 1)
    {
      if (n == 0) /* first valid line */
        xstart = x[0];
      g_array_append_val(garray, x[1]);
      n++;
    }

    g_strfreev(buff);
    line = file_read_line(fp);
  }
  xstop = x[0];
  fclose(fp);

  y = g_malloc(n * sizeof(gdouble));
  for (i = 0; i < n; i++)
    y[i] = g_array_index(garray, gdouble, i);

  if (n <= 0)
  {
    g_free(y);
    g_array_free(garray, TRUE);
    return;
  }

  /* Build proper graph using dat_graph functions. */
  graph = graph_new("Graph", model);
  graph->xmin = xstart;
  graph->xmax = xstop;
  graph->ymin = y[0];
  graph->ymax = y[0];
  for (i = 1; i < n; i++)
  {
    if (y[i] < graph->ymin) graph->ymin = y[i];
    if (y[i] > graph->ymax) graph->ymax = y[i];
  }

  /* X data: indices 0..n-1 */
  g_data_x gx;
  gx.x_size = n;
  gx.x = g_malloc(n * sizeof(gdouble));
  for (i = 0; i < n; i++)
    gx.x[i] = (gdouble) i / (gdouble) ((n > 1) ? n - 1 : 1) * (xstop - xstart) + xstart;
  dat_graph_set_x(gx, graph);

  /* Y data */
  g_data_y gy;
  gy.y_size = n;
  gy.y = y;
  gy.idx = NULL;
  gy.type = GRAPH_IY_TYPE;
  gy.line = GRAPH_LINE_THICK;
  gy.color = GRAPH_COLOR_DEFAULT;
  gy.symbol = NULL;
  gy.sym_color = NULL;
  gy.mixed_symbol = FALSE;
  dat_graph_add_y(gy, graph);

  /* Refresh tree and canvas to show the new graph. */
  extern void qt_tree_refresh(void);
  extern void qt_refresh_qt_tree(void);
  extern void redraw_canvas(gint);
  qt_tree_refresh();
  qt_refresh_qt_tree();
  redraw_canvas(1);

  g_array_free(garray, TRUE);
}

void graph_read(gchar *filename)
{
  struct model_pak *model;

  /* Use the active model; create one only as last resort. */
  model = sysenv.active_model;
  if (!model)
  {
    edit_model_create();
    model = sysenv.active_model;
  }

  /* Try CSV format first (handles both new and legacy two-column files). */
  if (graph_read_csv(filename, model))
    return;

  /* Try old structured text format (data_x / data_y blocks with missing-value markers). */
  if (graph_read_structured(filename, model))
    return;

  /* Fall back to legacy two-column space-separated parser. */
  graph_read_legacy(filename);
}
