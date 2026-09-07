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

#include "mdidialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QMessageBox>
#include <QListWidget>

/* Include C headers */
#undef slots
#undef signals

#include "gdis.h"
#include "model.h"
#include "coords.h"
#include "edit.h"
#include "matrix.h"
#include "file.h"
#include "interface.h"
#include "gui_shorts.h"
#include "mdi_pak.h"

#include <sys/times.h>

extern struct sysenv_pak sysenv;
extern struct mdi_pak mdi_data;
extern "C" void rot(gdouble *, gdouble, gdouble, gdouble);

MdiDialog::MdiDialog(QWidget *parent) : QDialog(parent), m_solvent(nullptr)
{
  setWindowTitle(tr("MD Initializer"));
  resize(400, 300);
  setupUI();
}

MdiDialog::~MdiDialog() {}

void MdiDialog::setupUI()
{
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(8, 6, 8, 6);
  mainLayout->setSpacing(8);

  /* Box dimension */
  auto *boxGroup = new QGroupBox(tr("Box dimension"));
  auto *boxLayout = new QVBoxLayout(boxGroup);
  boxLayout->setSpacing(4);

  auto *dimLabel = new QLabel(tr("Side length (lattice points)"));
  boxLayout->addWidget(dimLabel);

  m_spinBoxDim = new QSpinBox();
  m_spinBoxDim->setRange(3, 100);
  m_spinBoxDim->setValue(10);
  m_spinBoxDim->setSingleStep(1);
  boxLayout->addWidget(m_spinBoxDim);

  mainLayout->addWidget(boxGroup);

  /* Solvent model (model 0) */
  m_solvent = model_ptr(0, RECALL);
  if (m_solvent)
  {
    auto *solventGroup = new QGroupBox(tr("Solvent model"));
    auto *solventLayout = new QVBoxLayout(solventGroup);
    solventLayout->setSpacing(4);

    auto *solventLabel = new QLabel(tr("This will be the solvent: %1").arg(m_solvent->basename));
    solventLabel->setWordWrap(true);
    solventLayout->addWidget(solventLabel);

    mainLayout->addWidget(solventGroup);
  }

  /* Solute components (models 1+) */
  GSList *list = g_slist_next(sysenv.mal); /* skip model 0 (solvent) */
  int idx = 1;
  while (list)
  {
    struct model_pak *data = (struct model_pak *) list->data;

    /* Don't include any previous MDI model */
    if (data->id == MDI)
    {
      list = g_slist_next(list);
      idx++;
      continue;
    }

    auto *compGroup = new QGroupBox(tr("Component: %1").arg(data->basename));
    auto *compLayout = new QVBoxLayout(compGroup);
    compLayout->setSpacing(4);

    auto *compLabel = new QLabel(tr("Number required"));
    compLayout->addWidget(compLabel);

    auto *spin = new QSpinBox();
    spin->setRange(0, 100);
    spin->setValue(0);
    spin->setSingleStep(1);
    spin->setSuffix(tr(" molecules"));
    compLayout->addWidget(spin);

    m_spinComponents.append(spin);
    m_componentLabels.append(compLabel);

    mainLayout->addWidget(compGroup);

    list = g_slist_next(list);
    idx++;
  }

  mainLayout->addStretch();

  /* Buttons */
  auto *buttonLayout = new QHBoxLayout();
  auto *createBtn = new QPushButton(tr("Create"));
  auto *cancelBtn = new QPushButton(tr("Cancel"));
  buttonLayout->addWidget(createBtn);
  buttonLayout->addWidget(cancelBtn);
  mainLayout->addLayout(buttonLayout);

  connect(createBtn, &QPushButton::clicked, this, &MdiDialog::on_create_box);
  connect(cancelBtn, &QPushButton::clicked, this, &MdiDialog::on_cancel);
}

void MdiDialog::on_create_box()
{
  /* Set up mdi_data from dialog values */
  mdi_data.box_dim = m_spinBoxDim->value();

  /* Count non-MDI components INCLUDING solvent at index 0 */
  int num_comp = 1; /* solvent at index 0 */
  GSList *list = g_slist_next(sysenv.mal);
  while (list)
  {
    struct model_pak *data = (struct model_pak *) list->data;
    if (data->id != MDI)
      num_comp++;
    list = g_slist_next(list);
  }

  mdi_data.num_comp = num_comp;
  mdi_data.comp_idx = static_cast<gint *>(g_malloc0(num_comp * sizeof(gint)));
  mdi_data.comp_req = static_cast<gint *>(g_malloc0(num_comp * sizeof(gint)));
  mdi_data.comp_done = static_cast<gint *>(g_malloc0(num_comp * sizeof(gint)));

  /* Auto-calculate lattice spacing from largest model radius */
  gdouble biggest = -1.0;
  list = sysenv.mal;
  while (list)
  {
    struct model_pak *data = (struct model_pak *) list->data;
    if (data->rmax > biggest)
      biggest = data->rmax;
    list = g_slist_next(list);
  }
  mdi_data.latt_sep = 2.0 * biggest + 1.0;

  /* Fill comp_idx and comp_req */
  /* comp_idx[0] = solvent model index (always 0) */
  mdi_data.comp_idx[0] = 0;
  int ci = 1;
  int num_req = 0;
  list = g_slist_next(sysenv.mal); /* skip model 0 */
  int spin_idx = 0;
  while (list)
  {
    struct model_pak *data = (struct model_pak *) list->data;
    if (data->id == MDI)
    {
      list = g_slist_next(list);
      continue;
    }
    /* Store the model index (position in sysenv.mal) */
    mdi_data.comp_idx[ci] = g_slist_index(sysenv.mal, data);
    mdi_data.comp_req[ci] = spin_idx < (int) m_spinComponents.size() ? m_spinComponents[spin_idx]->value() : 0;
    num_req += mdi_data.comp_req[ci];
    ci++;
    spin_idx++;
    list = g_slist_next(list);
  }

  /* Solvent fills remaining box space */
  mdi_data.comp_req[0] = static_cast<gint>(pow(mdi_data.box_dim, 3)) - num_req;
  if (mdi_data.comp_req[0] < 9)
  {
    gui_text_show(ERROR, "Too many solute molecules required!\n");
    g_free(mdi_data.comp_idx);
    g_free(mdi_data.comp_req);
    g_free(mdi_data.comp_done);
    return;
  }

  /* Call the C function to create the box */
  mdi_model_create(num_comp);

  /* Clean up */
  g_free(mdi_data.comp_idx);
  g_free(mdi_data.comp_req);
  g_free(mdi_data.comp_done);

  accept();
}

void MdiDialog::on_cancel() { reject(); }

/* Qt bridge function — called from gui_mdi.c stub */
extern "C" void qt_mdi_dialog(void)
{
  extern QWidget *get_main_window_widget();
  MdiDialog *dlg = new MdiDialog(get_main_window_widget());
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->show();
}
extern "C" gint fill()
{
  gint num_sites, max_sites, num_cand;
  gint i, j, k, di, dj, dk, r2, s;
  gint req_tot, ci, ri, pos;
  gint *r2_min, max_min;
  gdouble ran2();
  struct cand_pak *cand;
  struct box_pak *site;

  /* allocate for sites - ie solvent not included XCPT corners */
  max_sites = pow(mdi_data.box_dim, 3) - mdi_data.comp_req[0] + 8;
  site = (struct box_pak *) g_malloc(max_sites * sizeof(struct box_pak));

  r2_min = (gint *) g_malloc(pow(mdi_data.box_dim, 3) * sizeof(gint));

  /* max out the number of candidate sites */
  cand = (struct cand_pak *) g_malloc0(pow(mdi_data.box_dim, 3) * sizeof(struct cand_pak)); /*FIX 178d27*/

/* NB: initially every point in box should be 0 => solvent */
#if DEBUG
  printf("Filling box: %d x %d x %d\n", mdi_data.box_dim, mdi_data.box_dim, mdi_data.box_dim);
#endif

  /*  NO cadidate inititalization */
  num_sites = 0;

  /* compute total number of components to dissolve */
  req_tot = 0;
  for (i = 1; i < mdi_data.num_comp; i++) /* omit solvent */
    req_tot += mdi_data.comp_req[i];

  ci = 1; /* starting component number */

  /* loop over total number to dissolve */
  for (ri = 0; ri < req_tot; ri++)
  {
    /* seek a component which doesn't yet have the required number solvated */
    while (ci < g_slist_length(sysenv.mal) && mdi_data.comp_done[ci] >= mdi_data.comp_req[ci])
      ci++;
    if (ci == g_slist_length(sysenv.mal))
    {
      printf("fill() error: mismatch in component requirements!\n");
      g_free(site);   /*FIX f75d7c*/
      g_free(r2_min); /*FIX 43cc34*/
      g_free(cand);   /*FIX b87255*/
      return (2);
    }
    mdi_data.comp_done[ci]++;

    /* evaluate all sites - sphere of clearance */
    pos = 0;
    max_min = 0;
    for (i = 0; i < mdi_data.box_dim; i++)
    {
      for (j = 0; j < mdi_data.box_dim; j++)
      {
        for (k = 0; k < mdi_data.box_dim; k++)
        {
          /* find minimum distance from this site to all candidate sites */
          r2_min[pos] = pow(mdi_data.box_dim, 3);
          for (s = 0; s < num_sites; s++)
          {
            if (s == max_sites)
            {
              printf("Error: site indexing beyond boundary!\n");
              g_free(site);   /*FROM f75d7c*/
              g_free(r2_min); /*FROM 43cc34*/
              g_free(cand);   /*FROM b87255*/
              return (2);
            }
            /* PBC */
            di = abs(site[s].x - i);
            if (di > mdi_data.box_dim / 2.0)
              di -= mdi_data.box_dim / 2.0;
            dj = abs(site[s].y - j);
            if (dj > mdi_data.box_dim / 2.0)
              dj -= mdi_data.box_dim / 2.0;
            dk = abs(site[s].z - k);
            if (dk > mdi_data.box_dim / 2.0)
              dk -= mdi_data.box_dim / 2.0;

            r2 = di * di + dj * dj + dk * dk;
            /* test for new minimum */
            if (r2 < r2_min[pos])
              r2_min[pos] = r2;
          }
          /* get the overall maximum (of the minima) */
          if (r2_min[pos] > max_min)
            max_min = r2_min[pos];
          pos++;
        }
      }
    }

    s = pos = 0;
    for (i = 0; i < mdi_data.box_dim; i++)
    {
      for (j = 0; j < mdi_data.box_dim; j++)
      {
        for (k = 0; k < mdi_data.box_dim; k++)
        {
          if (r2_min[pos] == max_min)
          {
            cand[s].pos = pos;
            cand[s].x = i;
            cand[s].y = j;
            cand[s].z = k;
            s++;
          }
          pos++;
        }
      }
    }

    num_cand = s;
    if (!num_cand)
    {
      printf("No candidate sites found!\n");
      g_free(r2_min);
      g_free(site);
      g_free(cand);
      return (2);
    }
#if DEBUG
    printf("Found %d candidate site(s)\n", num_cand);
#endif

    /* Select (at random) a candidate site into */
    /* which the current component will be placed */

    s = (gint) (ran2() * num_cand);

#if DEBUG
    printf("s=%d\n", s);
#endif

    /* do site update */
    site[num_sites].component = ci;
    site[num_sites].x = cand[s].x;
    site[num_sites].y = cand[s].y;
    site[num_sites].z = cand[s].z;
    num_sites++;

    /* do array data update */
    pos = cand[s].pos;
    *(mdi_data.array + pos) = ci;
  }

/* final data in nice array format */
#if DEBUG
  pos = 0;
  for (i = 0; i < mdi_data.box_dim; i++)
  {
    for (j = 0; j < mdi_data.box_dim; j++)
    {
      for (k = 0; k < mdi_data.box_dim; k++)
      {
        printf("%d", *(mdi_data.array + pos));
        pos++;
      }
      printf("\n");
    }
    printf("\n");
  }
  printf("fill() done\n");
#endif

  g_free(r2_min);
  g_free(site);
  g_free(cand);
  return (0);
}
extern "C" void write_dat(struct model_pak *dest)
{
  gint s, m, pos;
#ifdef UNUSED_BUT_SET
  gint dat;
#endif
  gdouble x, y, z, phi, theta, psi, start, stop, step;
  gdouble q[3], box_len;
  gdouble ran2();
  GSList *list;
  struct model_pak *data;
  struct core_pak *core, *copy;

#if DEBUG
  printf("Starting write_dat()...\n");
#endif

  box_len = mdi_data.box_dim * mdi_data.latt_sep;

  start = mdi_data.latt_sep / 2.0;
  stop = box_len;
  step = mdi_data.latt_sep;

#if DEBUG
  printf("Box dim = %f\n", box_len);
  printf("Box rng = %f,%f,%f\n", start, stop, step);
#endif

  pos = 0; /* pointer - current array position */
#ifdef UNUSED_BUT_SET
  dat = 0; /* new model - atom coords pointer */
#endif

  for (z = start; z < stop; z += step)
  {
    for (y = start; y < stop; y += step)
    {
      for (x = start; x < stop; x += step)
      {
        /* trap */
        if (pos >= pow(mdi_data.box_dim, 3))
        {
          printf("Error at (%f,%f,%f)\n", x, y, z);
          printf("Program bug: address (pos) out of bounds!\n");
          return;
        }

        /* get component at this (x,y,z) pos'n */
        s = *(mdi_data.array + pos);
        /* get the model this corresponds to */
        m = mdi_data.comp_idx[s];
        data = model_ptr(m, RECALL);
        pos++;
        /* create some random rotations */
        phi = 2.0 * G_PI * ran2();
        theta = G_PI - 2.0 * G_PI * ran2();
        psi = 2.0 * G_PI * ran2();

        for (list = data->cores; list; list = g_slist_next(list))
        {
          core = (struct core_pak *) list->data;
          if (core->status & DELETED)
            continue;

          /* do rotation */
          ARR3SET(q, core->rx);
          rot(&q[0], phi, theta, psi);
          /* do lattice site translation of coords
           * x,y,z are lattice indices; translate by latmat columns */
          q[0] += x * dest->latmat[0] + y * dest->latmat[3] + z * dest->latmat[6];
          q[1] += x * dest->latmat[1] + y * dest->latmat[4] + z * dest->latmat[7];
          q[2] += x * dest->latmat[2] + y * dest->latmat[5] + z * dest->latmat[8];

          /* write the atom data to the model structure */
          copy = dup_core(core);
          dest->cores = g_slist_prepend(dest->cores, copy);

          copy->status = NORMAL;
          copy->orig = TRUE;
          copy->primary = TRUE;
          copy->primary_core = NULL;
          copy->region = REGION1A;
          ARR3SET(copy->x, q);
        }
      }
    }
  }

  /* model info */
  dest->id = MDI;
  dest->fractional = FALSE;
  dest->periodic = 3;
  dest->pbc[0] = box_len;
  dest->pbc[1] = box_len;
  dest->pbc[2] = box_len;
  dest->pbc[3] = G_PI * 0.5;
  dest->pbc[4] = G_PI * 0.5;
  dest->pbc[5] = G_PI * 0.5;

  /* Compute lattice matrix so write_dat can use it for translations */
  matrix_lattice_init(dest);

#if DEBUG
  printf("write_dat() done\n");
#endif
}

gdouble ran2()
{
  gint i, j;
  gdouble f;

  i = j = 0;
  /* generate 2 random (non 0) integers */
  while (!i)
    i = rand();
  while (!j)
    j = rand();
  /* convert to a single float (0.0,1.0] */
  if (i > j)
    f = (gdouble) j / (gdouble) i;
  else
    f = (gdouble) i / (gdouble) j;

  return f;
}
#define DEBUG_MDI_MODEL_CREATE 0
extern "C" void mdi_model_create(gint new_flag)
{
  gint i, j, replace, ret;
  struct model_pak *data;
#ifndef __WIN32
  struct tms buffer;
#endif

  /* main array */
  mdi_data.array = (gint *) g_malloc0(mdi_data.box_dim * mdi_data.box_dim * mdi_data.box_dim * sizeof(gint));

  /* calculate the total number of atoms & bonds & sites in the box */
  for (i = 0; i < mdi_data.num_comp; i++)
  {
    // data = model_ptr(i, RECALL);/*FIX 1bb46b*/
    mdi_data.comp_done[i] = 0;
  }

  replace = 1;

  data = model_new();

  /* check & init */
  g_return_if_fail(data != NULL);
  data->id = MDI;
  strcpy(data->filename, "MDI_model");
  g_free(data->basename);
  data->basename = g_strdup("MDI model");

/* initialize random generator */
#if __WIN32
  j = 666;
#else
  j = times(&buffer);
#endif

#if DEBUG_MDI_MODEL_CREATE
  printf("Random seed = %d\n", j);
#endif
  srand(j);

  /* do the business */
  ret = fill();

#if DEBUG_MDI_MODEL_CREATE
  printf("fill() return code: %d\n", ret);
#endif
  if (ret == 2)
  {
    gui_text_show(ERROR, "Run was unsuccessful!\n");

    model_delete(data);
    g_free(mdi_data.array);
    return;
  }

  /* copy box into the model coord data array */
  write_dat(data);
  model_prep(data);

  /* update model tree display */
  if (replace)
  {
    tree_model_add(data);
    gui_model_select(data);
  } else
  {
    tree_model_add(data);
  }

#if DEBUG_MDI_MODEL_CREATE
  printf("done model init\n");
#endif

  g_free(mdi_data.array);

#if DEBUG_MDI_MODEL_CREATE
  printf("MDI done.\n");
#endif
}

/* MDI rotation helper */
extern "C" void rot(gdouble *coords, gdouble phi, gdouble theta, gdouble psi)
{
  gdouble sph, cph, sth, cth, sps, cps;
  gdouble rot[3][3];

  /* let's make things easier */
  sph = sin(phi);
  cph = cos(phi);
  sth = sin(theta);
  cth = cos(theta);
  sps = sin(psi);
  cps = cos(psi);

  /* make the matrix */
  rot[0][0] = cps * cph - cth * sph * sps;
  rot[0][1] = cps * sph + cth * cph * sps;
  rot[0][2] = sps * sth;
  rot[1][0] = -sps * cph - cth * sph * cps;
  rot[1][1] = -sps * sph + cth * cph * cps;
  rot[1][2] = cps * sth;
  rot[2][0] = sth * sph;
  rot[2][1] = -sth * cph;
  rot[2][2] = cth;

  /* multiply the coords with the matrix */
  vecmat(&rot[0][0], coords);
}
