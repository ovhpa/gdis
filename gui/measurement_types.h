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
 * Minimal type definitions for the Measurements dialog.
 */
#ifndef MEASUREMENT_TYPES_H
#define MEASUREMENT_TYPES_H

#include <glib.h>

/* Minimal struct definitions needed from gdis.h/pak.h */
#define MAX_ELEMENTS 120
#define MAX_DISPLAYED 9
#define FILELEN 512
#define LINELEN 300

struct core_pak {
  gchar atom_label[16];
  gint atom_code;
  gdouble pos[3];
  gdouble colour[3];
  gdouble bond_cutoff;
  gdouble sof;
  gint molecule;
  gint region;
  gint growth;
  gint translate;
  gdouble velocity[3];
  gdouble occupancy;
  gpointer extra;
};

struct model_pak {
  gchar name[FILELEN];
  gchar filename[FILELEN];
  GSList *cores;
  GSList *bonds;
  GSList *measure_list;
  GSList *elements;
  GSList *molecules;
  GSList *fragments;
  GSList *regions;
  gint colour_scheme;
  gint n_cores;
  gint n_bonds;
  gdouble cell[6];
  gdouble origin[3];
  gdouble scale;
  gpointer extra;
};

/* measure_pak is defined in measure.h - don't redefine here */

/* sysenv is defined in main.c - declare extern pointer */
extern struct sysenv_pak sysenv;

/* Forward declaration of sysenv_pak */
struct sysenv_pak {
  struct model_pak *active_model;
  GSList *mal;
  GSList *elements;
  gint num_elements;
};

#endif /* MEASUREMENT_TYPES_H */
