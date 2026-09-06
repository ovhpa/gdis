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

#include "textoverlay.h"
#include <QPainter>

TextOverlayWidget::TextOverlayWidget(QWidget *parent) : QWidget(parent)
{
  setAttribute(Qt::WA_TransparentForMouseEvents);
  setAttribute(Qt::WA_NoSystemBackground);
  setStyleSheet("background: transparent;");
}

void TextOverlayWidget::addOverlay(const TextOverlay &o)
{
  m_overlays.append(o);
  /* Update geometry to cover all overlays */
  int maxX = 0, maxY = 0;
  for (const auto &ov : m_overlays)
  {
    if (ov.x + ov.image.width() > maxX)
      maxX = ov.x + ov.image.width();
    if (ov.y + ov.image.height() > maxY)
      maxY = ov.y + ov.image.height();
  }
  setGeometry(0, 0, maxX, maxY);
  update();
}

void TextOverlayWidget::addOverlays(const QVector<TextOverlay> &overlays)
{
  m_overlays = overlays;
  int maxX = 0, maxY = 0;
  for (const auto &ov : m_overlays)
  {
    if (ov.x + ov.image.width() > maxX)
      maxX = ov.x + ov.image.width();
    if (ov.y + ov.image.height() > maxY)
      maxY = ov.y + ov.image.height();
  }
  /* Keep the widget sized to cover the full canvas area, not just text bounds.
   * The geometry is set by updateOverlayGeometry() to match the canvas. */
  if (maxX > 0 && maxY > 0)
    raise(); /* Ensure on top of everything */
  update();
}

void TextOverlayWidget::clearOverlays()
{
  m_overlays.clear();
  setGeometry(0, 0, 0, 0);
  update();
}

void TextOverlayWidget::paintEvent(QPaintEvent *)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  for (const auto &ov : m_overlays)
  {
    if (!ov.image.isNull())
    {
      painter.drawImage(ov.x, ov.y, ov.image);
    }
  }
}
