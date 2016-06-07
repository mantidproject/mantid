/***************************************************************************
        File                 : ImageSymbol.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
        Copyright            : (C) 2010 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
        Description          : A QwtSymbol displaying a custom images

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#include "ImageSymbol.h"
#include <QPainter>
#include <qwt_painter.h>

ImageSymbol::ImageSymbol(const QString &fileName)
    : QwtSymbol(QwtSymbol::StyleCnt, QBrush(), QPen(Qt::NoPen), QSize()),
      d_image_path(fileName) {
  d_pixmap.load(fileName);
  setSize(d_pixmap.size());
}

ImageSymbol::ImageSymbol(const QPixmap &pixmap, const QString &fileName)
    : QwtSymbol(QwtSymbol::StyleCnt, QBrush(), QPen(Qt::NoPen), QSize()),
      d_image_path(fileName) {
  d_pixmap = QPixmap(pixmap);
  setSize(d_pixmap.size());
}

ImageSymbol *ImageSymbol::clone() const {
  ImageSymbol *other = new ImageSymbol(d_image_path);
  *other = *this;

  return other;
}

/*!
  \brief Draw the symbol into a bounding rectangle.
  \param painter Painter
  \param r Bounding rectangle
*/
void ImageSymbol::draw(QPainter *p, const QRect &r) const {
  p->drawPixmap(r, d_pixmap);
}
