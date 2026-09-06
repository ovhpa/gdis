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
 * VASP Configuration Dialog for GDIS Qt6 GUI
 * VASP setup dialog with 8 tabs:
 * PRESET, CONVERGENCE, ELECT-I, ELECT-II, IONIC, KPOINTS, POTCAR, EXEC
 */

#include <glib.h>
#include "coords.h"

#include "vaspdialog.h"
#include "gdis_api.h"
#include "vasp_tooltips.h"
#include "svg_utils.h"

#include "gdis.h"
#include "file.h"
#include "parse.h"
#include "task.h"
#include "file_vasp.h"
#include "track.h"
#include "model.h"
#include "interface.h"
#include "gui_vasp_internal.h"
extern "C" struct vasp_calc_gui vasp_gui;

/* Declare the sync functions from gui_vasp.c */
extern "C" void vasp_gui_sync();
extern "C" void vasp_poscar_sync();
extern "C" void vasp_apply_simple();

/* Declare init functions from gui_vasp.c */
extern "C" void gui_vasp_init();

/* Declare save and exec from gui_vasp.c */
extern "C" gint save_vasp_calc();
extern "C" void exec_calc();

#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>

extern struct sysenv_pak sysenv;
extern struct elem_pak elements[];

/* Helper: extract unique element symbols from active model */
static void update_species_from_model()
{
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (!model || !model->cores)
    return;

  /* Free old */
  g_free(vasp_gui.calc.species_symbols);
  g_free(vasp_gui.calc.potcar_species);
  g_free(vasp_gui.calc.potcar_species_flavor);

  /* Collect unique element symbols from core->atom_code (atomic number) */
  GHashTable *seen = g_hash_table_new(g_int_hash, g_int_equal);
  GSList *list = model->cores;
  GString *species = g_string_new("");
  while (list)
  {
    struct core_pak *core = (struct core_pak *) list->data;
    gint code = core->atom_code;
    if (code <= 0)
    {
      list = g_slist_next(list);
      continue;
    }
    if (!g_hash_table_contains(seen, &code))
    {
      /* Look up symbol from elements array */
      for (gint i = 0; i < 120; i++)
      {
        if (elements[i].number == code)
        {
          if (species->len > 0)
            g_string_append(species, " ");
          g_string_append(species, elements[i].symbol);
          g_hash_table_insert(seen, &code, GINT_TO_POINTER(1));
          break;
        }
      }
    }
    list = g_slist_next(list);
  }

  vasp_gui.calc.species_symbols = g_strdup(species->str);
  /* potcar_species is only set when POTCAR file is loaded, not from model */
  g_string_free(species, TRUE);
  g_hash_table_unref(seen);
}

/**
 * Populate the POSCAR atom combo and species data from model core data.
 * Called from qt_show_vasp_dialog() before the dialog is shown.
 */
void VaspDialog::populatePoscarFromModel()
{
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (!model || !model->cores)
    return;

  /* Set model name from basename if not already set */
  if (!vasp_gui.calc.name && model->basename)
    vasp_gui.calc.name = g_strdup(model->basename);

  /* Update the name widget since refresh() already ran */
  if (vasp_gui.calc.name)
    m_nameEdit->setText(QString::fromUtf8(vasp_gui.calc.name));

  /* Count atoms per species */
  GHashTable *count = g_hash_table_new(g_int_hash, g_int_equal);
  GSList *list = model->cores;
  while (list)
  {
    struct core_pak *core = (struct core_pak *) list->data;
    gint code = core->atom_code;
    if (code <= 0)
    {
      list = g_slist_next(list);
      continue;
    }
    gint *c = (gint *) g_hash_table_lookup(count, &code);
    if (!c)
    {
      gint val = 1;
      g_hash_table_insert(count, &code, GINT_TO_POINTER(val));
    } else
    {
      g_hash_table_replace(count, &code, GINT_TO_POINTER(GPOINTER_TO_INT(c) + 1));
    }
    list = g_slist_next(list);
  }

  /* Build species list and numbers */
  GString *symStr = g_string_new("");
  GString *numStr = g_string_new("");
  GList *keys = g_hash_table_get_keys(count);
  GList *kp = keys;
  gboolean firstNum = TRUE;
  while (kp)
  {
    gint code = GPOINTER_TO_INT(kp->data);
    for (gint i = 0; i < 120; i++)
    {
      if (elements[i].number == code)
      {
        if (symStr->len > 0)
          g_string_append(symStr, " ");
        g_string_append(symStr, elements[i].symbol);
        break;
      }
    }
    gint *c = (gint *) g_hash_table_lookup(count, kp->data);
    if (!firstNum)
      g_string_append(numStr, " ");
    g_string_append_printf(numStr, "%d", GPOINTER_TO_INT(c));
    firstNum = FALSE;
    kp = g_list_next(kp);
  }

  g_free(vasp_gui.calc.species_symbols);
  vasp_gui.calc.species_symbols = g_strdup(symStr->str);
  g_free(vasp_gui.calc.species_numbers);
  vasp_gui.calc.species_numbers = g_strdup(numStr->str);

  /* Set lattice parameters from model (column-major: latmat[row + col*3]) */
  vasp_gui.calc.poscar_a0 = 1.0;
  vasp_gui.calc.poscar_ux = model->latmat[0];
  vasp_gui.calc.poscar_uy = model->latmat[3];
  vasp_gui.calc.poscar_uz = model->latmat[6];
  vasp_gui.calc.poscar_vx = model->latmat[1];
  vasp_gui.calc.poscar_vy = model->latmat[4];
  vasp_gui.calc.poscar_vz = model->latmat[7];
  vasp_gui.calc.poscar_wx = model->latmat[2];
  vasp_gui.calc.poscar_wy = model->latmat[5];
  vasp_gui.calc.poscar_wz = model->latmat[8];

  /* Sync direct/cartesian mode from model */
  vasp_gui.calc.poscar_direct = model->fractional;

  /* Populate Qt combo with atom entries */
  if (m_poscarAtomsCombo)
  {
    m_poscarAtomsCombo->clear();
    GSList *idxList = model->cores;
    gint atomIdx = 0;
    while (idxList)
    {
      struct core_pak *core = (struct core_pak *) idxList->data;
      gint code = core->atom_code;
      QString sym;
      for (gint i = 0; i < 120; i++)
      {
        if (elements[i].number == code)
        {
          sym = QString(elements[i].symbol);
          break;
        }
      }
      if (sym.isEmpty())
        sym = "X";

      /* Selective dynamics flags */
      QString sdStr;
      if (vasp_gui.calc.poscar_free == VPF_FREE)
      {
        sdStr = " T   T   T";
      } else if (vasp_gui.calc.poscar_free == VPF_FIXED)
      {
        sdStr = " F   F   F";
      } else if (vasp_gui.calc.selective_tx && atomIdx < vasp_gui.calc.selective_count)
      {
        sdStr = QString(" %1   %2   %3")
                    .arg(vasp_gui.calc.selective_tx[atomIdx] ? "T" : "F")
                    .arg(vasp_gui.calc.selective_ty[atomIdx] ? "T" : "F")
                    .arg(vasp_gui.calc.selective_tz[atomIdx] ? "T" : "F");
      } else
      {
        sdStr = " T   T   T";
      }

      /* Wrap fractional coords to (-0.5, 0.5) per VASP convention */
      gdouble wx = core->x[0], wy = core->x[1], wz = core->x[2];
      if (model->fractional && model->periodic > 0)
      {
        gdouble whole;
        wx = modf(wx, &whole);
        wy = modf(wy, &whole);
        wz = modf(wz, &whole);
        for (int d = 0; d < model->periodic; d++)
        {
          int j = (gint) (-2.0 * wx);
          wx += j;
          j = (gint) (-2.0 * wy);
          wy += j;
          j = (gint) (-2.0 * wz);
          wz += j;
        }
      }
      QString entry = QString("%1 %2 %3%4 ! atom: %5 (%6)")
                          .arg(wx, 0, 'f', 8)
                          .arg(wy, 0, 'f', 8)
                          .arg(wz, 0, 'f', 8)
                          .arg(sdStr)
                          .arg(atomIdx)
                          .arg(sym);
      m_poscarAtomsCombo->addItem(entry);
      atomIdx++;
      idxList = g_slist_next(idxList);
    }
    m_poscarAtomsCombo->addItem("ADD atom");
  }

  g_string_free(symStr, TRUE);
  g_string_free(numStr, TRUE);
  g_list_free(keys);
  g_hash_table_unref(count);
}

/* Extern the global vasp_gui from gui_vasp.c */
extern struct vasp_calc_gui vasp_gui;

VaspDialog::VaspDialog(QWidget *parent) : QDialog(parent)
{
  setWindowTitle(tr("VASP Setup"));
  resize(900, 650);

  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(8, 6, 8, 6);
  mainLayout->setSpacing(6);

  /* Top bar: MODEL NAME + CONNECTED XML (outside tabs) */
  auto *topLayout = new QFormLayout();
  m_nameEdit = new QLineEdit();
  topLayout->addRow(tr("MODEL NAME"), m_nameEdit);

  auto *xmlLayout = new QHBoxLayout();
  m_fileEntryEdit = new QLineEdit();
  m_fileEntryBtn = new QPushButton(tr("..."));
  xmlLayout->addWidget(m_fileEntryEdit);
  xmlLayout->addWidget(m_fileEntryBtn);
  topLayout->addRow(tr("CONNECTED XML"), xmlLayout);

  mainLayout->addLayout(topLayout);

  m_notebook = new QTabWidget(this);
  m_notebook->setTabPosition(QTabWidget::North);
  m_notebook->setDocumentMode(true);
  mainLayout->addWidget(m_notebook);

  setupPresetPage();
  setupConvergencePage();
  setupElectronicPage1();
  setupElectronicPage2();
  setupIonicPage();
  setupKpointsPage();
  setupPotcarPage();
  setupExecPage();

  /* Button bar */
  auto *btnLayout = new QHBoxLayout();
  btnLayout->addStretch();

  m_buttonSave = new QPushButton(tr("Save"), this);
  m_buttonExec = new QPushButton(tr("Execute"), this);
  m_buttonClose = new QPushButton(tr("Close"), this);
  btnLayout->addWidget(m_buttonSave);
  btnLayout->addWidget(m_buttonExec);
  btnLayout->addWidget(m_buttonClose);
  mainLayout->addLayout(btnLayout);

  connect(m_buttonSave, &QPushButton::clicked, this, &VaspDialog::on_save);
  connect(m_buttonExec, &QPushButton::clicked, this, &VaspDialog::on_run);
  connect(m_buttonClose, &QPushButton::clicked, this, &VaspDialog::on_close);
  connect(m_simpleApplyBtn, &QPushButton::clicked, this, [this]() {
    /* Sync Qt -> vasp_gui (simple interface fields only, not full vasp_gui_sync) */
    vasp_gui.simple_calcul_idx = m_simpleCalculCombo->currentIndex();
    vasp_gui.simple_system_idx = m_simpleSystemCombo->currentIndex();
    vasp_gui.simple_rgeom = m_simpleRgeomCheck->isChecked();
    vasp_gui.dimension = m_simpleDimSpin->value();
    vasp_gui.simple_kgrid_idx = m_simpleKgridCombo->currentIndex();
    vasp_gui.calc.job_nproc = m_simpleNpSpin->value();
    vasp_gui.calc.ncore = m_simpleNcoreSpin->value();
    vasp_gui.calc.kpar = m_simpleKparSpin->value();
    /* Also sync other Qt fields that vasp_apply_simple might need */
    g_free(vasp_gui.calc.name);
    vasp_gui.calc.name = g_strdup(m_nameEdit->text().toUtf8().constData());
    /* Apply simple preset — Qt version with message accumulation */
    gint calcul = vasp_gui.simple_calcul_idx;
    gint system = vasp_gui.simple_system_idx;
    gint kgrid = vasp_gui.simple_kgrid_idx;
    gint dim = (gint) vasp_gui.dimension;

    GString *msg_buf = g_string_new("Simple interface started.\n");

    auto append_msg = [&](const gchar *text) { g_string_append(msg_buf, text); };

    if (vasp_gui.simple_rgeom && calcul < 3)
    {
      append_msg("FAIL: ROUGHT GEOMETRY: OPTIMIZE GEOMETRY FIRST!\n");
    } else
    {
      /* KPOINTS */
      if (dim == 0)
      {
        vasp_gui.calc.kpoints_mode = VKP_GAMMA;
        vasp_gui.calc.kpoints_kx = 1.;
        vasp_gui.calc.kpoints_ky = 1.;
        vasp_gui.calc.kpoints_kz = 1.;
        append_msg("ATOM/MOLECULE in a box, setting only gamma point\n");
        vasp_gui.calc.kpoints_mode = VKP_GAMMA;
        vasp_gui.calc.kpoints_kx = 1.;
        vasp_gui.calc.kpoints_ky = 1.;
        vasp_gui.calc.kpoints_kz = 1.;
      } else
      {
        vasp_gui.calc.kpoints_mode = VKP_AUTO;
        vasp_gui.calc.kpoints_kx = (gdouble) (4.0 * dim * (kgrid + 1.0));
        append_msg(g_strdup_printf("SET AUTO gamma-centered grid w/ AUTO = %i\n", (gint) vasp_gui.calc.kpoints_kx));
      }

      /* Calculation type < 3: Energy, DOS/BANDS, Lattice Dynamics */
      if (calcul < 3)
      {
        vasp_gui.calc.prec = VP_ACCURATE;
        append_msg("SET PREC=ACCURATE\n");
        vasp_gui.calc.algo = VA_NORM;
        append_msg("SET ALGO=NORMAL\n");
        vasp_gui.calc.ldiag = TRUE;
        append_msg("SET LDIAG=.TRUE.\n");
      }
      vasp_gui.calc.use_prec = TRUE;

      /* LREAL */
      if (vasp_gui.calc.atoms_total > 16)
      {
        vasp_gui.calc.lreal = VLR_AUTO;
        append_msg("SET LREAL=Auto\n");
      } else
      {
        vasp_gui.calc.lreal = VLR_FALSE;
        append_msg("SET LREAL=.FALSE.\n");
      }

      /* Smearing */
      if (system > 0)
      {
        vasp_gui.calc.ismear = 0;
        append_msg("SET ISMEAR=0\n");
        vasp_gui.calc.sigma = 0.05;
        append_msg("SET SIGMA=0.05\n");
      } else
      {
        vasp_gui.calc.ismear = 1;
        append_msg("SET ISMEAR=1\n");
        vasp_gui.calc.sigma = 0.2;
        append_msg("SET SIGMA=0.2\n");
      }

      /* Dipole correction */
      append_msg("Please manually SET all dipole related settings in ELECT-I!\n");
      switch (dim)
      {
      case 0:
        vasp_gui.calc.idipol = VID_4;
        append_msg("SET IDIPOL=4\n");
        break;
      case 1:
        vasp_gui.calc.idipol = VID_3;
        append_msg("SET IDIPOL=3\n");
        append_msg("By convention, the major direction for a 1D material is the Z axis.\n");
        append_msg("If this is not the case, RESET IDIPOL accordingly!\n");
        break;
      case 2:
        vasp_gui.calc.idipol = VID_3;
        append_msg("SET IDIPOL=3\n");
        append_msg("By convention, the Z AXIS is normal to the surface of 2D material.\n");
        append_msg("If this is not the case, RESET IDIPOL accordingly!\n");
        break;
      default:
        vasp_gui.calc.ldipol = FALSE;
        append_msg("SET LDIPOL=.FALSE.\n");
        break;
      }

      /* Spin */
      if ((gint) (vasp_gui.calc.electron_total / 2) - (gdouble) (vasp_gui.calc.electron_total / 2.0) != 0.0)
      {
        vasp_gui.calc.ispin = TRUE;
        append_msg("SET ISPIN=2\n");
      } else
      {
        vasp_gui.calc.ispin = FALSE;
        append_msg("SET ISPIN=1\n");
      }

      /* Calculation-specific settings */
      switch (calcul)
      {
      case 0: /* Energy */
        if ((kgrid > 1) && (dim > 0))
        {
          vasp_gui.calc.ismear = -5;
          append_msg("SET ISMEAR=-5\n");
        }
        break;
      case 1: /* DOS/BANDS */
        if (dim == 0)
          append_msg("PB: COARSE KGRID BUT BAND/DOS REQUIRED!\n");
        if (vasp_gui.calc.have_paw)
        {
          vasp_gui.calc.lorbit = 12;
          append_msg("SET LORBIT=12\n");
        } else
        {
          vasp_gui.calc.lorbit = 2;
          append_msg("SET LORBIT=2\n");
        }
        vasp_gui.calc.nedos = 2001;
        append_msg("SET NEDOS=2001\n");
        vasp_gui.calc.emin = -10.;
        append_msg("SET EMIN=-10.0\n");
        vasp_gui.calc.emax = 10.;
        append_msg("SET EMAX=10.0\n");
        break;
      case 2: /* Lattice Dynamics */
        vasp_gui.calc.addgrid = TRUE;
        append_msg("SET ADDGRID=.TRUE.\n");
        vasp_gui.calc.lreal = VLR_FALSE;
        append_msg("SET LREAL=.FALSE.\n");
        vasp_gui.calc.ibrion = 6;
        append_msg("Default to finite Difference (IBRION=6)... SET IBRION=8 for Linear Perturbation!\n");
        vasp_gui.calc.potim = 0.015;
        append_msg("SET POTIM=0.015\n");
        if (vasp_gui.calc.ncore > 1)
        {
          append_msg("Lattice Dynamics does not support NCORE>1... RESETING NCORE\n");
          vasp_gui.calc.ncore = 1;
          append_msg("(note that kpar can be set > 1\n");
        }
        break;
      case 3: /* Geometry optimization */
        if (vasp_gui.simple_rgeom)
        {
          vasp_gui.calc.prec = VP_NORM;
          append_msg("SET PREC=NORMAL\n");
          vasp_gui.calc.use_prec = TRUE;
          vasp_gui.calc.algo = VA_FAST;
          append_msg("SET ALGO=FAST\n");
          vasp_gui.calc.nelmin = 5;
          append_msg("SET NELMIN=5\n");
          vasp_gui.calc.ediff = 1E-2;
          append_msg("SET EDIFF=1E-2\n");
          vasp_gui.calc.ediffg = -0.3;
          append_msg("SET EDIFFG=-0.3\n");
          vasp_gui.calc.nsw = 10;
          append_msg("SET NSW=10\n");
          vasp_gui.calc.ibrion = 2;
          append_msg("SET IBRION=2\n");
        } else
        {
          vasp_gui.calc.prec = VP_ACCURATE;
          append_msg("SET PREC=ACCURATE\n");
          vasp_gui.calc.use_prec = TRUE;
          vasp_gui.calc.algo = VA_NORM;
          append_msg("SET ALGO=NORMAL\n");
          vasp_gui.calc.ldiag = TRUE;
          append_msg("SET LDIAG=.TRUE.\n");
          vasp_gui.calc.addgrid = TRUE;
          append_msg("SET ADDGRID=.TRUE.\n");
          vasp_gui.calc.nelmin = 8;
          append_msg("SET NELMIN=8\n");
          vasp_gui.calc.ediff = 1E-5;
          append_msg("SET EDIFF=1E-5\n");
          vasp_gui.calc.ediffg = -0.01;
          append_msg("SET EDIFFG=-0.01\n");
          vasp_gui.calc.nsw = 20;
          append_msg("SET NSW=20\n");
          vasp_gui.calc.maxmix = 80;
          append_msg("SET MAXMIX=80\n");
          vasp_gui.calc.ibrion = 1;
          append_msg("SET IBRION=1\n");
          vasp_gui.calc.nfree = 10;
          append_msg("SET NFREE=10\n");
        }
        break;
      case 4: /* Molecular Dynamics */
        if (vasp_gui.simple_rgeom)
        {
          vasp_gui.calc.prec = VP_LOW;
          append_msg("SET PREC=LOW\n");
        } else
        {
          vasp_gui.calc.prec = VP_NORM;
          append_msg("SET PREC=NROMAL\n");
        }
        vasp_gui.calc.use_prec = TRUE;
        vasp_gui.calc.ediff = 1E-5;
        append_msg("SET EDIFF=1E-5\n");
        append_msg("Please set smearing manually!\n");
        vasp_gui.calc.ismear = -1;
        append_msg("SET ISMEAR=-1\n");
        vasp_gui.calc.sigma = 0.086;
        append_msg("SET SIGMA=0.086\n");
        vasp_gui.calc.algo = VA_VERYFAST;
        append_msg("SET ALGO=VERYFAST");
        vasp_gui.calc.maxmix = 40;
        append_msg("SET MAXMIX=40\n");
        vasp_gui.calc.isym = 0;
        append_msg("SET ISYM=0\n");
        vasp_gui.calc.nelmin = 4;
        append_msg("SET NELMIN=4\n");
        vasp_gui.calc.ibrion = 0;
        append_msg("SET IBRION=0\n");
        vasp_gui.calc.nsw = 100;
        append_msg("SET NSW=100\n");
        vasp_gui.calc.nwrite = 0;
        append_msg("SET NWRITE=0\n");
        vasp_gui.calc.lcharg = FALSE;
        append_msg("SET LCHARG=.FALSE.\n");
        vasp_gui.calc.lwave = FALSE;
        append_msg("SET LWAVE=.FALSE.\n");
        append_msg("Please set TEBEG and TEEND manually!\n");
        vasp_gui.calc.tebeg = 1000;
        append_msg("SET TEBEG=1000\n");
        vasp_gui.calc.teend = 1000;
        append_msg("SET TEEND=1000\n");
        vasp_gui.calc.smass = 3;
        append_msg("SET SMASS=3\n");
        vasp_gui.calc.nblock = 50;
        append_msg("SET NBLOCK=50\n");
        vasp_gui.calc.potim = 1.5;
        append_msg("SET POTIM=1.5\n");
        break;
      default:
        g_string_assign(msg_buf, "FAIL: UNKNOWN CALCUL SETTING!\n");
        break;
      }
    }

    /* Sync back: vasp_gui updated, now Qt needs to read it */
    g_free(vasp_gui.simple_message_buff);
    vasp_gui.simple_message_buff = g_strdup(msg_buf->str);
    g_string_free(msg_buf, FALSE);
    refresh();
  });

  connect(m_fileEntryBtn, &QPushButton::clicked, this, [this]() {
    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::ExistingFile);
    dlg.setNameFilter("VASP XML files (*.xml)");
    if (dlg.exec() == QDialog::Accepted)
    {
      QStringList files = dlg.selectedFiles();
      if (!files.isEmpty())
      {
        QString path = files.first();
        m_fileEntryEdit->setText(path);
        extern gint vasprun_update(gchar *, vasp_calc_struct *);
        vasprun_update(path.toUtf8().data(), &vasp_gui.calc);
        update_species_from_model();
        refresh();
        QMetaObject::invokeMethod(this, [this]() { repaint(); }, Qt::QueuedConnection);
      }
    }
  });

  /* Values already initialized by qt_show_vasp_dialog() via vasprun_update(data->filename) */
  /* Don't call vasprun_update(NULL) here — it would overwrite the correct values */

  apply_tooltips();
  refresh();
}

void VaspDialog::updateKpointsModeEnabled()
{
  /* Read mode directly from vasp_gui, not from combo (combo may not be set yet) */
  int mode = 0;
  switch (vasp_gui.calc.kpoints_mode)
  {
  case VKP_MAN:
    mode = 0;
    break;
  case VKP_LINE:
    mode = 1;
    break;
  case VKP_AUTO:
    mode = 2;
    break;
  case VKP_GAMMA:
    mode = 3;
    break;
  case VKP_MP:
    mode = 4;
    break;
  case VKP_BASIS:
    mode = 5;
    break;
  default:
    mode = 0;
    break;
  }

  bool enableCartesian = (mode == 0 || mode == 1 || mode == 5);
  bool enableKX = (mode == 2 || mode == 3 || mode == 4);
  bool enableKY = (mode == 3 || mode == 4);
  bool enableKZ = (mode == 3 || mode == 4);
  bool enableNKPTS = (mode == 0 || mode == 1 || mode == 5);
  bool enableSX = (mode == 3 || mode == 4);
  bool enableSY = (mode == 3 || mode == 4);
  bool enableSZ = (mode == 3 || mode == 4);
  bool enableCoordFrame = (mode == 0 || mode == 1 || mode == 5);
  bool enableW = (mode == 0);

  m_kpointsCartCheck->setEnabled(enableCartesian);
  m_kpointsKxSpin->setEnabled(enableKX);
  m_kpointsKySpin->setEnabled(enableKY);
  m_kpointsKzSpin->setEnabled(enableKZ);
  m_kpointsNkptsEdit->setEnabled(enableNKPTS);
  m_kpointsSxSpin->setEnabled(enableSX);
  m_kpointsSyEdit->setEnabled(enableSY);
  m_kpointsSzEdit->setEnabled(enableSZ);

  m_kpointsKptsCombo->setEnabled(enableCoordFrame);
  m_kpointsApplyBtn->setEnabled(enableCoordFrame);
  m_kpointsDelBtn->setEnabled(enableCoordFrame);
  m_kpointsIndexEdit->setEnabled(enableCoordFrame);
  m_kpointsXEdit->setEnabled(enableCoordFrame);
  m_kpointsYEdit->setEnabled(enableCoordFrame);
  m_kpointsZEdit->setEnabled(enableCoordFrame);
  m_kpointsWEdit->setEnabled(enableW);

  bool tetraEnabled = (mode == 0 && m_kpointsIsmearEdit->text().toInt() == -5);
  m_tetraCheck->setEnabled(tetraEnabled);
  m_tetraFrame->setEnabled(tetraEnabled && m_tetraCheck->isChecked());
}

VaspDialog::~VaspDialog() {}

/**
 * Qt equivalent of vasp_poscar_sync().
 * Reads atom entries from the Qt combo box and populates
 * vasp_gui.calc.species_symbols, species_numbers, poscar_symbol,
 * atoms_total, species_total.
 */
void VaspDialog::poscar_sync()
{
  /* if (!vasp_gui.poscar_dirty)
      return; <- BUG#1 vasp_gui.poscar_dirty was never set! */
  if (!m_poscarAtomsCombo || m_poscarAtomsCombo->count() == 0)
    return;
  /* Collect all non-"ADD atom" entries */
  struct AtomEntry {
    QString text;
    char symbol[3];
    int zval;
  };
  QVector<AtomEntry> entries;

  for (int i = 0; i < m_poscarAtomsCombo->count(); i++)
  {
    QString txt = m_poscarAtomsCombo->itemText(i);
    if (txt.trimmed() == "ADD atom")
      continue;
    AtomEntry e;
    e.text = txt;
    /* Parse "! atom: N (SYMBOL)" from the text */
    int parenOpen = txt.indexOf('(');
    int parenClose = txt.indexOf(')');
    if (parenOpen >= 0 && parenClose > parenOpen)
    {
      QString symStr = txt.mid(parenOpen + 1, parenClose - parenOpen - 1).trimmed();
      e.symbol[0] = symStr.toStdString().at(0);
      if (symStr.length() > 1)
        e.symbol[1] = symStr.toStdString().at(1);
      else
        e.symbol[1] = '\0';
    } else
    {
      e.symbol[0] = '\0';
      e.symbol[1] = '\0';
    }
    e.symbol[2] = '\0'; /* BUG#2 was here! */
    e.zval = elem_symbol_test(e.symbol);
    entries.append(e);
  }

  int natoms = entries.size();
  if (natoms == 0)
    return;

  /* Determine unique species */
  QVector<int> speciesZvals;
  QVector<QString> speciesSymbols;
  for (const auto &e : entries)
  {
    bool found = false;
    for (int j = 0; j < speciesZvals.size(); j++)
    {
      if (speciesZvals[j] == e.zval)
      {
        found = true;
        break;
      }
    }
    if (!found)
    {
      speciesZvals.append(e.zval);
      speciesSymbols.append(QString(e.symbol));
    }
  }
  int nspecies = speciesZvals.size();

  /* species_symbols: space-separated unique symbols */
  g_free(vasp_gui.calc.species_symbols);
  vasp_gui.calc.species_symbols = g_strdup(" ");
  for (int i = 0; i < nspecies; i++)
  {
    gchar *tamp = g_strdup_printf("%s %s", vasp_gui.calc.species_symbols, speciesSymbols[i].toUtf8().constData());
    g_free(vasp_gui.calc.species_symbols);
    vasp_gui.calc.species_symbols = tamp;
  }

  /* species_numbers: count per species in order of appearance */
  g_free(vasp_gui.calc.species_numbers);
  vasp_gui.calc.species_numbers = g_strdup(" ");
  for (int i = 0; i < nspecies; i++)
  {
    int count = 0;
    for (const auto &e : entries)
    {
      if (e.zval == speciesZvals[i])
        count++;
    }
    gchar *tamp = g_strdup_printf("%s %d", vasp_gui.calc.species_numbers, count);
    g_free(vasp_gui.calc.species_numbers);
    vasp_gui.calc.species_numbers = tamp;
  }

  /* First species as poscar_symbol */
  sscanf(entries[0].text.toUtf8().constData(), "%*[^!]! atom: %*i (%2[^)]", vasp_gui.calc.poscar_symbol);

  /* Rebuild combo with entries grouped by species */
  /* Preserve selective dynamics tags from original entries */
  m_poscarAtomsCombo->clear();
  int atomIdx = 0;
  for (int sp = 0; sp < nspecies; sp++)
  {
    for (int i = 0; i < natoms; i++)
    {
      if (entries[i].zval != speciesZvals[sp])
        continue;
      QString txt = entries[i].text;
      int parenOpen = txt.indexOf('(');
      int parenClose = txt.indexOf(')');
      QString symStr;
      if (parenOpen >= 0 && parenClose > parenOpen)
      {
        symStr = txt.mid(parenOpen + 1, parenClose - parenOpen - 1).trimmed();
      }
      /* Extract coords and selective tags from original text */
      QString coordsAndTags = txt.split('!')[0].trimmed();
      QString newTxt = QString("%1! atom: %2 (%3)").arg(coordsAndTags).arg(atomIdx).arg(symStr);
      m_poscarAtomsCombo->addItem(newTxt);
      atomIdx++;
    }
  }
  m_poscarAtomsCombo->addItem("ADD atom");

  vasp_gui.calc.atoms_total = natoms;
  vasp_gui.calc.species_total = nspecies;

  /* Update PRESET POSCAR and POTCAR species widgets since refresh() already ran */

  if (m_simplePoscarEdit)
    m_simplePoscarEdit->setText(QString::fromUtf8(vasp_gui.calc.species_symbols ? vasp_gui.calc.species_symbols : ""));
  if (m_potcarSpeciesEdit)
    m_potcarSpeciesEdit->setText(QString::fromUtf8(vasp_gui.calc.species_symbols ? vasp_gui.calc.species_symbols : ""));
}

/**
 * Qt version of calc_to_poscar. Writes POSCAR from Qt combo data.
 */
void VaspDialog::write_poscar(FILE *output)
{
  if (!vasp_gui.calc.name)
    fprintf(output, "UNNAMED MODEL ");
  else
    fprintf(output, "%s ", vasp_gui.calc.name);
  fprintf(output, "! GENERATED BY GDIS %4.2f.%d (C) %d\n", VERSION, PATCH, YEAR);
  fprintf(output, "%lf\n", vasp_gui.calc.poscar_a0);
  fprintf(output, "   %12.8lf   %12.8lf   %12.8lf\n", vasp_gui.calc.poscar_ux, vasp_gui.calc.poscar_uy,
          vasp_gui.calc.poscar_uz);
  fprintf(output, "   %12.8lf   %12.8lf   %12.8lf\n", vasp_gui.calc.poscar_vx, vasp_gui.calc.poscar_vy,
          vasp_gui.calc.poscar_vz);
  fprintf(output, "   %12.8lf   %12.8lf   %12.8lf\n", vasp_gui.calc.poscar_wx, vasp_gui.calc.poscar_wy,
          vasp_gui.calc.poscar_wz);
  fprintf(output, "%s\n", vasp_gui.calc.species_symbols);
  fprintf(output, "%s\n", vasp_gui.calc.species_numbers);
  if (vasp_gui.calc.poscar_sd)
    fprintf(output, "Selective dynamics\n");
  if (vasp_gui.calc.poscar_direct)
    fprintf(output, "Direct\n");
  else
    fprintf(output, "Cartesian\n");

  for (int i = 0; i < m_poscarAtomsCombo->count(); i++)
  {
    QString txt = m_poscarAtomsCombo->itemText(i);
    if (txt.trimmed() == "ADD atom")
      break;
    double x = 0, y = 0, z = 0;
    /* Parse coordinates from combo text */
    QRegularExpression rx("(-?[0-9.eE+]+)\\s+(-?[0-9.eE+]+)\\s+(-?[0-9.eE+]+)");
    QRegularExpressionMatch m = rx.match(txt);
    if (m.hasMatch())
    {
      x = m.captured(1).toDouble();
      y = m.captured(2).toDouble();
      z = m.captured(3).toDouble();
    }
    /* Selective dynamics flags: use poscar_free mode, parse combo text if VPF_MAN */
    char tx = 'F', ty = 'F', tz = 'F';
    if (vasp_gui.calc.poscar_free == VPF_FREE)
    {
      tx = 'T';
      ty = 'T';
      tz = 'T';
    } else if (vasp_gui.calc.poscar_free == VPF_MAN)
    {
      /* Parse "x y z T/F T/F T/F" from combo text */
      QString afterCoords =
          txt.mid(m.lastCapturedIndex() >= 3 ? txt.indexOf(m.captured(3)) + m.captured(3).length() : 0).trimmed();
      QStringList parts = afterCoords.split(' ', Qt::SkipEmptyParts);
      if (parts.size() >= 3)
      {
        tx = parts[0].startsWith('T') ? 'T' : 'F';
        ty = parts[1].startsWith('T') ? 'T' : 'F';
        tz = parts[2].startsWith('T') ? 'T' : 'F';
      }
    }
    fprintf(output, "   % .8lf   % .8lf   % .8lf  %c   %c   %c\n", x, y, z, tx, ty, tz);
  }
}

/**
 * Qt version of calc_to_kpoints. Writes KPOINTS from Qt combo data.
 */
void VaspDialog::write_kpoints(FILE *output)
{
  if (!vasp_gui.calc.name)
    fprintf(output, "UNNAMED MODEL ");
  else
    fprintf(output, "%s ", vasp_gui.calc.name);
  fprintf(output, "! GENERATED BY GDIS %4.2f.%d (C) %d\n", VERSION, PATCH, YEAR);

  int mode = vasp_gui.calc.kpoints_mode;
  if (mode == VKP_MAN || mode == VKP_LINE)
    fprintf(output, "%i\n", vasp_gui.calc.kpoints_nkpts);
  else
    fprintf(output, "0\n");

  switch (mode)
  {
  case VKP_LINE:
    fprintf(output, "Line-mode\n");
    fprintf(output, "%s\n", vasp_gui.calc.kpoints_cart ? "Cartesian" : "Reciprocal");
    break;
  case VKP_BASIS:
  case VKP_MAN:
    fprintf(output, "%s\n", vasp_gui.calc.kpoints_cart ? "Cartesian" : "Reciprocal");
    break;
  case VKP_GAMMA:
    fprintf(output, "Gamma\n");
    fprintf(output, "%i %i %i\n", (int) vasp_gui.calc.kpoints_kx, (int) vasp_gui.calc.kpoints_ky,
            (int) vasp_gui.calc.kpoints_kz);
    if (vasp_gui.calc.kpoints_sx != 0.0 && vasp_gui.calc.kpoints_sy != 0.0 && vasp_gui.calc.kpoints_sz != 0.0)
      fprintf(output, "%lf %lf %lf\n", vasp_gui.calc.kpoints_sx, vasp_gui.calc.kpoints_sy, vasp_gui.calc.kpoints_sz);
    return;
  case VKP_MP:
    fprintf(output, "Monkhorst-Pack\n");
    fprintf(output, "%i %i %i\n", (int) vasp_gui.calc.kpoints_kx, (int) vasp_gui.calc.kpoints_ky,
            (int) vasp_gui.calc.kpoints_kz);
    if (vasp_gui.calc.kpoints_sx != 0.0 && vasp_gui.calc.kpoints_sy != 0.0 && vasp_gui.calc.kpoints_sz != 0.0)
      fprintf(output, "%lf %lf %lf\n", vasp_gui.calc.kpoints_sx, vasp_gui.calc.kpoints_sy, vasp_gui.calc.kpoints_sz);
    return;
  case VKP_AUTO:
    fprintf(output, "Auto\n");
    fprintf(output, "%i\n", (int) vasp_gui.calc.kpoints_kx);
    return;
  default:
    break;
  }

  /* Print manual/line kpoint coordinates */
  int idx = 0;
  for (int i = 0; i < m_kpointsKptsCombo->count(); i++)
  {
    QString txt = m_kpointsKptsCombo->itemText(i);
    if (txt.trimmed() == "ADD kpoint")
      break;
    double x = 0, y = 0, z = 0, wt = 0;
    QRegularExpression rx("(-?[0-9.eE+]+)\\s+(-?[0-9.eE+]+)\\s+(-?[0-9.eE+]+)(\\s+(-?[0-9.eE+]+))?\\s*! kpoint:");
    QRegularExpressionMatch m2 = rx.match(txt);
    if (m2.hasMatch())
    {
      x = m2.captured(1).toDouble();
      y = m2.captured(2).toDouble();
      z = m2.captured(3).toDouble();
      if (m2.captured(5).isEmpty() && m2.lastCapturedIndex() >= 5)
        x = m2.captured(4).toDouble(); /* fallback: 4th group */
      else if (!m2.captured(5).isEmpty())
        wt = m2.captured(5).toDouble();
    }
    if (mode == VKP_MAN)
      fprintf(output, "%lf %lf %lf %lf ! kpoint: %i\n", x, y, z, wt, idx + 1);
    else
      fprintf(output, "%lf %lf %lf ! kpoint: %i\n", x, y, z, idx + 1);
    idx++;
  }

  /* Tetrahedron data */
  if (!vasp_gui.calc.kpoints_tetra)
    return;
  fprintf(output, "Tetrahedron\n");
  fprintf(output, "%i %lf\n", vasp_gui.calc.tetra_total, vasp_gui.calc.tetra_volume);
  for (int i = 0; i < m_tetraCombo->count(); i++)
  {
    QString txt = m_tetraCombo->itemText(i);
    if (txt.trimmed() == "ADD tetrahedron")
      break;
    double wt = 0;
    int a = 0, b = 0, c = 0, d = 0;
    QRegularExpression rx("(-?[0-9.eE+]+)\\s+(-?[0-9]+)\\s+(-?[0-9]+)\\s+(-?[0-9]+)\\s+(-?[0-9]+)");
    QRegularExpressionMatch m3 = rx.match(txt);
    if (m3.hasMatch())
    {
      wt = m3.captured(1).toDouble();
      a = m3.captured(2).toInt();
      b = m3.captured(3).toInt();
      c = m3.captured(4).toInt();
      d = m3.captured(5).toInt();
    }
    fprintf(output, "%lf %i %i %i %i ! tetrahedron: %i\n", wt, a, b, c, d, i + 1);
  }
}

/**
 * Qt version of calc_to_potcar. Writes POTCAR from file or folder.
 */
void VaspDialog::write_potcar(FILE *output)
{
  if (vasp_gui.have_potcar_folder)
  {
    /* Use folder mode: concatenate POTCAR files for each species flavor */
    if (!vasp_gui.calc.potcar_folder || !vasp_gui.calc.potcar_species_flavor)
      return;
    gchar *text = g_strdup(vasp_gui.calc.potcar_species_flavor);
    gint idx = 0;
    while (text[idx] == ' ' || text[idx] == '\t')
      idx++;
    if (text[idx] == '\0')
    {
      g_free(text);
      return;
    }
    gchar *ptr = &(text[idx]);
    gboolean hasend = FALSE;
    while (!hasend)
    {
      while (text[idx] != ' ')
      {
        if (text[idx] == '\0')
        {
          hasend = TRUE;
          break;
        }
        idx++;
      }
      text[idx] = '\0';
      idx++;
      gchar *filename = g_strdup_printf("%s/%s/POTCAR", vasp_gui.calc.potcar_folder, ptr);
      FILE *src = fopen(filename, "r");
      if (src)
      {
        gchar *line;
        while ((line = file_read_line(src)))
        {
          fprintf(output, "%s", line);
          g_free(line);
        }
        fclose(src);
      }
      g_free(filename);
      ptr = &(text[idx]);
    }
    g_free(text);
  } else
  {
    /* Use file mode: copy the selected POTCAR file */
    if (!vasp_gui.calc.potcar_file)
      return;
    FILE *src = fopen(vasp_gui.calc.potcar_file, "r");
    if (!src)
    {
      fprintf(stderr, "#ERR: can't open file %s for READ!\n", vasp_gui.calc.potcar_file);
      return;
    }
    gchar *line;
    while ((line = file_read_line(src)))
    {
      fprintf(output, "%s", line);
      g_free(line);
    }
    fclose(src);
  }
}

void VaspDialog::apply_tooltips()
{
  /* PRESET tab */
  set_widget_tooltip(m_nameEdit, TOOLTIP_NAME);
  set_widget_tooltip(m_simpleCalculCombo, TOOLTIP_CALC_TYPE);
  set_widget_tooltip(m_simpleRgeomCheck, TOOLTIP_RGEOM);
  set_widget_tooltip(m_simpleDimSpin, TOOLTIP_DIM);
  set_widget_tooltip(m_simpleSystemCombo, TOOLTIP_SYSTEM);
  set_widget_tooltip(m_simplePoscarEdit, TOOLTIP_POSCAR);
  set_widget_tooltip(m_simpleKgridCombo, TOOLTIP_KGRID);
  set_widget_tooltip(m_simpleSpeciesEdit, TOOLTIP_POTCAR_SPECIES);
  set_widget_tooltip(m_simplePotcarEdit, TOOLTIP_POTCAR_FILE);
  set_widget_tooltip(m_simpleNpSpin, TOOLTIP_NPROC);
  set_widget_tooltip(m_simpleNcoreSpin, TOOLTIP_NCORE);
  set_widget_tooltip(m_simpleKparSpin, TOOLTIP_KPAR);
  set_widget_tooltip(m_usePrecCheck, TOOLTIP_AUTO_PREC);
  set_widget_tooltip(m_autoGridCheck, TOOLTIP_AUTO_GRID);
  set_widget_tooltip(m_autoMixerCheck, TOOLTIP_AUTO_MIXER);
  set_widget_tooltip(m_autoElecCheck, TOOLTIP_AUTO_ELEC);
  set_widget_tooltip(m_simpleApplyBtn, TOOLTIP_SIMPLE_APPLY);

  /* CONVERGENCE tab - General */
  set_widget_tooltip(m_precCombo, TOOLTIP_PREC);
  set_widget_tooltip(m_encutEdit, TOOLTIP_ENCUT);
  set_widget_tooltip(m_enaugEdit, TOOLTIP_ENAUG);
  set_widget_tooltip(m_ediffEdit, TOOLTIP_EDIFF);
  set_widget_tooltip(m_algoCombo, TOOLTIP_ALGO);
  set_widget_tooltip(m_ldiagCheck, TOOLTIP_LDIAG);
  set_widget_tooltip(m_nsimEdit, TOOLTIP_NSIM);
  set_widget_tooltip(m_vtimeEdit, TOOLTIP_TIME);
  set_widget_tooltip(m_iwavprEdit, TOOLTIP_IWAVPR);
  set_widget_tooltip(m_ialgoCombo, TOOLTIP_IALGO);
  set_widget_tooltip(m_nbandsEdit, TOOLTIP_NBANDS);
  set_widget_tooltip(m_nelectEdit, TOOLTIP_NELECT);
  set_widget_tooltip(m_iniwavCheck, TOOLTIP_INIWAV);
  set_widget_tooltip(m_istartEdit, TOOLTIP_ISTART);
  set_widget_tooltip(m_ichargEdit, TOOLTIP_ICHARG);
  set_widget_tooltip(m_nelmEdit, TOOLTIP_NELM);
  set_widget_tooltip(m_nelmdlEdit, TOOLTIP_NELMDL);
  set_widget_tooltip(m_nelminEdit, TOOLTIP_NELMIN);

  /* CONVERGENCE tab - Mixing */
  set_widget_tooltip(m_mixerCombo, TOOLTIP_IMIX);
  set_widget_tooltip(m_mixpreCombo, TOOLTIP_MIXPRE);
  set_widget_tooltip(m_amixEdit, TOOLTIP_AMIX);
  set_widget_tooltip(m_bmixEdit, TOOLTIP_BMIX);
  set_widget_tooltip(m_aminEdit, TOOLTIP_AMIN);
  set_widget_tooltip(m_inimixCombo, TOOLTIP_INIMIX);
  set_widget_tooltip(m_maxmixEdit, TOOLTIP_MAXMIX);
  set_widget_tooltip(m_amixMagEdit, TOOLTIP_AMIX_MAG);
  set_widget_tooltip(m_bmixMagEdit, TOOLTIP_BMIX_MAG);
  set_widget_tooltip(m_wcEdit, TOOLTIP_WC);
  set_widget_tooltip(m_addgridCheck, TOOLTIP_ADDGRID);
  set_widget_tooltip(m_lmaxmixEdit, TOOLTIP_LMAXMIX);
  set_widget_tooltip(m_lmaxpawEdit, TOOLTIP_LMAXPAW);

  /* CONVERGENCE tab - Grid */
  set_widget_tooltip(m_ngxEdit, TOOLTIP_NGX);
  set_widget_tooltip(m_ngyEdit, TOOLTIP_NGY);
  set_widget_tooltip(m_ngzEdit, TOOLTIP_NGZ);
  set_widget_tooltip(m_ngxfEdit, TOOLTIP_NGXF);
  set_widget_tooltip(m_ngyfEdit, TOOLTIP_NGYF);
  set_widget_tooltip(m_ngzfEdit, TOOLTIP_NGZF);
  set_widget_tooltip(m_elecLrealCombo, TOOLTIP_LREAL);
  set_widget_tooltip(m_convRoptEdit, TOOLTIP_ROPT);

  /* ELECT-I tab */
  set_widget_tooltip(m_ismearEdit, TOOLTIP_ISMEAR);
  set_widget_tooltip(m_sigmaEdit, TOOLTIP_SIGMA);
  set_widget_tooltip(m_kgammaCheck, TOOLTIP_GAMMA);
  set_widget_tooltip(m_kspacingEdit, TOOLTIP_KSPACING);
  set_widget_tooltip(m_fermweEdit, TOOLTIP_FERMWE);
  set_widget_tooltip(m_fermdoEdit, TOOLTIP_FERDO);
  set_widget_tooltip(m_spinCheck, TOOLTIP_ISPIN);
  set_widget_tooltip(m_ggaCompatCheck, TOOLTIP_GGA_COMPAT);
  set_widget_tooltip(m_nupdownEdit, TOOLTIP_NUPDOWN);
  set_widget_tooltip(m_magmomEdit, TOOLTIP_MAGMOM);
  set_widget_tooltip(m_saxisEdit, TOOLTIP_SAXIS);
  set_widget_tooltip(m_metaggaCombo, TOOLTIP_METAGGA);
  set_widget_tooltip(m_lmetaggaCheck, TOOLTIP_LMETAGGA);
  set_widget_tooltip(m_lasphCheck, TOOLTIP_LASPH);
  set_widget_tooltip(m_lmixtauCheck, TOOLTIP_LMIXTAU);
  set_widget_tooltip(m_lmaxtauEdit, TOOLTIP_LMAXTAU);
  set_widget_tooltip(m_cmbjEdit, TOOLTIP_CMBJ);
  set_widget_tooltip(m_cmbjaEdit, TOOLTIP_CMBJA);
  set_widget_tooltip(m_cmbjbEdit, TOOLTIP_CMBJB);
  set_widget_tooltip(m_ldauTypeCombo, TOOLTIP_LDAUTYPE);
  set_widget_tooltip(m_ldauCheck, TOOLTIP_LDAU);
  set_widget_tooltip(m_ldaulEdit, TOOLTIP_LDAUL);
  set_widget_tooltip(m_ldauPrintCombo, TOOLTIP_LDAUPRINT);
  set_widget_tooltip(m_ldauuEdit, TOOLTIP_LDAUU);
  set_widget_tooltip(m_ldaujEdit, TOOLTIP_LDAUJ);
  set_widget_tooltip(m_idipolCombo, TOOLTIP_IDIPOL);
  set_widget_tooltip(m_ldipolCheck, TOOLTIP_LDIPOL);
  set_widget_tooltip(m_lmonoCheck, TOOLTIP_LMONO);
  set_widget_tooltip(m_dipolEdit, TOOLTIP_DIPOL);
  set_widget_tooltip(m_epsilonEdit, TOOLTIP_EPSILON);
  set_widget_tooltip(m_efieldEdit, TOOLTIP_EFIELD);
  set_widget_tooltip(m_lorbitCombo, TOOLTIP_LORBIT);
  set_widget_tooltip(m_nedosEdit, TOOLTIP_NEDOS);
  set_widget_tooltip(m_eminEdit, TOOLTIP_EMIN);
  set_widget_tooltip(m_emaxEdit, TOOLTIP_EMAX);
  set_widget_tooltip(m_efermiEdit, TOOLTIP_EFERMI);
  set_widget_tooltip(m_havePawCheck, TOOLTIP_HAVE_PAW);
  set_widget_tooltip(m_rwigsEdit, TOOLTIP_RWIGS);
  set_widget_tooltip(m_lopticsCheck, TOOLTIPLOPTICS);
  set_widget_tooltip(m_lepsilonCheck, TOOLTIP_LEPSILON);
  set_widget_tooltip(m_lrpaCheck, TOOLTIP_LRPA);
  set_widget_tooltip(m_lnoncollCheck, TOOLTIP_NONCOLLINEAR);
  set_widget_tooltip(m_lsorbitCheck, TOOLTIP_LSORBIT);

  /* IONIC tab */
  set_widget_tooltip(m_ibrionCombo, TOOLTIP_IBRION);
  set_widget_tooltip(m_nswEdit, TOOLTIP_NSW);
  set_widget_tooltip(m_ediffgEdit, TOOLTIP_EDIFFG);
  set_widget_tooltip(m_potimEdit, TOOLTIP_POTIM);
  set_widget_tooltip(m_pstressEdit, TOOLTIP_PSTRESS);
  set_widget_tooltip(m_isifCombo, TOOLTIP_ISIF);
  set_widget_tooltip(m_relaxIonsCheck, TOOLTIP_RELAX_ATOMIC);
  set_widget_tooltip(m_relaxShapeCheck, TOOLTIP_RELAX_SHAPE);
  set_widget_tooltip(m_relaxVolumeCheck, TOOLTIP_RELAX_VOLUME);
  set_widget_tooltip(m_nfreeEdit, TOOLTIP_NFREE);
  set_widget_tooltip(m_tebegEdit, TOOLTIP_TEBEG);
  set_widget_tooltip(m_teendEdit, TOOLTIP_TEEND);
  set_widget_tooltip(m_smassEdit, TOOLTIP_SMASS);
  set_widget_tooltip(m_nblockEdit, TOOLTIP_NBLOCK);
  set_widget_tooltip(m_kblockEdit, TOOLTIP_KBLOCK);
  set_widget_tooltip(m_npacoEdit, TOOLTIP_NPACO);
  set_widget_tooltip(m_apacoEdit, TOOLTIP_APACO);
  set_widget_tooltip(m_poscarSdCheck, TOOLTIP_TAGS);
  set_widget_tooltip(m_poscarFreeCombo, TOOLTIP_SEL_DYN);
  set_widget_tooltip(m_isymEdit, TOOLTIP_ISYM);
  set_widget_tooltip(m_symPrecEdit, TOOLTIP_SYMPREC);
  set_widget_tooltip(m_poscarA0Edit, TOOLTIP_POSCAR_A0);
  set_widget_tooltip(m_poscarDirectCheck, TOOLTIP_DIRECT);

  /* POSCAR editing */
  set_widget_tooltip(m_poscarUxEdit, TOOLTIP_POSCAR_UX);
  set_widget_tooltip(m_poscarUyEdit, TOOLTIP_POSCAR_UY);
  set_widget_tooltip(m_poscarUzEdit, TOOLTIP_POSCAR_UZ);
  set_widget_tooltip(m_poscarVxEdit, TOOLTIP_POSCAR_VX);
  set_widget_tooltip(m_poscarVyEdit, TOOLTIP_POSCAR_VY);
  set_widget_tooltip(m_poscarVzEdit, TOOLTIP_POSCAR_VZ);
  set_widget_tooltip(m_poscarWxEdit, TOOLTIP_POSCAR_WX);
  set_widget_tooltip(m_poscarWyEdit, TOOLTIP_POSCAR_WY);
  set_widget_tooltip(m_poscarWzEdit, TOOLTIP_POSCAR_WZ);
  set_widget_tooltip(m_poscarTxCheck, TOOLTIP_POSCAR_TX);
  set_widget_tooltip(m_poscarTyCheck, TOOLTIP_POSCAR_TY);
  set_widget_tooltip(m_poscarTzCheck, TOOLTIP_POSCAR_TZ);
  set_widget_tooltip(m_poscarAtomsCombo, TOOLTIP_POSCAR_ATOMS);
  set_widget_tooltip(m_poscarIndexEdit, TOOLTIP_POSCAR_INDEX);
  set_widget_tooltip(m_poscarSymbolEdit, TOOLTIP_POSCAR_SYMBOL);
  set_widget_tooltip(m_poscarXEdit, TOOLTIP_POSCAR_X);
  set_widget_tooltip(m_poscarYEdit, TOOLTIP_POSCAR_Y);
  set_widget_tooltip(m_poscarZEdit, TOOLTIP_POSCAR_Z);

  /* KPOINTS tab */
  set_widget_tooltip(m_kpointsModeCombo, TOOLTIP_KPT_MODE);
  set_widget_tooltip(m_kpointsCartCheck, TOOLTIP_KPT_CART);
  set_widget_tooltip(m_kpointsKxSpin, TOOLTIP_KPT_GRID);
  set_widget_tooltip(m_kpointsKySpin, TOOLTIP_KPT_GRID_Y);
  set_widget_tooltip(m_kpointsKzSpin, TOOLTIP_KPT_GRID_Z);
  set_widget_tooltip(m_kpointsNkptsEdit, TOOLTIP_KPT_TOTAL);
  set_widget_tooltip(m_kpointsSxSpin, TOOLTIP_KPT_SX);
  set_widget_tooltip(m_kpointsSyEdit, TOOLTIP_KPT_SY);
  set_widget_tooltip(m_kpointsSzEdit, TOOLTIP_KPT_SZ);
  set_widget_tooltip(m_kpointsKptsCombo, TOOLTIP_KPT_LIST);
  set_widget_tooltip(m_kpointsIndexEdit, TOOLTIP_KPT_INDEX);
  set_widget_tooltip(m_kpointsXEdit, TOOLTIP_KPT_X);
  set_widget_tooltip(m_kpointsYEdit, TOOLTIP_KPT_Y);
  set_widget_tooltip(m_kpointsZEdit, TOOLTIP_KPT_Z);
  set_widget_tooltip(m_kpointsWEdit, TOOLTIP_KPT_W);
  set_widget_tooltip(m_tetraCheck, TOOLTIP_TETRA);
  set_widget_tooltip(m_tetraTotalEdit, TOOLTIP_TETRA_N);
  set_widget_tooltip(m_tetraVolumeEdit, TOOLTIP_TETRA_VOLUME);
  set_widget_tooltip(m_tetraCombo, TOOLTIP_TETRA_LIST);
  set_widget_tooltip(m_tetraIndexEdit, TOOLTIP_TETRA_INDEX);
  set_widget_tooltip(m_tetraWEdit, TOOLTIP_TETRA_W);
  set_widget_tooltip(m_tetraPtAEdit, TOOLTIP_TETRA_A);
  set_widget_tooltip(m_tetraPtBEdit, TOOLTIP_TETRA_B);
  set_widget_tooltip(m_tetraPtCEdit, TOOLTIP_TETRA_C);
  set_widget_tooltip(m_tetraPtDEdit, TOOLTIP_TETRA_D);

  /* POTCAR tab */
  set_widget_tooltip(m_potcarSpeciesEdit, TOOLTIP_POTCAR_LIST);
  set_widget_tooltip(m_potcarSelectFileRadio, TOOLTIP_POTCAR_SELECT_FILE);
  set_widget_tooltip(m_potcarSelectFolderRadio, TOOLTIP_POTCAR_SELECT_FOLDER);
  set_widget_tooltip(m_potcarFileEdit, TOOLTIP_POTCAR_FILE_PATH);
  set_widget_tooltip(m_potcarFolderEdit, TOOLTIP_POTCAR_PATH);
  set_widget_tooltip(m_potcarFlavorCombo, TOOLTIP_POTCAR_FLAVOR);
  set_widget_tooltip(m_potcarDetectedSpeciesEdit, TOOLTIP_POTCAR_SPECIES_LIST);
  set_widget_tooltip(m_potcarDetectedFlavorEdit, TOOLTIP_POTCAR_DETECTED_FLAVOR);

  /* EXEC tab */
  set_widget_tooltip(m_jobVaspExeEdit, TOOLTIP_VASP_EXE);
  set_widget_tooltip(m_jobPathEdit, TOOLTIP_JOB_PATH);
  set_widget_tooltip(m_jobMpirunEdit, TOOLTIP_MPIRUN);
  set_widget_tooltip(m_jobNprocSpin, TOOLTIP_NPROC_EXEC);
  set_widget_tooltip(m_ncoreSpin, TOOLTIP_NCORE_EXEC);
  set_widget_tooltip(m_kparSpin, TOOLTIP_KPAR_EXEC);
  set_widget_tooltip(m_lplaneCheck, TOOLTIP_LPLANE);
  set_widget_tooltip(m_lscaluCheck, TOOLTIP_LSCALU);
  set_widget_tooltip(m_lscalapackCheck, TOOLTIP_LSCALAPACK);
  set_widget_tooltip(m_lwaveCheck, TOOLTIP_LWAVE);
  set_widget_tooltip(m_lchargCheck, TOOLTIP_LCHARG);
  set_widget_tooltip(m_lvtotCheck, TOOLTIP_LVTOT);
  set_widget_tooltip(m_lvharCheck, TOOLTIP_LVHAR);
  set_widget_tooltip(m_lelfCheck, TOOLTIP_LELF);

  /* EXEC tab button connections (must be outside refresh() to avoid duplicates) */
  connect(m_jobPathBtn, &QPushButton::clicked, this, [this]() {
    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::Directory);
    dlg.setWindowTitle("Select calculation directory");
    if (dlg.exec() == QDialog::Accepted)
    {
      QStringList dirs = dlg.selectedFiles();
      if (!dirs.isEmpty())
        m_jobPathEdit->setText(dirs.first());
    }
  });
  connect(m_jobMpirunBtn, &QPushButton::clicked, this, [this]() {
    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::ExistingFile);
    dlg.setNameFilter("mpirun executables (mpirun mpiexec *mpirun *mpiexec)");
    dlg.setWindowTitle("Select mpirun executable");
    if (dlg.exec() == QDialog::Accepted)
    {
      QStringList files = dlg.selectedFiles();
      if (!files.isEmpty())
        m_jobMpirunEdit->setText(files.first());
    }
  });

  /* Missing tooltips */
  set_widget_tooltip(m_fileEntryEdit, TOOLTIP_FILE_ENTRY);
  set_widget_tooltip(m_lrealCombo, TOOLTIP_LREAL);
  set_widget_tooltip(m_ggaCombo, TOOLTIP_GGA);
  set_widget_tooltip(m_voskownCheck, TOOLTIP_VOSKOWN);
  set_widget_tooltip(m_lnablaCheck, TOOLTIP_LNABLA);
  set_widget_tooltip(m_cshiftEdit, TOOLTIP_CSHIFT);
  set_widget_tooltip(m_kpointsIsmearEdit, TOOLTIP_ISMEAR);
  set_widget_tooltip(m_kpointsGammaCheck, TOOLTIP_GAMMA);
  set_widget_tooltip(m_kpointsKspacingEdit, TOOLTIP_KSPACING);
}

void VaspDialog::refresh()
{
  /* Sync from vasp_gui to Qt widgets */
  /* PRESET */
  m_simpleCalculCombo->setCurrentIndex(vasp_gui.simple_calcul_idx);
  m_simpleRgeomCheck->setChecked(vasp_gui.simple_rgeom);
  m_simpleDimSpin->setValue((int) vasp_gui.dimension);
  m_simpleSystemCombo->setCurrentIndex(vasp_gui.simple_system_idx);
  /* Recompute species_symbols from model if empty */
  if (!vasp_gui.calc.species_symbols || vasp_gui.calc.species_symbols[0] == '\0')
  {
    struct model_pak *model = (struct model_pak *) sysenv.active_model;
    if (model && model->cores)
    {
      QSet<gint> seen;
      GString *species = g_string_new("");
      for (GSList *l = model->cores; l; l = g_slist_next(l))
      {
        struct core_pak *core = (struct core_pak *) l->data;
        gint code = core->atom_code;
        if (code < 0 || code >= MAX_ELEMENTS)
          continue;
        if (seen.contains(code))
          continue;
        seen.insert(code);
        if (species->len > 0)
          g_string_append_printf(species, " ");
        g_string_append(species, elements[code].symbol);
      }
      g_free(vasp_gui.calc.species_symbols);
      vasp_gui.calc.species_symbols = g_strdup(species->str);
      g_string_free(species, FALSE);
    }
  }

  m_simplePoscarEdit->setText(QString::fromUtf8(vasp_gui.calc.species_symbols ? vasp_gui.calc.species_symbols : ""));
  m_simpleKgridCombo->setCurrentIndex(vasp_gui.simple_kgrid_idx);
  m_simpleSpeciesEdit->setText(QString::fromUtf8(vasp_gui.calc.potcar_species ? vasp_gui.calc.potcar_species : ""));
  m_simplePotcarEdit->setText(QString::fromUtf8(vasp_gui.calc.potcar_file ? vasp_gui.calc.potcar_file : ""));
  m_simpleNpSpin->setValue((int) vasp_gui.calc.job_nproc);
  m_simpleNcoreSpin->setValue(vasp_gui.calc.ncore);
  m_simpleKparSpin->setValue(vasp_gui.calc.kpar);
  if (m_simpleMessageText)
    m_simpleMessageText->setPlainText(
        QString::fromUtf8(static_cast<const char *>(vasp_gui.simple_message_buff ? vasp_gui.simple_message_buff : "")));

  /* CONVERGENCE */
  m_nameEdit->setText(QString::fromUtf8(vasp_gui.calc.name ? vasp_gui.calc.name : ""));
  m_fileEntryEdit->setText(QString::fromUtf8(vasp_gui.calc.job_path ? vasp_gui.calc.job_path : ""));
  m_usePrecCheck->setChecked(vasp_gui.calc.use_prec);
  m_encutEdit->setText(QString::number(vasp_gui.calc.encut, 'f', 2));
  m_enaugEdit->setText(QString::number(vasp_gui.calc.enaug, 'f', 2));
  m_ediffEdit->setText(QString::number(vasp_gui.calc.ediff, 'e', 2));
  m_convRoptEdit->setText(QString::fromUtf8(vasp_gui.calc.ropt ? vasp_gui.calc.ropt : ""));

  int precIdx = 0;
  switch (vasp_gui.calc.prec)
  {
  case VP_SINGLE:
    precIdx = 1;
    break;
  case VP_ACCURATE:
    precIdx = 2;
    break;
  case VP_HIGH:
    precIdx = 3;
    break;
  case VP_MED:
    precIdx = 4;
    break;
  case VP_LOW:
    precIdx = 5;
    break;
  case VP_NORM:
  default:
    precIdx = 0;
    break;
  }
  m_precCombo->setCurrentIndex(precIdx);

  int algoIdx = 0;
  switch (vasp_gui.calc.algo)
  {
  case VA_IALGO:
    algoIdx = 1;
    break;
  case VA_VERYFAST:
    algoIdx = 2;
    break;
  case VA_FAST:
    algoIdx = 3;
    break;
  case VA_CONJ:
    algoIdx = 4;
    break;
  case VA_ALL:
    algoIdx = 5;
    break;
  case VA_DAMPED:
    algoIdx = 6;
    break;
  case VA_SUBROT:
    algoIdx = 7;
    break;
  case VA_EIGEN:
    algoIdx = 8;
    break;
  case VA_NONE:
    algoIdx = 9;
    break;
  case VA_NOTHING:
    algoIdx = 10;
    break;
  case VA_EXACT:
    algoIdx = 11;
    break;
  case VA_DIAG:
    algoIdx = 12;
    break;
  default:
    algoIdx = 0;
    break;
  }
  (void) algoIdx; /* suppress unused warning */
  m_algoCombo->setCurrentIndex(algoIdx);
  /* Set initial IALGO enabled state based on ALGO combo */
  m_ialgoCombo->setEnabled(algoIdx == 1); /* index 1 = USE_IALGO */

  int ialgoIdx = 7; /* Default: VIA_KOSUGI (IALGO=38) */
  switch (vasp_gui.calc.ialgo)
  {
  case VIA_OE_FIXED:
    ialgoIdx = 0;
    break;
  case VIA_O_FIXED:
    ialgoIdx = 1;
    break;
  case VIA_SUBROT:
    ialgoIdx = 2;
    break;
  case VIA_STEEP:
    ialgoIdx = 3;
    break;
  case VIA_CG:
    ialgoIdx = 4;
    break;
  case VIA_PSTEEP:
    ialgoIdx = 5;
    break;
  case VIA_PCG:
    ialgoIdx = 6;
    break;
  case VIA_KOSUGI:
    ialgoIdx = 7;
    break;
  case VIA_ESTEEP:
    ialgoIdx = 8;
    break;
  case VIA_RMMP:
    ialgoIdx = 9;
    break;
  case VIA_PRMM:
    ialgoIdx = 10;
    break;
  case VIA_VAR_DAMP:
    ialgoIdx = 11;
    break;
  case VIA_VAR_QUENCH:
    ialgoIdx = 12;
    break;
  case VIA_VAR_PCG:
    ialgoIdx = 13;
    break;
  case VIA_EXACT:
    ialgoIdx = 14;
    break;
  default:
    ialgoIdx = 7;
    break;
  }
  m_ialgoCombo->setCurrentIndex(ialgoIdx);

  m_nsimEdit->setText(QString::number(vasp_gui.calc.nsim));
  m_vtimeEdit->setText(QString::number(vasp_gui.calc.vtime, 'f', 4));
  m_iwavprEdit->setText(QString::number(vasp_gui.calc.iwavpr));
  m_ldiagCheck->setChecked(vasp_gui.calc.ldiag);
  m_autoElecCheck->setChecked(vasp_gui.calc.auto_elec);
  m_nbandsEdit->setText(QString::number(vasp_gui.calc.nbands));
  m_nelectEdit->setText(QString::number(vasp_gui.calc.nelect, 'f', 2));
  m_iniwavCheck->setChecked(vasp_gui.calc.iniwav);
  m_istartEdit->setText(QString::number(vasp_gui.calc.istart));
  m_ichargEdit->setText(QString::number(vasp_gui.calc.icharg));

  int mixerIdx = 3;
  switch (vasp_gui.calc.imix)
  {
  case VM_1:
    mixerIdx = 1;
    break;
  case VM_2:
    mixerIdx = 2;
    break;
  case VM_4:
    mixerIdx = 3;
    break;
  default:
    mixerIdx = 0;
    break;
  }
  m_mixerCombo->setCurrentIndex(mixerIdx);

  m_nelmEdit->setText(QString::number(vasp_gui.calc.nelm));
  m_nelmdlEdit->setText(QString::number(vasp_gui.calc.nelmdl));
  m_nelminEdit->setText(QString::number(vasp_gui.calc.nelmin));

  int mixpreIdx = 1;
  switch (vasp_gui.calc.mixpre)
  {
  case VMP_1:
    mixpreIdx = 1;
    break;
  case VMP_2:
    mixpreIdx = 2;
    break;
  default:
    mixpreIdx = 0;
    break;
  }
  m_mixpreCombo->setCurrentIndex(mixpreIdx);

  m_amixEdit->setText(QString::number(vasp_gui.calc.amix, 'f', 4));
  m_bmixEdit->setText(QString::number(vasp_gui.calc.bmix, 'f', 4));
  m_aminEdit->setText(QString::number(vasp_gui.calc.amin, 'f', 4));

  int inimixIdx = 1;
  switch (vasp_gui.calc.inimix)
  {
  case VIM_1:
    inimixIdx = 1;
    break;
  default:
    inimixIdx = 0;
    break;
  }
  m_inimixCombo->setCurrentIndex(inimixIdx);

  m_maxmixEdit->setText(QString::number(vasp_gui.calc.maxmix));
  m_amixMagEdit->setText(QString::number(vasp_gui.calc.amix_mag, 'f', 4));
  m_bmixMagEdit->setText(QString::number(vasp_gui.calc.bmix_mag, 'f', 4));
  m_wcEdit->setText(QString::number(vasp_gui.calc.wc, 'f', 1));
  m_lmaxmixEdit->setText(QString::number(vasp_gui.calc.lmaxmix));
  m_lmaxpawEdit->setText(QString::number(vasp_gui.calc.lmaxpaw));
  if (m_addgridCheck)
    m_addgridCheck->setChecked(vasp_gui.calc.addgrid);
  if (m_autoGridCheck)
    m_autoGridCheck->setChecked(vasp_gui.calc.auto_grid);

  /* ELECT-I */
  m_ismearEdit->setText(QString::number(vasp_gui.calc.ismear));
  m_sigmaEdit->setText(QString::number(vasp_gui.calc.sigma, 'f', 4));
  m_kgammaCheck->setChecked(vasp_gui.calc.kgamma);
  m_kspacingEdit->setText(QString::number(vasp_gui.calc.kspacing, 'f', 4));
  if (m_fermweEdit)
    m_fermweEdit->setText(QString::fromUtf8(vasp_gui.calc.fermwe ? vasp_gui.calc.fermwe : ""));
  if (m_fermdoEdit)
    m_fermdoEdit->setText(QString::fromUtf8(vasp_gui.calc.fermdo ? vasp_gui.calc.fermdo : ""));

  int lrealIdx = 0;
  switch (vasp_gui.calc.lreal)
  {
  case VLR_ON:
    lrealIdx = 1;
    break;
  case VLR_TRUE:
    lrealIdx = 2;
    break;
  case VLR_FALSE:
    lrealIdx = 3;
    break;
  default:
    lrealIdx = 0;
    break;
  }
  m_lrealCombo->setCurrentIndex(lrealIdx);

  int ggaIdx = 0;
  switch (vasp_gui.calc.gga)
  {
  case VG_PE:
    ggaIdx = 1;
    break;
  case VG_PS:
    ggaIdx = 4;
    break;
  case VG_AM:
    ggaIdx = 3;
    break;
  case VG_91:
    ggaIdx = 0;
    break;
  case VG_RP:
    ggaIdx = 2;
    break;
  default:
    ggaIdx = 0;
    break;
  }
  m_ggaCombo->setCurrentIndex(ggaIdx);
  m_voskownCheck->setChecked(vasp_gui.calc.voskown);
  m_lasphCheck->setChecked(vasp_gui.calc.lasph);

  if (m_spinCheck)
    m_spinCheck->setChecked(vasp_gui.calc.ispin);
  if (m_lnoncollCheck)
    m_lnoncollCheck->setChecked(vasp_gui.calc.non_collinear);
  m_lsorbitCheck->setChecked(vasp_gui.calc.lsorbit);
  if (m_ggaCompatCheck)
    m_ggaCompatCheck->setChecked(vasp_gui.calc.gga_compat);
  m_saxisEdit->setText(QString::fromUtf8(vasp_gui.calc.saxis ? vasp_gui.calc.saxis : ""));
  if (m_nupdownEdit)
    m_nupdownEdit->setText(QString::number(vasp_gui.calc.nupdown, 'f', 1));
  if (m_magmomEdit)
    m_magmomEdit->setText(QString::fromUtf8(vasp_gui.calc.magmom ? vasp_gui.calc.magmom : ""));
  m_lmetaggaCheck->setChecked(vasp_gui.calc.lmetagga);
  if (m_metaggaCombo)
  {
    int mggaIdx = 0;
    switch (vasp_gui.calc.mgga)
    {
    case VMG_TPSS:
      mggaIdx = 0;
      break;
    case VMG_RTPSS:
      mggaIdx = 1;
      break;
    case VMG_M06L:
      mggaIdx = 2;
      break;
    case VMG_MBJ:
      mggaIdx = 3;
      break;
    default:
      mggaIdx = 3;
      break; /* MBJ is VASP default */
    }
    m_metaggaCombo->setCurrentIndex(mggaIdx);
  }
  if (m_lmixtauCheck)
    m_lmixtauCheck->setChecked(vasp_gui.calc.lmixtau);
  if (m_lmaxtauEdit)
    m_lmaxtauEdit->setText(QString::number(vasp_gui.calc.lmaxtau));
  if (m_cmbjEdit)
    m_cmbjEdit->setText(QString::fromUtf8(vasp_gui.calc.cmbj ? vasp_gui.calc.cmbj : ""));
  if (m_cmbjaEdit)
    m_cmbjaEdit->setText(QString::number(vasp_gui.calc.cmbja, 'f', 4));
  if (m_cmbjbEdit)
    m_cmbjbEdit->setText(QString::number(vasp_gui.calc.cmbjb, 'f', 4));
  m_ldauCheck->setChecked(vasp_gui.calc.ldau);

  int ldauIdx = 0;
  switch (vasp_gui.calc.ldau_type)
  {
  case VU_2:
    ldauIdx = 1;
    break;
  case VU_1:
    ldauIdx = 2;
    break;
  case VU_4:
    ldauIdx = 3;
    break;
  default:
    ldauIdx = 0;
    break;
  }
  m_ldauTypeCombo->setCurrentIndex(ldauIdx);
  m_ldaulEdit->setText(QString::fromUtf8(vasp_gui.calc.ldaul ? vasp_gui.calc.ldaul : ""));
  m_ldauuEdit->setText(QString::fromUtf8(vasp_gui.calc.ldauu ? vasp_gui.calc.ldauu : ""));
  m_ldaujEdit->setText(QString::fromUtf8(vasp_gui.calc.ldauj ? vasp_gui.calc.ldauj : ""));

  m_ldipolCheck->setChecked(vasp_gui.calc.ldipol);

  /* Initial enabled states for ELECT-I toggles */
  if (vasp_gui.calc.non_collinear)
  {
    m_spinCheck->setChecked(false);
    m_spinCheck->setEnabled(false);
  } else
  {
    m_spinCheck->setEnabled(true);
  }
  if (vasp_gui.calc.lsorbit)
  {
    m_lnoncollCheck->setChecked(true);
    m_lnoncollCheck->setEnabled(true);
  }
  if (vasp_gui.calc.lmetagga)
  {
    m_metaggaCombo->setEnabled(true);
    bool mbj = (vasp_gui.calc.mgga == VMG_MBJ);
    m_cmbjEdit->setEnabled(mbj);
    m_cmbjaEdit->setEnabled(mbj);
    m_cmbjbEdit->setEnabled(mbj);
  } else
  {
    m_metaggaCombo->setEnabled(false);
    m_cmbjEdit->setEnabled(false);
    m_cmbjaEdit->setEnabled(false);
    m_cmbjbEdit->setEnabled(false);
  }
  m_ldauTypeCombo->setEnabled(vasp_gui.calc.ldau);
  m_ldaulEdit->setEnabled(vasp_gui.calc.ldau);
  m_ldauPrintCombo->setEnabled(vasp_gui.calc.ldau);
  m_ldauuEdit->setEnabled(vasp_gui.calc.ldau);
  m_ldaujEdit->setEnabled(vasp_gui.calc.ldau);

  int idipolIdx = 0;
  switch (vasp_gui.calc.idipol)
  {
  case VID_0:
    idipolIdx = 0;
    break;
  case VID_1:
    idipolIdx = 1;
    break;
  case VID_2:
    idipolIdx = 2;
    break;
  case VID_3:
    idipolIdx = 3;
    break;
  case VID_4:
    idipolIdx = 4;
    break;
  default:
    idipolIdx = 0;
    break;
  }
  m_idipolCombo->setCurrentIndex(idipolIdx);
  if (m_dipolEdit)
    m_dipolEdit->setText(QString::fromUtf8(vasp_gui.calc.dipol ? vasp_gui.calc.dipol : ""));
  m_epsilonEdit->setText(QString::number(vasp_gui.calc.epsilon, 'f', 4));
  m_efieldEdit->setText(QString::number(vasp_gui.calc.efield, 'f', 6));
  m_lmonoCheck->setChecked(vasp_gui.calc.lmono);

  /* Re-apply IDIPOL enabled state */
  bool dipolActive = (idipolIdx != 0);
  bool dipolAllAxis = (idipolIdx == 4);
  m_ldipolCheck->setEnabled(dipolActive);
  m_lmonoCheck->setEnabled(dipolActive);
  if (m_dipolEdit)
    m_dipolEdit->setEnabled(dipolActive);
  m_epsilonEdit->setEnabled(dipolActive);
  m_efieldEdit->setEnabled(dipolActive && !dipolAllAxis);

  /* ELECT-II */
  if (m_havePawCheck)
    m_havePawCheck->setChecked(vasp_gui.calc.have_paw);

  int lorbitIdx = 0;
  switch (vasp_gui.calc.lorbit)
  {
  case 0:
    lorbitIdx = 0;
    break;
  case 1:
    lorbitIdx = 1;
    break;
  case 2:
    lorbitIdx = 2;
    break;
  case 5:
    lorbitIdx = 3;
    break;
  case 10:
    lorbitIdx = 4;
    break;
  case 11:
    lorbitIdx = 5;
    break;
  case 12:
    lorbitIdx = 6;
    break;
  default:
    lorbitIdx = 0;
    break;
  }
  m_lorbitCombo->setCurrentIndex(lorbitIdx);
  m_nedosEdit->setText(QString::number(vasp_gui.calc.nedos));
  m_eminEdit->setText(QString::number(vasp_gui.calc.emin, 'f', 4));
  m_emaxEdit->setText(QString::number(vasp_gui.calc.emax, 'f', 4));
  m_efermiEdit->setText(QString::number(vasp_gui.calc.efermi, 'f', 6));
  m_rwigsEdit->setText(QString::fromUtf8(vasp_gui.calc.rwigs ? vasp_gui.calc.rwigs : ""));

  m_lopticsCheck->setChecked(vasp_gui.calc.loptics);
  m_lepsilonCheck->setChecked(vasp_gui.calc.lepsilon);
  m_lrpaCheck->setChecked(vasp_gui.calc.lrpa);
  m_lnablaCheck->setChecked(vasp_gui.calc.lnabla);
  m_cshiftEdit->setText(QString::number(vasp_gui.calc.cshift, 'f', 6));

  /* Initial enabled state for ELECT-II */
  bool dosEnabled = (lorbitIdx != 0);
  m_nedosEdit->setEnabled(dosEnabled);
  m_eminEdit->setEnabled(dosEnabled);
  m_emaxEdit->setEnabled(dosEnabled);
  m_efermiEdit->setEnabled(dosEnabled);
  bool rwigsDisabled = (lorbitIdx == 4 || lorbitIdx == 5 || lorbitIdx == 6);
  m_rwigsEdit->setEnabled(dosEnabled && !rwigsDisabled);

  m_ngxEdit->setText(QString::number(vasp_gui.calc.ngx));
  m_ngyEdit->setText(QString::number(vasp_gui.calc.ngy));
  m_ngzEdit->setText(QString::number(vasp_gui.calc.ngz));
  m_ngxfEdit->setText(QString::number(vasp_gui.calc.ngxf));
  m_ngyfEdit->setText(QString::number(vasp_gui.calc.ngyf));
  m_ngzfEdit->setText(QString::number(vasp_gui.calc.ngzf));

  /* IONIC */
  m_nswEdit->setText(QString::number(vasp_gui.calc.nsw));
  int ibrionIdx = 0;
  switch (vasp_gui.calc.ibrion)
  {
  case -1:
    ibrionIdx = 0;
    break;
  case 0:
    ibrionIdx = 1;
    break;
  case 1:
    ibrionIdx = 2;
    break;
  case 2:
    ibrionIdx = 3;
    break;
  case 3:
    ibrionIdx = 4;
    break;
  case 4:
    ibrionIdx = 5;
    break;
  case 5:
    ibrionIdx = 6;
    break;
  case 6:
    ibrionIdx = 7;
    break;
  case 7:
    ibrionIdx = 8;
    break;
  case 8:
    ibrionIdx = 9;
    break;
  case 44:
    ibrionIdx = 10;
    break;
  default:
    ibrionIdx = 0;
    break;
  }
  m_ibrionCombo->setCurrentIndex(ibrionIdx);

  int isifIdx = 0;
  switch (vasp_gui.calc.isif)
  {
  case 0:
    isifIdx = 0;
    break;
  case 1:
    isifIdx = 1;
    break;
  case 2:
    isifIdx = 2;
    break;
  case 3:
    isifIdx = 3;
    break;
  case 4:
    isifIdx = 4;
    break;
  case 5:
    isifIdx = 5;
    break;
  case 6:
    isifIdx = 6;
    break;
  case 7:
    isifIdx = 7;
    break;
  default:
    isifIdx = 0;
    break;
  }
  /* Block signals to prevent toggle handlers from overwriting ISIF during refresh */
  m_isifCombo->blockSignals(true);
  m_relaxIonsCheck->blockSignals(true);
  m_relaxShapeCheck->blockSignals(true);
  m_relaxVolumeCheck->blockSignals(true);

  m_isifCombo->setCurrentIndex(isifIdx);
  m_relaxIonsCheck->setChecked(vasp_gui.rions);
  m_relaxShapeCheck->setChecked(vasp_gui.rshape);
  m_relaxVolumeCheck->setChecked(vasp_gui.rvolume);

  m_isifCombo->blockSignals(false);
  m_relaxIonsCheck->blockSignals(false);
  m_relaxShapeCheck->blockSignals(false);
  m_relaxVolumeCheck->blockSignals(false);
  m_ediffgEdit->setText(QString::number(vasp_gui.calc.ediffg, 'f', 4));
  m_pstressEdit->setText(QString::number(vasp_gui.calc.pstress, 'f', 4));
  m_nfreeEdit->setText(QString::number(vasp_gui.calc.nfree));
  m_potimEdit->setText(QString::number(vasp_gui.calc.potim, 'f', 4));
  m_tebegEdit->setText(QString::number(vasp_gui.calc.tebeg, 'f', 4));
  m_teendEdit->setText(QString::number(vasp_gui.calc.teend, 'f', 4));
  m_smassEdit->setText(QString::number(vasp_gui.calc.smass, 'f', 4));
  m_nblockEdit->setText(QString::number(vasp_gui.calc.nblock));
  m_kblockEdit->setText(QString::number(vasp_gui.calc.kblock));
  m_npacoEdit->setText(QString::number(vasp_gui.calc.npaco));
  m_apacoEdit->setText(QString::number(vasp_gui.calc.apaco, 'f', 4));
  m_isymEdit->setText(QString::number(vasp_gui.calc.isym));
  m_symPrecEdit->setText(QString::number(vasp_gui.calc.sym_prec, 'f', 6));
  m_poscarA0Edit->setText(QString::number(vasp_gui.calc.poscar_a0, 'f', 4));
  int pfIdx = 0;
  switch (vasp_gui.calc.poscar_free)
  {
  case VPF_FIXED:
    pfIdx = 0;
    break;
  case VPF_FREE:
    pfIdx = 1;
    break;
  case VPF_MAN:
    pfIdx = 2;
    break;
  default:
    pfIdx = 1;
    break;
  }
  m_poscarFreeCombo->setCurrentIndex(pfIdx);
  m_poscarSdCheck->setChecked(vasp_gui.calc.poscar_sd);
  bool manual = (pfIdx == 2);
  m_poscarSdCheck->setEnabled(manual);
  m_poscarTxCheck->setEnabled(manual);
  m_poscarTyCheck->setEnabled(manual);
  m_poscarTzCheck->setEnabled(manual);
  m_poscarDirectCheck->setChecked(vasp_gui.calc.poscar_direct);
  m_poscarUxEdit->setText(QString::number(vasp_gui.calc.poscar_ux, 'f', 4));
  m_poscarUyEdit->setText(QString::number(vasp_gui.calc.poscar_uy, 'f', 4));
  m_poscarUzEdit->setText(QString::number(vasp_gui.calc.poscar_uz, 'f', 4));
  m_poscarVxEdit->setText(QString::number(vasp_gui.calc.poscar_vx, 'f', 4));
  m_poscarVyEdit->setText(QString::number(vasp_gui.calc.poscar_vy, 'f', 4));
  m_poscarVzEdit->setText(QString::number(vasp_gui.calc.poscar_vz, 'f', 4));
  m_poscarWxEdit->setText(QString::number(vasp_gui.calc.poscar_wx, 'f', 4));
  m_poscarWyEdit->setText(QString::number(vasp_gui.calc.poscar_wy, 'f', 4));
  m_poscarWzEdit->setText(QString::number(vasp_gui.calc.poscar_wz, 'f', 4));
  /* Populate ATOMS combo from model */
  updatePoscarAtoms();

  /* Initial enabled state for MD fields */
  bool mdEnabled = (ibrionIdx == 1); /* IBRION=0 */
  m_tebegEdit->setEnabled(mdEnabled);
  m_teendEdit->setEnabled(mdEnabled);
  m_nblockEdit->setEnabled(mdEnabled);
  m_kblockEdit->setEnabled(mdEnabled);
  m_npacoEdit->setEnabled(mdEnabled);
  m_apacoEdit->setEnabled(mdEnabled);
  bool smassEnabled = (ibrionIdx == 1 || ibrionIdx == 4); /* IBRION=0 or IBRION=3 */
  m_smassEdit->setEnabled(smassEnabled);

  /* KPOINTS — from INCAR */
  m_kpointsIsmearEdit->setText(QString::number(vasp_gui.calc.ismear));
  m_kpointsGammaCheck->setChecked(vasp_gui.calc.kgamma);
  m_kpointsKspacingEdit->setText(QString::number(vasp_gui.calc.kspacing, 'f', 4));

  /* KPOINTS — General */
  int kpModeIdx = 0;
  switch (vasp_gui.calc.kpoints_mode)
  {
  case VKP_MAN:
    kpModeIdx = 0;
    break;
  case VKP_LINE:
    kpModeIdx = 1;
    break;
  case VKP_AUTO:
    kpModeIdx = 2;
    break;
  case VKP_GAMMA:
    kpModeIdx = 3;
    break;
  case VKP_MP:
    kpModeIdx = 4;
    break;
  case VKP_BASIS:
    kpModeIdx = 5;
    break;
  default:
    kpModeIdx = 0;
    break;
  }
  m_kpointsModeCombo->setCurrentIndex(kpModeIdx);
  m_kpointsCartCheck->setChecked(vasp_gui.calc.kpoints_cart);
  m_kpointsKxSpin->setValue((int) vasp_gui.calc.kpoints_kx);
  m_kpointsKySpin->setValue((int) vasp_gui.calc.kpoints_ky);
  m_kpointsKzSpin->setValue((int) vasp_gui.calc.kpoints_kz);
  m_kpointsNkptsEdit->setText(QString::number(vasp_gui.calc.kpoints_nkpts));
  m_kpointsSxSpin->setValue(vasp_gui.calc.kpoints_sx);
  m_kpointsSyEdit->setText(QString::number(vasp_gui.calc.kpoints_sy, 'f', 4));
  m_kpointsSzEdit->setText(QString::number(vasp_gui.calc.kpoints_sz, 'f', 4));
  m_tetraCheck->setChecked(vasp_gui.calc.kpoints_tetra);

  /* KPOINTS — Tetrahedron */
  m_tetraTotalEdit->setText(QString::number(vasp_gui.calc.tetra_total));
  m_tetraVolumeEdit->setText(QString::number(vasp_gui.calc.tetra_volume, 'f', 6));
  m_tetraWEdit->setText(QString::number(vasp_gui.calc.tetra_w, 'f', 4));
  m_tetraPtAEdit->setText(QString::number(vasp_gui.calc.tetra_a));
  m_tetraPtBEdit->setText(QString::number(vasp_gui.calc.tetra_b));
  m_tetraPtCEdit->setText(QString::number(vasp_gui.calc.tetra_c));
  m_tetraPtDEdit->setText(QString::number(vasp_gui.calc.tetra_d));

  /* POTCAR */
  m_potcarSpeciesEdit->setText(QString::fromUtf8(vasp_gui.calc.species_symbols ? vasp_gui.calc.species_symbols : ""));
  m_potcarFileEdit->setText(QString::fromUtf8(vasp_gui.calc.potcar_file ? vasp_gui.calc.potcar_file : ""));
  m_potcarFlavorCombo->setCurrentIndex(0);
  m_potcarFolderEdit->setText(QString::fromUtf8(vasp_gui.calc.potcar_folder ? vasp_gui.calc.potcar_folder : ""));
  m_potcarDetectedSpeciesEdit->setText(
      QString::fromUtf8(vasp_gui.calc.potcar_species ? vasp_gui.calc.potcar_species : ""));
  m_potcarDetectedFlavorEdit->setText(
      QString::fromUtf8(vasp_gui.calc.potcar_species_flavor ? vasp_gui.calc.potcar_species_flavor : ""));

  /* EXEC */
  m_jobVaspExeEdit->setText(QString::fromUtf8(vasp_gui.calc.job_vasp_exe ? vasp_gui.calc.job_vasp_exe : ""));
  m_jobMpirunEdit->setText(QString::fromUtf8(vasp_gui.calc.job_mpirun ? vasp_gui.calc.job_mpirun : ""));
  m_jobPathEdit->setText(QString::fromUtf8(sysenv.cwd ? sysenv.cwd : ""));
  m_jobNprocSpin->setValue((int) vasp_gui.calc.job_nproc);
  m_ncoreSpin->setValue((int) vasp_gui.calc.ncore);
  m_kparSpin->setValue((int) vasp_gui.calc.kpar);
  m_lplaneCheck->setChecked(vasp_gui.calc.lplane);
  m_lscaluCheck->setChecked(vasp_gui.calc.lscalu);
  m_lscalapackCheck->setChecked(vasp_gui.calc.lscalapack);
  m_lwaveCheck->setChecked(vasp_gui.calc.lwave);
  m_lchargCheck->setChecked(vasp_gui.calc.lcharg);
  m_lvtotCheck->setChecked(vasp_gui.calc.lvtot);
  m_lvharCheck->setChecked(vasp_gui.calc.lvhar);
  m_lelfCheck->setChecked(vasp_gui.calc.lelf);

  /* VASP executable button */
  connect(m_jobVaspExeBtn, &QPushButton::clicked, this, [this]() {
    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::ExistingFile);
    dlg.setNameFilter("VASP executables (vasp* vasp_gam vasp_std vasp_ncl)");
    dlg.setWindowTitle("Select VASP executable");
    if (dlg.exec() == QDialog::Accepted)
    {
      QStringList files = dlg.selectedFiles();
      if (!files.isEmpty())
        m_jobVaspExeEdit->setText(files.first());
    }
  });

  /* EXEC button connections are now in constructor — removed duplicate from refresh() */
}

/* Helper: sync Qt widgets back to vasp_gui */
void VaspDialog::sync()
{

  /* PRESET — simple interface fields */
  vasp_gui.simple_calcul_idx = m_simpleCalculCombo->currentIndex();
  vasp_gui.simple_system_idx = m_simpleSystemCombo->currentIndex();
  vasp_gui.simple_rgeom = m_simpleRgeomCheck->isChecked();
  vasp_gui.dimension = m_simpleDimSpin->value();
  vasp_gui.simple_kgrid_idx = m_simpleKgridCombo->currentIndex();
  vasp_gui.calc.job_nproc = m_simpleNpSpin->value();
  vasp_gui.calc.ncore = m_simpleNcoreSpin->value();
  vasp_gui.calc.kpar = m_simpleKparSpin->value();

  /* CONVERGENCE */
  gchar *oldName = vasp_gui.calc.name;
  vasp_gui.calc.name = NULL;
  g_free(oldName);
  if (!m_nameEdit->text().isEmpty())
    vasp_gui.calc.name = g_strdup(m_nameEdit->text().toUtf8().constData());
  else if (oldName == NULL)
    vasp_gui.calc.name = g_strdup("UNNAMED MODEL");

  if (vasp_gui.calc.prec != VP_NORM)
  {
    int idx = m_precCombo->currentIndex();
    switch (idx)
    {
    case 1:
      vasp_gui.calc.prec = VP_SINGLE;
      break;
    case 2:
      vasp_gui.calc.prec = VP_ACCURATE;
      break;
    case 3:
      vasp_gui.calc.prec = VP_HIGH;
      break;
    case 4:
      vasp_gui.calc.prec = VP_MED;
      break;
    case 5:
      vasp_gui.calc.prec = VP_LOW;
      break;
    default:
      vasp_gui.calc.prec = VP_NORM;
      break;
    }
  }

  /* Parse numeric fields */
  bool ok;
  vasp_gui.calc.encut = m_encutEdit->text().toDouble(&ok);
  vasp_gui.calc.enaug = m_enaugEdit->text().toDouble(&ok);
  vasp_gui.calc.ediff = m_ediffEdit->text().toDouble(&ok);
  vasp_gui.calc.use_prec = m_usePrecCheck->isChecked();

  int algoIdx = m_algoCombo->currentIndex();
  switch (algoIdx)
  {
  case 1:
    vasp_gui.calc.algo = VA_IALGO;
    break;
  case 2:
    vasp_gui.calc.algo = VA_VERYFAST;
    break;
  case 3:
    vasp_gui.calc.algo = VA_FAST;
    break;
  case 4:
    vasp_gui.calc.algo = VA_CONJ;
    break;
  case 5:
    vasp_gui.calc.algo = VA_ALL;
    break;
  case 6:
    vasp_gui.calc.algo = VA_DAMPED;
    break;
  case 7:
    vasp_gui.calc.algo = VA_SUBROT;
    break;
  case 8:
    vasp_gui.calc.algo = VA_EIGEN;
    break;
  case 9:
    vasp_gui.calc.algo = VA_NONE;
    break;
  case 10:
    vasp_gui.calc.algo = VA_NOTHING;
    break;
  case 11:
    vasp_gui.calc.algo = VA_EXACT;
    break;
  case 12:
    vasp_gui.calc.algo = VA_DIAG;
    break;
  default:
    vasp_gui.calc.algo = VA_NORM;
    break;
  }

  int ialgoIdx = m_ialgoCombo->currentIndex();
  switch (ialgoIdx)
  {
  case 0:
    vasp_gui.calc.ialgo = VIA_OE_FIXED;
    break;
  case 1:
    vasp_gui.calc.ialgo = VIA_O_FIXED;
    break;
  case 2:
    vasp_gui.calc.ialgo = VIA_SUBROT;
    break;
  case 3:
    vasp_gui.calc.ialgo = VIA_STEEP;
    break;
  case 4:
    vasp_gui.calc.ialgo = VIA_CG;
    break;
  case 5:
    vasp_gui.calc.ialgo = VIA_PSTEEP;
    break;
  case 6:
    vasp_gui.calc.ialgo = VIA_PCG;
    break;
  case 7:
    vasp_gui.calc.ialgo = VIA_KOSUGI;
    break;
  case 8:
    vasp_gui.calc.ialgo = VIA_ESTEEP;
    break;
  case 9:
    vasp_gui.calc.ialgo = VIA_RMMP;
    break;
  case 10:
    vasp_gui.calc.ialgo = VIA_PRMM;
    break;
  case 11:
    vasp_gui.calc.ialgo = VIA_VAR_DAMP;
    break;
  case 12:
    vasp_gui.calc.ialgo = VIA_VAR_QUENCH;
    break;
  case 13:
    vasp_gui.calc.ialgo = VIA_VAR_PCG;
    break;
  case 14:
    vasp_gui.calc.ialgo = VIA_EXACT;
    break;
  default:
    vasp_gui.calc.ialgo = VIA_KOSUGI;
    break;
  }

  vasp_gui.calc.ldiag = m_ldiagCheck->isChecked();
  vasp_gui.calc.nsim = m_nsimEdit->text().toInt(&ok);
  vasp_gui.calc.vtime = m_vtimeEdit->text().toDouble(&ok);
  vasp_gui.calc.iwavpr = m_iwavprEdit->text().toInt(&ok);
  vasp_gui.calc.auto_elec = m_autoElecCheck->isChecked();
  vasp_gui.calc.nbands = m_nbandsEdit->text().toInt(&ok);
  vasp_gui.calc.nelect = m_nelectEdit->text().toDouble(&ok);
  vasp_gui.calc.iniwav = m_iniwavCheck->isChecked();
  vasp_gui.calc.istart = m_istartEdit->text().toInt(&ok);
  vasp_gui.calc.icharg = m_ichargEdit->text().toInt(&ok);

  int mixerIdx = m_mixerCombo->currentIndex();
  switch (mixerIdx)
  {
  case 1:
    vasp_gui.calc.imix = VM_1;
    break;
  case 2:
    vasp_gui.calc.imix = VM_2;
    break;
  case 3:
    vasp_gui.calc.imix = VM_4;
    break;
  default:
    vasp_gui.calc.imix = VM_0;
    break;
  }

  QString nelmTxt = m_nelmEdit->text().trimmed();
  if (!nelmTxt.isEmpty())
  {
    vasp_gui.calc.nelm = nelmTxt.toInt(&ok);
  }
  QString nelmdlTxt = m_nelmdlEdit->text().trimmed();
  if (!nelmdlTxt.isEmpty())
  {
    vasp_gui.calc.nelmdl = nelmdlTxt.toInt(&ok);
  }
  QString nelminTxt = m_nelminEdit->text().trimmed();
  if (!nelminTxt.isEmpty())
  {
    vasp_gui.calc.nelmin = nelminTxt.toInt(&ok);
  }

  int mixpreIdx = m_mixpreCombo->currentIndex();
  switch (mixpreIdx)
  {
  case 1:
    vasp_gui.calc.mixpre = VMP_1;
    break;
  case 2:
    vasp_gui.calc.mixpre = VMP_2;
    break;
  default:
    vasp_gui.calc.mixpre = VMP_0;
    break;
  }

  vasp_gui.calc.amix = m_amixEdit->text().toDouble(&ok);
  vasp_gui.calc.bmix = m_bmixEdit->text().toDouble(&ok);
  vasp_gui.calc.amin = m_aminEdit->text().toDouble(&ok);

  int inimixIdx = m_inimixCombo->currentIndex();
  switch (inimixIdx)
  {
  case 1:
    vasp_gui.calc.inimix = VIM_1;
    break;
  default:
    vasp_gui.calc.inimix = VIM_0;
    break;
  }

  vasp_gui.calc.maxmix = m_maxmixEdit->text().toInt(&ok);
  vasp_gui.calc.amix_mag = m_amixMagEdit->text().toDouble(&ok);
  vasp_gui.calc.bmix_mag = m_bmixMagEdit->text().toDouble(&ok);
  vasp_gui.calc.wc = m_wcEdit->text().toDouble(&ok);
  vasp_gui.calc.lmaxmix = m_lmaxmixEdit->text().toInt(&ok);
  vasp_gui.calc.lmaxpaw = m_lmaxpawEdit->text().toInt(&ok);
  vasp_gui.calc.addgrid = m_addgridCheck->isChecked();
  vasp_gui.calc.auto_grid = m_autoGridCheck->isChecked();

  /* ELECT-I */
  vasp_gui.calc.ismear = m_ismearEdit->text().toInt(&ok);

  vasp_gui.calc.sigma = m_sigmaEdit->text().toDouble(&ok);
  vasp_gui.calc.kgamma = m_kgammaCheck->isChecked();
  vasp_gui.calc.kspacing = m_kspacingEdit->text().toDouble(&ok);
  g_free(vasp_gui.calc.fermwe);
  QString fermweText = m_fermweEdit->text().trimmed();
  vasp_gui.calc.fermwe = fermweText.isEmpty() ? NULL : g_strdup(fermweText.toUtf8().constData());
  g_free(vasp_gui.calc.fermdo);
  QString fermdoText = m_fermdoEdit->text().trimmed();
  vasp_gui.calc.fermdo = fermdoText.isEmpty() ? NULL : g_strdup(fermdoText.toUtf8().constData());

  int lrealIdx = m_lrealCombo->currentIndex();
  switch (lrealIdx)
  {
  case 1:
    vasp_gui.calc.lreal = VLR_ON;
    break;
  case 2:
    vasp_gui.calc.lreal = VLR_TRUE;
    break;
  case 3:
    vasp_gui.calc.lreal = VLR_FALSE;
    break;
  default:
    vasp_gui.calc.lreal = VLR_AUTO;
    break;
  }

  g_free(vasp_gui.calc.ropt);
  vasp_gui.calc.ropt = NULL;
  if (!m_convRoptEdit->text().isEmpty())
    vasp_gui.calc.ropt = g_strdup(m_convRoptEdit->text().toUtf8().constData());

  int ggaIdx = m_ggaCombo->currentIndex();
  switch (ggaIdx)
  {
  case 0:
    vasp_gui.calc.gga = VG_91;
    break;
  case 1:
    vasp_gui.calc.gga = VG_PE;
    break;
  case 2:
    vasp_gui.calc.gga = VG_RP;
    break;
  case 3:
    vasp_gui.calc.gga = VG_AM;
    break;
  case 4:
    vasp_gui.calc.gga = VG_PS;
    break;
  default:
    vasp_gui.calc.gga = VG_91;
    break;
  }

  vasp_gui.calc.voskown = m_voskownCheck->isChecked();
  vasp_gui.calc.lasph = m_lasphCheck->isChecked();
  if (m_lmixtauCheck)
    vasp_gui.calc.lmixtau = m_lmixtauCheck->isChecked();
  if (m_lmaxtauEdit)
    vasp_gui.calc.lmaxtau = m_lmaxtauEdit->text().toInt(&ok);
  g_free(vasp_gui.calc.cmbj);
  vasp_gui.calc.cmbj = NULL;
  if (!m_cmbjEdit->text().isEmpty())
    vasp_gui.calc.cmbj = g_strdup(m_cmbjEdit->text().toUtf8().constData());
  if (m_cmbjaEdit)
    vasp_gui.calc.cmbja = m_cmbjaEdit->text().toDouble(&ok);
  if (m_cmbjbEdit)
    vasp_gui.calc.cmbjb = m_cmbjbEdit->text().toDouble(&ok);
  if (m_spinCheck)
    vasp_gui.calc.ispin = m_spinCheck->isChecked();
  if (m_lnoncollCheck)
    vasp_gui.calc.non_collinear = m_lnoncollCheck->isChecked();
  vasp_gui.calc.lsorbit = m_lsorbitCheck->isChecked();
  if (m_ggaCompatCheck)
    vasp_gui.calc.gga_compat = m_ggaCompatCheck->isChecked();
  g_free(vasp_gui.calc.saxis);
  vasp_gui.calc.saxis = NULL;
  if (!m_saxisEdit->text().isEmpty())
    vasp_gui.calc.saxis = g_strdup(m_saxisEdit->text().toUtf8().constData());
  if (m_nupdownEdit)
    vasp_gui.calc.nupdown = m_nupdownEdit->text().toDouble(&ok);
  g_free(vasp_gui.calc.magmom);
  vasp_gui.calc.magmom = NULL;
  if (!m_magmomEdit->text().isEmpty())
    vasp_gui.calc.magmom = g_strdup(m_magmomEdit->text().toUtf8().constData());
  vasp_gui.calc.lmetagga = m_lmetaggaCheck->isChecked();

  int ldauIdx = m_ldauTypeCombo->currentIndex();
  switch (ldauIdx)
  {
  case 1:
    vasp_gui.calc.ldau_type = VU_2;
    break;
  case 2:
    vasp_gui.calc.ldau_type = VU_1;
    break;
  case 3:
    vasp_gui.calc.ldau_type = VU_4;
    break;
  default:
    vasp_gui.calc.ldau_type = VU_2;
    break;
  }

  g_free(vasp_gui.calc.ldaul);
  vasp_gui.calc.ldaul = NULL;
  if (m_ldaulEdit)
    vasp_gui.calc.ldaul = g_strdup(m_ldaulEdit->text().toUtf8().constData());
  g_free(vasp_gui.calc.ldauu);
  vasp_gui.calc.ldauu = NULL;
  if (m_ldauuEdit)
    vasp_gui.calc.ldauu = g_strdup(m_ldauuEdit->text().toUtf8().constData());
  g_free(vasp_gui.calc.ldauj);
  vasp_gui.calc.ldauj = g_strdup(m_ldaujEdit->text().toUtf8().constData());

  g_free(vasp_gui.calc.dipol);
  vasp_gui.calc.dipol = g_strdup(m_dipolEdit->text().isEmpty() ? "" : m_dipolEdit->text().toUtf8().constData());
  vasp_gui.calc.ldipol = m_ldipolCheck->isChecked();
  int idipolIdx = m_idipolCombo->currentIndex();
  switch (idipolIdx)
  {
  case 1:
    vasp_gui.calc.idipol = VID_1;
    break;
  case 2:
    vasp_gui.calc.idipol = VID_2;
    break;
  case 3:
    vasp_gui.calc.idipol = VID_3;
    break;
  default:
    vasp_gui.calc.idipol = VID_0;
    break;
  }

  vasp_gui.calc.epsilon = m_epsilonEdit->text().toDouble(&ok);
  vasp_gui.calc.efield = m_efieldEdit->text().toDouble(&ok);
  vasp_gui.calc.lmono = m_lmonoCheck->isChecked();

  /* ELECT-II */
  if (m_havePawCheck)
    vasp_gui.calc.have_paw = m_havePawCheck->isChecked();

  int lorbitIdx = m_lorbitCombo->currentIndex();
  switch (lorbitIdx)
  {
  case 0:
    vasp_gui.calc.lorbit = 0;
    break;
  case 1:
    vasp_gui.calc.lorbit = 1;
    break;
  case 2:
    vasp_gui.calc.lorbit = 2;
    break;
  case 3:
    vasp_gui.calc.lorbit = 5;
    break;
  case 4:
    vasp_gui.calc.lorbit = 10;
    break;
  case 5:
    vasp_gui.calc.lorbit = 11;
    break;
  case 6:
    vasp_gui.calc.lorbit = 12;
    break;
  default:
    vasp_gui.calc.lorbit = 0;
    break;
  }

  vasp_gui.calc.nedos = m_nedosEdit->text().toInt(&ok);
  if (!m_eminEdit->text().isEmpty())
    vasp_gui.calc.emin = m_eminEdit->text().toDouble(&ok);
  if (!m_emaxEdit->text().isEmpty())
    vasp_gui.calc.emax = m_emaxEdit->text().toDouble(&ok);
  vasp_gui.calc.efermi = m_efermiEdit->text().toDouble(&ok);
  g_free(vasp_gui.calc.rwigs);
  vasp_gui.calc.rwigs = NULL;
  if (!m_rwigsEdit->text().isEmpty())
    vasp_gui.calc.rwigs = g_strdup(m_rwigsEdit->text().toUtf8().constData());

  vasp_gui.calc.loptics = m_lopticsCheck->isChecked();
  vasp_gui.calc.lepsilon = m_lepsilonCheck->isChecked();
  vasp_gui.calc.lrpa = m_lrpaCheck->isChecked();
  vasp_gui.calc.lnabla = m_lnablaCheck->isChecked();
  vasp_gui.calc.cshift = m_cshiftEdit->text().toDouble(&ok);

  vasp_gui.calc.ngx = m_ngxEdit->text().toInt(&ok);
  vasp_gui.calc.ngy = m_ngyEdit->text().toInt(&ok);
  vasp_gui.calc.ngz = m_ngzEdit->text().toInt(&ok);
  vasp_gui.calc.ngxf = m_ngxfEdit->text().toInt(&ok);
  vasp_gui.calc.ngyf = m_ngyfEdit->text().toInt(&ok);
  vasp_gui.calc.ngzf = m_ngzfEdit->text().toInt(&ok);

  /* IONIC */
  if (!m_nswEdit->text().isEmpty())
    vasp_gui.calc.nsw = m_nswEdit->text().toInt(&ok);
  int ibrionIdx = m_ibrionCombo->currentIndex();
  switch (ibrionIdx)
  {
  case 0:
    vasp_gui.calc.ibrion = -1;
    break;
  case 1:
    vasp_gui.calc.ibrion = 0;
    break;
  case 2:
    vasp_gui.calc.ibrion = 1;
    break;
  case 3:
    vasp_gui.calc.ibrion = 2;
    break;
  case 4:
    vasp_gui.calc.ibrion = 3;
    break;
  case 5:
    vasp_gui.calc.ibrion = 4;
    break;
  case 6:
    vasp_gui.calc.ibrion = 5;
    break;
  case 7:
    vasp_gui.calc.ibrion = 6;
    break;
  case 8:
    vasp_gui.calc.ibrion = 7;
    break;
  case 9:
    vasp_gui.calc.ibrion = 8;
    break;
  case 10:
    vasp_gui.calc.ibrion = 44;
    break;
  default:
    vasp_gui.calc.ibrion = -1;
    break;
  }

  int isifIdx = m_isifCombo->currentIndex();
  switch (isifIdx)
  {
  case 0:
    vasp_gui.calc.isif = 0;
    break;
  case 1:
    vasp_gui.calc.isif = 1;
    break;
  case 2:
    vasp_gui.calc.isif = 2;
    break;
  case 3:
    vasp_gui.calc.isif = 3;
    break;
  case 4:
    vasp_gui.calc.isif = 4;
    break;
  case 5:
    vasp_gui.calc.isif = 5;
    break;
  case 6:
    vasp_gui.calc.isif = 6;
    break;
  case 7:
    vasp_gui.calc.isif = 7;
    break;
  default:
    vasp_gui.calc.isif = 0;
    break;
  }

  vasp_gui.rions = m_relaxIonsCheck->isChecked();
  vasp_gui.rshape = m_relaxShapeCheck->isChecked();
  vasp_gui.rvolume = m_relaxVolumeCheck->isChecked();
  /* Derive ISIF from relax checkboxes */
  if (vasp_gui.rions && !vasp_gui.rshape && !vasp_gui.rvolume)
    vasp_gui.calc.isif = 2;
  else if (vasp_gui.rions && vasp_gui.rshape && vasp_gui.rvolume)
    vasp_gui.calc.isif = 3;
  else if (vasp_gui.rions && vasp_gui.rshape && !vasp_gui.rvolume)
    vasp_gui.calc.isif = 4;
  else if (!vasp_gui.rions && vasp_gui.rshape && !vasp_gui.rvolume)
    vasp_gui.calc.isif = 5;
  else if (!vasp_gui.rions && vasp_gui.rshape && vasp_gui.rvolume)
    vasp_gui.calc.isif = 6;
  else if (!vasp_gui.rions && !vasp_gui.rshape && vasp_gui.rvolume)
    vasp_gui.calc.isif = 7;
  else if (vasp_gui.rions && !vasp_gui.rshape && vasp_gui.rvolume)
    vasp_gui.calc.isif = 7;
  else if (!vasp_gui.rions && !vasp_gui.rshape && !vasp_gui.rvolume)
    vasp_gui.calc.isif = 2;
  vasp_gui.calc.ediffg = m_ediffgEdit->text().toDouble(&ok);
  vasp_gui.calc.pstress = m_pstressEdit->text().toDouble(&ok);
  if (!m_nfreeEdit->text().isEmpty())
    vasp_gui.calc.nfree = m_nfreeEdit->text().toInt(&ok);
  vasp_gui.calc.potim = m_potimEdit->text().toDouble(&ok);
  vasp_gui.calc.tebeg = m_tebegEdit->text().toDouble(&ok);
  vasp_gui.calc.teend = m_teendEdit->text().toDouble(&ok);
  vasp_gui.calc.smass = m_smassEdit->text().toDouble(&ok);
  vasp_gui.calc.nblock = m_nblockEdit->text().toInt(&ok);
  if (!m_kblockEdit->text().isEmpty())
    vasp_gui.calc.kblock = m_kblockEdit->text().toInt(&ok);
  vasp_gui.calc.npaco = m_npacoEdit->text().toInt(&ok);
  vasp_gui.calc.apaco = m_apacoEdit->text().toDouble(&ok);
  vasp_gui.calc.isym = m_isymEdit->text().toInt(&ok);
  vasp_gui.calc.sym_prec = m_symPrecEdit->text().toDouble(&ok);
  vasp_gui.calc.poscar_a0 = m_poscarA0Edit->text().toDouble(&ok);
  /* Only write poscar_free if user changed it from default (FIXED) */
  int pfIdx = m_poscarFreeCombo->currentIndex();
  if (pfIdx != 0)
  {
    switch (pfIdx)
    {
    case 1:
      vasp_gui.calc.poscar_free = VPF_FREE;
      break;
    case 2:
      vasp_gui.calc.poscar_free = VPF_MAN;
      break;
    default:
      vasp_gui.calc.poscar_free = VPF_FREE;
      break;
    }
  }
  vasp_gui.calc.poscar_sd = m_poscarSdCheck->isChecked();
  vasp_gui.calc.poscar_direct = m_poscarDirectCheck->isChecked();
  /* Only write lattice vectors if non-empty (preserve model values) */
  if (!m_poscarUxEdit->text().isEmpty())
    vasp_gui.calc.poscar_ux = m_poscarUxEdit->text().toDouble(&ok);
  if (!m_poscarUyEdit->text().isEmpty())
    vasp_gui.calc.poscar_uy = m_poscarUyEdit->text().toDouble(&ok);
  if (!m_poscarUzEdit->text().isEmpty())
    vasp_gui.calc.poscar_uz = m_poscarUzEdit->text().toDouble(&ok);
  if (!m_poscarVxEdit->text().isEmpty())
    vasp_gui.calc.poscar_vx = m_poscarVxEdit->text().toDouble(&ok);
  if (!m_poscarVyEdit->text().isEmpty())
    vasp_gui.calc.poscar_vy = m_poscarVyEdit->text().toDouble(&ok);
  if (!m_poscarVzEdit->text().isEmpty())
    vasp_gui.calc.poscar_vz = m_poscarVzEdit->text().toDouble(&ok);
  if (!m_poscarWxEdit->text().isEmpty())
    vasp_gui.calc.poscar_wx = m_poscarWxEdit->text().toDouble(&ok);
  if (!m_poscarWyEdit->text().isEmpty())
    vasp_gui.calc.poscar_wy = m_poscarWyEdit->text().toDouble(&ok);
  if (!m_poscarWzEdit->text().isEmpty())
    vasp_gui.calc.poscar_wz = m_poscarWzEdit->text().toDouble(&ok);
  /* poscar_tx/ty/tz are per-atom, stored in selective arrays — don't overwrite with global sync */

  /* KPOINTS — from INCAR + General + Tetrahedron */
  vasp_gui.calc.ismear = m_kpointsIsmearEdit->text().toInt(&ok);
  vasp_gui.calc.kgamma = m_kpointsGammaCheck->isChecked();
  vasp_gui.calc.kspacing = m_kpointsKspacingEdit->text().toDouble(&ok);

  {
    int kpModeIdx = m_kpointsModeCombo->currentIndex();
    switch (kpModeIdx)
    {
    case 0:
      vasp_gui.calc.kpoints_mode = VKP_MAN;
      break;
    case 1:
      vasp_gui.calc.kpoints_mode = VKP_LINE;
      break;
    case 2:
      vasp_gui.calc.kpoints_mode = VKP_AUTO;
      break;
    case 3:
      vasp_gui.calc.kpoints_mode = VKP_GAMMA;
      break;
    case 4:
      vasp_gui.calc.kpoints_mode = VKP_MP;
      break;
    case 5:
      vasp_gui.calc.kpoints_mode = VKP_BASIS;
      break;
    default:
      vasp_gui.calc.kpoints_mode = VKP_MAN;
      break;
    }
  }
  vasp_gui.calc.kpoints_cart = m_kpointsCartCheck->isChecked();
  vasp_gui.calc.kpoints_kx = m_kpointsKxSpin->value();
  vasp_gui.calc.kpoints_ky = m_kpointsKySpin->value();
  vasp_gui.calc.kpoints_kz = m_kpointsKzSpin->value();
  vasp_gui.calc.kpoints_nkpts = m_kpointsNkptsEdit->text().toInt();
  vasp_gui.calc.kpoints_sx = m_kpointsSxSpin->value();
  vasp_gui.calc.kpoints_sy = m_kpointsSyEdit->text().toDouble(&ok);
  vasp_gui.calc.kpoints_sz = m_kpointsSzEdit->text().toDouble(&ok);
  vasp_gui.calc.kpoints_tetra = m_tetraCheck->isChecked();

  /* Sync k-point list from combo box */
  g_free(vasp_gui.calc.kpoints_list_x);
  g_free(vasp_gui.calc.kpoints_list_y);
  g_free(vasp_gui.calc.kpoints_list_z);
  g_free(vasp_gui.calc.kpoints_list_w);
  vasp_gui.calc.kpoints_list_count = 0;
  vasp_gui.calc.kpoints_list_x = nullptr;
  vasp_gui.calc.kpoints_list_y = nullptr;
  vasp_gui.calc.kpoints_list_z = nullptr;
  vasp_gui.calc.kpoints_list_w = nullptr;
  {
    int count = 0;
    for (int i = 0; i < m_kpointsKptsCombo->count(); i++)
    {
      QString t = m_kpointsKptsCombo->itemText(i);
      if (t != "ADD kpoint")
        count++;
    }
    if (count > 0)
    {
      vasp_gui.calc.kpoints_list_count = count;
      vasp_gui.calc.kpoints_list_x = g_new0(gdouble, count);
      vasp_gui.calc.kpoints_list_y = g_new0(gdouble, count);
      vasp_gui.calc.kpoints_list_z = g_new0(gdouble, count);
      vasp_gui.calc.kpoints_list_w = g_new0(gdouble, count);
      int idx = 0;
      for (int i = 0; i < m_kpointsKptsCombo->count(); i++)
      {
        QString t = m_kpointsKptsCombo->itemText(i);
        if (t == "ADD kpoint")
          continue;
        bool ok2;
        QStringList parts = t.split(' ', Qt::SkipEmptyParts);
        if (parts.size() >= 3)
        {
          vasp_gui.calc.kpoints_list_x[idx] = parts[0].toDouble(&ok2);
          vasp_gui.calc.kpoints_list_y[idx] = parts[1].toDouble(&ok2);
          vasp_gui.calc.kpoints_list_z[idx] = parts[2].toDouble(&ok2);
          if (vasp_gui.calc.kpoints_mode != VKP_LINE && parts.size() >= 4)
          {
            vasp_gui.calc.kpoints_list_w[idx] = parts[3].toDouble(&ok2);
          }
        }
        idx++;
      }
    }
  }

  /* Sync tetrahedron list from combo box */
  {
    int count = 0;
    for (int i = 0; i < m_tetraCombo->count(); i++)
    {
      QString t = m_tetraCombo->itemText(i);
      if (t != "ADD tetrahedron")
        count++;
    }
    vasp_gui.calc.tetra_total = count;
  }

  /* tetra_volume, tetra_w, tetra_a/b/c/d are for the currently selected tetrahedron */
  vasp_gui.calc.tetra_volume = m_tetraVolumeEdit->text().toDouble(&ok);
  vasp_gui.calc.tetra_w = m_tetraWEdit->text().toDouble(&ok);
  vasp_gui.calc.tetra_a = m_tetraPtAEdit->text().toInt(&ok);
  vasp_gui.calc.tetra_b = m_tetraPtBEdit->text().toInt(&ok);
  vasp_gui.calc.tetra_c = m_tetraPtCEdit->text().toInt(&ok);
  vasp_gui.calc.tetra_d = m_tetraPtDEdit->text().toInt(&ok);

  /* POTCAR */
  g_free(vasp_gui.calc.potcar_file);
  vasp_gui.calc.potcar_file = g_strdup(m_potcarFileEdit->text().toUtf8().constData());

  /* potcar_flavor is stored as a string in potcar_species_flavor */
  /* No potcar_flavor enum exists in the struct */

  g_free(vasp_gui.calc.potcar_folder);
  vasp_gui.calc.potcar_folder = g_strdup(m_potcarFolderEdit->text().toUtf8().constData());

  vasp_gui.have_potcar_folder = m_potcarSelectFolderRadio->isChecked();

  /* EXEC */
  g_free(vasp_gui.calc.job_vasp_exe);
  vasp_gui.calc.job_vasp_exe = g_strdup(m_jobVaspExeEdit->text().toUtf8().constData());
  g_free(vasp_gui.calc.job_mpirun);
  vasp_gui.calc.job_mpirun = g_strdup(m_jobMpirunEdit->text().toUtf8().constData());
  g_free(vasp_gui.calc.job_path);
  vasp_gui.calc.job_path = g_strdup(m_jobPathEdit->text().toUtf8().constData());
  vasp_gui.calc.job_nproc = m_jobNprocSpin->value();
  vasp_gui.calc.ncore = m_ncoreSpin->value();
  vasp_gui.calc.kpar = m_kparSpin->value();
  vasp_gui.calc.lplane = m_lplaneCheck->isChecked();
  vasp_gui.calc.lscalu = m_lscaluCheck->isChecked();
  vasp_gui.calc.lscalapack = m_lscalapackCheck->isChecked();
  vasp_gui.calc.lwave = m_lwaveCheck->isChecked();
  vasp_gui.calc.lcharg = m_lchargCheck->isChecked();
  vasp_gui.calc.lvtot = m_lvtotCheck->isChecked();
  vasp_gui.calc.lvhar = m_lvharCheck->isChecked();
  vasp_gui.calc.lelf = m_lelfCheck->isChecked();
}

void VaspDialog::setupPresetPage()
{
  auto *page = new QWidget();
  auto *mainLayout = new QVBoxLayout(page);
  mainLayout->setSpacing(8);

  /* General */
  {
    auto *frame = new QGroupBox(tr("General"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    /* Row 1: CALCUL | ROUGH GEOM. | DIM */
    m_simpleCalculCombo = new QComboBox();
    m_simpleCalculCombo->addItems({tr("Energy (single point)"), tr("DOS/BANDS (single point)"),
                                   tr("Lattice Dynamics (opt.)"), tr("Geometry (opt.)"),
                                   tr("Molecular Dynamics (opt.)")});
    auto *row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("CALCUL:")));
    row1->addWidget(m_simpleCalculCombo);
    row1->addSpacing(20);
    m_simpleRgeomCheck = new QCheckBox(tr("ROUGH GEOM."));
    row1->addWidget(m_simpleRgeomCheck);
    row1->addSpacing(20);
    m_simpleDimSpin = new QSpinBox();
    m_simpleDimSpin->setRange(0, 3);
    row1->addWidget(new QLabel(tr("DIM:")));
    row1->addWidget(m_simpleDimSpin);
    vbox->addLayout(row1);

    /* Row 2: SYSTEM | POSCAR */
    m_simpleSystemCombo = new QComboBox();
    m_simpleSystemCombo->addItems({tr("Metal"), tr("Semi-conducting"), tr("Insulator"), tr("Unknown")});
    auto *row2 = new QHBoxLayout();
    row2->addWidget(new QLabel(tr("SYSTEM:")));
    row2->addWidget(m_simpleSystemCombo);
    row2->addSpacing(20);
    m_simplePoscarEdit = new QLineEdit();
    m_simplePoscarEdit->setReadOnly(true);
    row2->addWidget(new QLabel(tr("POSCAR:")));
    row2->addWidget(m_simplePoscarEdit);
    vbox->addLayout(row2);

    /* Row 3: KPT GRID | POTCAR */
    m_simpleKgridCombo = new QComboBox();
    m_simpleKgridCombo->addItems({tr("COARSE"), tr("MEDIUM"), tr("FINE"), tr("ULTRA-FINE")});
    auto *row3 = new QHBoxLayout();
    row3->addWidget(new QLabel(tr("KPT GRID:")));
    row3->addWidget(m_simpleKgridCombo);
    row3->addSpacing(20);
    m_simpleSpeciesEdit = new QLineEdit();
    m_simpleSpeciesEdit->setReadOnly(true);
    row3->addWidget(new QLabel(tr("POTCAR:")));
    row3->addWidget(m_simpleSpeciesEdit);
    vbox->addLayout(row3);

    /* Row 4: POTCAR FILE | ... */
    m_simplePotcarEdit = new QLineEdit();
    m_simplePotcarBtn = new QPushButton(tr("..."));
    auto *row4 = new QHBoxLayout();
    row4->addWidget(new QLabel(tr("POTCAR FILE:")));
    row4->addWidget(m_simplePotcarEdit);
    row4->addWidget(m_simplePotcarBtn);
    vbox->addLayout(row4);

    /* Row 5: EXEC: NP= | NCORE= | KPAR= */
    auto *row5 = new QHBoxLayout();
    row5->addWidget(new QLabel(tr("EXEC:")));
    m_simpleNpSpin = new QSpinBox();
    m_simpleNpSpin->setRange(1, 256);
    row5->addWidget(new QLabel(tr("NP=")));
    row5->addWidget(m_simpleNpSpin);
    row5->addSpacing(10);
    m_simpleNcoreSpin = new QSpinBox();
    m_simpleNcoreSpin->setRange(1, 256);
    row5->addWidget(new QLabel(tr("NCORE=")));
    row5->addWidget(m_simpleNcoreSpin);
    row5->addSpacing(10);
    m_simpleKparSpin = new QSpinBox();
    m_simpleKparSpin->setRange(1, 256);
    row5->addWidget(new QLabel(tr("KPAR=")));
    row5->addWidget(m_simpleKparSpin);
    vbox->addLayout(row5);

    /* Connect NP spinner to constrain NCORE and KPAR */
    connect(m_simpleNpSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int np) {
      /* NCORE ∈ [1, NP/KPAR], KPAR ∈ [1, NP] */
      int kpar = m_simpleKparSpin->value();
      int maxNcore = (kpar > 0) ? np / kpar : 1;
      if (maxNcore < 1)
        maxNcore = 1;
      m_simpleNcoreSpin->setRange(1, maxNcore);
      m_simpleKparSpin->setRange(1, np);
      /* Clamp current values */
      if (m_simpleNcoreSpin->value() > maxNcore)
        m_simpleNcoreSpin->setValue(maxNcore);
      if (m_simpleKparSpin->value() > np)
        m_simpleKparSpin->setValue(np);
      /* Sync to vasp_gui.calc */
      vasp_gui.calc.job_nproc = np;
      vasp_gui.calc.ncore = m_simpleNcoreSpin->value();
      vasp_gui.calc.kpar = m_simpleKparSpin->value();
    });

    /* Also constrain when KPAR changes */
    connect(m_simpleKparSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int kpar) {
      int np = m_simpleNpSpin->value();
      int maxNcore = (kpar > 0) ? np / kpar : 1;
      if (maxNcore < 1)
        maxNcore = 1;
      m_simpleNcoreSpin->setRange(1, maxNcore);
      if (m_simpleNcoreSpin->value() > maxNcore)
        m_simpleNcoreSpin->setValue(maxNcore);
      vasp_gui.calc.ncore = m_simpleNcoreSpin->value();
      vasp_gui.calc.kpar = kpar;
    });

    mainLayout->addWidget(frame);
  }

  /* Disclaimer */
  {
    auto *frame = new QGroupBox(tr("DISCLAMER"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    auto *label1 =
        new QLabel(tr("This simplified interface is aimed at pre-loading parameters with default values "
                      "for a given calculation type. User should then check said defaults extremely carefuly..."));
    label1->setWordWrap(true);
    vbox->addWidget(label1);

    auto *hbox = new QHBoxLayout();
    auto *genLabel = new QLabel(tr("GENERATE ==>"));
    m_simpleApplyBtn = new QPushButton(tr("APPLY"));
    auto *checkLabel = new QLabel(tr("<== (and check...)"));
    hbox->addWidget(genLabel);
    hbox->addWidget(m_simpleApplyBtn);
    hbox->addWidget(checkLabel);
    vbox->addLayout(hbox);

    mainLayout->addWidget(frame);
  }

  /* Message */
  {
    auto *frame = new QGroupBox(tr("MESSAGE"), page);
    auto *vbox = new QVBoxLayout(frame);
    m_simpleMessageText = new QPlainTextEdit();
    m_simpleMessageText->setReadOnly(true);
    m_simpleMessageText->setMaximumHeight(100);
    vbox->addWidget(m_simpleMessageText);
    mainLayout->addWidget(frame);
  }

  mainLayout->addStretch();
  m_notebook->addTab(page, tr("PRESET"));
}

void VaspDialog::setupConvergencePage()
{
  auto *page = new QWidget();
  auto *mainLayout = new QVBoxLayout(page);
  mainLayout->setSpacing(8);

  /* === General frame === */
  {
    auto *frame = new QGroupBox(tr("General"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    /* Row 1: PREC | USE PREC | ENCUT | ENAUG | EDIFF */
    m_precCombo = new QComboBox();
    m_precCombo->addItems({tr("Normal"), tr("Single"), tr("Accurate"), tr("High"), tr("Medium"), tr("Low")});
    auto *row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("PREC=")));
    row1->addWidget(m_precCombo);
    row1->addSpacing(10);
    m_usePrecCheck = new QCheckBox(tr("USE PREC"));
    row1->addWidget(m_usePrecCheck);
    row1->addSpacing(10);
    m_encutEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("ENCUT=")));
    row1->addWidget(m_encutEdit);
    row1->addSpacing(10);
    m_enaugEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("ENAUG=")));
    row1->addWidget(m_enaugEdit);
    row1->addSpacing(10);
    m_ediffEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("EDIFF=")));
    row1->addWidget(m_ediffEdit);
    vbox->addLayout(row1);

    /* Row 2: ALGO | LDIAG | NSIM | TIME | IWAVPR */
    m_algoCombo = new QComboBox();
    m_algoCombo->addItems({tr("NORMAL"), tr("USE_IALGO"), tr("VERYFAST"), tr("FAST"), tr("CONJUGATE"), tr("ALL"),
                           tr("DAMPED"), tr("SUBROT"), tr("EIGENVAL"), tr("NONE"), tr("NOTHING"), tr("EXACT"),
                           tr("DIAG")});
    auto *row2 = new QHBoxLayout();
    row2->addWidget(new QLabel(tr("ALGO=")));
    row2->addWidget(m_algoCombo);
    row2->addSpacing(10);
    /* ALGO=USE_IALGO enables IALGO combo; all other ALGO values disable it */
    connect(m_algoCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
      m_ialgoCombo->setEnabled(idx == 1); /* index 1 = USE_IALGO */
    });
    m_ldiagCheck = new QCheckBox(tr("LDIAG"));
    row2->addWidget(m_ldiagCheck);
    row2->addSpacing(10);
    m_nsimEdit = new QLineEdit();
    row2->addWidget(new QLabel(tr("NSIM=")));
    row2->addWidget(m_nsimEdit);
    row2->addSpacing(10);
    m_vtimeEdit = new QLineEdit();
    row2->addWidget(new QLabel(tr("TIME=")));
    row2->addWidget(m_vtimeEdit);
    row2->addSpacing(10);
    m_iwavprEdit = new QLineEdit();
    row2->addWidget(new QLabel(tr("IWAVPR=")));
    row2->addWidget(m_iwavprEdit);
    vbox->addLayout(row2);

    /* Row 3: IALGO | AUTO | NBANDS | NELECT */
    m_ialgoCombo = new QComboBox();
    m_ialgoCombo->addItems({tr("2:FIXED ORB/1E"), tr("3:FIXED ORB"), tr("4:SUBROT ONLY"), tr("5:STEEPEST"),
                            tr("6:CONJUGATE GRADIENTS"), tr("7:PRECOND. STEEPEST"), tr("8:PRECOND. CG"),
                            tr("38:KOSUGI"), tr("44:RMM STEEPEST"), tr("46:RMM PRECOND."), tr("48:PRECOND. RMM"),
                            tr("53:VAR DAMPED MD"), tr("54:VAR DAMPED QUENCH MD"), tr("58:VAR PRECOND. CG"),
                            tr("90:EXACT")});
    auto *row3 = new QHBoxLayout();
    row3->addWidget(new QLabel(tr("IALGO=")));
    row3->addWidget(m_ialgoCombo);
    row3->addSpacing(10);
    m_autoElecCheck = new QCheckBox(tr("AUTO"));
    row3->addWidget(m_autoElecCheck);
    row3->addSpacing(10);
    m_nbandsEdit = new QLineEdit();
    row3->addWidget(new QLabel(tr("NBANDS=")));
    row3->addWidget(m_nbandsEdit);
    row3->addSpacing(10);
    m_nelectEdit = new QLineEdit();
    row3->addWidget(new QLabel(tr("NELECT=")));
    row3->addWidget(m_nelectEdit);
    vbox->addLayout(row3);

    /* Row 4: INIWAV | ISTART | ICHARG */
    m_iniwavCheck = new QCheckBox(tr("INIWAV"));
    auto *row4 = new QHBoxLayout();
    row4->addWidget(m_iniwavCheck);
    row4->addSpacing(10);
    m_istartEdit = new QLineEdit();
    row4->addWidget(new QLabel(tr("ISTART=")));
    row4->addWidget(m_istartEdit);
    row4->addSpacing(10);
    m_ichargEdit = new QLineEdit();
    row4->addWidget(new QLabel(tr("ICHARG=")));
    row4->addWidget(m_ichargEdit);
    vbox->addLayout(row4);

    mainLayout->addWidget(frame);
  }

  /* === Mixing frame === */
  {
    auto *frame = new QGroupBox(tr("Mixing"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    /* Row 1: IMIX | AUTO MIXER | NELM | NELMDL | NELMIN */
    m_mixerCombo = new QComboBox();
    m_mixerCombo->addItems({tr("0:NONE"), tr("1:KERKER"), tr("2:TCHEBYCHEV"), tr("4:BROYDEN")});
    auto *row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("IMIX=")));
    row1->addWidget(m_mixerCombo);
    row1->addSpacing(10);
    m_autoMixerCheck = new QCheckBox(tr("AUTO MIXER"));
    row1->addWidget(m_autoMixerCheck);
    row1->addSpacing(10);
    m_nelmEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("NELM=")));
    row1->addWidget(m_nelmEdit);
    row1->addSpacing(10);
    m_nelmdlEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("NELMDL=")));
    row1->addWidget(m_nelmdlEdit);
    row1->addSpacing(10);
    m_nelminEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("NELMIN=")));
    row1->addWidget(m_nelminEdit);
    vbox->addLayout(row1);

    /* Row 2: MIXPRE | (empty) | AMIX | BMIX | AMIN */
    m_mixpreCombo = new QComboBox();
    m_mixpreCombo->addItems({tr("0:NONE"), tr("1:F=20 INVERSE KERKER"), tr("2:F=200 INVERSE KERKER")});
    auto *row2 = new QHBoxLayout();
    row2->addWidget(new QLabel(tr("MIXPRE=")));
    row2->addWidget(m_mixpreCombo);
    row2->addSpacing(20); /* empty col3 */
    m_amixEdit = new QLineEdit();
    row2->addWidget(new QLabel(tr("AMIX=")));
    row2->addWidget(m_amixEdit);
    row2->addSpacing(10);
    m_bmixEdit = new QLineEdit();
    row2->addWidget(new QLabel(tr("BMIX=")));
    row2->addWidget(m_bmixEdit);
    row2->addSpacing(10);
    m_aminEdit = new QLineEdit();
    row2->addWidget(new QLabel(tr("AMIN=")));
    row2->addWidget(m_aminEdit);
    vbox->addLayout(row2);

    /* Row 3: INIMIX | MAXMIX | AMIX_MAG | BMIX_MAG | WC */
    m_inimixCombo = new QComboBox();
    m_inimixCombo->addItems({tr("0:LINEAR"), tr("1:KERKER")});
    auto *row3 = new QHBoxLayout();
    row3->addWidget(new QLabel(tr("INIMIX=")));
    row3->addWidget(m_inimixCombo);
    row3->addSpacing(10);
    m_maxmixEdit = new QLineEdit();
    row3->addWidget(new QLabel(tr("MAXMIX=")));
    row3->addWidget(m_maxmixEdit);
    row3->addSpacing(10);
    m_amixMagEdit = new QLineEdit();
    row3->addWidget(new QLabel(tr("AMIX_MAG=")));
    row3->addWidget(m_amixMagEdit);
    row3->addSpacing(10);
    m_bmixMagEdit = new QLineEdit();
    row3->addWidget(new QLabel(tr("BMIX_MAG=")));
    row3->addWidget(m_bmixMagEdit);
    row3->addSpacing(10);
    m_wcEdit = new QLineEdit();
    row3->addWidget(new QLabel(tr("WC=")));
    row3->addWidget(m_wcEdit);
    vbox->addLayout(row3);

    mainLayout->addWidget(frame);
  }

  /* === Projector frame === */
  {
    auto *frame = new QGroupBox(tr("Projector"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    /* Row 1: LREAL | (empty) | ROPT */
    m_lrealCombo = new QComboBox();
    m_lrealCombo->addItems({tr("Auto"), tr("On"), tr("TRUE"), tr("FALSE")});
    auto *row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("LREAL=")));
    row1->addWidget(m_lrealCombo);
    row1->addSpacing(20); /* empty col2 */
    m_convRoptEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("ROPT=")));
    row1->addWidget(m_convRoptEdit);
    vbox->addLayout(row1);

    /* Row 2: (empty) | ADDGRID | LMAXMIX | (empty) | LMAXPAW */
    auto *row2 = new QHBoxLayout();
    row2->addSpacing(20); /* empty col1 */
    m_addgridCheck = new QCheckBox(tr("ADDGRID"));
    row2->addWidget(m_addgridCheck);
    row2->addSpacing(10);
    m_lmaxmixEdit = new QLineEdit();
    row2->addWidget(new QLabel(tr("LMAXMIX=")));
    row2->addWidget(m_lmaxmixEdit);
    row2->addSpacing(20); /* empty col4 */
    m_lmaxpawEdit = new QLineEdit();
    row2->addWidget(new QLabel(tr("LMAXPAW=")));
    row2->addWidget(m_lmaxpawEdit);
    vbox->addLayout(row2);

    /* Row 3: (empty) | AUTO GRID | NGX | NGY | NGZ */
    auto *row3 = new QHBoxLayout();
    row3->addSpacing(20); /* empty col1 */
    m_autoGridCheck = new QCheckBox(tr("AUTO GRID"));
    row3->addWidget(m_autoGridCheck);
    row3->addSpacing(10);
    m_ngxEdit = new QLineEdit();
    row3->addWidget(new QLabel(tr("NGX=")));
    row3->addWidget(m_ngxEdit);
    row3->addSpacing(10);
    m_ngyEdit = new QLineEdit();
    row3->addWidget(new QLabel(tr("NGY=")));
    row3->addWidget(m_ngyEdit);
    row3->addSpacing(10);
    m_ngzEdit = new QLineEdit();
    row3->addWidget(new QLabel(tr("NGZ=")));
    row3->addWidget(m_ngzEdit);
    vbox->addLayout(row3);

    /* Row 4: (empty) | (empty) | NGXF | NGYF | NGZF */
    auto *row4 = new QHBoxLayout();
    row4->addSpacing(40); /* empty col1+2 */
    m_ngxfEdit = new QLineEdit();
    row4->addWidget(new QLabel(tr("NGXF=")));
    row4->addWidget(m_ngxfEdit);
    row4->addSpacing(10);
    m_ngyfEdit = new QLineEdit();
    row4->addWidget(new QLabel(tr("NGYF=")));
    row4->addWidget(m_ngyfEdit);
    row4->addSpacing(10);
    m_ngzfEdit = new QLineEdit();
    row4->addWidget(new QLabel(tr("NGZF=")));
    row4->addWidget(m_ngzfEdit);
    vbox->addLayout(row4);

    mainLayout->addWidget(frame);
  }

  mainLayout->addStretch();
  m_notebook->addTab(page, tr("CONVERGENCE"));

  /* === Toggle dependencies === */
  /* USE PREC → when checked (PREC decides), ENCUT/ENAUG are read-only; when unchecked, editable */
  connect(m_usePrecCheck, &QCheckBox::toggled, this, [this](bool v) {
    /* When USE PREC is OFF, user can manually set ENCUT/ENAUG */
    m_encutEdit->setEnabled(!v);
    m_enaugEdit->setEnabled(!v);
  });

  /* AUTO → enables/disables NSIM, TIME, IWAVPR, NBANDS, NELECT */
  connect(m_autoElecCheck, &QCheckBox::toggled, this, [this](bool v) {
    m_nsimEdit->setEnabled(!v);
    m_vtimeEdit->setEnabled(!v);
    m_iwavprEdit->setEnabled(!v);
    m_nbandsEdit->setEnabled(!v);
    m_nelectEdit->setEnabled(!v);
  });

  /* AUTO MIXER → enables/disables AMIX, BMIX, AMIN, IMIX */
  connect(m_autoMixerCheck, &QCheckBox::toggled, this, [this](bool v) {
    m_amixEdit->setEnabled(!v);
    m_bmixEdit->setEnabled(!v);
    m_aminEdit->setEnabled(!v);
    /* IMIX is disabled when AUTO MIXER is on */
    m_mixerCombo->setEnabled(!v);
    /* IMIX=BROYDEN + !AUTO MIXER → enables MIXPRE, INIMIX, WC, MAXMIX, AMIX_MAG, BMIX_MAG */
    bool broyden = (m_mixerCombo->currentIndex() == 3); /* 4:BROYDEN */
    bool showMixerExtras = broyden && !v;
    m_mixpreCombo->setEnabled(showMixerExtras);
    m_inimixCombo->setEnabled(showMixerExtras);
    m_wcEdit->setEnabled(showMixerExtras);
    m_maxmixEdit->setEnabled(showMixerExtras);
    m_amixMagEdit->setEnabled(showMixerExtras);
    m_bmixMagEdit->setEnabled(showMixerExtras);
  });

  /* IMIX change → re-evaluate MIXPRE/INIMIX/WC/MAXMIX/AMIX_MAG/BMIX_MAG visibility */
  connect(m_mixerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
    bool broyden = (idx == 3); /* 4:BROYDEN */
    bool showMixerExtras = broyden && !m_autoMixerCheck->isChecked();
    m_mixpreCombo->setEnabled(showMixerExtras);
    m_inimixCombo->setEnabled(showMixerExtras);
    m_wcEdit->setEnabled(showMixerExtras);
    m_maxmixEdit->setEnabled(showMixerExtras);
    m_amixMagEdit->setEnabled(showMixerExtras);
    m_bmixMagEdit->setEnabled(showMixerExtras);
  });

  /* AUTO GRID → enables/disables NGX, NGY, NGZ, NGXF, NGYF, NGZF */
  connect(m_autoGridCheck, &QCheckBox::toggled, this, [this](bool v) {
    m_ngxEdit->setEnabled(!v);
    m_ngyEdit->setEnabled(!v);
    m_ngzEdit->setEnabled(!v);
    m_ngxfEdit->setEnabled(!v);
    m_ngyfEdit->setEnabled(!v);
    m_ngzfEdit->setEnabled(!v);
  });

  /* Initialize toggle states (read from vasp_gui.calc, not widgets) */
  m_autoMixerCheck->setChecked(vasp_gui.calc.auto_mixer);
  m_encutEdit->setEnabled(vasp_gui.calc.use_prec);
  m_enaugEdit->setEnabled(vasp_gui.calc.use_prec);
  m_nsimEdit->setEnabled(!vasp_gui.calc.auto_elec);
  m_vtimeEdit->setEnabled(!vasp_gui.calc.auto_elec);
  m_iwavprEdit->setEnabled(!vasp_gui.calc.auto_elec);
  m_nbandsEdit->setEnabled(!vasp_gui.calc.auto_elec);
  m_nelectEdit->setEnabled(!vasp_gui.calc.auto_elec);
  m_amixEdit->setEnabled(!vasp_gui.calc.auto_mixer);
  m_bmixEdit->setEnabled(!vasp_gui.calc.auto_mixer);
  m_aminEdit->setEnabled(!vasp_gui.calc.auto_mixer);
  m_maxmixEdit->setEnabled(!vasp_gui.calc.auto_mixer);
  m_amixMagEdit->setEnabled(!vasp_gui.calc.auto_mixer);
  m_bmixMagEdit->setEnabled(!vasp_gui.calc.auto_mixer);
  /* IMIX is disabled when AUTO MIXER is on */
  m_mixerCombo->setEnabled(!vasp_gui.calc.auto_mixer);
  {
    bool broyden = (vasp_gui.calc.imix == VM_4);
    bool showMixerExtras = broyden && !vasp_gui.calc.auto_mixer;
    m_mixpreCombo->setEnabled(showMixerExtras);
    m_inimixCombo->setEnabled(showMixerExtras);
    m_wcEdit->setEnabled(showMixerExtras);
  }
  m_ngxEdit->setEnabled(!vasp_gui.calc.auto_grid);
  m_ngyEdit->setEnabled(!vasp_gui.calc.auto_grid);
  m_ngzEdit->setEnabled(!vasp_gui.calc.auto_grid);
  m_ngxfEdit->setEnabled(!vasp_gui.calc.auto_grid);
  m_ngyfEdit->setEnabled(!vasp_gui.calc.auto_grid);
  m_ngzfEdit->setEnabled(!vasp_gui.calc.auto_grid);

  /* [VASP init] debug removed */
}

void VaspDialog::setupElectronicPage1()
{
  auto *page = new QWidget();
  auto *mainLayout = new QVBoxLayout(page);
  mainLayout->setSpacing(8);

  /* === Smearing frame === */
  {
    auto *frame = new QGroupBox(tr("Smearing"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    /* Row 1: ISMEAR | SIGMA | KGAMMA | KSPACING */
    m_ismearEdit = new QLineEdit();
    auto *row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("ISMEAR=")));
    row1->addWidget(m_ismearEdit);
    row1->addSpacing(10);
    m_sigmaEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("SIGMA=")));
    row1->addWidget(m_sigmaEdit);
    row1->addSpacing(10);
    m_kgammaCheck = new QCheckBox(tr("KGAMMA"));
    row1->addWidget(m_kgammaCheck);
    row1->addSpacing(10);
    m_kspacingEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("KSPACING=")));
    row1->addWidget(m_kspacingEdit);
    vbox->addLayout(row1);

    /* Row 2: FERMWE | FERMDO */
    m_fermweEdit = new QLineEdit();
    auto *row2 = new QHBoxLayout();
    row2->addWidget(new QLabel(tr("FERMWE=")));
    row2->addWidget(m_fermweEdit);
    row2->addSpacing(20);
    m_fermdoEdit = new QLineEdit();
    row2->addWidget(new QLabel(tr("FERMDO=")));
    row2->addWidget(m_fermdoEdit);
    vbox->addLayout(row2);

    mainLayout->addWidget(frame);
  }

  /* === Spin frame === */
  {
    auto *frame = new QGroupBox(tr("Spin"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    /* Row 1: SPIN POLARIZED | NON COLLINEAR | LSORBIT | GGA_COMPAT | NUPDOWN */
    m_spinCheck = new QCheckBox(tr("SPIN POLARIZED"));
    auto *row1 = new QHBoxLayout();
    row1->addWidget(m_spinCheck);
    row1->addSpacing(10);
    m_lnoncollCheck = new QCheckBox(tr("NON COLLINEAR"));
    row1->addWidget(m_lnoncollCheck);
    row1->addSpacing(10);
    m_lsorbitCheck = new QCheckBox(tr("LSORBIT"));
    row1->addWidget(m_lsorbitCheck);
    row1->addSpacing(10);
    m_ggaCompatCheck = new QCheckBox(tr("GGA_COMPAT"));
    row1->addWidget(m_ggaCompatCheck);
    row1->addSpacing(10);
    m_nupdownEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("NUPDOWN=")));
    row1->addWidget(m_nupdownEdit);
    vbox->addLayout(row1);

    /* Row 2: MAGMOM | SAXIS */
    m_magmomEdit = new QLineEdit();
    auto *row2 = new QHBoxLayout();
    row2->addWidget(new QLabel(tr("MAGMOM=")));
    row2->addWidget(m_magmomEdit);
    row2->addSpacing(20);
    m_saxisEdit = new QLineEdit();
    row2->addWidget(new QLabel(tr("SAXIS=")));
    row2->addWidget(m_saxisEdit);
    vbox->addLayout(row2);

    mainLayout->addWidget(frame);
  }

  /* === Exchange Correlation frame === */
  {
    auto *frame = new QGroupBox(tr("Exchange Correlation"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    /* Row 1: GGA | VOSKOWN */
    m_ggaCombo = new QComboBox();
    m_ggaCombo->addItems({tr("91:PW91"), tr("PE:PBE"), tr("RP:rPBE"), tr("AM:AM05"), tr("PS:PBEsol")});
    auto *row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("GGA=")));
    row1->addWidget(m_ggaCombo);
    row1->addSpacing(20);
    m_voskownCheck = new QCheckBox(tr("VOSKOWN"));
    row1->addWidget(m_voskownCheck);
    vbox->addLayout(row1);

    /* Row 2: METAGGA | LMETAGGA | LASPH | LMIXTAU | LMAXTAU */
    m_metaggaCombo = new QComboBox();
    m_metaggaCombo->addItems({tr("TPSS"), tr("rTPSS"), tr("M06L"), tr("MBJ")});
    m_metaggaCombo->setCurrentIndex(3); /* MBJ (disabled until LMETAGGA) */
    auto *row2 = new QHBoxLayout();
    row2->addWidget(new QLabel(tr("METAGGA=")));
    row2->addWidget(m_metaggaCombo);
    row2->addSpacing(10);
    m_lmetaggaCheck = new QCheckBox(tr("LMETAGGA"));
    row2->addWidget(m_lmetaggaCheck);
    row2->addSpacing(10);
    m_lasphCheck = new QCheckBox(tr("LASPH"));
    row2->addWidget(m_lasphCheck);
    row2->addSpacing(10);
    m_lmixtauCheck = new QCheckBox(tr("LMIXTAU"));
    row2->addWidget(m_lmixtauCheck);
    row2->addSpacing(10);
    m_lmaxtauEdit = new QLineEdit();
    row2->addWidget(new QLabel(tr("LMAXTAU=")));
    row2->addWidget(m_lmaxtauEdit);
    vbox->addLayout(row2);

    /* Row 3: CMBJ | CMBJA | CMBJB */
    m_cmbjEdit = new QLineEdit();
    auto *row3 = new QHBoxLayout();
    row3->addWidget(new QLabel(tr("CMBJ=")));
    row3->addWidget(m_cmbjEdit);
    row3->addSpacing(10);
    m_cmbjaEdit = new QLineEdit();
    row3->addWidget(new QLabel(tr("CMBJA=")));
    row3->addWidget(m_cmbjaEdit);
    row3->addSpacing(10);
    m_cmbjbEdit = new QLineEdit();
    row3->addWidget(new QLabel(tr("CMBJB=")));
    row3->addWidget(m_cmbjbEdit);
    vbox->addLayout(row3);

    /* Row 4: LDAUTYPE | LDAU | LDAUL */
    m_ldauTypeCombo = new QComboBox();
    m_ldauTypeCombo->addItems({tr("1:LSDA+U Liechtenstein"), tr("2:LSDA+U Dudarev"), tr("4:LDA+U Liechtenstein")});
    auto *row4 = new QHBoxLayout();
    row4->addWidget(new QLabel(tr("LDAUTYPE=")));
    row4->addWidget(m_ldauTypeCombo);
    row4->addSpacing(10);
    m_ldauCheck = new QCheckBox(tr("LDAU"));
    row4->addWidget(m_ldauCheck);
    row4->addSpacing(10);
    m_ldaulEdit = new QLineEdit();
    row4->addWidget(new QLabel(tr("LDAUL=")));
    row4->addWidget(m_ldaulEdit);
    vbox->addLayout(row4);

    /* Row 5: LDAUPRINT | LDAUU | LDAUJ */
    m_ldauPrintCombo = new QComboBox();
    m_ldauPrintCombo->addItems({tr("0:Silent"), tr("1:Occupancy matrix"), tr("2:Full output")});
    auto *row5 = new QHBoxLayout();
    row5->addWidget(new QLabel(tr("LDAUPRINT=")));
    row5->addWidget(m_ldauPrintCombo);
    row5->addSpacing(10);
    m_ldauuEdit = new QLineEdit();
    row5->addWidget(new QLabel(tr("LDAUU=")));
    row5->addWidget(m_ldauuEdit);
    row5->addSpacing(10);
    m_ldaujEdit = new QLineEdit();
    row5->addWidget(new QLabel(tr("LDAUJ=")));
    row5->addWidget(m_ldaujEdit);
    vbox->addLayout(row5);

    mainLayout->addWidget(frame);
  }

  /* === Dipole frame === */
  {
    auto *frame = new QGroupBox(tr("Dipole"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    /* Row 1: IDIPOL | LDIPOL | LMONO | DIPOL */
    m_idipolCombo = new QComboBox();
    m_idipolCombo->addItems({tr("0:no calcul"), tr("1:u axis"), tr("2:v axis"), tr("3:w axis"), tr("4:all axis")});
    auto *row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("IDIPOL=")));
    row1->addWidget(m_idipolCombo);
    row1->addSpacing(10);
    m_ldipolCheck = new QCheckBox(tr("LDIPOL"));
    row1->addWidget(m_ldipolCheck);
    row1->addSpacing(10);
    m_lmonoCheck = new QCheckBox(tr("LMONO"));
    row1->addWidget(m_lmonoCheck);
    row1->addSpacing(10);
    m_dipolEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("DIPOL=")));
    row1->addWidget(m_dipolEdit);
    vbox->addLayout(row1);

    /* Row 2: (empty) | (empty) | (empty) | EPSILON | EFIELD */
    auto *row2 = new QHBoxLayout();
    row2->addSpacing(60); /* empty col0-2 */
    m_epsilonEdit = new QLineEdit();
    row2->addWidget(new QLabel(tr("EPSILON=")));
    row2->addWidget(m_epsilonEdit);
    row2->addSpacing(10);
    m_efieldEdit = new QLineEdit();
    row2->addWidget(new QLabel(tr("EFIELD=")));
    row2->addWidget(m_efieldEdit);
    vbox->addLayout(row2);

    mainLayout->addWidget(frame);
  }

  /* === Toggle dependencies === */

  /* NON COLLINEAR → unchecks/disables ISPIN */
  connect(m_lnoncollCheck, &QCheckBox::toggled, this, [this](bool v) {
    m_spinCheck->setChecked(false);
    m_spinCheck->setEnabled(!v);
  });

  /* LSORBIT → enables NON COLLINEAR */
  connect(m_lsorbitCheck, &QCheckBox::toggled, this, [this](bool v) {
    if (v)
    {
      m_lnoncollCheck->setChecked(true);
      m_lnoncollCheck->setEnabled(true);
    }
  });

  /* NON COLLINEAR unset → unchecks LSORBIT */
  connect(m_lnoncollCheck, &QCheckBox::toggled, this, [this](bool v) {
    if (!v)
    {
      m_lsorbitCheck->setChecked(false);
    }
  });

  /* LMETAGGA → enables/disables METAGGA combo and CMBJ fields */
  connect(m_lmetaggaCheck, &QCheckBox::toggled, this, [this](bool v) {
    m_metaggaCombo->setEnabled(v);
    if (v)
    {
      /* LMETAGGA on → enable CMBJ fields if MBJ is selected */
      bool mbj = (m_metaggaCombo->currentIndex() == 3);
      m_cmbjEdit->setEnabled(mbj);
      m_cmbjaEdit->setEnabled(mbj);
      m_cmbjbEdit->setEnabled(mbj);
    } else
    {
      /* LMETAGGA off → also disable CMBJ fields */
      m_cmbjEdit->setEnabled(false);
      m_cmbjaEdit->setEnabled(false);
      m_cmbjbEdit->setEnabled(false);
    }
  });

  /* METAGGA combo → enables/disables CMBJ fields when MBJ */
  connect(m_metaggaCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
    bool mbj = (idx == 3); /* MBJ */
    m_cmbjEdit->setEnabled(mbj);
    m_cmbjaEdit->setEnabled(mbj);
    m_cmbjbEdit->setEnabled(mbj);
  });

  /* LDAU → enables/disables LDAUTYPE, LDAUL, LDAUPRINT, LDAUU, LDAUJ */
  connect(m_ldauCheck, &QCheckBox::toggled, this, [this](bool v) {
    m_ldauTypeCombo->setEnabled(v);
    m_ldaulEdit->setEnabled(v);
    m_ldauPrintCombo->setEnabled(v);
    m_ldauuEdit->setEnabled(v);
    m_ldaujEdit->setEnabled(v);
  });

  /* IDIPOL → enables/disables LDIPOL, LMONO, DIPOL, EPSILON, EFIELD */
  connect(m_idipolCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
    bool active = (idx != 0);  /* not "no calcul" */
    bool allAxis = (idx == 4); /* "all axis" */
    m_ldipolCheck->setEnabled(active);
    m_lmonoCheck->setEnabled(active);
    m_dipolEdit->setEnabled(active);
    m_epsilonEdit->setEnabled(active);
    m_efieldEdit->setEnabled(active && !allAxis);
  });

  mainLayout->addStretch();
  m_notebook->addTab(page, tr("ELECT-I"));
}

void VaspDialog::setupElectronicPage2()
{
  auto *page = new QWidget();
  auto *mainLayout = new QVBoxLayout(page);
  mainLayout->setSpacing(8);

  /* === Density of States frame === */
  {
    auto *frame = new QGroupBox(tr("Density of States"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    /* Row 1: LORBIT | NEDOS | EMIN | EMAX | EFERMI */
    m_lorbitCombo = new QComboBox();
    m_lorbitCombo->addItems({tr("0:PROCAR"), tr("1:lm-PROCAR"), tr("2:phase+lm-PROCAR"), tr("5:PROOUT"),
                             tr("10:PROCAR"), tr("11:lm-PROCAR"), tr("12:phase+lm-PROCAR")});
    auto *row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("LORBIT=")));
    row1->addWidget(m_lorbitCombo);
    row1->addSpacing(10);
    m_nedosEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("NEDOS=")));
    row1->addWidget(m_nedosEdit);
    row1->addSpacing(10);
    m_eminEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("EMIN=")));
    row1->addWidget(m_eminEdit);
    row1->addSpacing(10);
    m_emaxEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("EMAX=")));
    row1->addWidget(m_emaxEdit);
    row1->addSpacing(10);
    m_efermiEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("EFERMI=")));
    row1->addWidget(m_efermiEdit);
    vbox->addLayout(row1);

    /* Row 2: (LORBIT>5) note | HAVE PAW | RWIGS */
    auto *row2 = new QHBoxLayout();
    row2->addWidget(new QLabel(tr("(LORBIT>5) => Req. PAW")));
    row2->addSpacing(10);
    m_havePawCheck = new QCheckBox(tr("HAVE PAW"));
    m_havePawCheck->setEnabled(false);
    row2->addWidget(m_havePawCheck);
    row2->addSpacing(10);
    m_rwigsEdit = new QLineEdit();
    row2->addWidget(new QLabel(tr("RWIGS=")));
    row2->addWidget(m_rwigsEdit);
    vbox->addLayout(row2);

    mainLayout->addWidget(frame);
  }

  /* === Linear Response frame === */
  {
    auto *frame = new QGroupBox(tr("Linear Response"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    /* Row 1: LOPTICS | LEPSILON | LRPA | LNABLA | CSHIFT */
    m_lopticsCheck = new QCheckBox(tr("LOPTICS"));
    auto *row1 = new QHBoxLayout();
    row1->addWidget(m_lopticsCheck);
    row1->addSpacing(10);
    m_lepsilonCheck = new QCheckBox(tr("LEPSILON"));
    row1->addWidget(m_lepsilonCheck);
    row1->addSpacing(10);
    m_lrpaCheck = new QCheckBox(tr("LRPA"));
    row1->addWidget(m_lrpaCheck);
    row1->addSpacing(10);
    m_lnablaCheck = new QCheckBox(tr("LNABLA"));
    row1->addWidget(m_lnablaCheck);
    row1->addSpacing(10);
    m_cshiftEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("CSHIFT=")));
    row1->addWidget(m_cshiftEdit);
    vbox->addLayout(row1);

    mainLayout->addWidget(frame);
  }

  /* === Toggle dependencies === */

  /* LORBIT=0 → disables NEDOS, EMIN, EMAX, EFERMI, RWIGS */
  /* LORBIT=10/11/12 → disables RWIGS only */
  connect(m_lorbitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
    bool dosEnabled = (idx != 0); /* not "0:PROCAR" */
    m_nedosEdit->setEnabled(dosEnabled);
    m_eminEdit->setEnabled(dosEnabled);
    m_emaxEdit->setEnabled(dosEnabled);
    m_efermiEdit->setEnabled(dosEnabled);
    /* RWIGS disabled for 10/11/12 (indices 4/5/6) */
    bool rwigsDisabled = (idx == 4 || idx == 5 || idx == 6);
    m_rwigsEdit->setEnabled(dosEnabled && !rwigsDisabled);
  });

  mainLayout->addStretch();
  m_notebook->addTab(page, tr("ELECT-II"));
}

void VaspDialog::setupIonicPage()
{
  auto *page = new QWidget();
  auto *mainLayout = new QVBoxLayout(page);
  mainLayout->setSpacing(8);

  /* === General frame === */
  {
    auto *frame = new QGroupBox(tr("General"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    /* Row 1: IBRION | NSW | EDIFFG | POTIM | PSTRESS */
    m_ibrionCombo = new QComboBox();
    m_ibrionCombo->addItems({tr("-1:Nothing"), tr("0:MD"), tr("1:Quasi-Newton"), tr("2:Conjugate-grad"), tr("3:Damped"),
                             tr("4:RMM-DIIS (ions)"), tr("5:Finite Differences"), tr("6:Finite Diff. with SYM"),
                             tr("7:Perturbation Theory"), tr("8:Pert. Theory with SYM"), tr("44:Transition State")});
    auto *row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("IBRION=")));
    row1->addWidget(m_ibrionCombo);
    row1->addSpacing(10);
    m_nswEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("NSW=")));
    row1->addWidget(m_nswEdit);
    row1->addSpacing(10);
    m_ediffgEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("EDIFFG=")));
    row1->addWidget(m_ediffgEdit);
    row1->addSpacing(10);
    m_potimEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("POTIM=")));
    row1->addWidget(m_potimEdit);
    row1->addSpacing(10);
    m_pstressEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("PSTRESS=")));
    row1->addWidget(m_pstressEdit);
    vbox->addLayout(row1);

    /* Row 2: ISIF | Relax Ions | Relax Shape | Relax Volume | NFREE */
    m_isifCombo = new QComboBox();
    m_isifCombo->addItems({tr("0:F_0_I_0_0"), tr("1:F_P_I_0_0"), tr("2:F_S_I_0_0"), tr("3:F_S_I_S_V"),
                           tr("4:F_S_I_S_0"), tr("5:F_S_0_S_0"), tr("6:F_S_0_S_V"), tr("7:F_S_0_0_V")});
    auto *row2 = new QHBoxLayout();
    row2->addWidget(new QLabel(tr("ISIF=")));
    row2->addWidget(m_isifCombo);
    row2->addSpacing(10);
    m_relaxIonsCheck = new QCheckBox(tr("atom positions"));
    row2->addWidget(m_relaxIonsCheck);
    row2->addSpacing(10);
    m_relaxShapeCheck = new QCheckBox(tr("cell shape"));
    row2->addWidget(m_relaxShapeCheck);
    row2->addSpacing(10);
    m_relaxVolumeCheck = new QCheckBox(tr("cell volume"));
    row2->addWidget(m_relaxVolumeCheck);
    row2->addSpacing(10);
    m_nfreeEdit = new QLineEdit();
    row2->addWidget(new QLabel(tr("NFREE=")));
    row2->addWidget(m_nfreeEdit);
    vbox->addLayout(row2);

    mainLayout->addWidget(frame);
  }

  /* === Molecular Dynamics frame === */
  {
    auto *frame = new QGroupBox(tr("Molecular Dynamics"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    /* Row 1: SET IBRION=0 FOR MD | TEBEG | TEEND | SMASS | (empty) */
    auto *row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("SET IBRION=0 FOR MD")));
    row1->addSpacing(10);
    m_tebegEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("TEBEG=")));
    row1->addWidget(m_tebegEdit);
    row1->addSpacing(10);
    m_teendEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("TEEND=")));
    row1->addWidget(m_teendEdit);
    row1->addSpacing(10);
    m_smassEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("SMASS=")));
    row1->addWidget(m_smassEdit);
    vbox->addLayout(row1);

    /* Row 2: (empty) | NBLOCK | KBLOCK | NPACO | APACO */
    auto *row2 = new QHBoxLayout();
    row2->addSpacing(50);
    m_nblockEdit = new QLineEdit();
    row2->addWidget(new QLabel(tr("NBLOCK=")));
    row2->addWidget(m_nblockEdit);
    row2->addSpacing(10);
    m_kblockEdit = new QLineEdit();
    row2->addWidget(new QLabel(tr("KBLOCK=")));
    row2->addWidget(m_kblockEdit);
    row2->addSpacing(10);
    m_npacoEdit = new QLineEdit();
    row2->addWidget(new QLabel(tr("NPACO=")));
    row2->addWidget(m_npacoEdit);
    row2->addSpacing(10);
    m_apacoEdit = new QLineEdit();
    row2->addWidget(new QLabel(tr("APACO=")));
    row2->addWidget(m_apacoEdit);
    vbox->addLayout(row2);

    mainLayout->addWidget(frame);
  }

  /* === POSCAR & SYMMETRY frame === */
  {
    auto *frame = new QGroupBox(tr("POSCAR & SYMMETRY"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    /* Row 1: DYN: | Sel. Dynamics | ISYM | _PREC= | A0= */
    m_poscarFreeCombo = new QComboBox();
    m_poscarFreeCombo->addItems({tr("All atom FIXED"), tr("All atom FREE"), tr("Manual selection")});
    auto *row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("DYN:")));
    row1->addWidget(m_poscarFreeCombo);
    row1->addSpacing(10);
    m_poscarSdCheck = new QCheckBox(tr("Sel. Dynamics"));
    row1->addWidget(m_poscarSdCheck);
    row1->addSpacing(10);
    m_isymEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("ISYM=")));
    row1->addWidget(m_isymEdit);
    row1->addSpacing(10);
    m_symPrecEdit = new QLineEdit();
    row1->addWidget(new QLabel(tr("_PREC=")));
    row1->addWidget(m_symPrecEdit);
    row1->addSpacing(10);
    m_poscarA0Edit = new QLineEdit();
    row1->addWidget(new QLabel(tr("A0=")));
    row1->addWidget(m_poscarA0Edit);
    vbox->addLayout(row1);

    /* Row 2: (empty) | Direct Coord. | UX= | UY= | UZ= */
    auto *row2 = new QHBoxLayout();
    row2->addSpacing(50);
    m_poscarDirectCheck = new QCheckBox(tr("Direct Coord."));
    row2->addWidget(m_poscarDirectCheck);
    row2->addSpacing(10);
    m_poscarUxEdit = new QLineEdit();
    row2->addWidget(new QLabel(tr("UX=")));
    row2->addWidget(m_poscarUxEdit);
    row2->addSpacing(10);
    m_poscarUyEdit = new QLineEdit();
    row2->addWidget(new QLabel(tr("UY=")));
    row2->addWidget(m_poscarUyEdit);
    row2->addSpacing(10);
    m_poscarUzEdit = new QLineEdit();
    row2->addWidget(new QLabel(tr("UZ=")));
    row2->addWidget(m_poscarUzEdit);
    vbox->addLayout(row2);

    /* Row 3: (empty) | (empty) | VX= | VY= | VZ= */
    auto *row3 = new QHBoxLayout();
    row3->addSpacing(50);
    m_poscarVxEdit = new QLineEdit();
    row3->addWidget(new QLabel(tr("VX=")));
    row3->addWidget(m_poscarVxEdit);
    row3->addSpacing(10);
    m_poscarVyEdit = new QLineEdit();
    row3->addWidget(new QLabel(tr("VY=")));
    row3->addWidget(m_poscarVyEdit);
    row3->addSpacing(10);
    m_poscarVzEdit = new QLineEdit();
    row3->addWidget(new QLabel(tr("VZ=")));
    row3->addWidget(m_poscarVzEdit);
    vbox->addLayout(row3);

    /* Row 4: (empty) | (empty) | WX= | WY= | WZ= */
    auto *row4 = new QHBoxLayout();
    row4->addSpacing(50);
    m_poscarWxEdit = new QLineEdit();
    row4->addWidget(new QLabel(tr("WX=")));
    row4->addWidget(m_poscarWxEdit);
    row4->addSpacing(10);
    m_poscarWyEdit = new QLineEdit();
    row4->addWidget(new QLabel(tr("WY=")));
    row4->addWidget(m_poscarWyEdit);
    row4->addSpacing(10);
    m_poscarWzEdit = new QLineEdit();
    row4->addWidget(new QLabel(tr("WZ=")));
    row4->addWidget(m_poscarWzEdit);
    vbox->addLayout(row4);

    /* Row 5: (empty) | (empty) | TX | TY | TZ */
    auto *row5 = new QHBoxLayout();
    row5->addSpacing(50);
    m_poscarTxCheck = new QCheckBox(tr("TX"));
    row5->addWidget(m_poscarTxCheck);
    row5->addSpacing(10);
    m_poscarTyCheck = new QCheckBox(tr("TY"));
    row5->addWidget(m_poscarTyCheck);
    row5->addSpacing(10);
    m_poscarTzCheck = new QCheckBox(tr("TZ"));
    row5->addWidget(m_poscarTzCheck);
    vbox->addLayout(row5);

    /* Row 6: ATOMS: | Apply | Delete */
    m_poscarAtomsCombo = new QComboBox();
    auto *row6 = new QHBoxLayout();
    row6->addWidget(new QLabel(tr("ATOMS:")));
    row6->addWidget(m_poscarAtomsCombo);
    m_poscarApplyBtn = new QPushButton(tr("Apply"));
    row6->addWidget(m_poscarApplyBtn);
    m_poscarDeleteBtn = new QPushButton(tr("Delete"));
    row6->addWidget(m_poscarDeleteBtn);
    vbox->addLayout(row6);

    /* Row 7: INDEX: | SYMBOL: | X= | Y= | Z= */
    auto *row7 = new QHBoxLayout();
    m_poscarIndexEdit = new QLineEdit();
    m_poscarIndexEdit->setEnabled(false);
    row7->addWidget(new QLabel(tr("INDEX:")));
    row7->addWidget(m_poscarIndexEdit);
    row7->addSpacing(10);
    m_poscarSymbolEdit = new QLineEdit();
    row7->addWidget(new QLabel(tr("SYMBOL:")));
    row7->addWidget(m_poscarSymbolEdit);
    row7->addSpacing(10);
    m_poscarXEdit = new QLineEdit();
    row7->addWidget(new QLabel(tr("X=")));
    row7->addWidget(m_poscarXEdit);
    row7->addSpacing(10);
    m_poscarYEdit = new QLineEdit();
    row7->addWidget(new QLabel(tr("Y=")));
    row7->addWidget(m_poscarYEdit);
    row7->addSpacing(10);
    m_poscarZEdit = new QLineEdit();
    row7->addWidget(new QLabel(tr("Z=")));
    row7->addWidget(m_poscarZEdit);
    vbox->addLayout(row7);

    mainLayout->addWidget(frame);
  }

  /* === Toggle dependencies === */

  /* IBRION=0 → enables MD fields; IBRION=3 → enables SMASS */
  connect(m_ibrionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
    bool mdEnabled = (idx == 1); /* IBRION=0 */
    m_tebegEdit->setEnabled(mdEnabled);
    m_teendEdit->setEnabled(mdEnabled);
    m_nblockEdit->setEnabled(mdEnabled);
    m_kblockEdit->setEnabled(mdEnabled);
    m_npacoEdit->setEnabled(mdEnabled);
    m_apacoEdit->setEnabled(mdEnabled);
    /* SMASS: enabled for IBRION=0 or IBRION=3 */
    bool smassEnabled = (idx == 1 || idx == 4); /* IBRION=0 or IBRION=3 */
    m_smassEdit->setEnabled(smassEnabled);
  });

  /* DYN: combo → enable/disable Sel. Dynamics and TX/TY/TZ */
  connect(m_poscarFreeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
    bool manual = (idx == 2); /* Manual selection */
    m_poscarSdCheck->setEnabled(manual);
    m_poscarTxCheck->setEnabled(manual);
    m_poscarTyCheck->setEnabled(manual);
    m_poscarTzCheck->setEnabled(manual);
  });

  /* ATOMS combo selection → populate INDEX/SYMBOL/X/Y/Z/TX/TY/TZ */
  connect(m_poscarAtomsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
    struct model_pak *model = (struct model_pak *) sysenv.active_model;
    if (!model || !model->cores)
      return;

    /* Last item is "ADD atom" — enable fields for new entry */
    int totalAtoms = vasp_gui.calc.atoms_total;
    if (idx == totalAtoms)
    {
      m_poscarIndexEdit->setEnabled(false);
      m_poscarSymbolEdit->setEnabled(true);
      m_poscarXEdit->setEnabled(true);
      m_poscarYEdit->setEnabled(true);
      m_poscarZEdit->setEnabled(true);
      m_poscarTxCheck->setEnabled(true);
      m_poscarTyCheck->setEnabled(true);
      m_poscarTzCheck->setEnabled(true);
      /* Clear fields for new entry */
      m_poscarSymbolEdit->clear();
      m_poscarXEdit->clear();
      m_poscarYEdit->clear();
      m_poscarZEdit->clear();
      return;
    }

    /* Select an existing atom */
    GSList *l = g_slist_nth(model->cores, idx);
    if (!l)
      return;
    struct core_pak *core = (struct core_pak *) l->data;

    m_poscarIndexEdit->setText(QString::number(idx));
    int code = core->atom_code;
    if (code >= 0 && code < MAX_ELEMENTS)
    {
      m_poscarSymbolEdit->setText(elements[code].symbol);
    } else
    {
      m_poscarSymbolEdit->setText("X");
    }
    m_poscarXEdit->setText(QString::number(core->x[0], 'f', 6));
    m_poscarYEdit->setText(QString::number(core->x[1], 'f', 6));
    m_poscarZEdit->setText(QString::number(core->x[2], 'f', 6));

    /* TX/TY/TZ: read from selective arrays */
    bool hasTags = (vasp_gui.calc.poscar_free == VPF_MAN);
    m_poscarTxCheck->setEnabled(hasTags);
    m_poscarTyCheck->setEnabled(hasTags);
    m_poscarTzCheck->setEnabled(hasTags);
    if (hasTags || vasp_gui.calc.poscar_sd)
    {
      if (vasp_gui.calc.selective_tx && idx < vasp_gui.calc.selective_count)
      {
        m_poscarTxCheck->setChecked(vasp_gui.calc.selective_tx[idx]);
        m_poscarTyCheck->setChecked(vasp_gui.calc.selective_ty[idx]);
        m_poscarTzCheck->setChecked(vasp_gui.calc.selective_tz[idx]);
      }
    }
  });

  /* Apply button → update selected atom */
  connect(m_poscarApplyBtn, &QPushButton::clicked, this, [this]() {
    struct model_pak *model = (struct model_pak *) sysenv.active_model;
    if (!model || !model->cores)
      return;
    int idx = m_poscarAtomsCombo->currentIndex();
    int totalAtoms = vasp_gui.calc.atoms_total;

    /* "ADD atom" mode — add new atom */
    if (idx == totalAtoms)
    {
      QString sym = m_poscarSymbolEdit->text().trimmed().isEmpty() ? "X" : m_poscarSymbolEdit->text().trimmed();
      struct core_pak *newCore = new_core(g_strdup(sym.toUtf8().constData()), model);
      newCore->x[0] = m_poscarXEdit->text().toDouble();
      newCore->x[1] = m_poscarYEdit->text().toDouble();
      newCore->x[2] = m_poscarZEdit->text().toDouble();
      /* Store selective flags <- we need to realloc! */
      gboolean *tmp_x = (gboolean *) g_realloc(vasp_gui.calc.selective_tx, vasp_gui.calc.selective_count + 1);
      gboolean *tmp_y = (gboolean *) g_realloc(vasp_gui.calc.selective_ty, vasp_gui.calc.selective_count + 1);
      gboolean *tmp_z = (gboolean *) g_realloc(vasp_gui.calc.selective_tz, vasp_gui.calc.selective_count + 1);
      if ((tmp_x == NULL) || (tmp_y == NULL) || (tmp_z == NULL))
      {
        fprintf(stderr, "Memory allocation trouble: something's gonna crash soon!\n");
        return;
      }
      vasp_gui.calc.selective_tx = tmp_x;
      vasp_gui.calc.selective_ty = tmp_y;
      vasp_gui.calc.selective_tz = tmp_z;
      vasp_gui.calc.selective_tx[vasp_gui.calc.selective_count] = m_poscarTxCheck->isChecked();
      vasp_gui.calc.selective_ty[vasp_gui.calc.selective_count] = m_poscarTyCheck->isChecked();
      vasp_gui.calc.selective_tz[vasp_gui.calc.selective_count] = m_poscarTzCheck->isChecked();
      vasp_gui.calc.selective_count++;
      /* also update num_atoms */
      model->num_atoms++;
    } else
    {
      /* Update existing atom */
      GSList *l = g_slist_nth(model->cores, idx);
      if (!l)
        return;
      struct core_pak *core = (struct core_pak *) l->data;

      int code = -1;
      QString sym = m_poscarSymbolEdit->text().trimmed();
      for (int i = 0; i < MAX_ELEMENTS; i++)
      {
        if (QString(elements[i].symbol) == sym)
        {
          code = i;
          break;
        }
      }
      if (code >= 0)
        core->atom_code = code;
      core->x[0] = m_poscarXEdit->text().toDouble();
      core->x[1] = m_poscarYEdit->text().toDouble();
      core->x[2] = m_poscarZEdit->text().toDouble();

      /* Update selective flags in vasp_gui.calc */
      if (!vasp_gui.calc.selective_tx)
      {
        /* On atom selection, this is actually an error, not a trigger to realloc! */
        fprintf(stderr, "Memory error: uninitialized selective_tx!\n");
        return;
      }
      gint maxIdx = vasp_gui.calc.selective_count;
      if (idx >= maxIdx)
      {
        /* On atom selection, this is _also_ an error! */
        fprintf(stderr, "Memory error: selected an atom out of max_atoms!\n");
        return;
      }
      vasp_gui.calc.selective_tx[idx] = m_poscarTxCheck->isChecked();
      vasp_gui.calc.selective_ty[idx] = m_poscarTyCheck->isChecked();
      vasp_gui.calc.selective_tz[idx] = m_poscarTzCheck->isChecked();
    }

    /* Rebuild ATOMS combo */
    updatePoscarAtoms();
  });

  /* Delete button → remove selected atom */
  connect(m_poscarDeleteBtn, &QPushButton::clicked, this, [this]() {
    struct model_pak *model = (struct model_pak *) sysenv.active_model;
    if (!model || !model->cores)
      return;
    int idx = m_poscarAtomsCombo->currentIndex();
    int totalAtoms = vasp_gui.calc.atoms_total;

    /* Can't delete "ADD atom" entry */
    if (idx >= totalAtoms)
      return;

    GSList *l = g_slist_nth(model->cores, idx);
    if (!l)
      return;

    /* Rebuild vasp_gui.calc.selective_t{x,y,z} !!! */
    for (gint ix1 = idx + 1; ix1 < model->num_atoms; ix1++)
    {
      /* shift after idx */
      vasp_gui.calc.selective_tx[ix1] = vasp_gui.calc.selective_tx[ix1 + 1];
    }
    gboolean *tmp_x = (gboolean *) g_realloc(vasp_gui.calc.selective_tx, vasp_gui.calc.selective_count - 1);
    gboolean *tmp_y = (gboolean *) g_realloc(vasp_gui.calc.selective_ty, vasp_gui.calc.selective_count - 1);
    gboolean *tmp_z = (gboolean *) g_realloc(vasp_gui.calc.selective_tz, vasp_gui.calc.selective_count - 1);
    if ((tmp_x == NULL) || (tmp_y == NULL) || (tmp_z == NULL))
    {
      fprintf(stderr, "Memory allocation trouble: something's gonna crash soon!\n");
      return;
    }
    vasp_gui.calc.selective_tx = tmp_x;
    vasp_gui.calc.selective_ty = tmp_y;
    vasp_gui.calc.selective_tz = tmp_z;

    model->cores = g_slist_remove(model->cores, l->data);
    model->num_atoms--;

    /* Rebuild ATOMS combo */
    updatePoscarAtoms();
  });

  /* ISIF ↔ relax checkboxes relationship */
  connect(m_isifCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
    /* ISIF values: 0=F_0_I_0_0, 1=F_P_I_0_0, 2=F_S_I_0_0, 3=F_S_I_S_V, 4=F_S_I_S_0,
       5=F_S_0_S_0, 6=F_S_0_S_V, 7=F_S_0_0_V */
    bool relaxIons = (idx >= 2);                          /* ISIF >= 2 relaxes ions */
    bool relaxShape = (idx >= 3 && idx != 5 && idx != 7); /* ISIF 3,4 relax shape; 5,7 don't */
    bool relaxVolume = (idx == 3 || idx == 6);            /* ISIF 3,6 relax volume */
    m_relaxIonsCheck->setChecked(relaxIons);
    m_relaxShapeCheck->setChecked(relaxShape);
    m_relaxVolumeCheck->setChecked(relaxVolume);
  });

  connect(m_relaxIonsCheck, &QCheckBox::toggled, this, [this](bool v) {
    /* Update ISIF to match relax ions state */
    int current = m_isifCombo->currentIndex();
    if (v && current < 2)
    {
      m_isifCombo->setCurrentIndex(2); /* F_S_I_0_0 */
    } else if (!v && current >= 2)
    {
      m_isifCombo->setCurrentIndex(0); /* F_0_I_0_0 */
    }
  });

  connect(m_relaxShapeCheck, &QCheckBox::toggled, this, [this](bool v) {
    int current = m_isifCombo->currentIndex();
    if (v && current < 3)
    {
      m_isifCombo->setCurrentIndex(3); /* F_S_I_S_V */
    } else if (!v && current >= 3 && current != 5 && current != 7)
    {
      /* Unset shape relaxation */
      int idx = m_isifCombo->currentIndex();
      if (idx == 3)
        m_isifCombo->setCurrentIndex(2); /* F_S_I_0_0 */
      else if (idx == 4)
        m_isifCombo->setCurrentIndex(5); /* F_S_0_S_0 */
    }
  });

  connect(m_relaxVolumeCheck, &QCheckBox::toggled, this, [this](bool v) {
    int current = m_isifCombo->currentIndex();
    if (v && current != 3 && current != 6)
    {
      m_isifCombo->setCurrentIndex(3); /* F_S_I_S_V */
    } else if (!v && (current == 3 || current == 6))
    {
      if (current == 3)
        m_isifCombo->setCurrentIndex(4); /* F_S_I_S_0 */
      else if (current == 6)
        m_isifCombo->setCurrentIndex(7); /* F_S_0_0_V */
    }
  });

  /* Sel. Dynamics ↔ DYN combo */
  connect(m_poscarSdCheck, &QCheckBox::toggled, this, [this](bool v) {
    if (v)
    {
      m_poscarFreeCombo->setCurrentIndex(2); /* Manual selection */
    }
  });

  /* Direct Coord. toggle — convert all atom coordinates */
  connect(m_poscarDirectCheck, &QCheckBox::toggled, this, [this](bool) {
    /* Save current combo index */
    int savedIndex = m_poscarAtomsCombo->currentIndex();

    /* Read lattice vectors */
    double a0 = m_poscarA0Edit->text().toDouble();
    double ux = m_poscarUxEdit->text().toDouble();
    double uy = m_poscarUyEdit->text().toDouble();
    double uz = m_poscarUzEdit->text().toDouble();
    double vx = m_poscarVxEdit->text().toDouble();
    double vy = m_poscarVyEdit->text().toDouble();
    double vz = m_poscarVzEdit->text().toDouble();
    double wx = m_poscarWxEdit->text().toDouble();
    double wy = m_poscarWyEdit->text().toDouble();
    double wz = m_poscarWzEdit->text().toDouble();

    /* Compute lattice vector sums */
    double ux_vx_wx = ux + vx + wx;
    double uy_vy_wy = uy + vy + wy;
    double uz_vz_wz = uz + vz + wz;

    /* Convert each atom entry */
    bool isDirect = m_poscarDirectCheck->isChecked();
    int count = m_poscarAtomsCombo->count();
    for (int i = 0; i < count; i++)
    {
      QString entry = m_poscarAtomsCombo->itemText(i);
      if (entry.contains("ADD atom"))
        continue;

      /* Parse: "x y z  T/F T/F T/F ! atom: N (SYMBOL)" */
      double x = 0, y = 0, z = 0;
      QString suffix;
      QRegularExpression re(
          R"(([-+]?\d+\.?\d*(?:[eE][-+]?\d+)?)\s+([-+]?\d+\.?\d*(?:[eE][-+]?\d+)?)\s+([-+]?\d+\.?\d*(?:[eE][-+]?\d+)?)\s+(.*))");
      QRegularExpressionMatch m = re.match(entry);
      if (m.hasMatch())
      {
        x = m.captured(1).toDouble();
        y = m.captured(2).toDouble();
        z = m.captured(3).toDouble();
        suffix = m.captured(4);

        if (isDirect)
        {
          /* was cartesian, now changed to direct */
          x = x / ux_vx_wx;
          y = y / uy_vy_wy;
          z = z / uz_vz_wz;
        } else
        {
          /* was direct, now changed to cartesian */
          x = x * ux_vx_wx;
          y = y * uy_vy_wy;
          z = z * uz_vz_wz;
        }

        m_poscarAtomsCombo->setItemText(
            i, QString("%1 %2 %3  %4").arg(x, 0, 'f', 8).arg(y, 0, 'f', 8).arg(z, 0, 'f', 8).arg(suffix));
      }
    }

    /* Restore combo index */
    m_poscarAtomsCombo->setCurrentIndex(savedIndex);

    /* Also convert the X=/Y=/Z= fields if they have values */
    bool hasX = !m_poscarXEdit->text().trimmed().isEmpty();
    bool hasY = !m_poscarYEdit->text().trimmed().isEmpty();
    bool hasZ = !m_poscarZEdit->text().trimmed().isEmpty();
    if (hasX && hasY && hasZ)
    {
      double x = m_poscarXEdit->text().toDouble();
      double y = m_poscarYEdit->text().toDouble();
      double z = m_poscarZEdit->text().toDouble();

      if (isDirect)
      {
        x = x / ux_vx_wx;
        y = y / uy_vy_wy;
        z = z / uz_vz_wz;
      } else
      {
        x = x * ux_vx_wx;
        y = y * uy_vy_wy;
        z = z * uz_vz_wz;
      }

      m_poscarXEdit->setText(QString("%1").arg(x, 0, 'f', 8));
      m_poscarYEdit->setText(QString("%1").arg(y, 0, 'f', 8));
      m_poscarZEdit->setText(QString("%1").arg(z, 0, 'f', 8));
    }
  });

  mainLayout->addStretch();
  m_notebook->addTab(page, tr("IONIC"));
}

void VaspDialog::updatePoscarAtoms()
{
  if (!m_poscarAtomsCombo)
    return;
  m_poscarAtomsCombo->clear();
  struct model_pak *model = (struct model_pak *) sysenv.active_model;
  if (!model || !model->cores)
    return;
  GSList *list2 = model->cores;
  int idx = 0;
  for (GSList *l = list2; l; l = g_slist_next(l))
  {
    struct core_pak *core = (struct core_pak *) l->data;
    gchar sym[4];
    int code = core->atom_code;
    if (code < 0 || code >= MAX_ELEMENTS)
    {
      g_snprintf(sym, sizeof(sym), "X");
    } else
    {
      g_snprintf(sym, sizeof(sym), "%s", elements[code].symbol);
    }
    gchar *entry;
    gboolean tx = FALSE, ty = FALSE, tz = FALSE;
    if (vasp_gui.calc.selective_tx && idx < vasp_gui.calc.selective_count)
    {
      tx = vasp_gui.calc.selective_tx[idx];
      ty = vasp_gui.calc.selective_ty[idx];
      tz = vasp_gui.calc.selective_tz[idx];
    }
    /* Wrap fractional coords to (-0.5, 0.5) per VASP convention */
    gdouble wx = core->x[0], wy = core->x[1], wz = core->x[2];
    if (model->fractional && model->periodic > 0)
    {
      gdouble whole;
      wx = modf(wx, &whole);
      wy = modf(wy, &whole);
      wz = modf(wz, &whole);
      for (int d = 0; d < model->periodic; d++)
      {
        int j = (gint) (-2.0 * wx);
        wx += j;
        j = (gint) (-2.0 * wy);
        wy += j;
        j = (gint) (-2.0 * wz);
        wz += j;
      }
    }
    entry = g_strdup_printf("%.8lf %.8lf %.8lf  %s\t%s\t%s ! atom: %d (%s)", wx, wy, wz, tx ? "T" : "F", ty ? "T" : "F",
                            tz ? "T" : "F", idx, sym);
    m_poscarAtomsCombo->addItem(QString::fromUtf8(entry));
    g_free(entry);
    idx++;
  }
  vasp_gui.calc.atoms_total = idx;
  /* Add "ADD atom" entry */
  m_poscarAtomsCombo->addItem(tr("ADD atom"));
  /* Select last item (ADD atom) */
  m_poscarAtomsCombo->setCurrentIndex(idx);
}

void VaspDialog::setupKpointsPage()
{
  auto *page = new QWidget();
  auto *mainLayout = new QVBoxLayout(page);
  mainLayout->setSpacing(8);

  /* --- from INCAR frame (duplicates ELECT-I ISMEAR/KGAMMA/KSPACING) --- */
  {
    auto *frame = new QGroupBox(tr("from INCAR"), page);
    auto *layout = new QGridLayout(frame);
    layout->setSpacing(4);

    m_kpointsIsmearEdit = new QLineEdit();
    layout->addWidget(new QLabel(tr("ISMEAR=")), 0, 0);
    layout->addWidget(m_kpointsIsmearEdit, 0, 1);

    m_kpointsGammaCheck = new QCheckBox(tr("KGAMMA"));
    layout->addWidget(m_kpointsGammaCheck, 0, 2);

    m_kpointsKspacingEdit = new QLineEdit();
    layout->addWidget(new QLabel(tr("KSPACING=")), 0, 3);
    layout->addWidget(m_kpointsKspacingEdit, 0, 4);

    mainLayout->addWidget(frame);
  }

  /* --- General frame --- */
  {
    auto *frame = new QGroupBox(tr("General"), page);
    auto *layout = new QGridLayout(frame);
    layout->setSpacing(4);

    /* Row 0: mode + Cartesian + KX + KY + KZ */
    m_kpointsModeCombo = new QComboBox();
    m_kpointsModeCombo->addItems({tr("Manual Entry"), tr("Line Mode"), tr("Automatic (M&P)"), tr("Gamma (M&P)"),
                                  tr("Monkhorst-Pack classic"), tr("Basis set definition")});
    /* Set mode from vasp_gui and update field enable states */
    int kpModeIdx = 0;
    switch (vasp_gui.calc.kpoints_mode)
    {
    case VKP_MAN:
      kpModeIdx = 0;
      break;
    case VKP_LINE:
      kpModeIdx = 1;
      break;
    case VKP_AUTO:
      kpModeIdx = 2;
      break;
    case VKP_GAMMA:
      kpModeIdx = 3;
      break;
    case VKP_MP:
      kpModeIdx = 4;
      break;
    case VKP_BASIS:
      kpModeIdx = 5;
      break;
    default:
      kpModeIdx = 0;
      break;
    }
    m_kpointsModeCombo->setCurrentIndex(kpModeIdx);
    layout->addWidget(m_kpointsModeCombo, 0, 0, 1, 2);

    m_kpointsCartCheck = new QCheckBox(tr("Cartesian"));
    layout->addWidget(m_kpointsCartCheck, 0, 2);

    layout->addWidget(new QLabel(tr("KX")), 0, 3);
    m_kpointsKxSpin = new QSpinBox();
    m_kpointsKxSpin->setRange(1, 100);
    m_kpointsKxSpin->setValue(1);
    layout->addWidget(m_kpointsKxSpin, 0, 4);

    layout->addWidget(new QLabel(tr("KY")), 0, 5);
    m_kpointsKySpin = new QSpinBox();
    m_kpointsKySpin->setRange(1, 100);
    m_kpointsKySpin->setValue(1);
    layout->addWidget(m_kpointsKySpin, 0, 6);

    layout->addWidget(new QLabel(tr("KZ")), 0, 7);
    m_kpointsKzSpin = new QSpinBox();
    m_kpointsKzSpin->setRange(1, 100);
    m_kpointsKzSpin->setValue(1);
    layout->addWidget(m_kpointsKzSpin, 0, 8);

    /* Row 1: NKPTS + Tetrahedron + SX + SY + SZ */
    layout->addWidget(new QLabel(tr("NKPTS=")), 1, 0);
    m_kpointsNkptsEdit = new QLineEdit();
    m_kpointsNkptsEdit->setPlaceholderText("0");
    layout->addWidget(m_kpointsNkptsEdit, 1, 1);

    m_tetraCheck = new QCheckBox(tr("Tetrahedron"));
    layout->addWidget(m_tetraCheck, 1, 2);

    layout->addWidget(new QLabel(tr("SX=")), 1, 3);
    m_kpointsSxSpin = new QDoubleSpinBox();
    m_kpointsSxSpin->setRange(-10.0, 10.0);
    m_kpointsSxSpin->setDecimals(4);
    layout->addWidget(m_kpointsSxSpin, 1, 4);

    layout->addWidget(new QLabel(tr("SY=")), 1, 5);
    m_kpointsSyEdit = new QLineEdit();
    m_kpointsSyEdit->setText("0.0000");
    layout->addWidget(m_kpointsSyEdit, 1, 6);

    layout->addWidget(new QLabel(tr("SZ=")), 1, 7);
    m_kpointsSzEdit = new QLineEdit();
    m_kpointsSzEdit->setText("0.0000");
    layout->addWidget(m_kpointsSzEdit, 1, 8);

    mainLayout->addWidget(frame);
  }

  /* --- Coordinate frame --- */
  {
    auto *frame = new QGroupBox(tr("Coordinate"), page);
    auto *layout = new QGridLayout(frame);
    layout->setSpacing(4);
    layout->setContentsMargins(4, 4, 4, 4);

    /* Row 0: KPOINT: combo + Apply + Delete */
    m_kpointsKptsCombo = new QComboBox();
    m_kpointsKptsCombo->addItem(tr("ADD kpoint"));
    m_kpointsApplyBtn = new QPushButton(tr("Apply"));
    m_kpointsDelBtn = new QPushButton(tr("Delete"));
    layout->addWidget(new QLabel(tr("KPOINT:")), 0, 0);
    layout->addWidget(m_kpointsKptsCombo, 0, 1, 1, 7);
    layout->addWidget(m_kpointsApplyBtn, 0, 8);
    layout->addWidget(m_kpointsDelBtn, 0, 9);

    /* Row 1: INDEX + X + Y + Z + W */
    layout->addWidget(new QLabel(tr("INDEX:")), 1, 0);
    m_kpointsIndexEdit = new QLineEdit();
    m_kpointsIndexEdit->setText("0");
    m_kpointsIndexEdit->setReadOnly(true);
    layout->addWidget(m_kpointsIndexEdit, 1, 1);

    layout->addWidget(new QLabel(tr("X=")), 1, 2);
    m_kpointsXEdit = new QLineEdit();
    m_kpointsXEdit->setText("0.0000");
    layout->addWidget(m_kpointsXEdit, 1, 3);

    layout->addWidget(new QLabel(tr("Y=")), 1, 4);
    m_kpointsYEdit = new QLineEdit();
    m_kpointsYEdit->setText("0.0000");
    layout->addWidget(m_kpointsYEdit, 1, 5);

    layout->addWidget(new QLabel(tr("Z=")), 1, 6);
    m_kpointsZEdit = new QLineEdit();
    m_kpointsZEdit->setText("0.0000");
    layout->addWidget(m_kpointsZEdit, 1, 7);

    layout->addWidget(new QLabel(tr("W=")), 1, 8);
    m_kpointsWEdit = new QLineEdit();
    m_kpointsWEdit->setText("0.0000");
    layout->addWidget(m_kpointsWEdit, 1, 9);

    mainLayout->addWidget(frame);
  }

  /* --- Tetrahedron frame --- */
  {
    auto *frame = new QGroupBox(tr("Tetrahedron"), page);
    m_tetraFrame = frame;
    auto *layout = new QGridLayout(frame);
    layout->setSpacing(4);

    /* Row 0: TOTAL= + EXACT VOLUME: + V= + for (A,B,C,D) */
    layout->addWidget(new QLabel(tr("TOTAL=")), 0, 0);
    m_tetraTotalEdit = new QLineEdit();
    layout->addWidget(m_tetraTotalEdit, 0, 1);

    layout->addWidget(new QLabel(tr("EXACT VOLUME:")), 0, 2);
    layout->addWidget(new QLabel(tr("V=")), 0, 3);
    m_tetraVolumeEdit = new QLineEdit();
    layout->addWidget(m_tetraVolumeEdit, 0, 4);

    auto *cornersLabel = new QLabel(tr("for (A,B,C,D) tetrahedron"));
    layout->addWidget(cornersLabel, 0, 5, 1, 4);

    /* Row 1: TETRAHEDRON: combo + Apply + Delete */
    layout->addWidget(new QLabel(tr("TETRAHEDRON:")), 1, 0);
    m_tetraCombo = new QComboBox();
    m_tetraCombo->addItem(tr("ADD tetrahedron"));
    layout->addWidget(m_tetraCombo, 1, 1, 1, 9);

    m_tetraApplyBtn = new QPushButton(tr("Apply"));
    m_tetraDelBtn = new QPushButton(tr("Delete"));
    layout->addWidget(m_tetraApplyBtn, 1, 10);
    layout->addWidget(m_tetraDelBtn, 1, 11);

    /* Row 2: INDEX + Degen. W= + PtA: + PtB: + PtC: + PtD: */
    layout->addWidget(new QLabel(tr("INDEX:")), 2, 0);
    m_tetraIndexEdit = new QLineEdit();
    m_tetraIndexEdit->setText("0");
    m_tetraIndexEdit->setReadOnly(true);
    layout->addWidget(m_tetraIndexEdit, 2, 1);

    layout->addWidget(new QLabel(tr("Degen. W=")), 2, 2);
    m_tetraWEdit = new QLineEdit();
    layout->addWidget(m_tetraWEdit, 2, 3);

    layout->addWidget(new QLabel(tr("PtA:")), 2, 4);
    m_tetraPtAEdit = new QLineEdit();
    layout->addWidget(m_tetraPtAEdit, 2, 5);

    layout->addWidget(new QLabel(tr("PtB:")), 2, 6);
    m_tetraPtBEdit = new QLineEdit();
    layout->addWidget(m_tetraPtBEdit, 2, 7);

    layout->addWidget(new QLabel(tr("PtC:")), 2, 8);
    m_tetraPtCEdit = new QLineEdit();
    layout->addWidget(m_tetraPtCEdit, 2, 9);

    layout->addWidget(new QLabel(tr("PtD:")), 2, 10);
    m_tetraPtDEdit = new QLineEdit();
    layout->addWidget(m_tetraPtDEdit, 2, 11);

    mainLayout->addWidget(frame);
  }

  /* KPOINTS mode → enable/disable fields */
  auto updateModeEnabled = [this]() {
    int mode = m_kpointsModeCombo->currentIndex();

    /* Manual Entry (0): Cartesian, NKPTS, SX/SY/SZ, Coordinate frame; disable KX/KY/KZ */
    /* Line Mode (1): Cartesian, NKPTS, Coordinate frame (no W); disable KX/KY/KZ, SX/SY/SZ, W */
    /* Automatic (M&P) (2): KX; disable Cartesian, KY/KZ, NKPTS, SX/SY/SZ, Coordinate */
    /* Gamma (M&P) (3): KX/KY/KZ, SX/SY/SZ; disable Cartesian, NKPTS, Coordinate */
    /* Monkhorst-Pack classic (4): same as Gamma */
    /* Basis set definition (5): Cartesian, NKPTS, Coordinate (no W); disable KX/KY/KZ, SX/SY/SZ, W */

    bool enableCartesian = (mode == 0 || mode == 1 || mode == 5);
    bool enableKX = (mode == 2 || mode == 3 || mode == 4);
    bool enableKY = (mode == 3 || mode == 4);
    bool enableKZ = (mode == 3 || mode == 4);
    bool enableNKPTS = (mode == 0 || mode == 1 || mode == 5);
    bool enableSX = (mode == 3 || mode == 4);
    bool enableSY = (mode == 3 || mode == 4);
    bool enableSZ = (mode == 3 || mode == 4);
    bool enableCoordFrame = (mode == 0 || mode == 1 || mode == 5);
    bool enableW = (mode == 0); /* W enabled only for Manual Entry

    m_kpointsCartCheck->setEnabled(enableCartesian);
    m_kpointsKxSpin->setEnabled(enableKX);
    m_kpointsKySpin->setEnabled(enableKY);
    m_kpointsKzSpin->setEnabled(enableKZ);
    m_kpointsNkptsEdit->setEnabled(enableNKPTS);
    m_kpointsSxSpin->setEnabled(enableSX);
    m_kpointsSyEdit->setEnabled(enableSY);
    m_kpointsSzEdit->setEnabled(enableSZ);

    /* Coordinate frame */
    m_kpointsKptsCombo->setEnabled(enableCoordFrame);
    m_kpointsApplyBtn->setEnabled(enableCoordFrame);
    m_kpointsDelBtn->setEnabled(enableCoordFrame);
    m_kpointsIndexEdit->setEnabled(enableCoordFrame);
    m_kpointsXEdit->setEnabled(enableCoordFrame);
    m_kpointsYEdit->setEnabled(enableCoordFrame);
    m_kpointsZEdit->setEnabled(enableCoordFrame);
    m_kpointsWEdit->setEnabled(enableW);

    /* Tetrahedron: only Manual Entry + ISMEAR=-5 */
    bool tetraEnabled = (mode == 0 && m_kpointsIsmearEdit->text().toInt() == -5);
    m_tetraCheck->setEnabled(tetraEnabled);
    m_tetraFrame->setEnabled(tetraEnabled && m_tetraCheck->isChecked());
  };

  connect(m_kpointsIsmearEdit, &QLineEdit::textChanged, this,
          [this, updateModeEnabled](const QString &) { updateModeEnabled(); });

  connect(m_kpointsModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
    int mode = idx;
    bool enableCartesian = (mode == 0 || mode == 1 || mode == 5);
    bool enableKX = (mode == 2 || mode == 3 || mode == 4);
    bool enableKY = (mode == 3 || mode == 4);
    bool enableKZ = (mode == 3 || mode == 4);
    bool enableNKPTS = (mode == 0 || mode == 1 || mode == 5);
    bool enableSX = (mode == 3 || mode == 4);
    bool enableSY = (mode == 3 || mode == 4);
    bool enableSZ = (mode == 3 || mode == 4);
    bool enableCoordFrame = (mode == 0 || mode == 1 || mode == 5);
    bool enableW = (mode == 0);
    m_kpointsCartCheck->setEnabled(enableCartesian);
    m_kpointsKxSpin->setEnabled(enableKX);
    m_kpointsKySpin->setEnabled(enableKY);
    m_kpointsKzSpin->setEnabled(enableKZ);
    m_kpointsNkptsEdit->setEnabled(enableNKPTS);
    m_kpointsSxSpin->setEnabled(enableSX);
    m_kpointsSyEdit->setEnabled(enableSY);
    m_kpointsSzEdit->setEnabled(enableSZ);
    m_kpointsKptsCombo->setEnabled(enableCoordFrame);
    m_kpointsApplyBtn->setEnabled(enableCoordFrame);
    m_kpointsDelBtn->setEnabled(enableCoordFrame);
    m_kpointsIndexEdit->setEnabled(enableCoordFrame);
    m_kpointsXEdit->setEnabled(enableCoordFrame);
    m_kpointsYEdit->setEnabled(enableCoordFrame);
    m_kpointsZEdit->setEnabled(enableCoordFrame);
    m_kpointsWEdit->setEnabled(enableW);
    bool tetraEnabled = (mode == 0 && m_kpointsIsmearEdit->text().toInt() == -5);
    m_tetraCheck->setEnabled(tetraEnabled);
    m_tetraFrame->setEnabled(tetraEnabled && m_tetraCheck->isChecked());
  });

  connect(m_tetraCheck, &QCheckBox::toggled, this, [this, updateModeEnabled](bool) { updateModeEnabled(); });

  /* KPOINT combo selection → load X/Y/Z/W from combo text */
  connect(m_kpointsKptsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
    if (idx < 0)
      return;
    QString text = m_kpointsKptsCombo->itemText(idx);
    m_kpointsIndexEdit->setText(QString::number(idx + 1));
    if (text == "ADD kpoint")
      return;
    /* Parse: "x y z [w] ! kpoint: n" */
    bool ok;
    QStringList parts = text.split(' ', Qt::SkipEmptyParts);
    if (parts.size() >= 3)
    {
      m_kpointsXEdit->setText(parts[0]);
      m_kpointsYEdit->setText(parts[1]);
      m_kpointsZEdit->setText(parts[2]);
      if (vasp_gui.calc.kpoints_mode == VKP_LINE)
      {
        /* Line mode: no weight */
        m_kpointsWEdit->setText("0.0000");
      } else if (parts.size() >= 4)
      {
        m_kpointsWEdit->setText(parts[3]);
      }
    }
  });

  /* Apply k-point (add or modify) */
  connect(m_kpointsApplyBtn, &QPushButton::clicked, this, [this]() {
    bool ok;
    int idx = m_kpointsKptsCombo->currentIndex();
    if (idx < 0)
      return;
    QString text = m_kpointsKptsCombo->itemText(idx);

    double x = m_kpointsXEdit->text().toDouble(&ok);
    double y = m_kpointsYEdit->text().toDouble(&ok);
    double z = m_kpointsZEdit->text().toDouble(&ok);
    double w = m_kpointsWEdit->text().toDouble(&ok);

    /* Update vasp_gui.calc fields */
    vasp_gui.calc.kpoints_i = idx + 1;
    vasp_gui.calc.kpoints_x = x;
    vasp_gui.calc.kpoints_y = y;
    vasp_gui.calc.kpoints_z = z;
    vasp_gui.calc.kpoints_w = w;

    /* Build combo text */
    QString comboText;
    if (vasp_gui.calc.kpoints_mode == VKP_LINE)
    {
      comboText = QString("%1 %2 %3 ! kpoint: %4").arg(x, 0, 'f', 8).arg(y, 0, 'f', 8).arg(z, 0, 'f', 8).arg(idx + 1);
    } else
    {
      comboText = QString("%1 %2 %3 %4 ! kpoint: %5")
                      .arg(x, 0, 'f', 8)
                      .arg(y, 0, 'f', 8)
                      .arg(z, 0, 'f', 8)
                      .arg(w, 0, 'f', 8)
                      .arg(idx + 1);
    }

    if (text == "ADD kpoint")
    {
      /* Insert at current position */
      m_kpointsKptsCombo->insertItem(idx, comboText);
    } else
    {
      /* Modify existing entry */
      m_kpointsKptsCombo->setItemText(idx, comboText);
    }

    /* Re-select current entry */
    m_kpointsKptsCombo->setCurrentIndex(idx);
  });

  /* Delete k-point */
  connect(m_kpointsDelBtn, &QPushButton::clicked, this, [this]() {
    int idx = m_kpointsKptsCombo->currentIndex();
    if (idx < 0)
    {
      m_kpointsKptsCombo->setCurrentIndex(0);
      return;
    }
    QString text = m_kpointsKptsCombo->itemText(idx);
    if (text == "ADD kpoint")
      return;

    m_kpointsKptsCombo->removeItem(idx);

    /* Rebuild combo indices */
    for (int i = 0; i < m_kpointsKptsCombo->count(); i++)
    {
      QString t = m_kpointsKptsCombo->itemText(i);
      if (t != "ADD kpoint")
      {
        /* Renumber */
        bool ok;
        QStringList parts = t.split(' ', Qt::SkipEmptyParts);
        if (parts.size() >= 3)
        {
          double x = parts[0].toDouble(&ok);
          double y = parts[1].toDouble(&ok);
          double z = parts[2].toDouble(&ok);
          double w = 0.0;
          if (vasp_gui.calc.kpoints_mode != VKP_LINE && parts.size() >= 4)
          {
            w = parts[3].toDouble(&ok);
          }
          QString newT;
          if (vasp_gui.calc.kpoints_mode == VKP_LINE)
          {
            newT = QString("%1 %2 %3 ! kpoint: %4").arg(x, 0, 'f', 8).arg(y, 0, 'f', 8).arg(z, 0, 'f', 8).arg(i + 1);
          } else
          {
            newT = QString("%1 %2 %3 %4 ! kpoint: %5")
                       .arg(x, 0, 'f', 8)
                       .arg(y, 0, 'f', 8)
                       .arg(z, 0, 'f', 8)
                       .arg(w, 0, 'f', 8)
                       .arg(i + 1);
          }
          m_kpointsKptsCombo->setItemText(i, newT);
        }
      }
    }

    /* Select next or previous entry */
    int newIdx = qMin(idx, m_kpointsKptsCombo->count() - 1);
    if (newIdx < 0)
      newIdx = 0;
    m_kpointsKptsCombo->setCurrentIndex(newIdx);
  });

  /* Tetrahedron combo selection → load W/PtA/PtB/PtC/PtD from combo text */
  connect(m_tetraCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
    if (idx < 0)
      return;
    QString text = m_tetraCombo->itemText(idx);
    m_tetraIndexEdit->setText(QString::number(idx));
    if (text == "ADD tetrahedron")
      return;
    /* Parse: "w PtA PtB PtC PtD ! tetrahedron: n" */
    bool ok;
    QStringList parts = text.split(' ', Qt::SkipEmptyParts);
    if (parts.size() >= 5)
    {
      m_tetraWEdit->setText(parts[0]);
      m_tetraPtAEdit->setText(parts[1]);
      m_tetraPtBEdit->setText(parts[2]);
      m_tetraPtCEdit->setText(parts[3]);
      m_tetraPtDEdit->setText(parts[4]);
    }
  });

  /* Apply tetrahedron (add or modify) */
  connect(m_tetraApplyBtn, &QPushButton::clicked, this, [this]() {
    bool ok;
    int idx = m_tetraCombo->currentIndex();
    if (idx < 0)
      return;
    QString text = m_tetraCombo->itemText(idx);

    double w = m_tetraWEdit->text().toDouble(&ok);
    int a = m_tetraPtAEdit->text().toInt(&ok);
    int b = m_tetraPtBEdit->text().toInt(&ok);
    int c = m_tetraPtCEdit->text().toInt(&ok);
    int d = m_tetraPtDEdit->text().toInt(&ok);

    /* Update vasp_gui.calc fields */
    vasp_gui.calc.tetra_i = idx;
    vasp_gui.calc.tetra_w = w;
    vasp_gui.calc.tetra_a = a;
    vasp_gui.calc.tetra_b = b;
    vasp_gui.calc.tetra_c = c;
    vasp_gui.calc.tetra_d = d;

    /* Build combo text: "w PtA PtB PtC PtD ! tetrahedron: n" */
    QString comboText =
        QString("%1 %2 %3 %4 %5 ! tetrahedron: %6").arg(w, 0, 'f', 4).arg(a).arg(b).arg(c).arg(d).arg(idx);

    if (text == "ADD tetrahedron")
    {
      m_tetraCombo->insertItem(idx, comboText);
    } else
    {
      m_tetraCombo->setItemText(idx, comboText);
    }

    m_tetraCombo->setCurrentIndex(idx);
  });

  /* Delete tetrahedron */
  connect(m_tetraDelBtn, &QPushButton::clicked, this, [this]() {
    int idx = m_tetraCombo->currentIndex();
    if (idx < 0)
    {
      m_tetraCombo->setCurrentIndex(0);
      return;
    }
    QString text = m_tetraCombo->itemText(idx);
    if (text == "ADD tetrahedron")
      return;

    m_tetraCombo->removeItem(idx);

    /* Renumber remaining entries */
    for (int i = 0; i < m_tetraCombo->count(); i++)
    {
      QString t = m_tetraCombo->itemText(i);
      if (t == "ADD tetrahedron")
        continue;
      bool ok;
      QStringList parts = t.split(' ', Qt::SkipEmptyParts);
      if (parts.size() >= 5)
      {
        QString newT = QString("%1 %2 %3 %4 %5 ! tetrahedron: %6")
                           .arg(parts[0])
                           .arg(parts[1])
                           .arg(parts[2])
                           .arg(parts[3])
                           .arg(parts[4])
                           .arg(i);
        m_tetraCombo->setItemText(i, newT);
      }
    }

    int newIdx = qMin(idx, m_tetraCombo->count() - 1);
    if (newIdx < 0)
      newIdx = 0;
    m_tetraCombo->setCurrentIndex(newIdx);
  });

  mainLayout->addStretch();
  m_notebook->addTab(page, tr("KPOINTS"));
}

/* Helper: register a matching folder as a flavor option in the combo */
void VaspDialog::potcar_folder_register(gchar *item)
{
  gchar elem[3] = {item[0], '\0', '\0'};
  if (g_ascii_islower(item[1]))
    elem[1] = item[1];
  gchar *text = g_strdup_printf("%s: %s", elem, item);
  m_potcarFlavorCombo->addItem(QString::fromUtf8(text));
  g_free(text);
}

/* Helper: check if a folder name's element symbol exactly matches a species */
static gboolean folder_matches_species(gchar *item, const gchar *speciesSymbols)
{
  if (!speciesSymbols)
    return FALSE;

  gchar sym[3] = {item[0], '\0', '\0'};
  if (g_ascii_islower(item[1]))
    sym[1] = item[1];

  /* Search for exact species symbol match */
  const gchar *p = speciesSymbols;
  while (*p)
  {
    /* Compare symbol length */
    if (sym[1] == '\0')
    {
      /* Single char species: must match exactly (next char is space or end) */
      if (*p == sym[0] && (p[1] == ' ' || p[1] == '\0'))
        return TRUE;
    } else
    {
      /* Two char species: must match exactly (next char is space or end) */
      if (p[0] == sym[0] && p[1] == sym[1] && (p[2] == ' ' || p[2] == '\0'))
        return TRUE;
    }
    p++;
  }
  return FALSE;
}

/* Helper: parse POTCAR folder to extract species and flavors */
void VaspDialog::potcar_folder_get_info(gchar *folderPath)
{
  update_species_from_model();
  if (!folderPath)
    return;
  GSList *folders = file_dir_list(folderPath, FALSE);
  g_free(folderPath);

  /* Wipe existing flavor combo entries */
  m_potcarFlavorCombo->clear();

  /* Collect ALL matching folders into combo (in folder order, user picks) */
  for (GSList *f = folders; f; f = g_slist_next(f))
  {
    gchar *item = (gchar *) f->data;
    if (g_str_has_prefix(item, "."))
      continue;
    if (folder_matches_species(item, vasp_gui.calc.species_symbols))
    {
      potcar_folder_register(item);
    }
  }

  /* Build species list and flavors in POSCAR order */
  GString *speciesStr = g_string_new("");
  GString *flavorStr = g_string_new("");

  /* Parse species symbols from POSCAR */
  const gchar *p = vasp_gui.calc.species_symbols;
  while (*p)
  {
    /* Extract one species symbol */
    gchar sym[3] = {0};
    int len = 0;
    while (*p && *p != ' ' && len < 2)
      sym[len++] = *p++;
    sym[len] = '\0';
    if (len == 0)
    {
      if (*p)
        p++;
      continue;
    }
    if (*p == ' ')
      p++;

    /* Add to species string */
    if (speciesStr->len > 0)
      g_string_append(speciesStr, " ");
    g_string_append(speciesStr, sym);

    /* Try exact match first, then prefix match */
    gboolean found = FALSE;
    /* Pass 1: exact match */
    for (int i = 0; i < m_potcarFlavorCombo->count() && !found; i++)
    {
      QString comboText = m_potcarFlavorCombo->itemText(i);
      int colonPos = comboText.indexOf(':');
      if (colonPos >= 0)
      {
        QString comboSymbol = comboText.left(colonPos).trimmed();
        QString comboFlavor = comboText.mid(colonPos + 1).trimmed();
        if (comboSymbol == QString(sym) && comboFlavor == QString(sym))
        {
          if (flavorStr->len > 0)
            g_string_append(flavorStr, " ");
          g_string_append(flavorStr, comboFlavor.toUtf8().constData());
          found = TRUE;
        }
      }
    }
    /* Pass 2: prefix match */
    if (!found)
    {
      for (int i = 0; i < m_potcarFlavorCombo->count(); i++)
      {
        QString comboText = m_potcarFlavorCombo->itemText(i);
        int colonPos = comboText.indexOf(':');
        if (colonPos >= 0)
        {
          QString comboSymbol = comboText.left(colonPos).trimmed();
          QString comboFlavor = comboText.mid(colonPos + 1).trimmed();
          if (comboSymbol == QString(sym) && comboFlavor.startsWith(sym))
          {
            if (flavorStr->len > 0)
              g_string_append(flavorStr, " ");
            g_string_append(flavorStr, comboFlavor.toUtf8().constData());
            found = TRUE;
            break;
          }
        }
      }
    }
  }

  g_free(vasp_gui.calc.potcar_species);
  vasp_gui.calc.potcar_species = g_strdup(speciesStr->str);
  g_free(vasp_gui.calc.potcar_species_flavor);
  vasp_gui.calc.potcar_species_flavor = g_strdup(flavorStr->str);

  m_potcarDetectedSpeciesEdit->setText(QString::fromUtf8(vasp_gui.calc.potcar_species));
  m_potcarDetectedFlavorEdit->setText(QString::fromUtf8(vasp_gui.calc.potcar_species_flavor));

  g_string_free(speciesStr, TRUE);
  g_string_free(flavorStr, TRUE);
  g_slist_free(folders);
}

void VaspDialog::setupPotcarPage()
{
  auto *page = new QWidget();
  auto *mainLayout = new QVBoxLayout(page);
  mainLayout->setSpacing(8);

  /* --- General frame --- */
  {
    auto *frame = new QGroupBox(tr("General"), page);
    auto *layout = new QGridLayout(frame);
    layout->setSpacing(4);

    /* Row 0: SPECIES: (disabled, spans full width) */
    m_potcarSpeciesEdit = new QLineEdit();
    m_potcarSpeciesEdit->setReadOnly(true);
    m_potcarSpeciesEdit->setEnabled(false);
    layout->addWidget(new QLabel(tr("SPECIES:")), 0, 0);
    layout->addWidget(m_potcarSpeciesEdit, 0, 1);

    /* Row 1: note label */
    auto *noteLabel = new QLabel(tr("(each species will require a separate POTCAR information)"));
    noteLabel->setWordWrap(true);
    layout->addWidget(noteLabel, 1, 0, 1, 2);

    mainLayout->addWidget(frame);
  }

  /* --- get POTCAR information frame --- */
  {
    auto *frame = new QGroupBox(tr("get POTCAR information"), page);
    auto *layout = new QGridLayout(frame);
    layout->setSpacing(4);

    /* Row 0: radio buttons + FILE: + button */
    m_potcarSelectFileRadio = new QRadioButton(tr("Use POTCAR file"));
    m_potcarSelectFolderRadio = new QRadioButton(tr("Use POTCAR path"));
    auto *radioLayout = new QVBoxLayout();
    radioLayout->setContentsMargins(0, 0, 0, 0);
    radioLayout->setSpacing(4);
    radioLayout->addWidget(m_potcarSelectFileRadio);
    radioLayout->addWidget(m_potcarSelectFolderRadio);
    layout->addLayout(radioLayout, 0, 0, 2, 1);

    m_potcarSelectFileRadio->setChecked(true);

    layout->addWidget(new QLabel(tr("FILE:")), 0, 1);
    m_potcarFileEdit = new QLineEdit();
    layout->addWidget(m_potcarFileEdit, 0, 2);
    m_potcarFileBtn = new QPushButton(tr("..."));
    layout->addWidget(m_potcarFileBtn, 0, 3);

    layout->addWidget(new QLabel(tr("PATH:")), 1, 1);
    m_potcarFolderEdit = new QLineEdit();
    layout->addWidget(m_potcarFolderEdit, 1, 2);
    m_potcarFolderBtn = new QPushButton(tr("..."));
    layout->addWidget(m_potcarFolderBtn, 1, 3);

    /* Row 2: FLAVOR: + button */
    layout->addWidget(new QLabel(tr("FLAVOR:")), 2, 1);
    m_potcarFlavorCombo = new QComboBox();
    m_potcarFlavorCombo->addItems({tr("PAW"), tr("US (ultrasoft)"), tr("HB (hard)")});
    layout->addWidget(m_potcarFlavorCombo, 2, 2);
    m_potcarApplyBtn = new QPushButton(tr("Apply"));
    layout->addWidget(m_potcarApplyBtn, 2, 3);

    mainLayout->addWidget(frame);
  }

  /* --- POTCAR results frame --- */
  {
    auto *frame = new QGroupBox(tr("POTCAR results"), page);
    auto *layout = new QGridLayout(frame);
    layout->setSpacing(4);

    /* Row 0: DETECTED label + SPECIES: */
    auto *detectedLabel = new QLabel(tr("DETECTED"));
    detectedLabel->setFont(QFont(detectedLabel->font().family(), detectedLabel->font().pointSize(), QFont::Bold));
    layout->addWidget(detectedLabel, 0, 0);

    layout->addWidget(new QLabel(tr("SPECIES:")), 0, 1);
    m_potcarDetectedSpeciesEdit = new QLineEdit();
    m_potcarDetectedSpeciesEdit->setReadOnly(true);
    layout->addWidget(m_potcarDetectedSpeciesEdit, 0, 2);

    /* Row 1: FLAVOR: */
    layout->addWidget(new QLabel(tr("FLAVOR:")), 1, 1);
    m_potcarDetectedFlavorEdit = new QLineEdit();
    m_potcarDetectedFlavorEdit->setReadOnly(true);
    layout->addWidget(m_potcarDetectedFlavorEdit, 1, 2);

    mainLayout->addWidget(frame);
  }

  /* Radio toggle: enable/disable fields based on selection */
  connect(m_potcarSelectFileRadio, &QRadioButton::toggled, this, &VaspDialog::on_potcar_mode_changed);
  connect(m_potcarSelectFolderRadio, &QRadioButton::toggled, this, &VaspDialog::on_potcar_mode_changed);

  /* FILE: ... button */
  connect(m_potcarFileBtn, &QPushButton::clicked, this, [this]() {
    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::ExistingFile);
    dlg.setNameFilter("POTCAR files (POTCAR)");
    dlg.setWindowTitle("Select a POTCAR File");
    if (dlg.exec() == QDialog::Accepted)
    {
      QStringList files = dlg.selectedFiles();
      if (!files.isEmpty())
      {
        QString path = files.first();
        m_potcarFileEdit->setText(path);
        g_free(vasp_gui.calc.potcar_file);
        vasp_gui.calc.potcar_file = g_strdup(path.toUtf8().constData());

        /* Parse POTCAR to get species and detect PAW */
        FILE *fp = fopen(path.toUtf8().constData(), "r");
        if (fp)
        {
          g_free(vasp_gui.calc.potcar_species);
          vasp_gui.calc.potcar_species = g_strdup(" ");
          vasp_gui.calc.have_paw = FALSE;

          gchar *line = file_read_line(fp);
          if (line)
          {
            /* First line: pseudotype (PAW/PSCTR) + element symbols */
            gchar sym[4] = {0};
            if (sscanf(line, " %c%c%c", &(sym[0]), &(sym[1]), &(sym[2])) == 3)
            {
              sym[3] = '\0';
              if (g_ascii_strcasecmp(sym, "PAW") == 0)
              {
                vasp_gui.calc.have_paw = TRUE;
                if (m_havePawCheck)
                  m_havePawCheck->setChecked(TRUE);
              }
            }
            g_free(line);
          }

          while ((line = file_read_line(fp)))
          {
            if (strstr(line, "TITEL") != NULL)
            {
              /* Match original: sscanf(line, " TITEL = %*s %c%c%*s", &(sym[0]), &(sym[1])) */
              gchar sym[4] = {0};
              sym[2] = '\0';
              sscanf(line, " TITEL = %*s %c%c%*s", &(sym[0]), &(sym[1]));
              gchar *tamp = g_strdup_printf("%s %s", vasp_gui.calc.potcar_species, sym);
              g_free(vasp_gui.calc.potcar_species);
              vasp_gui.calc.potcar_species = tamp;
            }
            g_free(line);
          }
          fclose(fp);

          m_potcarDetectedSpeciesEdit->setText(QString::fromUtf8(vasp_gui.calc.potcar_species));
          /* Also update simple_species */
          m_simpleSpeciesEdit->setText(QString::fromUtf8(vasp_gui.calc.potcar_species));
          /* Clear flavor fields — FILE mode doesn't use flavors */
          g_free(vasp_gui.calc.potcar_species_flavor);
          vasp_gui.calc.potcar_species_flavor = NULL;
          m_potcarDetectedFlavorEdit->setText("");
          m_potcarFlavorCombo->clear();
        }
      }
    }
  });

  /* PATH: ... button */
  connect(m_potcarFolderBtn, &QPushButton::clicked, this, [this]() {
    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::Directory);
    dlg.setWindowTitle("Select the POTCAR folder");
    if (dlg.exec() == QDialog::Accepted)
    {
      QStringList dirs = dlg.selectedFiles();
      if (!dirs.isEmpty())
      {
        QString path = dirs.first();
        m_potcarFolderEdit->setText(path);
        g_free(vasp_gui.calc.potcar_folder);
        vasp_gui.calc.potcar_folder = g_strdup(path.toUtf8().constData());
        /* Parse folder for species info and populate flavor combo */
        potcar_folder_get_info(g_strdup(path.toUtf8().constData()));
      }
    }
  });

  /* Apply flavor button */
  connect(m_potcarApplyBtn, &QPushButton::clicked, this, [this]() {
    int currentIdx = m_potcarFlavorCombo->currentIndex();
    if (currentIdx < 0)
      return;
    QString comboText = m_potcarFlavorCombo->itemText(currentIdx);
    int colonPos = comboText.indexOf(':');
    if (colonPos < 0)
      return;
    QString symbol = comboText.left(colonPos).trimmed();
    QString flavor = comboText.mid(colonPos + 1).trimmed();

    if (!vasp_gui.calc.potcar_species_flavor)
      return;

    /* Find the index of this symbol in the species list */
    QString speciesList = QString::fromUtf8(vasp_gui.calc.species_symbols);
    QStringList speciesParts = speciesList.split(' ', Qt::SkipEmptyParts);
    int symIdx = -1;
    for (int i = 0; i < speciesParts.size(); i++)
    {
      if (speciesParts[i] == symbol)
      {
        symIdx = i;
        break;
      }
    }
    if (symIdx < 0)
      return;

    /* Replace the flavor at that position */
    QString currentFlavor = QString::fromUtf8(vasp_gui.calc.potcar_species_flavor);
    QStringList flavorParts = currentFlavor.split(' ', Qt::SkipEmptyParts);
    while (flavorParts.size() < speciesParts.size())
      flavorParts.append("");
    flavorParts[symIdx] = flavor;

    g_free(vasp_gui.calc.potcar_species_flavor);
    vasp_gui.calc.potcar_species_flavor = g_strdup(flavorParts.join(' ').toUtf8().constData());
    m_potcarDetectedFlavorEdit->setText(vasp_gui.calc.potcar_species_flavor);
  });

  /* Initial state */
  on_potcar_mode_changed();

  mainLayout->addStretch();
  m_notebook->addTab(page, tr("POTCAR"));
}

void VaspDialog::setupExecPage()
{
  auto *page = new QWidget();
  auto *mainLayout = new QVBoxLayout(page);
  mainLayout->setSpacing(8);

  /* --- General frame --- */
  {
    auto *frame = new QGroupBox(tr("General"), page);
    auto *layout = new QGridLayout(frame);
    layout->setSpacing(4);

    /* Row 0: VASP EXEC: FILE= + ... */
    layout->addWidget(new QLabel(tr("VASP EXEC:")), 0, 0);
    m_jobVaspExeEdit = new QLineEdit();
    layout->addWidget(m_jobVaspExeEdit, 0, 1, 1, 4);
    m_jobVaspExeBtn = new QPushButton(tr("..."));
    layout->addWidget(m_jobVaspExeBtn, 0, 5);

    /* Row 1: CALCUL PATH: PATH= + ... */
    layout->addWidget(new QLabel(tr("CALCUL PATH:")), 1, 0);
    m_jobPathEdit = new QLineEdit();
    layout->addWidget(m_jobPathEdit, 1, 1, 1, 4);
    m_jobPathBtn = new QPushButton(tr("..."));
    layout->addWidget(m_jobPathBtn, 1, 5);

    /* Row 2: OUTPUT: LWAVE + LCHARG + LVTOT + LVHAR + LELF */
    layout->addWidget(new QLabel(tr("OUTPUT:")), 2, 0);
    m_lwaveCheck = new QCheckBox(tr("LWAVE"));
    layout->addWidget(m_lwaveCheck, 2, 1);
    m_lchargCheck = new QCheckBox(tr("LCHARG"));
    layout->addWidget(m_lchargCheck, 2, 2);
    m_lvtotCheck = new QCheckBox(tr("LVTOT"));
    layout->addWidget(m_lvtotCheck, 2, 3);
    m_lvharCheck = new QCheckBox(tr("LVHAR"));
    layout->addWidget(m_lvharCheck, 2, 4);
    m_lelfCheck = new QCheckBox(tr("LELF"));
    layout->addWidget(m_lelfCheck, 2, 5);

    mainLayout->addWidget(frame);
  }

  /* --- Parallel Optimization frame --- */
  {
    auto *frame = new QGroupBox(tr("Parallel Optimization"), page);
    auto *layout = new QGridLayout(frame);
    layout->setSpacing(4);

    /* Row 0: MPIRUN: FILE= + ... */
    layout->addWidget(new QLabel(tr("MPIRUN:")), 0, 0);
    m_jobMpirunEdit = new QLineEdit();
    layout->addWidget(m_jobMpirunEdit, 0, 1, 1, 4);
    m_jobMpirunBtn = new QPushButton(tr("..."));
    layout->addWidget(m_jobMpirunBtn, 0, 5);

    /* Row 1: NP= + NCORE= + KPAR= + LPLANE + LSCALU + LSCALAPACK */
    layout->addWidget(new QLabel(tr("NP=")), 1, 0);
    m_jobNprocSpin = new QSpinBox();
    m_jobNprocSpin->setRange(1, 1000);
    layout->addWidget(m_jobNprocSpin, 1, 1);

    layout->addWidget(new QLabel(tr("NCORE=")), 1, 2);
    m_ncoreSpin = new QSpinBox();
    m_ncoreSpin->setRange(1, 1000);
    layout->addWidget(m_ncoreSpin, 1, 3);

    layout->addWidget(new QLabel(tr("KPAR=")), 1, 4);
    m_kparSpin = new QSpinBox();
    m_kparSpin->setRange(1, 1000);
    layout->addWidget(m_kparSpin, 1, 5);

    /* Sync EXEC spinners with PRESET spinners (bidirectional) */
    connect(m_simpleNpSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int v) { m_jobNprocSpin->setValue(v); });
    connect(m_jobNprocSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) {
      m_simpleNpSpin->setValue(v);
      /* Re-apply range constraints from PRESET */
      applyParallelConstraints();
    });
    connect(m_simpleNcoreSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int v) { m_ncoreSpin->setValue(v); });
    connect(m_ncoreSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) {
      m_simpleNcoreSpin->setValue(v);
      applyParallelConstraints();
    });
    connect(m_simpleKparSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int v) { m_kparSpin->setValue(v); });
    connect(m_kparSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) {
      m_simpleKparSpin->setValue(v);
      applyParallelConstraints();
    });

    m_lplaneCheck = new QCheckBox(tr("LPLANE"));
    layout->addWidget(m_lplaneCheck, 1, 6);
    m_lscaluCheck = new QCheckBox(tr("LSCALU"));
    layout->addWidget(m_lscaluCheck, 1, 7);
    m_lscalapackCheck = new QCheckBox(tr("LSCALAPACK"));
    layout->addWidget(m_lscalapackCheck, 1, 8);

    mainLayout->addWidget(frame);
  }

  /* --- Distant calculation frame --- */
  {
    auto *frame = new QGroupBox(tr("Distant calculation"), page);
    auto *vbox = new QVBoxLayout(frame);
    vbox->setSpacing(4);

    auto *label = new QLabel(tr("Remote execution of VASP is currently UNDER CONSTRUCTION."));
    label->setWordWrap(true);
    vbox->addWidget(label);

    label = new QLabel(tr("For now, please save files and manually transfer them to the distant server, then submit "
                          "and retreive files as usual."));
    label->setWordWrap(true);
    vbox->addWidget(label);

    label = new QLabel(tr("The future GDIS job interface will hopefully automate the whole process..."));
    label->setWordWrap(true);
    vbox->addWidget(label);

    label = new QLabel(tr("...and will be made available in the VASP calculation interface at that time."));
    label->setWordWrap(true);
    vbox->addWidget(label);

    auto *sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    vbox->addWidget(sep);

    label = new QLabel(tr("--OVHPA"));
    vbox->addWidget(label);

    mainLayout->addWidget(frame);
  }

  mainLayout->addStretch();
  m_notebook->addTab(page, tr("EXEC"));

  /* Finalize KPOINTS field enable/disable states */
  updateKpointsModeEnabled();
}

void VaspDialog::on_save()
{
  sync();
  poscar_sync();

  gchar *filename;
  FILE *fp;

  /* INCAR */
  filename = g_strdup_printf("%s/INCAR", vasp_gui.calc.job_path);
  fp = fopen(filename, "w");
  if (!fp)
  {
    g_free(filename);
    QMessageBox::critical(this, tr("VASP"), tr("Failed to open INCAR for writing."));
    return;
  }
  vasp_calc_to_incar(fp, vasp_gui.calc);
  fclose(fp);
  g_free(filename);

  /* POSCAR */
  filename = g_strdup_printf("%s/POSCAR", vasp_gui.calc.job_path);
  fp = fopen(filename, "w");
  if (!fp)
  {
    g_free(filename);
    QMessageBox::critical(this, tr("VASP"), tr("Failed to open POSCAR for writing."));
    return;
  }
  write_poscar(fp);
  fclose(fp);
  g_free(filename);

  /* KPOINTS */
  filename = g_strdup_printf("%s/KPOINTS", vasp_gui.calc.job_path);
  fp = fopen(filename, "w");
  if (!fp)
  {
    g_free(filename);
    QMessageBox::critical(this, tr("VASP"), tr("Failed to open KPOINTS for writing."));
    return;
  }
  write_kpoints(fp);
  fclose(fp);
  g_free(filename);

  /* POTCAR */
  filename = g_strdup_printf("%s/POTCAR", vasp_gui.calc.job_path);
  fp = fopen(filename, "w");
  if (!fp)
  {
    g_free(filename);
    QMessageBox::critical(this, tr("VASP"), tr("Failed to open POTCAR for writing."));
    return;
  }
  write_potcar(fp);
  fclose(fp);
  g_free(filename);

  QMessageBox::information(this, tr("VASP"), tr("VASP input files saved successfully."));
}

void VaspDialog::on_run()
{
  sync();
  poscar_sync();

  /* Save using Qt write functions */
  gchar *filename;
  FILE *fp;

  filename = g_strdup_printf("%s/INCAR", vasp_gui.calc.job_path);
  fp = fopen(filename, "w");
  if (!fp)
  {
    g_free(filename);
    QMessageBox::critical(this, tr("VASP"), tr("Failed to open INCAR for writing."));
    return;
  }
  vasp_calc_to_incar(fp, vasp_gui.calc);
  fclose(fp);
  g_free(filename);

  filename = g_strdup_printf("%s/POSCAR", vasp_gui.calc.job_path);
  fp = fopen(filename, "w");
  if (!fp)
  {
    g_free(filename);
    QMessageBox::critical(this, tr("VASP"), tr("Failed to open POSCAR for writing."));
    return;
  }
  write_poscar(fp);
  fclose(fp);
  g_free(filename);

  filename = g_strdup_printf("%s/KPOINTS", vasp_gui.calc.job_path);
  fp = fopen(filename, "w");
  if (!fp)
  {
    g_free(filename);
    QMessageBox::critical(this, tr("VASP"), tr("Failed to open KPOINTS for writing."));
    return;
  }
  write_kpoints(fp);
  fclose(fp);
  g_free(filename);

  filename = g_strdup_printf("%s/POTCAR", vasp_gui.calc.job_path);
  fp = fopen(filename, "w");
  if (!fp)
  {
    g_free(filename);
    QMessageBox::critical(this, tr("VASP"), tr("Failed to open POTCAR for writing."));
    return;
  }
  write_potcar(fp);
  fclose(fp);
  g_free(filename);

  /* Build vasp_exec_struct and launch task */
  vasp_exec_struct *vasp_exec = (vasp_exec_struct *) g_malloc(sizeof(vasp_exec_struct));
  vasp_exec->job_vasp_exe = g_strdup(vasp_gui.calc.job_vasp_exe ? vasp_gui.calc.job_vasp_exe : "vasp");
  vasp_exec->job_mpirun = g_strdup(vasp_gui.calc.job_mpirun ? vasp_gui.calc.job_mpirun : "mpirun");
  vasp_exec->job_path = g_strdup(vasp_gui.calc.job_path ? vasp_gui.calc.job_path : ".");
  vasp_exec->job_nproc = vasp_gui.calc.job_nproc;

  task_new("VASP", (gpointer) run_vasp_exec, (gpointer) vasp_exec, (gpointer) cleanup_vasp_exec, (gpointer) vasp_exec,
           (gpointer) sysenv.active_model);

  /* Set up tracking */
  gchar *vasprun_filename = g_strdup_printf("%s/vasprun.xml", vasp_gui.calc.job_path);
  struct model_pak *result_model = model_new();
  model_init(result_model);
  strcpy(result_model->filename, vasprun_filename);
  result_model->vasp = NULL;
  result_model->track_me = TRUE;
  result_model->silent = TRUE;
  g_free(vasprun_filename);

  /* In Qt mode: add the tracking model to the tree immediately so the graph
   * appears when track_vasp() creates it. The model starts empty and gets
   * populated as tracking reads vasprun.xml frames. */
  if (!sysenv.tpane)
  {
    /* model_prep() initializes camera and other fields required for rendering */
    extern gint model_prep(struct model_pak *);
    model_prep(result_model);
    result_model->mode = FREE;
    result_model->rmax = 5.0 * RMAX_FUDGE;

    extern void tree_model_add(struct model_pak *);
    extern void tree_select_model(struct model_pak *);
    extern void redraw_canvas(gint);
    tree_model_add(result_model);
    tree_select_model(result_model);
    redraw_canvas(SINGLE);
  }

  /* Start tracking via g_timeout_add_full — runs in GLib thread */
  g_timeout_add_full(G_PRIORITY_DEFAULT, TRACKING_TIMEOUT, track_vasp, result_model, track_vasp_cleanup);

  QMessageBox::information(this, tr("VASP"), tr("VASP calculation submitted.\nCheck the task manager for progress."));
  close();
}

void VaspDialog::on_close()
{
  sync();
  close();
}

/* Placeholder callbacks for UI interactions */
void VaspDialog::on_prec_changed(int) {}
void VaspDialog::on_algo_changed(int) {}
void VaspDialog::on_ialgo_changed(int) {}
void VaspDialog::on_mixer_changed(int) {}
void VaspDialog::on_mixpre_changed(int) {}
void VaspDialog::on_inimix_changed(int) {}
void VaspDialog::on_ibrion_changed(int) {}
void VaspDialog::on_kpoints_mode_changed(int) {}
void VaspDialog::on_calculation_type_changed(int) {}

void VaspDialog::on_potcar_mode_changed()
{
  bool fileMode = m_potcarSelectFileRadio->isChecked();
  m_potcarFileEdit->setEnabled(fileMode);
  m_potcarFileBtn->setEnabled(fileMode);
  m_potcarFolderEdit->setEnabled(!fileMode);
  m_potcarFolderBtn->setEnabled(!fileMode);
  m_potcarFlavorCombo->setEnabled(!fileMode);
  m_potcarApplyBtn->setEnabled(!fileMode);
}
void VaspDialog::on_apply_simple()
{

  /* Sync Qt -> vasp_gui first */
  sync();

  gint calcul = vasp_gui.simple_calcul_idx;
  gint system = vasp_gui.simple_system_idx;
  gint kgrid = vasp_gui.simple_kgrid_idx;
  gint dim = (gint) vasp_gui.dimension;

  /* Clear message buffer */
  g_free(vasp_gui.simple_message_buff);
  vasp_gui.simple_message_buff = g_strdup("Simple interface started.\n");

  /* Rough geometry check */
  if (vasp_gui.simple_rgeom && calcul < 3)
  {
    g_free(vasp_gui.simple_message_buff);
    vasp_gui.simple_message_buff = g_strdup("FAIL: ROUGH GEOMETRY: OPTIMIZE GEOMETRY FIRST!\n");
    return;
  }

  /* KPOINTS */
  if (dim == 0)
  {
    vasp_gui.calc.kpoints_mode = VKP_GAMMA;
    vasp_gui.calc.kpoints_kx = 1.;
    vasp_gui.calc.kpoints_ky = 1.;
    vasp_gui.calc.kpoints_kz = 1.;
    g_free(vasp_gui.simple_message_buff);
    vasp_gui.simple_message_buff = g_strdup("ATOM/MOLECULE in a box, setting only gamma point\n");
  } else
  {
    vasp_gui.calc.kpoints_mode = VKP_AUTO;
    vasp_gui.calc.kpoints_kx = (gdouble) (4.0 * dim * (kgrid + 1.0));
    g_free(vasp_gui.simple_message_buff);
    gchar *msg = g_strdup_printf("SET AUTO gamma-centered grid w/ AUTO = %i\n", (gint) vasp_gui.calc.kpoints_kx);
    vasp_gui.simple_message_buff = msg;
  }

  /* Calculation type < 3: Energy, DOS/BANDS, Lattice Dynamics */
  if (calcul < 3)
  {
    vasp_gui.calc.prec = VP_ACCURATE;
    vasp_gui.calc.algo = VA_NORM;
    vasp_gui.calc.ldiag = TRUE;
  }
  vasp_gui.calc.use_prec = TRUE;

  /* LREAL */
  if (vasp_gui.calc.atoms_total > 16)
  {
    vasp_gui.calc.lreal = VLR_AUTO;
  } else
  {
    vasp_gui.calc.lreal = VLR_FALSE;
  }

  /* Smearing */
  if (system > 0)
  {
    vasp_gui.calc.ismear = 0;
    vasp_gui.calc.sigma = 0.05;
  } else
  {
    vasp_gui.calc.ismear = 1;
    vasp_gui.calc.sigma = 0.2;
  }

  /* Dipole correction */
  switch (dim)
  {
  case 0:
    vasp_gui.calc.idipol = VID_4;
    break;
  case 1:
    vasp_gui.calc.idipol = VID_3;
    break;
  case 2:
    vasp_gui.calc.idipol = VID_3;
    break;
  default:
    vasp_gui.calc.ldipol = FALSE;
    break;
  }

  /* Spin */
  if ((gint) (vasp_gui.calc.electron_total / 2) - (gdouble) (vasp_gui.calc.electron_total / 2.0) != 0.0)
  {
    vasp_gui.calc.ispin = TRUE;
  } else
  {
    vasp_gui.calc.ispin = FALSE;
  }

  /* Calculation-specific settings */
  switch (calcul)
  {
  case 0: /* Energy (single point) */
    if ((kgrid > 1) && (dim > 0))
    {
      vasp_gui.calc.ismear = -5;
    }
    break;

  case 1: /* DOS/BANDS */
    if (vasp_gui.calc.have_paw)
      vasp_gui.calc.lorbit = 12;
    else
      vasp_gui.calc.lorbit = 2;
    vasp_gui.calc.nedos = 2001;
    vasp_gui.calc.emin = -10.;
    vasp_gui.calc.emax = 10.;
    break;

  case 2: /* Lattice Dynamics */
    vasp_gui.calc.addgrid = TRUE;
    vasp_gui.calc.lreal = VLR_FALSE;
    vasp_gui.calc.ibrion = 6;
    vasp_gui.calc.potim = 0.015;
    if (vasp_gui.calc.ncore > 1)
    {
      vasp_gui.calc.ncore = 1;
    }
    break;

  case 3: /* Geometry optimization */
    if (vasp_gui.simple_rgeom)
    {
      vasp_gui.calc.prec = VP_NORM;
      vasp_gui.calc.use_prec = TRUE;
      vasp_gui.calc.algo = VA_FAST;
      vasp_gui.calc.nelmin = 5;
      vasp_gui.calc.ediff = 1E-2;
      vasp_gui.calc.ediffg = -0.3;
      vasp_gui.calc.nsw = 10;
      vasp_gui.calc.ibrion = 2;
    } else
    {
      vasp_gui.calc.prec = VP_ACCURATE;
      vasp_gui.calc.use_prec = TRUE;
      vasp_gui.calc.algo = VA_NORM;
      vasp_gui.calc.ldiag = TRUE;
      vasp_gui.calc.addgrid = TRUE;
      vasp_gui.calc.nelmin = 8;
      vasp_gui.calc.ediff = 1E-5;
      vasp_gui.calc.ediffg = -0.01;
      vasp_gui.calc.nsw = 20;
      vasp_gui.calc.maxmix = 80;
      vasp_gui.calc.ibrion = 1;
      vasp_gui.calc.nfree = 10;
    }
    break;

  case 4: /* Molecular Dynamics */
    if (vasp_gui.simple_rgeom)
      vasp_gui.calc.prec = VP_LOW;
    else
      vasp_gui.calc.prec = VP_NORM;
    vasp_gui.calc.use_prec = TRUE;
    vasp_gui.calc.ediff = 1E-5;
    vasp_gui.calc.ismear = -1;
    vasp_gui.calc.sigma = 0.086;
    vasp_gui.calc.algo = VA_VERYFAST;
    vasp_gui.calc.maxmix = 40;
    vasp_gui.calc.isym = 0;
    vasp_gui.calc.nelmin = 4;
    vasp_gui.calc.ibrion = 0;
    vasp_gui.calc.nsw = 100;
    vasp_gui.calc.nwrite = 0;
    vasp_gui.calc.lcharg = FALSE;
    vasp_gui.calc.lwave = FALSE;
    vasp_gui.calc.tebeg = 1000;
    vasp_gui.calc.teend = 1000;
    vasp_gui.calc.smass = 3;
    vasp_gui.calc.nblock = 50;
    vasp_gui.calc.potim = 1.5;
    break;

  default:
    g_free(vasp_gui.simple_message_buff);
    vasp_gui.simple_message_buff = g_strdup("FAIL: UNKNOWN CALCUL SETTING!\n");
    return;
  }

  refresh();
}

void VaspDialog::applyParallelConstraints()
{
  int np = m_simpleNpSpin->value();
  int kpar = m_simpleKparSpin->value();
  int maxNcore = (kpar > 0) ? np / kpar : 1;
  if (maxNcore < 1)
    maxNcore = 1;

  /* Apply constraints to PRESET spinners */
  m_simpleNcoreSpin->setRange(1, maxNcore);
  m_simpleKparSpin->setRange(1, np);
  if (m_simpleNcoreSpin->value() > maxNcore)
    m_simpleNcoreSpin->setValue(maxNcore);
  if (m_simpleKparSpin->value() > np)
    m_simpleKparSpin->setValue(np);

  /* Apply same constraints to EXEC spinners */
  m_ncoreSpin->setRange(1, maxNcore);
  m_kparSpin->setRange(1, np);
  if (m_ncoreSpin->value() > maxNcore)
    m_ncoreSpin->setValue(maxNcore);
  if (m_kparSpin->value() > np)
    m_kparSpin->setValue(np);
}

/* Bridge function */
extern "C" void qt_show_vasp_dialog(void)
{
  extern void gui_vasp_init();
  extern void vasp_gui_refresh();

  /* Initialize vasp_gui with defaults */
  struct model_pak *data = (struct model_pak *) sysenv.active_model;
  if (!data)
    return;

  vasp_gui.have_xml = (data->id == VASP);
  gui_vasp_init();
  if (vasp_gui.have_xml && data->filename)
    g_free(vasp_gui.calc.job_path); /* will be set by vasprun_update from sysenv.cwd */
  vasprun_update(vasp_gui.have_xml ? data->filename : NULL, &vasp_gui.calc);
  /* Override job_path with the actual XML file path for CONNECTED XML display */
  if (vasp_gui.have_xml && data->filename)
    vasp_gui.calc.job_path = g_strdup(data->filename);

  /* Load POSCAR from the same directory as the XML to get species/atom data */
  if (vasp_gui.have_xml && data->filename)
  {
    gchar *dir = g_path_get_dirname(data->filename);
    gchar *poscar_path = g_strdup_printf("%s/POSCAR", dir);
    g_free(dir);
    FILE *pf = fopen(poscar_path, "r");
    if (pf)
    {
      /* get species_symbols & species_numbers */
      g_free(vasp_gui.calc.species_symbols);
      vasp_gui.calc.species_symbols = NULL;
      g_free(vasp_gui.calc.species_numbers);
      vasp_gui.calc.species_numbers = NULL;
      /* 1 detect VASP4/VASP5 */
      gchar *line = file_read_line(pf); /* VASP title line */
      g_free(line);
      line = file_read_line(pf); /* lattice parameter */
      g_free(line);
      line = file_read_line(pf); /* lattice vector u */
      g_free(line);
      line = file_read_line(pf); /* lattice vector v */
      g_free(line);
      line = file_read_line(pf); /* lattice vector w */
      g_free(line);
      line = file_read_line(pf); /* line of interest */
      char *ptr = &(line[0]);
      while ((*ptr != '\0') && (*ptr == ' '))
        ptr++; /* first non space character */
      gboolean vasp5 = !((*ptr >= '0') && (*ptr <= '9'));
      if (vasp5)
      {
        /* get species; VASP4 format _does not_ have a species line */
        char *tok = g_strstrip(line);                  /* strip line of leading and trailing whitespace */
        vasp_gui.calc.species_symbols = g_strdup(tok); /* simply ^^' */
        g_free(line);
        line = file_read_line(pf); /* number line */
      }
      char *tok2 = g_strstrip(line);                  /* strip line of leading and trailing whitespace */
      vasp_gui.calc.species_numbers = g_strdup(tok2); /* simply ^^' */
      g_free(line);
      fclose(pf);
    }
    g_free(poscar_path);
  }
poscar_done:

  /* Derive rions/rshape/rvolume from ISIF (these are GUI-only fields) */
  switch (vasp_gui.calc.isif)
  {
  case 0:
    vasp_gui.rions = FALSE;
    vasp_gui.rshape = FALSE;
    vasp_gui.rvolume = FALSE;
    break;
  case 1:
    vasp_gui.rions = FALSE;
    vasp_gui.rshape = FALSE;
    vasp_gui.rvolume = FALSE;
    break;
  case 2:
    vasp_gui.rions = TRUE;
    vasp_gui.rshape = FALSE;
    vasp_gui.rvolume = FALSE;
    break;
  case 3:
    vasp_gui.rions = TRUE;
    vasp_gui.rshape = TRUE;
    vasp_gui.rvolume = TRUE;
    break;
  case 4:
    vasp_gui.rions = TRUE;
    vasp_gui.rshape = TRUE;
    vasp_gui.rvolume = FALSE;
    break;
  case 5:
    vasp_gui.rions = FALSE;
    vasp_gui.rshape = TRUE;
    vasp_gui.rvolume = FALSE;
    break;
  case 6:
    vasp_gui.rions = FALSE;
    vasp_gui.rshape = TRUE;
    vasp_gui.rvolume = TRUE;
    break;
  case 7:
    vasp_gui.rions = FALSE;
    vasp_gui.rshape = FALSE;
    vasp_gui.rvolume = TRUE;
    break;
  default:
    vasp_gui.rions = TRUE;
    vasp_gui.rshape = FALSE;
    vasp_gui.rvolume = FALSE;
    break;
  }
  /* [VASP open] debug removed */
  update_species_from_model();

  extern QWidget *get_main_window_widget();
  VaspDialog *dlg = new VaspDialog(get_main_window_widget());
  dlg->populatePoscarFromModel();
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->show();
  dlg->raise();
  dlg->activateWindow();
}
