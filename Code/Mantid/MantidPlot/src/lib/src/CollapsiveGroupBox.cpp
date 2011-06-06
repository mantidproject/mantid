/***************************************************************************
	File                 : CollapsiveGroupBox.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
	Copyright            : (C) 2010 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
	Description          : A collapsive QGroupBox

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
#include "CollapsiveGroupBox.h"

CollapsiveGroupBox::CollapsiveGroupBox(const QString & title, QWidget *parent) : QGroupBox(title, parent)
{
	setCheckable(true);
	connect(this, SIGNAL(toggled(bool)), this, SLOT(setExpanded(bool)));
}

void CollapsiveGroupBox::setCollapsed(bool collapsed)
{
	foreach (QObject *o, children()){
		if (o->isWidgetType())
			((QWidget *)o)->setVisible(collapsed);
	}

	setFlat(collapsed);
}

void CollapsiveGroupBox::setExpanded(bool expanded)
{
	foreach (QObject *o, children()){
		if (o->isWidgetType())
			((QWidget *)o)->setVisible(expanded);
	}

	setFlat(!expanded);
}
