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

#ifndef VASP_TOOLTIPS_H
#define VASP_TOOLTIPS_H

/*
 * VASP Setup Dialog Tooltips
 *
 * These tooltips are preserved EXACTLY from the original implementation
 * and contain precise VASP parameter instructions as published in the
 * VASP manual chapters. They are kept in a separate header for
 * potential internationalization support.
 */

/* PRESET tab */
static const char *TOOLTIP_NAME = "The model name will be used in VASP files\nas well as GDIS display.";
static const char *TOOLTIP_FILE_ENTRY =
    "Use the result of a calculation (vasprun.xml)\nas a model to fill up settings of the current one.\nNOTE: "
    "important settings (such as NELM) will be wrong\nand all settings should be checked again. Carefully.";
static const char *TOOLTIP_CALC_TYPE = "What is the intended calculation type?";
static const char *TOOLTIP_RGEOM = "Is the current geometry:\nFAR from groundstate geometry\nor NOT-SO-FAR from it?";
static const char *TOOLTIP_DIM =
    "What is the dimension of system?\nThe DIM (0D, 1D, 2D, or 3D) will be used\nto determine some parameters in INCAR "
    "and KPOINTS\nNO CHANGE will be made to the current geometry!";
static const char *TOOLTIP_SYSTEM = "What is the electronic properties of the intended material?";
static const char *TOOLTIP_POSCAR = "Atomic symbols of the species\ndetected from the model geometry.";
static const char *TOOLTIP_KGRID = "What is the intended k-point grid density?";
static const char *TOOLTIP_POTCAR_SPECIES =
    "From POTCAR file, the following species are detected.\nThis should match the POSCAR definition above.";
static const char *TOOLTIP_POTCAR_FILE = "Where is the POTCAR file for this calculation?";
static const char *TOOLTIP_NPROC = "How many CPU(s) should be used for calculation?";
static const char *TOOLTIP_NCORE = "How many CPU(s) should work on one band?";
static const char *TOOLTIP_KPAR =
    "How many k-points should be worked on at a time?\nNote: each k-point is worked on by NCORE CPU(s).";
static const char *TOOLTIP_AUTO_PREC = "Let PREC decides on ENCUT, NG{X,Y,Z}, NG{X,Y,Z}F, and ROPT.";
static const char *TOOLTIP_AUTO_GRID = "Automatically sets NG{X,Y,Z}, and NG{X,Y,Z}F.";
static const char *TOOLTIP_AUTO_MIXER = "Use automatic mixing settings.";
static const char *TOOLTIP_AUTO_ELEC = "Automatically sets NSIM, TIME, IWAVPR, NBANDS, and NELECT";
static const char *TOOLTIP_SIMPLE_APPLY = "Use this to obtain a preset of setting.\nIf calculation is already set "
                                          "using this will\noverwrite settings resulting in an unpredictable state.";

/* CONVERGENCE tab - General */
static const char *TOOLTIP_PREC = "PREC: Ch. 6.11 DEFAULT: Normal\nSets numerical accuracy by changing\nENCUT, "
                                  "NG{X,Y,Z}, NG{X,Y,Z}F, and ROPT.\n(can be override manually)";
static const char *TOOLTIP_ENCUT = "ENCUT: Ch. 6.9 DEFAULT: MAX(ENMAX)\nPlane-wave basis cutoff energy (eV).\nDefault "
                                   "MAX(ENMAX) is taken from POTCAR file.";
static const char *TOOLTIP_ENAUG =
    "ENAUG: Ch. 6.10 DEFAULT: EAUG\nKinetic cutoff for augmentation charges.\nDefault EAUG is taken from POTCAR file.";
static const char *TOOLTIP_EDIFF =
    "EDIFF: Ch. 6.18 DEFAULT: 10^{-4}\nSet the criterion (eV) for electronic convergence.";
static const char *TOOLTIP_ALGO = "ALGO: Ch. 6.46 DEFAULT: Normal\nSelect the electronic minimization algorithm.\nIt "
                                  "is overrided by IALGO when specified.";
static const char *TOOLTIP_LDIAG = "LDIAG: Ch. 6.47 DEFAULT: TRUE\nSwitch on the sub-space rotation.";
static const char *TOOLTIP_NSIM =
    "NSIM: Ch. 6.48 DEFAULT: 4\nNumber of bands optimized at a time\nin blocked minimization algorithm.";
static const char *TOOLTIP_TIME =
    "TIME: Ch. 6.51 DEFAULT: 0.4\nSelect the trial time or value step\nfor minimization algorithm IALGO=5X.";
static const char *TOOLTIP_IWAVPR =
    "IWAVPR: Ch. 6.26 DEFAULT: 2(IBRION=0-2)\nDetermine how charge density/orbitals are predicted\nfrom one ionic "
    "configuration to the next.\nDefault is 0 if IBRION is different from 0-2.";
static const char *TOOLTIP_IALGO =
    "IALGO: Ch. 6.47 DEFAULT: 38\nSets the minimization algorithm number.\nIt is advised to use ALGO instead of "
    "IALGO\ndue to instabilities in extra IALGO algorithms.";
static const char *TOOLTIP_NBANDS = "NBANDS: Ch. 6.5 DEFAULT: NELEC/2+NIONS/2\nNumber of bands in the "
                                    "calculation.\nDefault for spin-polarized calcualtions is 0.6*NELECT+NMAG";
static const char *TOOLTIP_NELECT = "NELECT: Ch. 6.35 DEFAULT: Ne valence\nSets the number of electrons\nCan be use "
                                    "add an extra charge background\nas an homogeneous background-charge.";
static const char *TOOLTIP_INIWAV = "INIWAV: Ch. 6.16 DEFAULT: 1\nDetermine how initial wavefunctions are filled\nThe "
                                    "only option is the default random filling.";
static const char *TOOLTIP_ISTART = "ISTART: Ch. 6.14 DEFAULT 1\nDetermine whether WAVECAR should be read.\nDefault is "
                                    "0 when no WAVECAR file is found.";
static const char *TOOLTIP_ICHARG = "ICHARG: Ch. 6.15 DEFAULT 2\nDetermine how the initial charge density is "
                                    "calculated\nDefault is 0 if ISTART is not 0.";
static const char *TOOLTIP_NELM = "NELM: Ch. 6.17 DEFAULT: 60\nMaximum number of electronic steps.";
static const char *TOOLTIP_NELMDL = "NELMDL: Ch. 6.17 DEFAULT -5(if IALGO=x8)\nSets the number of initial "
                                    "non-selfconsistent steps\ndefault is 0 if ISTART is not 0.";
static const char *TOOLTIP_NELMIN = "NELMIN: Ch. 6.17 DEFAULT: 2\nMinimum number of electronic steps.";

/* CONVERGENCE tab - Mixing */
static const char *TOOLTIP_IMIX = "IMIX: Ch. 6.49 DEFAULT: 4\nDetermine the type of mixing.";
static const char *TOOLTIP_AUTO_MIXER_CHECK = "Use automatic mixing settings.";
static const char *TOOLTIP_MIXPRE = "MIXPRE: Ch. 6.49 DEFAULT: 1\nSelect the preconditioning for Broyden mixing.";
static const char *TOOLTIP_AMIX =
    "AMIX: Ch. 6.49 DEFAULT: 0.4\nlinear mixing parameter\nDefault is 0.8 for ultrasoft pseudopotentials.";
static const char *TOOLTIP_BMIX = "BMIX: Ch. 6.49 DEFAULT: 1.0\ncutoff wavevector for Kerker mixing.";
static const char *TOOLTIP_AMIN = "AMIN: Ch. 6.49 DEFAULT: 0.1\nminimal mixing parameter.";
static const char *TOOLTIP_INIMIX = "INIMIX: Ch. 6.49 DEFAULT: 1\ntype of initial mixing in Broyden mixing.";
static const char *TOOLTIP_MAXMIX = "MAXMIX: Ch. 6.49 DEFAULT: -45\nMaximum number of steps stored in Broyden mixer.";
static const char *TOOLTIP_AMIX_MAG = "AMIX_MAG: Ch. 6.49 DEFAULT: 1.6\nlinear mixing parameter for magnetization.";
static const char *TOOLTIP_BMIX_MAG =
    "BMIX_MAG: Ch. 6.49 DEFAULT: 1.0\ncutoff wavevector for Kerker mixing with magnetization.";
static const char *TOOLTIP_WC = "WC: Ch. 6.49 DEFAULT: 1000\nstep weight factor for Broyden mixing.";
static const char *TOOLTIP_ADDGRID =
    "ADDGRID: Ch. 6.63 DEFAULT: FALSE\nSelect the third fine grid for augmentation charge.";
static const char *TOOLTIP_LMAXMIX =
    "LMAXMIX: Ch. 6.63 DEFAULT: 2\nMaximum l-quantum number passed to charge density mixer.";
static const char *TOOLTIP_LMAXPAW = "LMAXPAW: Ch. 6.63 DEFAULT: 2*lmax\nMaximum l-quantum number for the evaluation "
                                     "of the PAW\non-site terms on the radial grid.";

/* CONVERGENCE tab - Grid */
static const char *TOOLTIP_LREAL = "LREAL: Ch. 6.39 DEFAULT: FALSE\nDetermine whether projection operators are "
                                   "evaluated in\nreal-space or reciprocal-space.";
static const char *TOOLTIP_ROPT =
    "ROPT: Ch. 6.39 DEFAULT: -\nROPT determine the precision of the real-space operator\nfor LREAL=AUTO and LREAL=On.";
static const char *TOOLTIP_NGX = "NGX: Ch. 6.3 DEFAULT: -\nnumber of FFT-mesh grid-points along the x direction.";
static const char *TOOLTIP_NGY = "NGY: Ch. 6.3 DEFAULT: -\nnumber of FFT-mesh grid-points along the y direction.";
static const char *TOOLTIP_NGZ = "NGZ: Ch. 6.3 DEFAULT: -\nnumber of FFT-mesh grid-points along the z direction.";
static const char *TOOLTIP_NGXF =
    "NGXF: Ch. 6.3 DEFAULT: -\nnumber of fine FFT-mesh grid-points along the x direction.";
static const char *TOOLTIP_NGYF =
    "NGYF: Ch. 6.3 DEFAULT: -\nnumber of fine FFT-mesh grid-points along the y direction.";
static const char *TOOLTIP_NGZF =
    "NGZF: Ch. 6.3 DEFAULT: -\nnumber of fine FFT-mesh grid-points along the z direction.";

/* ELECT-I tab */
static const char *TOOLTIP_ISMEAR = "ISMEAR: Ch. 6.38 DEFAULT: 1\nDetermine how the partial occupations are set.\nIt "
                                    "is advised to change it to ISMEAR=0\nfor semiconducting or insulating materials.";
static const char *TOOLTIP_SIGMA = "SIGMA: Ch. 6.38 DEFAULT: 0.2\nDetermine the width of the smearing (eV)\nFor "
                                   "ISMEAR=0 (semiconductors or insulators)\na small SIGMA=0.05 can be chosen.";
static const char *TOOLTIP_GAMMA = "KGAMMA: Ch. 6.4 DEFAULT: TRUE\nWhen KPOINTS file is not present KGAMMA\nindicate "
                                   "if the k-grid is centered around gamma point.";
static const char *TOOLTIP_KSPACING = "KSPACING: Ch. 6.4 DEFAULT: 0.5\nWhen KPOINTS file is not present "
                                      "KSPACING\nDetermine the smallest spacing between k-points (Ang^-1).";
static const char *TOOLTIP_FERMWE =
    "FERMWE: Ch. 6.38 DEFAULT -\nFor ISMEAR=-2 explicitly sets\noccupancy for each band.";
static const char *TOOLTIP_FERDO =
    "FERDO: Ch. 6.38 DEFAULT -\nFor ISMEAR=-2 and ISPIN=2 sets\noccupancy for down spin of each band.";
static const char *TOOLTIP_ISPIN = "ISPIN: Ch. 6.12 DEFAULT 1\nSpin polarized calculation sets ISPIN=2.";
static const char *TOOLTIP_NONCOLLINEAR =
    "LNONCOLLINEAR: Ch. 6.68.1 DEFAULT FALSE\nAllow to perform fully, non-collinear\nmagnetic structure calculations.";
static const char *TOOLTIP_LSORBIT = "LSORBIT: Ch. 6.68.2 DEFAULT: FALSE\nSwitch on spin-orbit coupling (for "
                                     "PAW)\nautomatically sets LNONCOLLINEAR when TRUE.";
static const char *TOOLTIP_GGA = "GGA: Ch. 6.40 DEFAULT -\nSets the gradient corrected functional.\nThe default is "
                                 "selected according to the POTCAR file.";
static const char *TOOLTIP_VOSKOWN = "VOSKOWN: Ch. 6.41 DEFAULT 0\nSets the Vosko, Wilk and Nusair method\nfor "
                                     "interpolation of correlation functional).\nUseful only if GGA=91 (PW91).";
static const char *TOOLTIP_GGA_COMPAT =
    "GGA_COMPAT: Ch. 6.42 DEFAULT: TRUE\nRestores the lattice symmetry\nfor gradient corrected functionals.";
static const char *TOOLTIP_NUPDOWN =
    "NUPDOWN: Ch. 6.36 DEFAULT -1\nSets the difference between number\nof electrons in up and down spin component.";
static const char *TOOLTIP_MAGMOM =
    "MAGMOM: Ch. 6.13 DEFAULT: NIONS\nSets the initial magnetic moment for each atom\nuseful only for calculation with "
    "no WAVECAR or CHGCAR\nor magnetic calculations from a non-magnetic start.\nFor non-collinear calculations default "
    "value is 3*NIONS.";
static const char *TOOLTIP_SAXIS = "SAXIS: Ch. 6.68.2 DEFAULT -\nSets the quantisation axis for spin.\nThe default is "
                                   "SAXIS = eps 0 1\nwhere eps is a small quantity.";
static const char *TOOLTIP_METAGGA = "METAGGA: Ch. 6.43 DEFAULT none\nSets the meta-GGA functional.";
static const char *TOOLTIP_LMETAGGA = "LMETAGGA (secret) DEFAULT FALSE\nIgnored (but recognized and read) by "
                                      "VASP.\nUsed (here) to enable a METAGGA calculation.";
static const char *TOOLTIP_LASPH = "LASPH: Ch. 6.44 DEFAULT FALSE\nSets calculation of non-spherical "
                                   "contributions\nfrom the core electrons to the XC potential.";
static const char *TOOLTIP_LMAXTAU =
    "LMAXTAU: Ch. 6.43.1 DEFAULT 0\nSets maximum l-quantum number\nto calculate PAW one-center expansion\nof the "
    "kinetic energy density.\nDefault is 6 if LASPH is set to TRUE.";
static const char *TOOLTIP_LMIXTAU = "LMIXTAU: Ch. 6.43.2 DEFAULT FALSE\nSets inclusion of kinetic energy density\nto "
                                     "the mixer for meta-GGA calculations.";
static const char *TOOLTIP_CMBJ = "CMBJ: Ch 6.43 DEFAULT: -\nFor MBJ meta-GGA calculations CMBJ\nsets the mixing "
                                  "coefficient for each electronic step.\nUnused if CMBJ is specified.";
static const char *TOOLTIP_CMBJA = "CMBJA: Ch. 6.43 DEFAULT -0.012\nAlpha parameter to calculate "
                                   "CMBJ\nSelfconsistently at each electronic step.\nUnused if CMBJ is specified.";
static const char *TOOLTIP_CMBJB = "CMBJB: Ch. 6.43 DEFAULT 1.023\nBeta parameter to calculate CMBJ\nSelfconsistently "
                                   "at each electronic step.\nUnused if CMBJ is specified.";
static const char *TOOLTIP_LDAUTYPE = "LDAUTYPE: Ch. 6.70 DEFAULT: 2\nSets the type of L(S)DA+U calculation.";
static const char *TOOLTIP_LDAU = "LDAU: Ch. 6.70 DEFAULT: FALSE\nSwitches on L(S)DA+U calculation.";
static const char *TOOLTIP_LDAUL = "LDAUL: Ch. 6.70 DEFAULT: 2\nSets the l-quantum number (for each species)\nfor "
                                   "which the on-site interaction is added.";
static const char *TOOLTIP_LDAUPRINT = "LDAUPRINT: Ch. 6.70 DEFAULT 0\nSets the level of verbosity of L(S)DA+U.";
static const char *TOOLTIP_LDAUU =
    "LDAUU: Ch. 6.70 DEFAULT: -\nSets effective on-site Coulomb interaction (for each species).";
static const char *TOOLTIP_LDAUJ =
    "LDAUJ: Ch. 6.70 DEFAULT: -\nSets effective on-site Exchange interaction (for each species).";
static const char *TOOLTIP_IDIPOL =
    "IDIPOL: Ch. 6.64 DEFAULT: -\nSets the direction of the calculated dipole moment\nparallel to the 1st (1), 2nd (2) "
    "or 3rd (3) lattice vector.\nIDIPOL=4 trigger full dipole calculation\n0 switches off dipole calculation (remove "
    "IDIPOL).";
static const char *TOOLTIP_LDIPOL = "LDIPOL: Ch. 6.64 DEFAULT: FALSE\nSets potential dipole correction of "
                                    "the\nslab/molecule periodic-boundary induced errors.";
static const char *TOOLTIP_LMONO = "LMONO: Ch. 6.64 DEFAULT: FALSE\nSets potential monopole correction of "
                                   "the\nslab/molecule periodic-boundary induced errors.";
static const char *TOOLTIP_DIPOL = "DIPOL: Ch. 6.64 DEFAULT: -\nSets the center of the net charge distribution.\nIf "
                                   "left blank a guess value is sets at runtime.";
static const char *TOOLTIP_EPSILON = "EPSILON: Ch. 6.64 DEFAULT: 1\nSets the dielectric constant of the medium.";
static const char *TOOLTIP_EFIELD =
    "EFIELD: Ch. 6.64 DEFAULT: 0\nApply an external electrostatic field\nin a slab, or molecule calculation.\nOnly a "
    "single component can be given\nparallel to IDIPOL direction (1-3).";
static const char *TOOLTIP_LORBIT = "LORBIT: Ch. 6.34 DEFAULT: 0\nSets the spd- and site projected\nwavefunction "
                                    "character of each band\nand the local partial DOS.";
static const char *TOOLTIP_NEDOS = "NEDOS: Ch. 6.37 DEFAULT: 301\nNumber of points for which DOS is calculated.";
static const char *TOOLTIP_EMIN = "EMIN: Ch. 6.37 DEFAULT: low eig-eps\nSets the smallest energy for which DOS is "
                                  "calculated.\nDefault is lowest Kohn-Sham eigenvalue minus a small quantity.";
static const char *TOOLTIP_EMAX = "EMAX: Ch. 6.37 DEFAULT: high eig+eps\nSets the highest energy for which DOS is "
                                  "calculated.\nDefault is highest Kohn-Sham eigenvalue plus a small quantity.";
static const char *TOOLTIP_EFERMI =
    "EFERMI: (secret) DEFAULT: EREF\nSets initial Fermi energy before it is calculated.\nUsually there is no use to "
    "change this setting.\nEREF is actually another \"secret\" parameter.";
static const char *TOOLTIP_HAVE_PAW = "Is set if a PAW POTCAR file is detected.";
static const char *TOOLTIP_RWIGS =
    "RWIGS: Ch. 6.33 DEFAULT: -\nSets the Wigner Seitz radius for each species.\nDefault value is read from POTCAR.";
static const char *TOOLTIPLOPTICS =
    "LOPTICS: Ch. 6.72.1 DEFAULT: FALSE\nSwitch frequency dependent dielectric matrix calculation\nover a grid "
    "determined by NEDOS.\nIt is advised to increase NEDOS & NBANDS.";
static const char *TOOLTIP_LEPSILON =
    "LEPSILON: Ch. 6.72.4 DEFAULT: FALSE\nSwitch dielectric matrix calculation using\ndensity functional perturbation "
    "theory.\nConcurrent to LOPTICS, it is not recommended\nto run both at the same time.";
static const char *TOOLTIP_LRPA =
    "LRPA: Ch. 6.72.5 DEFAULT: FALSE\nSwitch local field effects calculation\non the Hartree level only (no XC).";
static const char *TOOLTIP_LNABLA =
    "LNABLA: Ch. 6.72.3 DEFAULT: FALSE\nSwitch to the simple transversal expressions\nof the frequency dependent "
    "dielectric matrix.\nUsually there is no use to change this setting.";
static const char *TOOLTIP_CSHIFT =
    "CSHIFT: Ch. 6.72.2 DEFAULT: 0.1\nSets the complex shift nu of the Kramers-Kronig\ntransformation of the "
    "dielectric function.\nIf CSHIFT is decreased, NEDOS should be increased.";

/* IONIC tab */
static const char *TOOLTIP_IBRION =
    "IBRION: Ch. 6.22 DEFAULT -1\nSets the algorithm for the ions motion.\nDefault is IBRION=0 if NSW>1.";
static const char *TOOLTIP_NSW = "NSW: Ch. 6.20 DEFAULT 0\nSet the maximum number of ionic steps.";
static const char *TOOLTIP_EDIFFG =
    "EDIFFG: Ch. 6.19 DEFAULT EDIFF*10\nSets the energy criterion for ending ionic relaxation\n>0 value are for energy "
    "difference (eV)\n<0 values are for absolute forces value (eV/Ang).";
static const char *TOOLTIP_POTIM =
    "POTIM: Ch. 6.23 DEFAULT 0.5\nFor IBRION=0 (MD) sets time step (fs)\nfor IBRION=1-3 sets force scaling constant.";
static const char *TOOLTIP_PSTRESS =
    "PSTRESS: Ch. 6.25 DEFAULT 0\nSets the Pulay stress correction (kBar)\nalso used to set an external pressure.";
static const char *TOOLTIP_ISIF =
    "ISIF: Ch. 6.24 DEFAULT 2\nSwitch to calculate F(force) and S(stress)\nand to relax I(ion) S(cell shape) and "
    "V(cell voume)\n0 = no calcul/no relaxation P = only total Pressure\nDefault is 0(F_0_I_0_0) if IBRION=0.";
static const char *TOOLTIP_RELAX_ATOMIC = "Switches atomic relaxation.\nUpdates ISIF accordingly (but not IBRION).";
static const char *TOOLTIP_RELAX_SHAPE = "Switches cell shape relaxation.\nUpdates ISIF accordingly (but not IBRION).";
static const char *TOOLTIP_RELAX_VOLUME =
    "Switches cell volume relaxation.\nUpdates ISIF accordingly (but not IBRION).";
static const char *TOOLTIP_NFREE =
    "NFREE: Ch. 6.22 DEFAULT: -\nSets the number of ionic steps kept in history\nfor IBRION=1 Quasi-Newton "
    "algorithm.\nFor finite difference IBRION=5-6 NFREE\nsets the number of displacement/direction/atom.";
static const char *TOOLTIP_TEBEG =
    "TEBEG: Ch. 6.29 DEFAULT: 0\nSets the start temperature of MD run.\nTo be corrected by (Nion-1)/Nion.";
static const char *TOOLTIP_TEEND =
    "TEEND: Ch. 6.29 DEFAULT: TEBEG\nSets the end temperature of MD run.\nTo be corrected by (Nion-1)/Nion.";
static const char *TOOLTIP_SMASS =
    "SMASS: Ch. 6.30 DEFAULT 0\nControls the velocities during MD.\nFor IBRION=3 SMASS is used as a damping factor.";
static const char *TOOLTIP_NBLOCK =
    "NBLOCK: Ch. 6.21 DEFAULT: 1\nSets number of steps after which DOS/pair correlation\nfunction are calculated, and "
    "XDATCAR updated.\nFor SMASS=-1 kinetic energy is scaled every NBLOCK.";
static const char *TOOLTIP_KBLOCK = "KBLOCK: Ch. 6.21 DEFAULT: NSW\nAfter KBLOCK*NBLOCK iterations DOS and\naverage "
                                    "pair correlation function is written\nto DOSCAR and PCDAT, respectively.";
static const char *TOOLTIP_NPACO = "NPACO Ch. 6.31 DEFAULT: 256\nNumber of slots of pair correlation function.";
static const char *TOOLTIP_APACO =
    "APACO: Ch. 6.31 DEFAULT: 16\nMax distance (Ang) for calculation\nof pair correlation function.";
static const char *TOOLTIP_SEL_DYN = "Set position tags to \"F F F\" (FIXED)\n\"T T T\" (FREE) or user modifications.";
static const char *TOOLTIP_TAGS = "Allow use of tags for ion motion (Ch. 5.7).";
static const char *TOOLTIP_ISYM =
    "ISYM: Ch. 6.27 DEFAULT 2\nSwitch symmetry method (0=OFF)\nDefault is 1 for ultrasoft, 2 for PAW PP.";
static const char *TOOLTIP_SYMPREC =
    "SYMPREC: Ch. 6.27 DEFAULT 10^-5\nSets the accuracy of ion positions\nin determining the system symmetry.";
static const char *TOOLTIP_POSCAR_A0 = "Lattice parameter (Ch. 5.7)\nSets the lattice constant (Ang.) parameter\nevery "
                                       "lattice vector will be scaled with.";
static const char *TOOLTIP_DIRECT = "Sets whether ions positions are given\nin direct lattice or cartesian coordinate.";

/* POSCAR editing */
static const char *TOOLTIP_POSCAR_UX = "First lattice vector x component.";
static const char *TOOLTIP_POSCAR_UY = "First lattice vector y component.";
static const char *TOOLTIP_POSCAR_UZ = "First lattice vector z component.";
static const char *TOOLTIP_POSCAR_VX = "Second lattice vector x component.";
static const char *TOOLTIP_POSCAR_VY = "Second lattice vector y component.";
static const char *TOOLTIP_POSCAR_VZ = "Second lattice vector z component.";
static const char *TOOLTIP_POSCAR_WX = "Third lattice vector x component.";
static const char *TOOLTIP_POSCAR_WY = "Third lattice vector y component.";
static const char *TOOLTIP_POSCAR_WZ = "Third lattice vector z component.";
static const char *TOOLTIP_POSCAR_TX = "Tag for the motion of current ion\nin first lattice coordinate.";
static const char *TOOLTIP_POSCAR_TY = "Tag for the motion of current ion\nin second lattice coordinate.";
static const char *TOOLTIP_POSCAR_TZ = "Tag for the motion of current ion\nin third lattice coordinate.";
static const char *TOOLTIP_POSCAR_ATOMS = "Atoms positions in POSCAR format.";
static const char *TOOLTIP_POSCAR_INDEX = "Atom number (in POSCAR ORDER).";
static const char *TOOLTIP_POSCAR_SYMBOL =
    "Atom symbol (will correspond to POSCAR).\nChanging symbol will change index accordingly.";
static const char *TOOLTIP_POSCAR_X =
    "atom component X: x in cartesian mode\nand first lattice coordinate in direct mode.";
static const char *TOOLTIP_POSCAR_Y =
    "atom component Y: y in cartesian mode\nand second lattice coordinate in direct mode.";
static const char *TOOLTIP_POSCAR_Z =
    "atom component Z: z in cartesian mode\nand third lattice coordinate in direct mode.";

/* KPOINTS tab */
static const char *TOOLTIP_KPT_MODE = "Sets how the k-points are generated/entered as defined in Ch. 5.5.";
static const char *TOOLTIP_KPT_CART = "Switch cartesian/reciprocal lattice coordinate mode.";
static const char *TOOLTIP_KPT_GRID = "Grid size for automated generation\n1st component only for M&P.";
static const char *TOOLTIP_KPT_GRID_Y = "2nd Component of the M&P grid size.";
static const char *TOOLTIP_KPT_GRID_Z = "3rd Component of the M&P grid size.";
static const char *TOOLTIP_KPT_TOTAL = "Total number of k-points (0 for automated methods).";
static const char *TOOLTIP_TETRA =
    "Switch on the tetrahedron method for entering k-points.\nISMEAR must be set to -5 for this method!";
static const char *TOOLTIP_KPT_SX = "In case a grid is used to generate k-points\nThis will set a shift from grid "
                                    "origin\nin 1st reciprocal lattice component.";
static const char *TOOLTIP_KPT_SY = "In case a grid is used to generate k-points\nThis will set a shift from grid "
                                    "origin\nin 2nd reciprocal lattice component.";
static const char *TOOLTIP_KPT_SZ = "In case a grid is used to generate k-points\nThis will set a shift from grid "
                                    "origin\nin 3rd reciprocal lattice component.";
static const char *TOOLTIP_KPT_LIST = "List of the k-points coordinates.\nThe index after comment sign \"! kpoint:\" "
                                      "can be used\nfor entering tetrahedron edge definition.";
static const char *TOOLTIP_KPT_INDEX = "The index of current k-point.\n(can be changed by deletion/addition only)";
static const char *TOOLTIP_KPT_X =
    "Coordinate of k-point in 1st component of selected mode.\n(cartesian or reciprocal)";
static const char *TOOLTIP_KPT_Y =
    "Coordinate of k-point in 2nd component of selected mode.\n(cartesian or reciprocal)";
static const char *TOOLTIP_KPT_Z =
    "Coordinate of k-point in 3rd component of selected mode.\n(cartesian or reciprocal)";
static const char *TOOLTIP_KPT_W = "Symmetry degeneration weight of k-point.\nSum of all weight does not have to be "
                                   "1\nbut relative ratio should be respected.";
static const char *TOOLTIP_TETRA_N = "Total number of tetrahedrons.";
static const char *TOOLTIP_TETRA_VOLUME = "Volume weight for a single tetrahedron.\nAll have a volume defined by the "
                                          "ratio of\ntetrahedron volume / Brillouin zone Volume.";
static const char *TOOLTIP_TETRA_LIST = "Tetrahedron list, consisting of\nsymmetry degeneration weight followed "
                                        "by\nfour corner points of each tetrahedron\nin k-point indices given above.";
static const char *TOOLTIP_TETRA_INDEX =
    "The index of current tetrahedron.\n(can be changed by deletion/addition only)";
static const char *TOOLTIP_TETRA_W = "Symmetry degeneration weight.\nUnlike k-points, weight must be normalized\nie. "
                                     "total Brillouin zone Volume should be recovered\nby summing all degeneration * V";
static const char *TOOLTIP_TETRA_A = "1st corner of the tetrahedron (A) in k-point index.";
static const char *TOOLTIP_TETRA_B = "2nd corner of the tetrahedron (B) in k-point index.";
static const char *TOOLTIP_TETRA_C = "3rd corner of the tetrahedron (C) in k-point index.";
static const char *TOOLTIP_TETRA_D = "4th corner of the tetrahedron (D) in k-point index.";

/* POTCAR tab */
static const char *TOOLTIP_POTCAR_LIST =
    "List of atomic symbol of the species\ndetected in the POSCAR ionic information.";
static const char *TOOLTIP_POTCAR_SELECT_FILE =
    "Load a previously prepared POTCAR file.\nThe species (and species order) should match the POSCAR ion information.";
static const char *TOOLTIP_POTCAR_SELECT_FOLDER =
    "Use the directory where all POTCAR information are available.\nUsually a directory containing atomic symbols "
    "sub-directories.";
static const char *TOOLTIP_POTCAR_FILE_PATH = "POTCAR file, including its full path.";
static const char *TOOLTIP_POTCAR_PATH = "Full PATH to the POTCAR sub-directories.";
static const char *TOOLTIP_POTCAR_FLAVOR =
    "When Using PATH method, for each species a choice\nbetween different pseudopotential setting (flavors) should "
    "match EXACTLY those of the POSCAR ion information.";
static const char *TOOLTIP_POTCAR_SPECIES_LIST =
    "List of atomic symbol of the species\ndetected from the POTCAR setting.";
static const char *TOOLTIP_POTCAR_DETECTED_FLAVOR =
    "List of the pseudopotential flavors (PATH method)\ndetected from the POTCAR setting.";

/* EXEC tab */
static const char *TOOLTIP_VASP_EXE = "Location (full path) of the vasp executable.";
static const char *TOOLTIP_JOB_PATH =
    "Location (full path) of the calculation directory.\nit is IMPORTANT to check this parameter so that\n* no other "
    "calculation is overwritten\n* calculation will not start in an unexpected directory.";
static const char *TOOLTIP_MPIRUN =
    "Location (full path) of mpirun executable.\nRequired for parallel calculation only.";
static const char *TOOLTIP_NPROC_EXEC =
    "Total number of CPU used for calculation.\nwill be used as N in \"mpirun -np N vasp\"\nto start vasp calculation.";
static const char *TOOLTIP_NCORE_EXEC =
    "NCORE: Ch. 6.57 DEFAULT: 1\nSets how many cores works on one orbital.\nNote that NCORE is limited by KPAR.";
static const char *TOOLTIP_KPAR_EXEC = "KPAR: Ch. 6.57 DEFAULT: 1\nSets the number of k-points treated in "
                                       "parallel.\nEach k-point is worked on by NCORE CPUs.";
static const char *TOOLTIP_LWAVE = "LWAVE: Ch. 6.52 DEFAULT: TRUE\nIf set the WAVECAR (wavefunctions) file is written.";
static const char *TOOLTIP_LCHARG =
    "LCHARG: Ch. 6.52 DEFAULT: TRUE\nIf set the CHGCAR (charge density) file is written.";
static const char *TOOLTIP_LVTOT =
    "LVTOT: Ch. 6.53 DEFAULT: FALSE\nIf set the LOCPOT (local potential) file is written.";
static const char *TOOLTIP_LVHAR =
    "LVHAR: Ch. 6.54 DEFAULT: FALSE\nIf set the full local potential is written to LOCPOT\n(ie. ionic+Hartree+XC) "
    "otherwise only electrostatic\n(ie. ionic+Hartree) is written.";
static const char *TOOLTIP_LELF =
    "LELF: Ch 6.55 DEFAULT: FALSE\nIf set the ELFCAR (electron localization function)\nfile is written.";
static const char *TOOLTIP_LPLANE =
    "LPLANE: Ch. 6.57 DEFAULT: TRUE\nIf set the real-space data distribution is done plane wise.";
static const char *TOOLTIP_LSCALU = "LSCALU: Ch. 6.59 DEFAULT: FALSE\nIf set parallel LU decomposition will be used.";
static const char *TOOLTIP_LSCALAPACK = "LSCALAPACK: Ch. 6.59 DEFAULT: FALSE\nIf set scaLAPACK routines will be used.";

#endif /* VASP_TOOLTIPS_H */
