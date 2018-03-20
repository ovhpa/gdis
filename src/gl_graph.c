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
#include "graph.h"
#include "matrix.h"
#include "numeric.h"
#include "opengl.h"
#include "file.h"
#include "parse.h"
#include "dialog.h"
#include "interface.h"
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
/* externals */
extern struct sysenv_pak sysenv;
extern gint gl_fontsize;

/*******************/
/* data structures */
/*******************/
struct graph_pak
{
gint grafted;
gchar *treename;

/* graph generation parameters */
gdouble wavelength;

/* flags */
gint xlabel;
gint ylabel;

graph_type type;

/* graph layout */
gint xticks;
gint yticks;
gdouble xmin;
gdouble xmax;
gdouble ymin;
gdouble ymax;

/* NB: all sets are required to be <= size */
gint size;
GSList *set_list;

/* peak selection */
gint select;
gchar *select_label;
};

/******************************************/
/* extract info from graph data structure */
/******************************************/
gchar *graph_treename(gpointer data)
{
struct graph_pak *graph = data;
return(graph->treename);
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

free_slist(graph->set_list);
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

for (list=model->graph_list ; list ; list=g_slist_next(list))
  {
  graph = list->data;

  free_slist(graph->set_list);
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
static gint n=0;
struct graph_pak *graph;

g_assert(model != NULL);

graph = g_malloc(sizeof(struct graph_pak));

graph->treename = g_strdup_printf("%s_%d", name, n);
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
graph->type=GRAPH_REGULAR;

/* append to preserve intuitive graph order on the model tree */
model->graph_list = g_slist_append(model->graph_list, graph);

model->graph_active = graph;

n++;

return(graph);
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
  
if (ymax > graph->ymin)
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
g_assert(ticks > 1);/* FIXME: ticks shouldn't matter if label=FALSE */

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
g_assert(ticks > 1);/* FIXME: ticks shouldn't matter if label=FALSE */

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

ptr = g_malloc(size*sizeof(gdouble));

memcpy(ptr, x, size*sizeof(gdouble));

graph->xmin = x1;
graph->xmax = x2;
graph_init_y(x, graph);

graph->set_list = g_slist_append(graph->set_list, ptr);
}

/*********************************/
/* add borned (x,y) data (ovhpa) */
/*********************************/
void graph_add_borned_data(gint size,gdouble *x,gdouble x_min,gdouble x_max,gdouble y_min,gdouble y_max,gint type,gpointer data)
{
gdouble *ptr;
struct graph_pak *graph = data;

g_assert(graph != NULL);

/* try to prevent the user supplying different sized data */
/* TODO - sample in some fashion if different? */
if(type==GRAPH_BAND){
	/*the first set of a GRAPH_BAND is _not_ the same size*/
	if(graph->set_list!=NULL) {/*in other cases, proceed as usual*/
		if (graph->size) g_assert(graph->size == size);
		else graph->size = size;
	}
}else{
if (graph->size)
  g_assert(graph->size == size);
else
  graph->size = size;
}

ptr = g_malloc(size*sizeof(gdouble));

memcpy(ptr, x, size*sizeof(gdouble));

graph->xmin = x_min;
graph->xmax = x_max;
/* because graph_init_y destroy supplied graph limits */
graph->ymin = y_min;
graph->ymax = y_max;
graph->type=type;

graph->set_list = g_slist_append(graph->set_list, ptr);
}
/************************************/
/* graph data extraction primitives */
/************************************/
gdouble graph_xmin(gpointer data)
{
struct graph_pak *graph = data;
return(graph->xmin);
}

gdouble graph_xmax(gpointer data)
{
struct graph_pak *graph = data;
return(graph->xmax);
}

gint graph_ylabel(gpointer data)
{
struct graph_pak *graph = data;
return(graph->ylabel);
}

gdouble graph_wavelength(gpointer data)
{
struct graph_pak *graph = data;
return(graph->wavelength);
}

gint graph_grafted(gpointer data)
{
struct graph_pak *graph = data;
return(graph->grafted);
}

/****************/
/* draw a graph */
/****************/
#define DEBUG_GRAPH_1D 0
void graph_draw_1d_regular(struct canvas_pak *canvas, struct graph_pak *graph)
{/*previously the only type of graph*/
gint i, x, y, oldx, oldy, ox, oy, sx, sy;
gint flag;
gchar *text;
gdouble *ptr;
gdouble xf, yf, dx, dy;
GSList *list;

#if DEBUG_GRAPH_1D
printf("x range: [%f - %f]\n", graph->xmin, graph->xmax);
printf("y range: [%f - %f]\n", graph->ymin, graph->ymax);
#endif

/* compute origin */
ox = canvas->x + 4*gl_fontsize;
if (graph->ylabel)
  ox += 4*gl_fontsize;

oy = canvas->y + canvas->height - 2*gl_fontsize;
if (graph->xlabel)
  oy -= 2*gl_fontsize;

/* increments for screen drawing */
dy = (canvas->height-8.0*gl_fontsize);
dx = (canvas->width-2.0*ox);

/* axes label colour */
glColor3f(sysenv.render.fg_colour[0],
          sysenv.render.fg_colour[1],
          sysenv.render.fg_colour[2]);
glLineWidth(2.0);


/* x labels */
oldx = ox;
for (i=0 ; i<graph->xticks ; i++)
  {
/* get real index */
  xf = (gdouble) i / (gdouble) (graph->xticks-1);

  x = ox + xf*dx;

  if (graph->xlabel)
    {/*only calculate real value when needed*/
    xf *= (graph->xmax-graph->xmin);
    xf += graph->xmin;
    text = g_strdup_printf("%.2f", xf);
    gl_print_window(text, x-2*gl_fontsize, oy+2*gl_fontsize, canvas);
    g_free(text);
    }

/* axis segment + tick */
  glBegin(GL_LINE_STRIP);
  gl_vertex_window(oldx, oy, canvas);
  gl_vertex_window(x, oy, canvas);
  gl_vertex_window(x, oy+5, canvas);
  glEnd();

  oldx = x;
  }

/* y labels */
oldy = oy;
for (i=0 ; i<graph->yticks ; i++)
  {
/* get screen position */
  yf = (gdouble) i / (gdouble) (graph->yticks-1);
  y = -yf*dy;
  y += oy;

/* label */
  if (graph->ylabel)
    {/*only calculate real value when needed*/
    yf *= (graph->ymax - graph->ymin);
    yf += graph->ymin;
    if (graph->ymax > 999.999999)
      text = g_strdup_printf("%.2e", yf);
    else
      text = g_strdup_printf("%7.2f", yf);
    gl_print_window(text, 0, y-1, canvas);
    g_free(text);
    }

/* axis segment + tick */
  glBegin(GL_LINE_STRIP);
  gl_vertex_window(ox, oldy, canvas);
  gl_vertex_window(ox, y-1, canvas);
  gl_vertex_window(ox-5, y-1, canvas);
  glEnd();

  oldy = y;
  }

/* data drawing colour */
glColor3f(sysenv.render.title_colour[0],
          sysenv.render.title_colour[1],
          sysenv.render.title_colour[2]);
glLineWidth(1.0);


flag = FALSE;
sx = sy = 0;

for (list=graph->set_list ; list ; list=g_slist_next(list))
  {
  ptr = (gdouble *) list->data;

  glBegin(GL_LINE_STRIP);
  for (i=0 ; i<graph->size ; i++)
    {
    xf = (gdouble) i / (gdouble) (graph->size-1);
    x = ox + xf*dx;

/* scale real value to screen coords */
    yf = ptr[i];
    yf -= graph->ymin;
    yf /= (graph->ymax - graph->ymin);
    yf *= dy;

    y = (gint) yf;
    y *= -1;
    y += oy;

if (i == graph->select)
  {
  sx = x;
  sy = y-1;
  flag = TRUE;
  }

/* lift y axis 1 pixel up so y=0 won't overwrite the x axis */
    gl_vertex_window(x, y-1, canvas);
    }
  glEnd();

/* draw peak selector (if any) */
/* TODO - turn off is click outside range? */
  if (flag)
    {
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0x0303);
    glColor3f(0.9, 0.7, 0.4);
    glLineWidth(2.0);
    glBegin(GL_LINES);
    gl_vertex_window(sx, sy-10, canvas);
    gl_vertex_window(sx, 3*gl_fontsize, canvas);
    glEnd();

    if (graph->select_label)
      {
      gint xoff;

      xoff = strlen(graph->select_label);
      xoff *= gl_fontsize;
      xoff /= 4;
      gl_print_window(graph->select_label, sx-xoff, 2*gl_fontsize, canvas);
      }
    glDisable(GL_LINE_STIPPLE);
    }
  }


}

void graph_draw_1d_frequency(struct canvas_pak *canvas, struct graph_pak *graph)
{
gint i, x, y, oldx, oldy, ox, oy;
gchar *text;
gdouble *ptr;
gdouble xf, yf, dx, dy;
GSList *list;

/* compute origin */
ox = canvas->x + 4*gl_fontsize;
if (graph->ylabel) ox+=4*gl_fontsize;

oy = canvas->y + canvas->height - 2*gl_fontsize;
if (graph->xlabel) oy-=2*gl_fontsize;

/* increments for screen drawing */
dy = (canvas->height-8.0*gl_fontsize);
dx = (canvas->width-2.0*ox);

/* axes label colour */
glColor3f(sysenv.render.fg_colour[0],sysenv.render.fg_colour[1],sysenv.render.fg_colour[2]);
glLineWidth(2.0);

/* x labels */
oldx = ox;
for (i=0 ; i<graph->xticks ; i++)
  {
/* get real index */
  xf = (gdouble) i / (gdouble) (graph->xticks-1);
  x = ox + xf*dx;
  if (graph->xlabel)
    {/*only calculate real value when needed*/
    xf *= (graph->xmax-graph->xmin);
    xf += graph->xmin;
    text = g_strdup_printf("%.2f", xf);
    gl_print_window(text, x-2*gl_fontsize, oy+2*gl_fontsize, canvas);
    g_free(text);
    }
/* axis segment + tick */
  glBegin(GL_LINE_STRIP);
  gl_vertex_window(oldx, oy, canvas);
  gl_vertex_window(x, oy, canvas);
  gl_vertex_window(x, oy+5, canvas);
  glEnd();
  oldx = x;
  }

/* y labels */
oldy = oy;
for (i=0 ; i<graph->yticks ; i++)
  {
/* get screen position */
  yf = (gdouble) i / (gdouble) (graph->yticks-1);
  y = -yf*dy;
  y += oy;
/* label */
  if (graph->ylabel)
    {/*only calculate real value when needed*/
    yf *= (graph->ymax - graph->ymin);
    yf += graph->ymin;
    if (graph->ymax > 999.999999)
      text = g_strdup_printf("%.2e", yf);
    else
      text = g_strdup_printf("%7.2f", yf);
    gl_print_window(text, 0, y-1, canvas);
    g_free(text);
    }
/* axis segment + tick */
  glBegin(GL_LINE_STRIP);
  gl_vertex_window(ox, oldy, canvas);
  gl_vertex_window(ox, y-1, canvas);
  gl_vertex_window(ox-5, y-1, canvas);
  glEnd();
  oldy = y;
  }

/* data drawing colour */
glColor3f(sysenv.render.title_colour[0],sysenv.render.title_colour[1],sysenv.render.title_colour[2]);
glLineWidth(1.0);

/* plot 1 unit rectangle */
for (list=graph->set_list ; list ; list=g_slist_next(list))
  {
        ptr = (gdouble *) list->data;
        glBegin(GL_LINE_STRIP);
        i=0;
        while(i<graph->size){
                /*each peak is listed here*/
                xf = ptr[i]-1.0;/*1/2 size of rectangle*/
                xf -= graph->xmin;
                xf /= (graph->xmax - graph->xmin);
                x = ox + xf*dx;
                y = oy;/*base*/
                gl_vertex_window(x, y-1, canvas);/*go to*/
                yf = ptr[i+1];/*aka intensity*/
                yf -= graph->ymin;
                yf /= (graph->ymax - graph->ymin);
                yf *= dy;
                y = (gint) yf;
                y *= -1;
                y += oy;
                gl_vertex_window(x, y-1, canvas);/*go up*/
                xf = ptr[i]+1.0;/*1/2 size of rectangle*/
                xf -= graph->xmin;
                xf /= (graph->xmax - graph->xmin);
                x = ox + xf*dx;
                gl_vertex_window(x, y-1, canvas);/*move up*/
                y = oy;/*base*/
                gl_vertex_window(x, y-1, canvas);/*go down*/
/* TODO: peak selection? */
                i+=2;
        }
        glEnd();
  }
}
void graph_draw_1d_band(struct canvas_pak *canvas, struct graph_pak *graph)
{
gint i, x, y, oldx, oldy, ox, oy;
gchar *text;
gdouble *ptr;
gdouble xf, yf, dx, dy;
gdouble nbands, nkpoints, efermi, ispin;
gdouble *eval;
GSList *list;

/* compute origin */
ox = canvas->x + 4*gl_fontsize;
if (graph->ylabel) ox+=4*gl_fontsize;

oy = canvas->y + canvas->height - 2*gl_fontsize;
if (graph->xlabel) oy-=2*gl_fontsize;

/* increments for screen drawing */
dy = (canvas->height-8.0*gl_fontsize);
dx = (canvas->width-2.0*ox);

/* axes label colour */
glColor3f(sysenv.render.fg_colour[0],sysenv.render.fg_colour[1],sysenv.render.fg_colour[2]);
glLineWidth(2.0);

/* x labels */
oldx = ox;
for (i=0 ; i<graph->xticks ; i++)
  {
/* get real index */
  xf = (gdouble) i / (gdouble) (graph->xticks-1);
  x = ox + xf*dx;
  if (graph->xlabel)
    {/*only calculate real value when needed*/
    xf *= (graph->xmax-graph->xmin);
    xf += graph->xmin;
    text = g_strdup_printf("%.2f", xf);
    gl_print_window(text, x-2*gl_fontsize, oy+2*gl_fontsize, canvas);
    g_free(text);
    }
/* axis segment + tick */
  glBegin(GL_LINE_STRIP);
  gl_vertex_window(oldx, oy, canvas);
  gl_vertex_window(x, oy, canvas);
  gl_vertex_window(x, oy+5, canvas);
  glEnd();
  oldx = x;
  }

/* y labels */
oldy = oy;
for (i=0 ; i<graph->yticks ; i++)
  {
/* get screen position */
  yf = (gdouble) i / (gdouble) (graph->yticks-1);
  y = -yf*dy;
  y += oy;
/* label */
  if (graph->ylabel)
    {/*only calculate real value when needed*/
    yf *= (graph->ymax - graph->ymin);
    yf += graph->ymin;
    if (graph->ymax > 999.999999)
      text = g_strdup_printf("%.2e", yf);
    else
      text = g_strdup_printf("%7.2f", yf);
    gl_print_window(text, 0, y-1, canvas);
    g_free(text);
    }
/* axis segment + tick */
  glBegin(GL_LINE_STRIP);
  gl_vertex_window(ox, oldy, canvas);
  gl_vertex_window(ox, y-1, canvas);
  gl_vertex_window(ox-5, y-1, canvas);
  glEnd();
  oldy = y;
  }

/* data drawing colour */
glColor3f(sysenv.render.title_colour[0],sysenv.render.title_colour[1],sysenv.render.title_colour[2]);
glLineWidth(1.0);

/*the first set of GRAPH_BAND data have a special 4 value header*/
list=graph->set_list;
ptr = (gdouble *) list->data;
nbands=ptr[0];
nkpoints=ptr[1];
efermi=ptr[2];
ispin=ptr[3];
/*x is then stored as the next nkpoints value */
eval=g_malloc(nkpoints*sizeof(gdouble));/*TODO: eliminate the need of g_malloc/g_free*/
for(i=4;i<(graph->size+4);i++) eval[i-4]=ptr[i];
/*now back to "regular"*/
for (list=g_slist_next(list) ; list ; list=g_slist_next(list))
  {
  ptr = (gdouble *) list->data;

  glBegin(GL_LINE_STRIP);
  for (i=0 ; i<graph->size ; i++)
    {
    xf = eval[i];
	if((xf<graph->xmin)||(xf>graph->xmax)) continue;
    xf -= graph->xmin;
    xf /= (graph->xmax - graph->xmin);
    x = ox + xf*dx;

    yf = ptr[i];
	if((yf<graph->ymin)||(yf>graph->ymax)) continue;
    yf -= graph->ymin;
    yf /= (graph->ymax - graph->ymin);
    yf *= dy;

    y = (gint) yf;
    y *= -1;
    y += oy;

    gl_vertex_window(x, y-1, canvas);
    }
  glEnd();

  }
/*BAND requires a special axis at fermi level*/
  glBegin(GL_LINE_STRIP);
  glColor3f(0.9, 0.7, 0.4);
  glLineWidth(2.0);
  x = ox;
  yf = (0.0-1.0*graph->ymin)/(graph->ymax - graph->ymin);
  y = oy - dy*yf;
  gl_vertex_window(x, y-1, canvas);
  x = ox+dx;
  gl_vertex_window(x, y-1, canvas);
  glEnd();

g_free(eval);
}
void graph_draw_1d_dos(struct canvas_pak *canvas, struct graph_pak *graph)
{
gint i, x, y, oldx, oldy, ox, oy;
gchar *text;
gdouble *ptr;
gdouble xf, yf, dx, dy;
gint shift;
gdouble sz;
GSList *list;

/* compute origin */
ox = canvas->x + 4*gl_fontsize;
if (graph->ylabel) ox += 4*gl_fontsize;
oy = canvas->y + canvas->height - 2*gl_fontsize;
if (graph->xlabel) oy -= 2*gl_fontsize;

/* increments for screen drawing */
dy = (canvas->height-8.0*gl_fontsize);
dx = (canvas->width-2.0*ox);

/* axes label colour */
glColor3f(sysenv.render.fg_colour[0],sysenv.render.fg_colour[1],sysenv.render.fg_colour[2]);
glLineWidth(2.0);

/* x labels */
/* we WANT 0 to be a tick, if nticks>2 */
if(graph->xticks>2) {
	sz=(graph->xmax-graph->xmin)/(graph->xticks);/*size of a tick*/
	shift=(gint)(graph->xmin/sz);
	oldx=ox;
	for (i=0 ; i<=graph->xticks ; i++)
	  {
	/* get real index */
		xf=(i*sz+shift*sz);
		xf -= graph->xmin;
		xf /= (graph->xmax-graph->xmin);
		x = ox + xf*dx;
		if(x>ox+dx) continue;
	  if (graph->xlabel)
	    {
	    xf=(i+shift)*sz;/*label name*/
	    text = g_strdup_printf("%.2f", xf);
	    gl_print_window(text, x-2*gl_fontsize, oy+2*gl_fontsize, canvas);
	    g_free(text);
	    }
	/* axis segment + tick */
	  glBegin(GL_LINE_STRIP);
	  gl_vertex_window(oldx, oy, canvas);
	  gl_vertex_window(x, oy, canvas);
	  gl_vertex_window(x, oy+5, canvas);
	  glEnd();
	  oldx = x;
	  }
	/*we need to add a last segment*/
	x=ox+dx;
	glBegin(GL_LINE_STRIP);
	gl_vertex_window(oldx, oy, canvas);
	gl_vertex_window(x, oy, canvas);
	glEnd();
}else{/*do the usual xmin,xmax ticks*/
	oldx = ox;
	for (i=0 ; i<graph->xticks ; i++)
	  {
	/* get real index */
	  xf = (gdouble) i / (gdouble) (graph->xticks-1);
	  x = ox + xf*dx;
	  if (graph->xlabel)
	    {/*only calculate real value when needed*/
	    xf *= (graph->xmax-graph->xmin);
	    xf += graph->xmin;
	    text = g_strdup_printf("%.2f", xf);
	    gl_print_window(text, x-2*gl_fontsize, oy+2*gl_fontsize, canvas);
	    g_free(text);
	    }
	/* axis segment + tick */
	  glBegin(GL_LINE_STRIP);
	  gl_vertex_window(oldx, oy, canvas);
	  gl_vertex_window(x, oy, canvas);
	  gl_vertex_window(x, oy+5, canvas);
	  glEnd();
	  oldx = x;
	  }
}

/* y labels - useful? */
oldy = oy;
for (i=0 ; i<graph->yticks ; i++)
  {
/* get screen position */
  yf = (gdouble) i / (gdouble) (graph->yticks-1);
  y = -yf*dy;
  y += oy;
/* label */
  if (graph->ylabel)
    {/*only calculate real value when needed*/
    yf *= (graph->ymax - graph->ymin);
    yf += graph->ymin;
    if (graph->ymax > 999.999999)
      text = g_strdup_printf("%.2e", yf);
    else
      text = g_strdup_printf("%7.2f", yf);
    gl_print_window(text, 0, y-1, canvas);
    g_free(text);
    }
/* axis segment + tick */
  glBegin(GL_LINE_STRIP);
  gl_vertex_window(ox, oldy, canvas);
  gl_vertex_window(ox, y-1, canvas);
  gl_vertex_window(ox-5, y-1, canvas);
  glEnd();
  oldy = y;
  }

/* data drawing colour */
glColor3f(sysenv.render.title_colour[0],sysenv.render.title_colour[1],sysenv.render.title_colour[2]);
glLineWidth(1.0);

  for (list=graph->set_list ; list ; list=g_slist_next(list))
        {
        ptr = (gdouble *) list->data;
        glBegin(GL_LINE_STRIP);
        for (i=0 ; i<graph->size ; i+=2) {
                xf = ptr[i];
		if((xf<graph->xmin)||(xf>graph->xmax)) continue;
                xf -= graph->xmin;
                xf /= (graph->xmax - graph->xmin);
                x = ox+xf*dx;
                yf = ptr[i+1];
                yf -= graph->ymin;
                yf /= (graph->ymax - graph->ymin);
                yf *= dy;
                y = (gint) yf;
                y *= -1;
                y += oy;
                gl_vertex_window(x, y-1, canvas);
        }
        glEnd();
  }
/*DOS requires a special axis at e=0 (fermi level)*/
  glBegin(GL_LINE_STRIP);
  glColor3f(0.9, 0.7, 0.4);
  glLineWidth(2.0);
  xf = -1.0*graph->xmin/(graph->xmax - graph->xmin);
  x = ox+xf*dx;
  yf = -1.0*graph->ymin/(graph->ymax - graph->ymin);
  y = oy - dy*yf;
  gl_vertex_window(x, y-1, canvas);
  y = oy - dy;
  gl_vertex_window(x, y-1, canvas);
  glEnd();

}

void graph_draw_1d_dos90(struct canvas_pak *canvas, struct graph_pak *graph)
{
gint i, x, y, oldx, oldy, ox, oy;
gchar *text;
gdouble *ptr;
gdouble xf, yf, dx, dy;
GSList *list;

/* compute origin */
ox = canvas->x + 4*gl_fontsize;
if (graph->ylabel) ox+=4*gl_fontsize;

oy = canvas->y + canvas->height - 2*gl_fontsize;
if (graph->xlabel) oy-=2*gl_fontsize;

/* increments for screen drawing */
dy = (canvas->height-8.0*gl_fontsize);
dx = (canvas->width-2.0*ox);

/* axes label colour */
glColor3f(sysenv.render.fg_colour[0],sysenv.render.fg_colour[1],sysenv.render.fg_colour[2]);
glLineWidth(2.0);

/* x labels */
oldx = ox;
for (i=0 ; i<graph->xticks ; i++)
  {
/* get real index */
  xf = (gdouble) i / (gdouble) (graph->xticks-1);
  x = ox + xf*dx;
  if (graph->xlabel)
    {/*only calculate real value when needed*/
    xf *= (graph->xmax-graph->xmin);
    xf += graph->xmin;
    text = g_strdup_printf("%.2f", xf);
    gl_print_window(text, x-2*gl_fontsize, oy+2*gl_fontsize, canvas);
    g_free(text);
    }
/* axis segment + tick */
  glBegin(GL_LINE_STRIP);
  gl_vertex_window(oldx, oy, canvas);
  gl_vertex_window(x, oy, canvas);
  gl_vertex_window(x, oy+5, canvas);
  glEnd();
  oldx = x;
  }
/* y labels */
oldy = oy;
for (i=0 ; i<graph->yticks ; i++)
  {
/* get screen position */
  yf = (gdouble) i / (gdouble) (graph->yticks-1);
  y = -yf*dy;
  y += oy;
/* label */
  if (graph->ylabel)
    {/*only calculate real value when needed*/
    yf *= (graph->ymax - graph->ymin);
    yf += graph->ymin;
    if (graph->ymax > 999.999999)
      text = g_strdup_printf("%.2e", yf);
    else
      text = g_strdup_printf("%7.2f", yf);
    gl_print_window(text, 0, y-1, canvas);
    g_free(text);
    }
/* axis segment + tick */
  glBegin(GL_LINE_STRIP);
  gl_vertex_window(ox, oldy, canvas);
  gl_vertex_window(ox, y-1, canvas);
  gl_vertex_window(ox-5, y-1, canvas);
  glEnd();
  oldy = y;
  }

/* data drawing colour */
glColor3f(sysenv.render.title_colour[0],sysenv.render.title_colour[1],sysenv.render.title_colour[2]);
glLineWidth(1.0);

for (list=graph->set_list ; list ; list=g_slist_next(list))
  {
  ptr = (gdouble *) list->data;
  glBegin(GL_LINE_STRIP);
  for (i=0 ; i<graph->size ; i++)
    {
    xf = (gdouble) i / (gdouble) (graph->size-1);
    xf *= dy;
    x = (gint) xf;
    x *= -1;
    x += oy;
    yf = ptr[i];
    yf -= graph->ymin;
    yf /= (graph->ymax - graph->ymin);
    y = ox + yf*dx;/*here will be dx*0.3 to obtain small DOS left part (TODO)*/
    gl_vertex_window(y, x-1, canvas);/*this one should not be reversed.. right?*/
    }
  glEnd();
  }
}
void graph_draw_1d_bandos(struct canvas_pak *canvas, struct graph_pak *graph)
{
gint i, x, y, oldx, oldy, ox, oy;
gchar *text;
gdouble *ptr;
gdouble xf, yf, dx, dy;
GSList *list;

/* compute origin */
ox = canvas->x + 4*gl_fontsize;
if (graph->ylabel) ox+=4*gl_fontsize;

oy = canvas->y + canvas->height - 2*gl_fontsize;
if (graph->xlabel) oy-=2*gl_fontsize;

/* increments for screen drawing */
dy = (canvas->height-8.0*gl_fontsize);
dx = (canvas->width-2.0*ox);

/* axes label colour */
glColor3f(sysenv.render.fg_colour[0],sysenv.render.fg_colour[1],sysenv.render.fg_colour[2]);
glLineWidth(2.0);

/* x labels */
oldx = ox;
for (i=0 ; i<graph->xticks ; i++)
  {
/* get real index */
  xf = (gdouble) i / (gdouble) (graph->xticks-1);
  x = ox + xf*dx;
  if (graph->xlabel)
    {/*only calculate real value when needed*/
    xf *= (graph->xmax-graph->xmin);
    xf += graph->xmin;
    text = g_strdup_printf("%.2f", xf);
    gl_print_window(text, x-2*gl_fontsize, oy+2*gl_fontsize, canvas);
    g_free(text);
    }
/* axis segment + tick */
  glBegin(GL_LINE_STRIP);
  gl_vertex_window(oldx, oy, canvas);
  gl_vertex_window(x, oy, canvas);
  gl_vertex_window(x, oy+5, canvas);
  glEnd();
  oldx = x;
  }

/* y labels */
oldy = oy;
for (i=0 ; i<graph->yticks ; i++)
  {
/* get screen position */
  yf = (gdouble) i / (gdouble) (graph->yticks-1);
  y = -yf*dy;
  y += oy;
/* label */
  if (graph->ylabel)
    {/*only calculate real value when needed*/
    yf *= (graph->ymax - graph->ymin);
    yf += graph->ymin;
    if (graph->ymax > 999.999999)
      text = g_strdup_printf("%.2e", yf);
    else
      text = g_strdup_printf("%7.2f", yf);
    gl_print_window(text, 0, y-1, canvas);
    g_free(text);
    }
/* axis segment + tick */
  glBegin(GL_LINE_STRIP);
  gl_vertex_window(ox, oldy, canvas);
  gl_vertex_window(ox, y-1, canvas);
  gl_vertex_window(ox-5, y-1, canvas);
  glEnd();
  oldy = y;
  }

/* data drawing colour */
glColor3f(sysenv.render.title_colour[0],sysenv.render.title_colour[1],sysenv.render.title_colour[2]);
glLineWidth(1.0);

/* copy of DOS90 TODO: proper bandos */
for (list=graph->set_list ; list ; list=g_slist_next(list))
  {
  ptr = (gdouble *) list->data;
  glBegin(GL_LINE_STRIP);
  for (i=0 ; i<graph->size ; i++)
    {
    xf = (gdouble) i / (gdouble) (graph->size-1);
    xf *= dy;
    x = (gint) xf;
    x *= -1;
    x += oy;
    yf = ptr[i];
    yf -= graph->ymin;
    yf /= (graph->ymax - graph->ymin);
    y = ox + yf*dx;/*here dx*0.3 to obtain small DOS left part (TODO)*/
    gl_vertex_window(y, x-1, canvas);
    }
  glEnd();
  }
}
void graph_draw_1d(struct canvas_pak *canvas, struct graph_pak *graph)
{/* NEW graph selector */
	switch(graph->type){
	case GRAPH_REGULAR:
		graph_draw_1d_regular(canvas,graph);
		break;
	case GRAPH_FREQUENCY:
		graph_draw_1d_frequency(canvas,graph);
		break;
	case GRAPH_BAND:
		graph_draw_1d_band(canvas,graph);
		break;
	case GRAPH_DOS:
		graph_draw_1d_dos(canvas,graph);
		break;
	case GRAPH_DOS90:
		graph_draw_1d_dos90(canvas,graph);
		break;
	case GRAPH_BANDOS:

		break;
	case GRAPH_UNKNOWN:
	default:
		fprintf(stderr,"ERROR: graph type unkonwn");
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
void graph_write(gchar *name, gpointer ptr_graph)
{
gint i;
gchar *filename;
gdouble x, *y;
GSList *list;
FILE *fp;
struct graph_pak *graph = ptr_graph;

/* checks */
g_assert(graph != NULL);
g_assert(name != NULL);

/* init */
filename = g_build_filename(sysenv.cwd, name, NULL);
fp = fopen(filename,"wt");
if (!fp)
  return;

/* write */
for (list=graph->set_list ; list ; list=g_slist_next(list))
  {
  y = (gdouble *) list->data;

  for (i=0 ; i<graph->size ; i++)
    {
    x = (gdouble) i / (gdouble) graph->size;
    x *= (graph->xmax - graph->xmin);
    x += graph->xmin;

    fprintf(fp, "%f %f\n", x, y[i]);
    }
  }
fclose(fp);
g_free(filename);
}

/*******************/
/* graph importing */
/*******************/
#define GRAPH_SIZE_MAX 4
void graph_read(gchar *filename)
{
gint i, j, n, num_tokens;
gchar *fullpath, *line, **buff;
gdouble xstart, xstop, x[GRAPH_SIZE_MAX], *y;
GArray *garray;
gpointer graph;
struct model_pak *model;
FILE *fp;

/* TODO - close dialog */
dialog_destroy_type(FILE_SELECT);

/* read / create model / plot etc */
model = sysenv.active_model;
if (!model)
  {
  edit_model_create();
  model = sysenv.active_model;
  }

garray = g_array_new(FALSE, FALSE, sizeof(gdouble));

/* read */
fullpath = g_build_filename(sysenv.cwd, filename, NULL);
fp = fopen(fullpath, "rt");
line = file_read_line(fp);
n=0;
while (line)
  {
  buff = tokenize(line, &num_tokens);
  g_free(line);

/* get all numbers */
  j = 0;
  for (i=0 ; i<num_tokens ; i++)
    {
    if (str_is_float(*(buff+i)))
      {
      if (j < GRAPH_SIZE_MAX)
        x[j++] = str_to_float(*(buff+i));
      }
    }

/* need x & y (2 items) minimum */
  if (j > 1)
    {
    if (!n)
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
for (i=0 ; i<n ; i++)
  y[i] = g_array_index(garray, gdouble, i);

graph = graph_new("Graph", model);
graph_add_data(n, y, xstart, xstop, graph);

g_array_free(garray, TRUE);
g_free(y);
}

