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

#ifndef SURFACEDIALOG_H
#define SURFACEDIALOG_H

#include <QDialog>
#include <QTreeWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QScrollArea>
#include <QSplitter>

/* Include gdis_api.h for gint typedef (needed by MOC) */
#include "gdis_api.h"

struct model_pak;
struct plane_pak;
struct shift_pak;

/* Surface dialog data — mirrors the surfdata global */
struct surf_dialog_data {
  struct model_pak *model;
  gint miller[3];
  gdouble shift;
  gdouble region[2];
  gint rankValue;
};

class SurfaceDialog : public QDialog
{
  Q_OBJECT

public:
  explicit SurfaceDialog(struct model_pak *model, QWidget *parent = nullptr);
  ~SurfaceDialog();

private slots:
  void on_create_surface();
  void on_add_plane_with_shift();
  void on_add_valid_shifts();
  void on_add_ranked_faces();
  void on_make_morph();
  void on_surf_task(gint type);
  void on_calc_shifts();
  void on_calc_energy();
  void on_conv_regions();
  void on_make_faces();
  void on_delete_selected();
  void on_delete_invalid();
  void on_delete_all();
  void on_collapse_all();
  void on_expand_all();
  void on_select_all();
  void on_selection_changed();
  void on_morph_type_changed(const QString &text);
  void on_shift_commit();
  void on_export_planes();
  void on_import_planes();
  void on_gulp_dialog();
  void on_miller_changed();
  void on_shift_changed();
  void on_region_changed();

private:
  void setupUI();
  void setupMenuBar();
  void setupLeftPanel();
  void setupRightPanel();
  void setupBottomButtons();
  void setupTreeColumns();
  /* populateTree is a static free function, not a member */
  void graftPlane(struct plane_pak *plane);
  void graftShifts(struct plane_pak *plane);
  void graftShift(QTreeWidgetItem *parent, struct shift_pak *shift);
  void removeShifts(struct plane_pak *plane);
  void updateShiftValues(struct shift_pak *shift, QTreeWidgetItem *item);
  void updateEnergyEntries();
  void updateHklFamily();
  void syncFromDialog();
  void syncToDialog();
  void updatePlaneEnergy(struct plane_pak *plane, struct model_pak *model);

  struct model_pak *m_model;
  struct surf_dialog_data m_surfdata;
  bool m_isMorph;

  /* Menu */
  QMenu *m_fileMenu;
  QMenu *m_editMenu;

  /* Panels */
  QWidget *m_leftPanel;
  QWidget *m_rightPanel;

  /* Left panel widgets (non-morph mode) */
  QSpinBox *m_spinMiller[3];
  QDoubleSpinBox *m_spinShift;
  QSpinBox *m_spinRegion[2];
  QCheckBox *m_chkAllowPolar;
  QCheckBox *m_chkAllowBondCleaving;
  QCheckBox *m_chkIgnoreSymmetry;
  QCheckBox *m_chkPreserveDepthPeriodicity;
  QCheckBox *m_chkPreserveAtomOrdering;
  QDoubleSpinBox *m_spinDipoleTolerance;
  QCheckBox *m_chkConvergeEatt;
  QCheckBox *m_chkConvergeR1;
  QCheckBox *m_chkConvergeR2;

  /* Left panel widgets (morph mode) */
  QGroupBox *m_morphTypeGroup;
  QRadioButton *m_rbBfdh, *m_rbEqun, *m_rbGrun, *m_rbEqre, *m_rbGrre;
  QLineEdit *m_morphEnergy[4]; /* Esurf(u), Eatt(u), Esurf(r), Eatt(r) */

  /* Right panel — tree view */
  QTreeWidget *m_treeView;
  QStringList m_columnHeaders;

  /* Bottom widgets */
  QLineEdit *m_hklFamilyEntry;

  /* Ranked faces */
  QSpinBox *m_spinRankValue;

  /* Nuclei sculpting (non-morph mode only) */
  QSpinBox *m_spinNucleiSize;
  QCheckBox *m_chkSculptShiftUse;
};

#endif // SURFACEDIALOG_H
