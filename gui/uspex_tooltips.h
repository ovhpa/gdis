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

#ifndef USPEX_TOOLTIPS_H
#define USPEX_TOOLTIPS_H

/*
 * USPEX Setup Dialog Tooltips
 *
 * Auto-generated from gui/uspexdialog.cpp setWidgetTooltip calls.
 * These tooltips are preserved EXACTLY from the original implementation
 * and contain precise USPEX parameter instructions as published in the
 * USPEX manual chapters. They are kept in a separate header for
 * potential internationalization support.
 */

/* SYSTEM tab — General */
static const char *TOOLTIP_NAME = "The model name will be used in USPEX files\nas well as GDIS display.";
static const char *TOOLTIP_FILE_ENTRY =
    "Use the previous result of a USPEX calculation\nto fill the parameters of the ";
static const char *TOOLTIP_CALCULATIONMETHOD =
    "calculationMethod: Ch. 4.1 DEFAULT: USPEX\nSet the method of calculation.";
static const char *TOOLTIP_CALCTYPE_DIM =
    "Set the dimension of the system.\n3, 2, 1, and 0 are equilvalent to 3D, 2D, ";
static const char *TOOLTIP_EXTERNALPRESSURE =
    "ExternalPressure: Ch. 4.1, 5.7 DEFAULT: none\nExternal pressure (GPa) for   // calculation.";
static const char *TOOLTIP_VER_104 = "Use USPEX v. 10.4 instead of 9.4.4.";
static const char *TOOLTIP_CALCULATIONTYPE =
    "calculationType: Ch. 4.1 DEFAULT: 300\nSets the dimensionality, molecularity, and variability\nof the ";
static const char *TOOLTIP_MAG = "Set a magnetic calculation.";
static const char *TOOLTIP_MOL = "Set the molecularity of the system.";
static const char *TOOLTIP_VAR = "Set the variability of chemical composition.";
/* SYSTEM tab — AtomType */
static const char *TOOLTIP_ATOMTYPE =
    "atomType: Ch. 4.1 DEFAULT: none\nThis list regroups several tags:\natomType, numSpecies, and valences.";
static const char *TOOLTIP_ATOM_SYM = "atomSym: - DEFAULT: none\nAtomic symbol of current species.";
static const char *TOOLTIP_ATOM_TYP = "atomTyp: - DEFAULT: none\nAtomic number of current species.";
static const char *TOOLTIP_APPLY_ATOM = ""; /* NO TOOLTIP */
static const char *TOOLTIP_ATOM_NUM = "atomNum: - DEFAULT: none\nNumber of atoms in current species.";
static const char *TOOLTIP_ATOM_VAL =
    "atomVal: - DEFAULT: auto\nValence of the current species.\nAutomatically determined if zero.";
static const char *TOOLTIP_REMOVE_ATOM = ""; /* NO TOOLTIP */
static const char *TOOLTIP_NUMSPECIES = "numSpecies: Ch. 4.1 DEFAULT: none\nSpecifies the number of atoms of each ";
static const char *TOOLTIP_BLOCKSPECIES = "The number of atoms of each types for this block.";
/* SYSTEM tab — Bonds & U */
static const char *TOOLTIP_GOODBONDS =
    "goodBonds: Ch. 4.1 DEFAULT: auto\nSet the minimum distance at which a bond is considered.";
static const char *TOOLTIP_BOND_D = "Minimum bond distance between selected and others species.";
static const char *TOOLTIP_LDAU = "ldaU: Ch. 4.1 DEFAULT: all 0\nHubbard U value (per atom) in L(S)DA+U method.";
static const char *TOOLTIP_AUTO_BONDS = "Automatically determine bonds (recommended).";
/* SYSTEM tab — Optimization & Constraints */
static const char *TOOLTIP_OPTTYPE = "optType: Ch. 4.1 DEFAULT: 1(Enthalpy)\nSelect the properties to optimize.";
static const char *TOOLTIP_NEW_OPTTYPE =
    "optType: Ch. 4.1 DEFAULT: MIN_enthalpy\nSelect the properties to optimize in the new\nVER 10.1 USPEX format.";
static const char *TOOLTIP_SEL_NEW_OPT =
    "Use the NEW block optType format.\nMandatory for multiobjective optimization.";
static const char *TOOLTIP_ANTI_OPT =
    "anti-opt: - DEFAULT: FALSE\nIf set REVERSE the direction of optimization.\ni.e. MIN -> MAX & MAX -> MIN";
static const char *TOOLTIP_CHECKMOLECULES =
    "checkMolecules: Ch. 4.1 DEFAULT: TRUE\nCheck and discard broken/merged molecules.";
static const char *TOOLTIP_CHECKCONNECTIVITY =
    "checkConnectivity: Ch. 4.1 DEFAULT: FALSE\nCalculate hardness and add connectivity in softmutation.";
static const char *TOOLTIP_LATTICEVALUES =
    "Latticevalues: Ch. 4.6 DEFAULT: auto\nInitial volume of the unit cell, or known lattice parameters.";
static const char *TOOLTIP_LATTICEFORMAT =
    "FORMAT: whether Lattice values correspond to a series of\nVolumes, lattive ";
static const char *TOOLTIP_SPLITINTO =
    "splitInto: Ch. 4.4 DEFAULT: 1\nNumber of identical subcells or pseudosubcells in the unitcell.";
static const char *TOOLTIP_AUTO_C_LAT = "AUTO_LAT: use automatic value for Latticevalues.";
/* SYSTEM tab — Ion Distances & Molecules */
static const char *TOOLTIP_IONDISTANCES =
    "IonDistance: Ch. 4.5 DEFAULT: auto\nTriangular matrix of the minimum allowed\ninteratomic distances.";
static const char *TOOLTIP_DISTANCES = "distances: minimum allowed distance from the current atom to others.";
static const char *TOOLTIP_MINVECTORLENGTH =
    "minVectorLength: Ch. 4.5, 5.7 DEFAULT: auto\nMinimum length of a new lattice parameter.";
static const char *TOOLTIP_CE = "constraint_enhancement: minimum allowed distance from the current atom to others.";
static const char *TOOLTIP_AUTO_C_ION = "AUTO_ION: use automatic values for ion constraints.";
static const char *TOOLTIP_MOLCENTERS = "MolCenters: Ch. 4.5 DEFAULT: none\nTriangular matrix of the minimum ";
static const char *TOOLTIP_CENTERS = "centers: minimum allowed distance from the current molecule center to others.";
/* STRUCTURE tab — Population & Selection */
static const char *TOOLTIP_POPULATIONSIZE =
    "populationSize: Ch. 4.2 DEFAULT: auto\nNumber of structures in each generation.";
static const char *TOOLTIP_INITIALPOPSIZE =
    "initialPopSize: Ch. 4.2 DEFAULT: populationSize\nNumber of structures in initial generation.";
static const char *TOOLTIP_NUMGENERATIONS = "numGenerations: Ch. 4.2 DEFAULT: 100\nMaximum number of generations.";
static const char *TOOLTIP_STOPCRIT = "stopCrit: Ch. 4.2 DEFAULT: auto\nMaximum number of generations.";
/* STRUCTURE tab — Magnetic Ratios */
static const char *TOOLTIP_MAG_NM =
    "magRatio: Ch. 4.1 DEFAULT: 0.1\n(magnetic calculation) Initial ratio of structures\nwith a non-magnetic order.";
static const char *TOOLTIP_MAG_FMLS = "magRatio: Ch. 4.1 DEFAULT: 0.225\n(magnetic calculation) Initial ratio of ";
static const char *TOOLTIP_MAG_AFML = "magRatio: Ch. 4.1 DEFAULT: 0.225\n(magnetic calculation) Initial ratio of ";
static const char *TOOLTIP_MAG_FMLH = "magRatio: Ch. 4.1 DEFAULT: 0.0\n(magnetic calculation) Initial ratio of ";
static const char *TOOLTIP_MAG_FMHS = "magRatio: Ch. 4.1 DEFAULT: 0.225\n(magnetic calculation) Initial ratio of ";
static const char *TOOLTIP_MAG_AFMH = "magRatio: Ch. 4.1 DEFAULT: 0.225\n(magnetic calculation) Initial ratio of ";
static const char *TOOLTIP_MAG_AFLH = "magRatio: Ch. 4.1 DEFAULT: 0.0\n(magnetic calculation) Initial ratio of ";
/* STRUCTURE tab — Best & Fitness */
static const char *TOOLTIP_BESTFRAC =
    "bestFrac: Ch. 4.3 DEFAULT: 0.7\nFraction of current generation used to generate the next.";
static const char *TOOLTIP_KEEPBESTHM =
    "keepBestHM: Ch. 4.3 DEFAULT: auto\nNumber of best structures that will survive in next generation.";
static const char *TOOLTIP_REOPTOLD =
    "reoptOld: Ch. 4.3 DEFAULT: FALSE\nIf set surviving structures will be re-optimized.";
static const char *TOOLTIP_FITLIMIT =
    "fitLimit: Ch. 4.3 DEFAULT: none\nStop calculation when fitLimit fitness is reached.";
/* STRUCTURE tab — Variation Operators */
static const char *TOOLTIP_SYMMETRIES =
    "symmetries: Ch. 4.4 DEFAULT: auto\nPossible space groups for crystals,\nplane ";
static const char *TOOLTIP_FRACGENE = "fracGene: Ch. 4.4 DEFAULT: 0.5\nRatio of structures obtained by heredity.";
static const char *TOOLTIP_FRACRAND = "fracRand: Ch. 4.4 DEFAULT: 0.2\nRatio of structures obtained randomly.";
static const char *TOOLTIP_FRACTOPRAND =
    "fracTopRand: Ch. 4.4 DEFAULT: 0.2\nRatio of structures obtained by topological random generator.";
static const char *TOOLTIP_FRACPERM = "fracPerm: Ch. 4.4 DEFAULT: auto\nRatio of structures obtained by permutation.";
static const char *TOOLTIP_FRACATOMSMUT =
    "fracAtomsMut: Ch. 4.4 DEFAULT: 0.1\nRatio of structures obtained by softmutation.";
static const char *TOOLTIP_FRACROTMUT =
    "fracRotMut: Ch. 4.4 DEFAULT: auto\nRatio of structures obtained by mutation of molecular orientation.";
static const char *TOOLTIP_FRACLATMUT =
    "fracLatMut: Ch. 4.4 DEFAULT: auto\nRatio of structures obtained by lattice mutation.";
static const char *TOOLTIP_FRACSPINMUT =
    "fracSpinMut: Ch. 4.4 DEFAULT: 0.1\nRatio of structures obtained by spin mutation.";
static const char *TOOLTIP_HOWMANYSWAPS = "howManySwaps: Ch. 4.4 DEFAULT: auto\nNumber of pairwise swaps for "
                                          "permutation\ndistributed uniformly between [1,howManySwaps].";
static const char *TOOLTIP_SPECIFICSWAPS =
    "specificSwaps: Ch. 4.4 DEFAULT: blank\nWhich atoms are allow to swap during permutation.";
static const char *TOOLTIP_SPECIFICTRANS = "specificTrans: Ch. 5.1 DEFAULT: blank\nList of allowed transmutation.";
/* STRUCTURE tab — Mutation */
static const char *TOOLTIP_MUTATIONDEGREE =
    "mutationDegree: Ch. 4.13 DEFAULT: auto\nMaximum displacement in softmutation (Ang).";
static const char *TOOLTIP_MUTATIONRATE =
    "mutationRate: Ch. 4.13 DEFAULT: 0.5\nStd. dev. of epsilon in strain matrix for lattice mutation.";
static const char *TOOLTIP_DISPLACEINLATMUTATION =
    "DisplaceInLatmutation: Ch. ?.? DEFAULT: 1.0\nSets softmutation as part of lattice mutation\nand gives ";
static const char *TOOLTIP_AUTOFRAC =
    "AutoFrac: Ch. 4.4 DEFAULT: FALSE\nIf set variation parameters will be optimized during run.";
/* STRUCTURE tab — Fingerprint */
static const char *TOOLTIP_RMAXFING = "RmaxFing: Ch. 4.9 DEFAULT: 10.0\nFingerprint cutoff distance (Ang.).";
static const char *TOOLTIP_DELTAFING = "deltaFing: Ch. 4.9 DEFAULT: 0.08\nDiscretization of the fingerprint function.";
static const char *TOOLTIP_SIGMAFING = "sigmaFing: Ch. 4.9 DEFAULT: 0.03\nGaussian broadening of interatomic distance.";
static const char *TOOLTIP_TOLERANCEFING =
    "toleranceFing: Ch. 4.9 DEFAULT: 0.008\nMinimal cosine distance for\n2 different structures.";
/* STRUCTURE tab — AntiSeeds */
static const char *TOOLTIP_ANTISEEDSACTIVATION =
    "antiSeedsActivation: Ch. 4.10 DEFAULT: 5000\nGeneration at which antiseed is active.";
static const char *TOOLTIP_ANTISEEDSMAX =
    "antiSeedsMax: Ch. 4.10 DEFAULT: 0.000\nGaussian height in mean square deviation of the generation ";
static const char *TOOLTIP_ANTISEEDSSIGMA =
    "antiSeedsSigma: Ch. 4.10 DEFAULT: 0.001\nGaussian width in average distances between generated ";
/* STRUCTURE tab — Space Group */
static const char *TOOLTIP_DOSPACEGROUP = "doSpaceGroup: Ch. 4.11 DEFAULT: TRUE\nActivate space group determination.";
static const char *TOOLTIP_SYMTOLERANCE = "SymTolerance: Ch. 4.11 DEFAULT: 0.10\nPrecision for symmetry determination.";
/* ADVANCED tab — Composition & Transmutation */
static const char *TOOLTIP_FIRSTGENEMAX =
    "firstGeneMax: Ch. 5.1 DEFAULT: 11\nNumber of composition sampled in 1st generation.";
static const char *TOOLTIP_MINAT =
    "minAt: Ch. 5.1 DEFAULT: none\nMinimum number of atoms (molecules) in unitcell\nfor the first generation.";
static const char *TOOLTIP_MAXAT =
    "maxAt: Ch. 5.1 DEFAULT: none\nMaximum number of atoms (molecules) in unitcell\nfor the first generation.";
static const char *TOOLTIP_FRACTRANS =
    "fracTrans: Ch. 5.1 DEFAULT: 0.1\nFraction of structures obtained by transmutation.";
static const char *TOOLTIP_HOWMANYTRANS =
    "howManyTrans: Ch. 5.1 DEFAULT: 0.2\nMaximum ratio of transmutated atoms in a structure.";
/* ADVANCED tab — Specific Folder */
static const char *TOOLTIP_USE_SPECIFIC = "Use a previously defined Specific folder.";
static const char *TOOLTIP_SET_SPECIFIC = "Set parameter to create a Specific folder.";
static const char *TOOLTIP_SPE_FOLDER = "Specific folder.";
/* ADVANCED tab — AI / Optimization Steps */
static const char *TOOLTIP_NUM_OPT_STEPS =
    "Set the number of optimisation steps.\nIncluding fixed-cells and full relaxation steps.";
static const char *TOOLTIP_CURR_STEP = "Select current optimisation step number.";
static const char *TOOLTIP_AUTO_STEP = "Let GDIS generate some \"Simplified\"\nINPUTS for this step, given\nthe "
                                       "calculation code.\nWARNING: GDIS use some general settings that may fail.";
static const char *TOOLTIP_ISFIXED =
    "Set the current step as a fixed relaxation.\nUseful only with a mix between fixed and full relaxations.";
static const char *TOOLTIP_ABINITIOCODE =
    "abinitioCode: Ch. 4.8 DEFAULT: 1\nCode used for the calculations in this step.";
static const char *TOOLTIP_KRESOLSTART =
    "KresolStart: Ch. 4.8 DEFAULT: 0.2-0.08\nReciprocal-space resolution for this optimization step (2*pi/Ang).";
static const char *TOOLTIP_VACUUMSIZE =
    "vacuumSize: Ch. 4.8 DEFAULT: 10.0\nVacuum size (Ang.) between neighbor atoms from ";
static const char *TOOLTIP_AI_INPUT =
    "Input for the current calculation step.\nFile name as to end with *_N where N is the step number.";
static const char *TOOLTIP_AI_OPT =
    "Complementary (optional) file for the current calculation step.\nWhen present, file ";
static const char *TOOLTIP_AI_LIB = "Select flavor(s) that apply to current potential.\nIt is either a per-species ";
static const char *TOOLTIP_AI_LIB_FLAVOR =
    "Select flavor(s) that apply to current potential.\nIt is either a per-species ";
static const char *TOOLTIP_AI_LIB_SEL = "The flavor of selected libraries.";
static const char *TOOLTIP_COMMANDEXECUTABLE =
    "commandExecutable: Ch. 4.8 DEFAULT: none\nCurrent optimisation step executable of submission script.";
/* ADVANCED tab — USPEX Launch */
static const char *TOOLTIP_JOB_USPEX_EXE = "The USPEX script executable.";
static const char *TOOLTIP_WHICHCLUSTER =
    "whichCluster: Ch. 4.8 DEFAULT: 0\nType of job submission, including:\n0 - no job-script;\n1 - local ";
static const char *TOOLTIP_PHASEDIAGRAM =
    "PhaseDiagram: Ch. 4.8 DEFAULT: FALSE\nSet calculation of an estimated phase ";
static const char *TOOLTIP_NUMPROCESSORS = "numProcessors: Ch. ? DEFAULT: 1\nNumber of processors used in each steps.";
static const char *TOOLTIP_NUMPARALLELCALCS =
    "numParallelCalcs: Ch. 4.8 DEFAULT: 1\nNumber of structure relaxations in parallel.";
static const char *TOOLTIP_SEL_OCTAVE = "Use octave instead of matlab.";
static const char *TOOLTIP_JOB_PATH = "Select USPEX calculation folder.\nThis folder is where run_uspex will be ";
static const char *TOOLTIP_REMOTEFOLDER =
    "remoteFolder: Ch. 4.8 DEFAULT: none\nWhen using remote submission, this hold ";
/* ADVANCED tab — Restart & Statistics */
static const char *TOOLTIP_PICKUPYN =
    "pickUpYN: deprecated DEFAULT: 0\nSet to restart a calculation, deprecated on version <= 10.";
static const char *TOOLTIP_PICKUPGEN = "pickUpGen: Ch. 4.7 DEFAULT: 0\nSelect the generation at which USPEX should ";
static const char *TOOLTIP_PICKUPFOLDER = "pickUpFolder: Ch. 4.7 DEFAULT: 0\nSelect the folder number from which to ";
static const char *TOOLTIP_RESTART_CLEANUP =
    "Cleanup the calculation directory before restart,\nie. remove still_running, NOT_YET, etc.";
static const char *TOOLTIP_REPEATFORSTATISTICS =
    "repeatForStatistics: Ch. 4.12 DEFAULT: 1\nNumber of USPEX repeated job ";
static const char *TOOLTIP_STOPFITNESS = "stopFitness: Ch. 4.12 DEFAULT: none\nThe fitness value at which USPEX ";
static const char *TOOLTIP_FIXRNDSEED =
    "fixRndSeed: Ch. 4.12 DEFAULT: 0\nFor non-zero values, fix the random seed\nto check that for two USPEX ";
static const char *TOOLTIP_COLLECTFORCES =
    "collectForces: Ch. 4.12 DEFAULT: FALSE\nCollect relaxation information from ";
static const char *TOOLTIP_ORDERING_ACTIVE =
    "ordering_active: Ch. 4.12 DEFAULT: TRUE\nSwitch the \"biasing of variation ";
static const char *TOOLTIP_SYMMETRIZE = "symmetrize: Ch. 4.13 DEFAULT: FALSE\nTransform all structure to \"standard ";
static const char *TOOLTIP_VALENCEELECTR =
    "valenceElectr: Ch. 4.13 DEFAULT: AUTO\nNumber of valence electrons.\nOverrides the tabulated values.";
static const char *TOOLTIP_PERCSLICESHIFT = "percSliceShift: Ch. 4.13 DEFAULT: 1.0\nProbability of shifting slabs.";
static const char *TOOLTIP_MINSICE = "minSlice: Ch. 4.13 DEFAULT: N/A\nMinimum Slice thickness (Ang.).";
static const char *TOOLTIP_DYNAMICALBESTHM =
    "dynamicalBestHM: Ch. ?.? DEFAULT: 2\nSpecify if and how the number of surviving structure will vary.\nThis "
    "keyword has disapear in the 10.1 version of USPEX.";
static const char *TOOLTIP_MAXSLICE =
    "maxSlice: Ch. 4.13 DEFAULT: N/A\nMaximum Slice thickness (Ang.).\nRecommended value is ~6 Ang.";
static const char *TOOLTIP_MAXDISTHEREDITY =
    "maxDistHeredity: Ch. 4.13 DEFAULT: 0.5\nMax fingerprint distance between structures chosen for heredity.";
static const char *TOOLTIP_SOFTMUTONLY =
    "softMutOnly: Ch. ?.? DEFAULT: 0\nWhich/how many generations are produced from ";
static const char *TOOLTIP_NUMBERPARENTS =
    "numberparents: Ch. 4.13 DEFAULT: 2\nNumber of parents chosen for heredity\nfor cluster calculation.";
static const char *TOOLTIP_MANYPARENTS =
    "manyParents: Ch. 4.13 DEFAULT: 0\nDetermine if and how more slices\nand parents structures are ";
/* SPECIFIC tab — META (Metadynamics) */
static const char *TOOLTIP_FULLRELAX =
    "FullRelax: Ch. 5.7 DEFAULT: 2\nPerform full relaxation of which structures for ";
static const char *TOOLTIP_MAXVECTORLENGTH =
    "maxVectorLength: Ch. 5.7 DEFAULT: none\nAdd a correction force to keep cell length below this setting.";
static const char *TOOLTIP_GAUSSIANWIDTH =
    "GaussianWidth: Ch. 5.7 DEFAULT: AUTO\nWidth of Gaussian added to PES to accelerate phase ";
static const char *TOOLTIP_GAUSSIANHEIGHT =
    "GaussianHeight: Ch. 5.7 DEFAULT: AUTO\nHeight of Gaussian added to PES to accelerate phase "
    "transition.\nRecommended values is L.dh^2.G with L = average cell length,\ndh = GaussW, and G = shear modulus.";
static const char *TOOLTIP_META_MODEL =
    "Select the model from which metadynamics is started.\nA good structure, relaxed at ExtP is necessary.";
/* SPECIFIC tab — PSO */
static const char *TOOLTIP_PSO_SOFTMUT = "PSO_softMut: Ch. 5.8 DEFAULT: 1\nSoft mutation weight.";
static const char *TOOLTIP_PSO_BESTSTRUC =
    "PSO_BestStruc: Ch. 5.8 DEFAULT: 1\nWeight of heredity with best position of a given PSO particle.";
static const char *TOOLTIP_PSO_BESTEVER =
    "PSO_BestEver: Ch. 5.8 DEFAULT: 1\nWeight of heredity with globally best PSO particle.";
/* SPECIFIC tab — VCNEB */
static const char *TOOLTIP_VCNEBTYPE_METHOD = "Choose between VC-NEB method and simple structure relaxation.";
static const char *TOOLTIP_VCNEBTYPE = "vcnebType: Ch. 6.2 DEFAULT: 110\nType of VC-NEB calculation.";
static const char *TOOLTIP_VCNEBTYPE_IMG_NUM = "Set whether number of images should be kept fixed.";
static const char *TOOLTIP_VCNEBTYPE_SPRING = "Set whether spring constants should be kept fixed.";
static const char *TOOLTIP_OPTREADIMAGES = "optReadImages: Ch. 6.2 DEFAULT: 2\nSet the method for reading Images file.";
static const char *TOOLTIP_OPTIMIZERTYPE =
    "optimizerType: Ch. 6.2 DEFAULT: 1\nSelect the optimization algorithm (SD or FIRE).";
static const char *TOOLTIP_NUMIMAGES = "numImages: Ch. 6.2 DEFAULT: 9\nInitial number of images.";
static const char *TOOLTIP_NUMSTEPS =
    "numSteps: Ch. 6.2 DEFAULT: 600\nMaximum VC-NEB step iterations.\nA value of at least 500 is recommended.";
static const char *TOOLTIP_OPTFREEZING =
    "optFreezing: Ch. 6.2 DEFAULT: FALSE\nActivate freezing of Image structure when ConvThreshold is reached.";
static const char *TOOLTIP_DT = "dt: Ch. 6.2 DEFAULT: 0.05\nTime step for structure relaxation.";
static const char *TOOLTIP_CONVTHRESHOLD =
    "ConvThreshold: Ch. 6.2 DEFAULT: 0.003\nHalting condition (ev/Ang.) for RMS forces on images.";
static const char *TOOLTIP_VARPATHLENGTH =
    "VarPathLength: Ch. 6.2 DEFAULT: AUTO\nCriterion to determine image creation/deletion for variable image ";
static const char *TOOLTIP_OPTRELAXTYPE = "optRelaxType: Ch. 6.2 DEFAULT: 3\nStructure relaxation mode.";
static const char *TOOLTIP_K_MIN = "K_min: Ch. 6.2 DEFAULT: 5\nMinimum spring constant (eV/Ang.^2).";
static const char *TOOLTIP_K_MAX = "K_max: Ch. 6.2 DEFAULT: 5\nMaximum spring constant (eV/Ang.^2).";
static const char *TOOLTIP_KCONSTANT = "Kconstant: Ch. 6.2 DEFAULT: 5\nFixed spring constant (eV/Ang.^2).";
/* SPECIFIC tab — CI/DI */
static const char *TOOLTIP_OPTMETHODCIDI =
    "optMethodCIDI: Ch. 6.2 DEFAULT: 0\nOption for Climbing-Image (CI) and Descending-Image (DI).";
static const char *TOOLTIP_STARTCIDISTEP = "startCIDIStep: Ch. 6.2 DEFAULT: 100\nStarting step for CI/DI method.";
static const char *TOOLTIP_PICKUPIMAGES =
    "pickupImages: Ch. 6.2 DEFAULT: AUTO\nNumber/which images to be picked up for CI/DI method.";
static const char *TOOLTIP_PRINTSTEP = "PrintStep: Ch. 6.2 DEFAULT: 1\nSave restart file every PrintStep times.";
static const char *TOOLTIP_FORMATTYPE =
    "FormatType: Ch. 6.2 DEFAULT: 2\nFormat of structures in PATH output directory.";
static const char *TOOLTIP_IMG_MODEL = "Select the original Images model.";
/* SPECIFIC tab — Surfaces */
static const char *TOOLTIP_SUBSTRATE_MODEL =
    "Select the model to use as a substrate\nie. without buffer, surface, and vacuum region.";
static const char *TOOLTIP_RECONSTRUCT = "reconstruct: Ch. 5.4 DEFAULT: 1\nNumber of replication of the surface cell.";
static const char *TOOLTIP_THICKNESSS = "thicknessS: Ch. 5.4 DEFAULT: 2.0\nThickness (Ang.) of the surface region.";
static const char *TOOLTIP_THICKNESSB = "thicknessB: Ch. 5.4 DEFAULT: 3.0\nThickness (Ang.) of the buffer region.";
/* SPECIFIC tab — Molecules */
static const char *TOOLTIP_MOL_MODEL = "Select the model(s) for molecular calculation.";
static const char *TOOLTIP_NUM_MOL = "Select the number of molecules.";
static const char *TOOLTIP_MOL_GDIS = "Select the molecule from GDIS model.";
static const char *TOOLTIP_CURR_MOL = "Current molecule number.";
static const char *TOOLTIP_MOL_GULP = "Use GULP chemical labels and charge Zmatrix form.";
/* SPECIFIC tab — BoltzTraP */
static const char *TOOLTIP_TE_GOAL = "TE_goal: Ch. 5.2 DEFAULT: ZT\nSet the component of ZT to be optimized.";
static const char *TOOLTIP_BOLTZTRAP_T_MAX =
    "BoltzTraP_T_max: Ch. 5.2 DEFAULT: 800.0\nMaximum BoltzTraP calculation temperature.";
static const char *TOOLTIP_BOLTZTRAP_T_DELTA =
    "BoltzTraP_T_delta: Ch. 5.2 DEFAULT: 50.0\nBoltzTraP calculation temperature increment.";
static const char *TOOLTIP_BOLTZTRAP_T_EFCUT =
    "BoltzTraP_T_efcut: Ch. 5.2 DEFAULT: 0.15\nBoltzTraP calculation chemical potential (eV) interval.";
static const char *TOOLTIP_CMD_BOLTZTRAP = "BoltzTraP software command/script (optional).";
static const char *TOOLTIP_TE_T_INTEREST =
    "TE_T_interest: Ch. 5.2 DEFAULT: 300.0Y\nTarget temperature to optimize thermoelectric efficiency.";
static const char *TOOLTIP_TE_THRESHOLD =
    "TE_threshold: Ch. 5.2 DEFAULT: 0.5\nStructures with a ZT figure of merit\nbelow this threshold will be discarded.";
/* SPECIFIC tab — TPS */
static const char *TOOLTIP_NUMITERATIONS = "numIterations: Ch. 6.3 DEFAULT: 1000\nMaximum number of TPS iterations.";
static const char *TOOLTIP_SHIFT_RATIO =
    "shiftRatio: Ch. 6.3 DEFAULT: 0.1\nFraction of shooter-after-shifter operations.";
static const char *TOOLTIP_AMPLITUDESHOOT_AB =
    "amplitudeShoot: Ch. 6.3 DEFAULT: 0.1\nMomentum amplitude for A->B shooting.";
static const char *TOOLTIP_AMPLITUDESHOOT_BA =
    "amplitudeShoot: Ch. 6.3 DEFAULT: 0.1\nMomentum amplitude for B->A shooting.";
static const char *TOOLTIP_MAGNITUDESHOOT_SUCCESS =
    "magnitudeShoot: Ch. 6.3 DEFAULT: 1.05\nAmplitude increasing factor on MD trajectory success.";
static const char *TOOLTIP_MAGNITUDESHOOT_FAILURE =
    "magnitudeShoot: Ch. 6.3 DEFAULT: 1.05\nAmplitude decreasing factor on MD trajectory failure.";
static const char *TOOLTIP_SPECIESSYMBOL =
    "speciesSymbol: Ch. 6.3 DEFAULT: none\nIdentity of all chemical species (atoms/molecules).";
static const char *TOOLTIP_MASS = "mass: Ch. 6.3 DEFAULT: Auto\nMass of each corresponding species.";
static const char *TOOLTIP_ORDERPARATYPE =
    "orderParaType: Ch. 6.3 DEFAULT: none\nSelect if the method of order parameter calculation is:\nthe fingerprint "
    "method (TRUE)\na user-defined method (FALSE).\nContrary to USPEX lack of default, TRUE is pre-selected.";
static const char *TOOLTIP_CMDORDERPARAMETER =
    "cmdOrderParameter: Ch. 6.3 DEFAULT: none\nUser-defined command for order parameter calculation.";
static const char *TOOLTIP_OP_CRITERIA_START =
    "opCriteria: Ch. 6.3 DEFAULT: none\nAllowable degree of similarity between starting states.";
static const char *TOOLTIP_CMDENTHALPYTEMPERATURE = "cmdEnthalpyTemperature: Ch. 6.3 DEFAULT: none\nUser-defined "
                                                    "command for enthalpy/temperature\nextraction from the MD results.";
static const char *TOOLTIP_OP_CRITERIA_END =
    "opCriteria: Ch. 6.3 DEFAULT: none\nAllowable degree of similarity between ending states.";
static const char *TOOLTIP_ORDERPARAMETERFILE =
    "orderParameterFile: Ch. 6.3 DEFAULT: fp.dat\nOrder parameter history file.";
static const char *TOOLTIP_ENTHALPYTEMPERATUREFILE =
    "enthalpyTemperatureFile: Ch. 6.3 DEFAULT: HT.dat\nEnthalpy and temperature history file.";
static const char *TOOLTIP_TRAJECTORYFILE = "trajectoryFile: Ch. 6.3 DEFAULT: traj.dat\nMD trajectory file.";
static const char *TOOLTIP_MDRSTARTFILE = "MDrestartFile: Ch. 6.3 DEFAULT: traj.restart\nMD restart file.";
/* SPECIFIC tab — Stoichiometry */
static const char *TOOLTIP_STOICHIOMETRYSTART =
    "StoichiometryStart: Ch. 5.4 DEFAULT: ?\nDefine the initial stoichiometry of the BULK.";
static const char *TOOLTIP_E_AB = "E_AB: Ch. 5.4 DEFAULT: ?\nDFT energy (eV/formula) of AmBn.";
static const char *TOOLTIP_MU_A = "Mu_A: Ch. 5.4 DEFAULT: ?\nDFT energy (eV/atom) of elemental A.";
static const char *TOOLTIP_MU_B = "Mu_B: Ch. 5.4 DEFAULT: ?\nDFT energy (eV/atom) of elemental B.";

#endif /* USPEX_TOOLTIPS_H */
