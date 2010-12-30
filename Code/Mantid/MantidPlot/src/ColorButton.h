/***************************************************************************
    File                 : ColorButton.h
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

#ifndef COLORBUTTON_H
#define COLORBUTTON_H

#include <QPushButton>

//! A button used for color selection
class ColorButton : public QPushButton
{
	Q_OBJECT

public:
	//! Constructor
	ColorButton(QWidget *parent = 0);
	//! Set the color of the display part
	void setColor(const QColor& c);
	//! Get the color of the display part
	QColor color(){return d_color;};

signals:
    void colorChanged();

private slots:
    void pickColor();

private:
    void updateColor();
	QColor d_color;
};

#endif
