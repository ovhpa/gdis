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
along with this program; if not, write to the Free Software Foundation,
Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

The GNU GPL can also be found at http://www.gnu.org
*/

/* constants */

enum { MEASURE_BOND, MEASURE_INTRA, MEASURE_INTER, MEASURE_DISTANCE, MEASURE_ANGLE, MEASURE_TORSION };

/* data structure */
#define MEASURE_MAX_CORES 4

struct measure_pak {
  int type; /* distance, angle, etc. */
  char *value;
  void *core[MEASURE_MAX_CORES];   /* participants */
  int image[MEASURE_MAX_CORES][3]; /* periodic image offset for each core */
  double colour[3];
};

/* prototypes */

#ifdef __cplusplus
extern "C" {
#endif

/* Measurement creation/testing */
void *measure_bond_test(struct core_pak **, double, double, struct model_pak *);
void *measure_distance_test(int, struct core_pak **, double, double, struct model_pak *);
void *measure_angle_test(struct core_pak **, double, double, struct model_pak *);
void *measure_torsion_test(struct core_pak **, double, double, struct model_pak *);

/* Measurement search */
void measure_bond_search(const char **, double, double, struct model_pak *);
void measure_distance_search(const char **, int, double, double, struct model_pak *);
void measure_bangle_search(const char **, double, double, struct model_pak *);
void measure_angle_search(const char **, double *, struct model_pak *);

/* Measurement type/value access */
int measure_type_get(void *);
char *measure_value_get(void *);
void *measure_cores_get(void *);
void measure_colour_get(double *, void *);
void measure_coord_get(double *, int, void *, struct model_pak *);
int measure_has_core(struct core_pak *, void *);

/* Measurement label/constituent creation */
char *measure_type_label_create(void *);
char *measure_constituents_create(void *);

/* Measurement colour */
void measure_colour_set(double, double, double, void *);

/* Measurement free/dump */
void measure_free(void *, struct model_pak *);
void measure_free_all(struct model_pak *);
void measure_dump_all(struct model_pak *);
void measure_select_all(void);

/* Measurement tree grafting */
void meas_graft_model(struct model_pak *);

/* Measurement update */
double measure_update_single(void *, struct model_pak *);
void measure_update_global(struct model_pak *);

/* Measurement geometry calculations */
gdouble measure_distance(gdouble *, gdouble *);
gdouble measure_angle(gdouble *, gdouble *, gdouble *);
gdouble measure_dihedral(gdouble *, gdouble *, gdouble *, gdouble *);
gdouble measure_torsion(struct core_pak **);

#ifdef __cplusplus
}
#endif
