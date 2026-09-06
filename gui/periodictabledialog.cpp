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
 * Periodic Table dialog for GDIS Qt6 GUI
 */

#include "periodictabledialog.h"
#include "gdis_api.h"
#include "glcanvas.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

/*
 * Periodic table element entries.
 * Each entry: {atomic_number, row, col}
 * Row/col are 1-indexed Qt grid positions.
 *
 * Layout:
 *   Row 1:  H (1,1) ... He (18,1)
 *   Row 2:  Li (1,2) ... Ne (18,2)
 *   Row 3:  Na (1,3) ... Ar (18,3)
 *   Row 4:  K (1,4) Ca (2,4) Sc-Zn (3-12,4) Ga (13,4) ... Kr (18,4)
 *   Row 5:  Rb (1,5) Sr (2,5) Y-Cd (3-12,5) In (13,5) ... Xe (18,5)
 *   Row 6:  Cs (1,6) Ba (2,6) La (3,6) Hf (4,6) ... Rn (18,6)
 *   Row 7:  Fr (1,7) Ra (2,7) Ac (3,7) Rf (4,7) ... Og (18,7)
 *   Row 8:  (separator row - empty)
 *   Row 9:  Ce (4,9) ... Lu (17,9)  (lanthanides)
 *   Row 10: Th (4,10) ... Lr (17,10) (actinides)
 */

struct elem_entry {
  int number;
  int row;
  int col;
};

static const elem_entry periodic_table[] = {
    // Row 1
    {1, 1, 1},  // H
    {2, 1, 18}, // He

    // Row 2
    {3, 2, 1},   // Li
    {4, 2, 2},   // Be
    {5, 2, 13},  // B
    {6, 2, 14},  // C
    {7, 2, 15},  // N
    {8, 2, 16},  // O
    {9, 2, 17},  // F
    {10, 2, 18}, // Ne

    // Row 3
    {11, 3, 1},  // Na
    {12, 3, 2},  // Mg
    {13, 3, 13}, // Al
    {14, 3, 14}, // Si
    {15, 3, 15}, // P
    {16, 3, 16}, // S
    {17, 3, 17}, // Cl
    {18, 3, 18}, // Ar

    // Row 4
    {19, 4, 1},  // K
    {20, 4, 2},  // Ca
    {21, 4, 3},  // Sc
    {22, 4, 4},  // Ti
    {23, 4, 5},  // V
    {24, 4, 6},  // Cr
    {25, 4, 7},  // Mn
    {26, 4, 8},  // Fe
    {27, 4, 9},  // Co
    {28, 4, 10}, // Ni
    {29, 4, 11}, // Cu
    {30, 4, 12}, // Zn
    {31, 4, 13}, // Ga
    {32, 4, 14}, // Ge
    {33, 4, 15}, // As
    {34, 4, 16}, // Se
    {35, 4, 17}, // Br
    {36, 4, 18}, // Kr

    // Row 5
    {37, 5, 1},  // Rb
    {38, 5, 2},  // Sr
    {39, 5, 3},  // Y
    {40, 5, 4},  // Zr
    {41, 5, 5},  // Nb
    {42, 5, 6},  // Mo
    {43, 5, 7},  // Tc
    {44, 5, 8},  // Ru
    {45, 5, 9},  // Rh
    {46, 5, 10}, // Pd
    {47, 5, 11}, // Ag
    {48, 5, 12}, // Cd
    {49, 5, 13}, // In
    {50, 5, 14}, // Sn
    {51, 5, 15}, // Sb
    {52, 5, 16}, // Te
    {53, 5, 17}, // I
    {54, 5, 18}, // Xe

    // Row 6
    {55, 6, 1},  // Cs
    {56, 6, 2},  // Ba
    {57, 6, 3},  // La
    {72, 6, 4},  // Hf
    {73, 6, 5},  // Ta
    {74, 6, 6},  // W
    {75, 6, 7},  // Re
    {76, 6, 8},  // Os
    {77, 6, 9},  // Ir
    {78, 6, 10}, // Pt
    {79, 6, 11}, // Au
    {80, 6, 12}, // Hg
    {81, 6, 13}, // Tl
    {82, 6, 14}, // Pb
    {83, 6, 15}, // Bi
    {84, 6, 16}, // Po
    {85, 6, 17}, // At
    {86, 6, 18}, // Rn

    // Row 7
    {87, 7, 1},   // Fr
    {88, 7, 2},   // Ra
    {89, 7, 3},   // Ac
    {104, 7, 4},  // Rf
    {105, 7, 5},  // Db
    {106, 7, 6},  // Sg
    {107, 7, 7},  // Bh
    {108, 7, 8},  // Hs
    {109, 7, 9},  // Mt
    {110, 7, 10}, // Ds
    {111, 7, 11}, // Rg
    {112, 7, 12}, // Cn
    {113, 7, 13}, // Nh
    {114, 7, 14}, // Fl
    {115, 7, 15}, // Mc
    {116, 7, 16}, // Lv
    {117, 7, 17}, // Ts
    {118, 7, 18}, // Og

    // Row 9: Lanthanides (Ce-Lu)
    {58, 9, 4},  // Ce
    {59, 9, 5},  // Pr
    {60, 9, 6},  // Nd
    {61, 9, 7},  // Pm
    {62, 9, 8},  // Sm
    {63, 9, 9},  // Eu
    {64, 9, 10}, // Gd
    {65, 9, 11}, // Tb
    {66, 9, 12}, // Dy
    {67, 9, 13}, // Ho
    {68, 9, 14}, // Er
    {69, 9, 15}, // Tm
    {70, 9, 16}, // Yb
    {71, 9, 17}, // Lu

    // Row 10: Actinides (Th-Lr)
    {90, 10, 4},   // Th
    {91, 10, 5},   // Pa
    {92, 10, 6},   // U
    {93, 10, 7},   // Np
    {94, 10, 8},   // Pu
    {95, 10, 9},   // Am
    {96, 10, 10},  // Cm
    {97, 10, 11},  // Bk
    {98, 10, 12},  // Cf
    {99, 10, 13},  // Es
    {100, 10, 14}, // Fm
    {101, 10, 15}, // Md
    {102, 10, 16}, // No
    {103, 10, 17}, // Lr
};

static const int NUM_ENTRIES = sizeof(periodic_table) / sizeof(periodic_table[0]);

PeriodicTableDialog::PeriodicTableDialog(QWidget *parent) : QDialog(parent)
{
  setWindowTitle("Periodic Table of the Elements");
  setMinimumSize(900, 550);

  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setSpacing(4);
  mainLayout->setContentsMargins(8, 8, 8, 8);

  auto *title = new QLabel("Periodic Table of the Elements", this);
  title->setStyleSheet("font-size: 14pt; font-weight: bold;");
  mainLayout->addWidget(title);

  /* Grid layout for the periodic table */
  auto *gridLayout = new QGridLayout();
  gridLayout->setSpacing(3);
  gridLayout->setContentsMargins(4, 4, 4, 4);

  /* Build element buttons */
  int num_elems = qt_get_num_elements();

  for (int i = 0; i < NUM_ENTRIES; i++)
  {
    const auto &e = periodic_table[i];
    int elem_num = e.number;

    if (elem_num > num_elems)
      continue;

    int number;
    char symbol[4] = {0};
    char name[32] = {0};
    double weight, cova, vdw, charge;
    double cr, cg, cb;

    if (qt_get_elem_data(elem_num, &number, symbol, name, &weight, &cova, &vdw, &charge, &cr, &cg, &cb) != 0)
      continue;

    /* Determine font color: black for light backgrounds (white-ish), white for dark */
    double brightness = cr * 0.299 + cg * 0.587 + cb * 0.114;
    const char *font_color = (brightness > 0.6) ? "black" : "white";

    auto *btn = new QPushButton(QString(symbol), this);
    btn->setMinimumSize(42, 36);
    btn->setMaximumSize(65, 50);
    btn->setStyleSheet(QString("QPushButton { "
                               "background-color: rgb(%1, %2, %3); "
                               "color: %4; "
                               "font-weight: bold; "
                               "font-size: 12pt; "
                               "border: 1px solid #555; "
                               "padding: 2px; }")
                           .arg((int) (cr * 255))
                           .arg((int) (cg * 255))
                           .arg((int) (cb * 255))
                           .arg(font_color));

    btn->setToolTip(QString("%1  (n:%2)\nWeight: %3\nCovalent radius: %4 Å\nVdW radius: %5 Å")
                        .arg(name)
                        .arg(number)
                        .arg(weight, 0, 'f', 4)
                        .arg(cova, 0, 'f', 2)
                        .arg(vdw, 0, 'f', 2));

    /* Store the element number for click handling */
    btn->setProperty("elem_number", elem_num);
    QObject::connect(btn, &QPushButton::clicked, this, &PeriodicTableDialog::on_element_clicked);

    gridLayout->addWidget(btn, e.row, e.col);
  }

  mainLayout->addLayout(gridLayout);

  /* Buttons row */
  auto *btnLayout = new QHBoxLayout();
  auto *refreshBtn = new QPushButton("Refresh", this);
  auto *closeBtn = new QPushButton("Close", this);
  btnLayout->addStretch();
  btnLayout->addWidget(refreshBtn);
  btnLayout->addWidget(closeBtn);
  mainLayout->addLayout(btnLayout);

  QObject::connect(refreshBtn, &QPushButton::clicked, this, [this]() {
    qt_refresh_model_from_table();
    auto *canvas = static_cast<GLCanvas *>(qt_get_gl_canvas());
    if (canvas)
      canvas->update();
  });
  QObject::connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void PeriodicTableDialog::on_element_clicked()
{
  auto *btn = qobject_cast<QPushButton *>(sender());
  if (!btn)
    return;

  int elem_num = btn->property("elem_number").toInt();
  if (elem_num <= 0)
    return;

  qt_show_element_dialog(elem_num);
}

/* Qt bridge: show periodic table dialog */
extern "C" void qt_show_periodic_table_dialog(void)
{
  extern QWidget *get_main_window_widget();
  PeriodicTableDialog *dlg = new PeriodicTableDialog(get_main_window_widget());
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->show();
}
