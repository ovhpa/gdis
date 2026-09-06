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
#define USPEX_PAGE_SYSTEM 0
#define USPEX_PAGE_STRUCTURES 1
#define USPEX_PAGE_CALCULATION 2
#define USPEX_PAGE_ADVANCED 3
#define USPEX_PAGE_SPECIFIC 4
/*fixed parameters*/
#define USPEX_MAX_NUM_OPT_STEPS 16
/* gui structure */
struct uspex_calc_gui {
  /*window information*/
  gpointer *window;
  /*connection to calculation parameters*/
  uspex_calc_struct calc;
  /*actual GUI*/
  gint cur_page;
  gboolean is_dirty;
  gpointer *name;
  gpointer *file_entry;
  gchar *file_entry_path;
  gboolean have_output;
  gpointer *specific_page; /*because this page can be locked if nothing relevant*/
                           /*4.1 Type of run & System*/
  gpointer *calculationMethod;
  gpointer *calculationType;
  gpointer *_calctype_dim;
  gdouble _dim; /*absurd spin on double*/
  gpointer *_calctype_mol;
  gpointer *_calctype_var;
  gpointer *_calctype_mag;   /*VER 10.1*/
  gpointer *_calctype_mag_2; /*VER 10.1*/
  gpointer *optType;
  gboolean have_new_optType; /*VER 10.1*/
  gpointer *new_optType;     /*VER 10.1*/
  gpointer *sel_new_opt;
  gchar *_tmp_new_optType; /*VER 10.1*/
  /*atoms definition: apply/remove*/
  gint _tmp_nspecies; /*for BACKUP*/
  gpointer *atomType;
  gpointer *_atom_sym;
  gpointer *_atom_typ;
  gpointer *_atom_num;
  gpointer *_atom_val;
  /*temporary values*/
  gchar _tmp_atom_sym[3];
  gint _tmp_atom_typ;
  gint _tmp_atom_num;
  gint _tmp_atom_val;
  /*numSpecies block*/
  gpointer *numSpecies;
  gchar *_tmp_blockSpecies;
  gpointer *blockSpecies;
  gpointer *Species_apply_button;
  gpointer *Species_delete_button;
  /*bond definition: apply/auto*/
  gpointer *goodBonds;
  gpointer *_bond_d;
  gchar *_tmp_bond_d;
  gboolean auto_bonds;
  gpointer *checkMolecules;
  gpointer *checkConnectivity;
  gpointer *fitLimit; /*VER 10.1*/
  gchar *_tmp_ldaU;   /*VER 10.1*/
  gpointer *ldaU;     /*VER 10.1*/
                      /*4.2 Population*/
  gpointer *populationSize;
  gpointer *initialPopSize;
  gpointer *numGenerations;
  gpointer *stopCrit;
  gpointer *mag_nm;   /*VER 10.1*/
  gpointer *mag_fmls; /*VER 10.1*/
  gpointer *mag_fmhs; /*VER 10.1*/
  gpointer *mag_afml; /*VER 10.1*/
  gpointer *mag_afmh; /*VER 10.1*/
  gpointer *mag_fmlh; /*VER 10.1*/
  gpointer *mag_aflh; /*VER 10.1*/
                      /*4.3 Survival of the fittest & Selection*/
  gpointer *bestFrac;
  gpointer *keepBestHM;
  gpointer *reoptOld;
  /*4.4 Structure generation and variation operators*/
  gpointer *symmetries;
  gpointer *fracGene;
  gpointer *fracRand;
  gpointer *fracTopRand; /*VER 10.1*/
  gpointer *fracPerm;
  gpointer *fracAtomsMut;
  gpointer *fracRotMut;
  gpointer *fracLatMut;
  gpointer *fracSpinMut; /*VER 10.1*/
  gpointer *howManySwaps;
  gpointer *specificSwaps;
  gpointer *mutationDegree;
  gpointer *mutationRate;
  gpointer *DisplaceInLatmutation;
  gpointer *AutoFrac;
  /*4.5 Constrains*/
  gpointer *IonDistances;
  gpointer *_distances;
  gpointer *minVectorLength;
  gpointer *MolCenters;
  gpointer *_centers;
  gpointer *_centers_button;
  gpointer *constraint_enhancement;
  gboolean auto_C_ion;
  /*4.6 Cell*/
  gpointer *Latticevalues;
  gpointer *_latticeformat;
  gpointer *_latticevalue;
  gchar *_tmp_latticevalue;
  gpointer *splitInto;
  gboolean auto_C_lat;
  /*4.7 restart*/
  gpointer *pickUpYN;
  gpointer *pickUpGen;
  gpointer *pickUpFolder;
  gboolean restart_cleanup;
  /*4.8 Ab initio*/
  gboolean have_specific;      /*NEW: specific toggles*/
  gpointer *use_specific;      /*NEW: specific toggles*/
  gpointer *set_specific;      /*NEW: specific toggles*/
  gchar *_tmp_spe_folder;      /*NEW: specific toggles*/
  gpointer *spe_folder;        /*NEW: specific toggles*/
  gpointer *spe_folder_button; /**/
  gdouble _tmp_num_opt_steps;  /*because spin type is double*/
  gpointer *_num_opt_steps;    /*usually determined indirectly*/
  gdouble _tmp_curr_step;      /*because spin type is double*/
  gpointer *_curr_step;
  gboolean _tmp_isfixed;
  gpointer *_isfixed;
  gpointer *abinitioCode;
  gpointer *KresolStart;
  gpointer *vacuumSize;
  gboolean auto_step;
  gchar **_tmp_ai_input;
  gpointer *ai_input;
  gpointer *ai_input_button;
  gchar **_tmp_ai_opt;
  gpointer *ai_opt;
  gpointer *ai_opt_button;
  gpointer *ai_generate;
  gpointer *ai_lib;               /*NEW: Specific auto-update*/
  gpointer *ai_lib_button;        /*NEW: Specific auto-update*/
  gchar **_tmp_ai_lib_folder;     /*NEW: Specific auto-update*/
  gpointer *ai_lib_flavor;        /*NEW: Specific auto-update*/
  gpointer *ai_lib_flavor_button; /*NEW: Specific auto-update*/
  gpointer *ai_lib_sel;           /*NEW: Specific auto-update*/
  gchar **_tmp_ai_lib_sel;        /*NEW: Specific auto-update*/
  gpointer *numProcessors;
  gpointer *numParallelCalcs;
  gchar **_tmp_commandExecutable;
  gpointer *commandExecutable;
  gpointer *whichCluster;
  gpointer *remoteFolder;
  gpointer *PhaseDiagram;
  /*4.9 fingerprint*/
  gpointer *RmaxFing;
  gpointer *deltaFing;
  gpointer *sigmaFing;
  gpointer *toleranceFing;
  /*4.10 Antiseed*/
  gpointer *antiSeedsActivation;
  gpointer *antiSeedsMax;
  gpointer *antiSeedsSigma;
  /*4.11 spacegroup*/
  gpointer *doSpaceGroup;
  gpointer *SymTolerance;
  /*4.12 developers*/
  gpointer *repeatForStatistics;
  gpointer *stopFitness;
  gpointer *fixRndSeed;
  gpointer *collectForces;
  /*4.13 seldom used*/
  gpointer *ordering_active;
  gpointer *symmetrize;
  gpointer *valenceElectr;
  gpointer *percSliceShift;
  gpointer *dynamicalBestHM;
  gpointer *softMutOnly;
  gpointer *maxDistHeredity;
  gpointer *manyParents;
  gpointer *minSlice;
  gpointer *maxSlice;
  gpointer *numberparents;
  /*5.1 molecular: ADDITIONAL*/
  gpointer *mol_model;
  gpointer *mol_model_button;
  gdouble _tmp_num_mol;
  gpointer *num_mol;
  gpointer *mol_gdis;
  gdouble _tmp_curr_mol;
  gpointer *curr_mol;
  gboolean mol_as_gulp;
  gpointer *mol_gulp;
  gint *_tmp_mols_gdis;
  gboolean *_tmp_mols_gulp;
  gpointer *mol_apply_button;
  /*5.2 BoltzTraP*/
  gboolean have_ZT;
  gpointer *BoltzTraP_T_max;
  gpointer *BoltzTraP_T_delta;
  gpointer *BoltzTraP_T_efcut;
  gpointer *TE_T_interest;
  gpointer *TE_threshold;
  gpointer *TE_goal;
  gchar *_tmp_cmd_BoltzTraP; /*optional, and unused (for now)*/
  gpointer *cmd_BoltzTraP;   /*optional, and unused (for now)*/
  gpointer *cmd_BoltzTraP_button;
  /*5.3 Surfaces*/
  gpointer *thicknessS;
  gpointer *thicknessB;
  gpointer *reconstruct;
  gpointer *StoichiometryStart;     /*almost undocumented*/
  gpointer *E_AB;                   /*almost undocumented*/
  gpointer *Mu_A;                   /*almost undocumented*/
  gpointer *Mu_B;                   /*almost undocumented*/
  gpointer *substrate_model;        /*additional*/
  gpointer *substrate_model_button; /*additional*/
                                    /*5.4 Clusters*/
                                    /*5.5 variable composition*/
  gpointer *firstGeneMax;
  gpointer *minAt;
  gpointer *maxAt;
  gpointer *fracTrans;
  gpointer *howManyTrans;
  gpointer *specificTrans;
  /*5.6 metadynamics*/
  gpointer *ExternalPressure;
  gpointer *GaussianWidth;
  gpointer *GaussianHeight;
  gpointer *FullRelax;
  gpointer *maxVectorLength;
  gpointer *meta_model;        /*additional*/
  gpointer *meta_model_button; /*additional*/
                               /*5.7 Particle swarn optimization*/
  gpointer *PSO_softMut;
  gpointer *PSO_BestStruc;
  gpointer *PSO_BestEver;
  /*6.1 Variable-Cell Nudged Elastic Band */
  gpointer *vcnebType;
  gpointer *_vcnebtype_method;
  gpointer *_vcnebtype_img_num;
  gpointer *_vcnebtype_spring;
  gpointer *numImages;
  gpointer *numSteps;
  gpointer *optReadImages;
  gpointer *optimizerType;
  gpointer *optRelaxType;
  gpointer *dt;
  gpointer *ConvThreshold;
  gpointer *VarPathLength;
  gpointer *K_min;
  gpointer *K_max;
  gpointer *Kconstant;
  gpointer *optFreezing;
  gpointer *optMethodCIDI;
  gpointer *startCIDIStep;
  gpointer *pickupImages;
  gpointer *FormatType;
  gpointer *PrintStep;
  gpointer *img_model;        /*additional*/
  gpointer *img_model_button; /*additional*/
                              /*6.2 Transition Path Sampling*/
  gpointer *numIterations;
  gpointer *speciesSymbol;
  gpointer *mass;
  gpointer *amplitudeShoot_AB;
  gpointer *amplitudeShoot_BA;
  gpointer *magnitudeShoot_success;
  gpointer *magnitudeShoot_failure;
  gpointer *shiftRatio;
  gpointer *orderParaType;
  gpointer *opCriteria_start;
  gpointer *opCriteria_end;
  gpointer *cmdOrderParameter;
  gpointer *cmdOrderParameter_button;
  gpointer *cmdEnthalpyTemperature;
  gpointer *cmdEnthalpyTemperature_button;
  gpointer *orderParameterFile;
  gpointer *orderParameterFile_button;
  gpointer *enthalpyTemperatureFile;
  gpointer *enthalpyTemperatureFile_button;
  gpointer *trajectoryFile;
  gpointer *trajectoryFile_button;
  gpointer *MDrestartFile;
  gpointer *MDrestartFile_button;
  /* CALCUL */
  gpointer *job_uspex_exe;
  gpointer *job_path;
  gboolean have_result;
  gboolean have_v1030;
  gpointer *sel_v1030_1;
  gpointer *sel_v1030_3;
  gpointer *sel_octave;
  gboolean have_octave;
  gint index;
  /*buttons*/
  gpointer *button_save;
  gpointer *button_exec;
};
