/*
Copyright (C) 2018 by Okadome Valencia

hubert.valencia _at_ imass.nagoya-u.ac.jp

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

/*page numbers*/
#define VASP_PAGE_SIMPLIFIED 0
#define VASP_PAGE_CONVERGENCE 1
#define VASP_PAGE_ELECTRONIC 2
#define VASP_PAGE_ELEC2 3
#define VASP_PAGE_IONIC 4
#define VASP_PAGE_KPOINTS 5
#define VASP_PAGE_POTCAR 6
#define VASP_PAGE_EXEC 7
/* gui structure */
struct vasp_calc_gui {
  /*just a recall*/
  void *window;
  vasp_calc_struct calc;
  /*actual GUI*/
  /*simple interface*/
  void *simple_calcul;
  void *simple_system;
  gboolean simple_rgeom;
  void *simple_dim;
  gdouble dimension;
  void *simple_kgrid;
  /* Qt-compatible indices (set by VaspDialog::sync) */
  gint simple_calcul_idx;
  gint simple_system_idx;
  gint simple_kgrid_idx;
  void *simple_poscar;
  void *simple_species;
  void *simple_potcar;
  void *simple_potcar_button;
  void *simple_np;
  void *simple_ncore;
  void *simple_kpar;
  void *simple_apply;
  void *simple_message;
  void *simple_message_buff;
  /*full interface*/
  gint cur_page;
  void *name;
  void *file_entry;
  void *prec;
  void *encut;
  void *enaug;
  void *ediff;
  void *algo;
  void *ialgo;
  void *nsim;
  void *vtime;
  void *nbands;
  void *nelect;
  void *iwavpr;
  void *ismear;
  void *ismear_3; /*intentional duplicate*/
  void *sigma;
  void *fermwe;
  void *fermdo;
  void *kgamma;
  void *kgamma_3; /*intentional duplicates*/
  void *kspacing;
  void *kspacing_3; /*intentional duplicates*/
  void *lreal;
  void *ropt;
  void *lmaxmix;
  void *lmaxpaw;
  void *istart;
  void *icharg;
  void *nupdown;
  void *magmom;
  void *lmaxtau;
  void *lnoncoll;
  void *saxis;
  void *gga;
  void *lsorbit;
  void *metagga;
  void *cmbj;
  void *cmbja;
  void *cmbjb;
  void *ldau;
  void *ldau_print;
  void *ldaul;
  void *ldauu;
  void *ldauj;
  /*mixer*/
  void *nelm;
  void *nelmdl;
  void *nelmin;
  void *mixer;
  void *amix;
  void *bmix;
  void *amin;
  void *amix_mag;
  void *bmix_mag;
  void *maxmix;
  void *wc;
  void *inimix;
  void *mixpre;
  /* dipol */
  void *idipol;
  void *ldipol;
  void *lmono;
  void *dipol;
  void *epsilon;
  void *efield;
  /* dos */
  void *lorbit;
  void *have_paw;
  void *nedos;
  void *emin;
  void *emax;
  void *efermi;
  void *rwigs;
  /* linear response */
  void *loptics;
  void *lepsilon;
  void *lrpa;
  void *lnabla;
  void *lcalceps;
  void *cshift;
  /* grid */
  void *ngx;
  void *ngy;
  void *ngz;
  void *ngxf;
  void *ngyf;
  void *ngzf;
  /*ionic*/
  void *nsw;
  void *ibrion;
  void *isif;
  void *relax_ions;
  void *relax_shape;
  void *relax_volume;
  void *ediffg;
  void *pstress;
  void *nfree;
  void *potim;
  /*md*/
  void *tebeg;
  void *teend;
  void *smass;
  void *nblock;
  void *kblock;
  void *npaco;
  void *apaco;
  /*symmetry*/
  void *isym;
  void *sym_prec;
  /* POSCAR */
  void *poscar_free;
  void *poscar_direct;
  void *poscar_a0;
  void *poscar_ux;
  void *poscar_uy;
  void *poscar_uz;
  void *poscar_vx;
  void *poscar_vy;
  void *poscar_vz;
  void *poscar_wx;
  void *poscar_wy;
  void *poscar_wz;
  void *poscar_atoms;
  void *poscar_index;
  void *poscar_symbol;
  void *poscar_x;
  void *poscar_y;
  void *poscar_z;
  void *poscar_tx;
  void *poscar_ty;
  void *poscar_tz;
  /* KPOINTS */
  void *kpoints_nkpts;
  void *kpoints_mode;
  void *kpoints_cart;
  void *kpoints_kx;
  void *kpoints_ky;
  void *kpoints_kz;
  void *kpoints_sx;
  void *kpoints_sy;
  void *kpoints_sz;
  void *kpoints_coord;
  void *have_tetra;
  void *kpoints_kpts;
  void *kpoints_i;
  void *kpoints_x;
  void *kpoints_y;
  void *kpoints_z;
  void *kpoints_w;
  void *kpoints_tetra;
  void *tetra;
  void *tetra_total;
  void *tetra_volume;
  void *tetra_i;
  void *tetra_w;
  void *tetra_a;
  void *tetra_b;
  void *tetra_c;
  void *tetra_d;
  /* POTCAR */
  void *poscar_species;
  void *species_flavor;
  void *species_button;
  gboolean have_potcar_folder;
  void *potcar_select_file;
  void *potcar_file;
  void *potcar_file_button;
  void *potcar_select_folder;
  void *potcar_folder;
  void *potcar_folder_button;
  void *potcar_species;
  void *potcar_species_flavor;
  /* PERF */
  void *ncore;
  void *kpar;
  /*switches*/
  gboolean have_xml;
  gboolean poscar_dirty;
  gboolean rions;
  gboolean rshape;
  gboolean rvolume;
  gboolean have_result;
  /* CALCUL */
  void *job_vasp_exe; /*will be taken directly from gdis in the future*/
  void *job_mpirun;
  void *job_path;
  void *job_nproc;
  /*buttons*/
  void *button_save;
  void *button_exec;
  gint index;
};
