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

#ifndef TEXTOVERLAY_H
#define TEXTOVERLAY_H

#include <QWidget>
#include <QVector>

struct TextOverlay {
  int x, y;
  QImage image;
};

class TextOverlayWidget : public QWidget
{
  Q_OBJECT
public:
  explicit TextOverlayWidget(QWidget *parent = nullptr);
  void addOverlay(const TextOverlay &o);
  void addOverlays(const QVector<TextOverlay> &overlays);
  void clearOverlays();

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  QVector<TextOverlay> m_overlays;
};

#endif // TEXTOVERLAY_H
