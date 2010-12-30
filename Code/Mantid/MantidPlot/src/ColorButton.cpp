/***************************************************************************
    File                 : ColorButton.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2007 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : A button used for color selection

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
#include "ColorButton.h"
#include <QPainter>
#include <QColorDialog>

ColorButton::ColorButton(QWidget *parent) : QPushButton(parent),
	d_color(Qt::black)
{
   updateColor();
   connect(this, SIGNAL(clicked()), this, SLOT(pickColor()));
}

void ColorButton::setColor(const QColor& c)
{
	if (d_color == c)
        return;

    d_color = c;
    updateColor();
    emit colorChanged();
}

void ColorButton::updateColor()
{
	QRect r = QRect(0, 0, 27, 15);
    QPixmap pix(QSize(28, 16));
	pix.fill(d_color);

	QPainter p;
	p.begin(&pix);
	p.drawRect(r);
	p.end();

	setIcon(QIcon(pix));
}

void ColorButton::pickColor()
{
    QColor c = QColorDialog::getColor(d_color, this);
	if ( !c.isValid() )
		return ;

	setColor(c);
}
