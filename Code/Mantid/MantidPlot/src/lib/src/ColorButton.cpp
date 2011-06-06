/***************************************************************************
    File                 : ColorButton.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
	Copyright            : (C) 2009 - 2010 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : A wrapper around QtColorPicker from QtSolutions

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
#include "ColorBox.h"
#include <QColorDialog>

ColorButton::ColorButton(QWidget *parent) : QtColorPicker(parent)
{
	QStringList color_names = ColorBox::defaultColorNames();
	QList<QColor> defaultColors = ColorBox::defaultColors();
	for (int i = 0; i < ColorBox::numPredefinedColors(); i++)
		insertColor(defaultColors[i], color_names[i]);

	QList<QColor> colors = ColorBox::colorList();
	color_names = ColorBox::colorNames();
	for (int i = 0; i < colors.count(); i++){
		QColor c = colors[i];
		if (!defaultColors.contains(c))
			insertColor(c, color_names[i]);
	}

	connect(this, SIGNAL(colorChanged(const QColor & )), this, SIGNAL(colorChanged()));
}
