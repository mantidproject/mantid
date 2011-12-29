/***************************************************************************
	File                 : ImageSymbol.h
    Project              : QtiPlot
    --------------------------------------------------------------------
	Copyright            : (C) 2010 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
	Description          : A QwtSymbol displaying a custom image

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
#include <qwt_symbol.h>

//! A QwtSymbol displaying a custom image
class ImageSymbol: public QwtSymbol
{
public:
	ImageSymbol(const QString& fileName);
	ImageSymbol(const QPixmap& pixmap, const QString& fileName = QString());

	virtual ImageSymbol *clone() const;
	virtual void draw(QPainter *p, const QRect &r) const;

	QPixmap pixmap(){return d_pixmap;};
	QString imagePath(){return d_image_path;};

private:
	QString d_image_path;
	QPixmap d_pixmap;
};
