/***************************************************************************
    File                 : ColorBox.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
	Copyright            : (C) 2006-2010 by Ion Vasilief, Alex Kargovsky
    Email (use @ for *)  : ion_vasilief*yahoo.fr, kargovsky*yumr.phys.msu.su
    Description          : A combo box to select a standard color

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
#include "ColorBox.h"

#include <QSettings>
#include <QPixmap>
#include <QPainter>
#include <algorithm>

const QColor ColorBox::colors[] = {
  QColor(Qt::black),
  QColor(Qt::red),
  QColor(Qt::green),
  QColor(Qt::blue),
  QColor(Qt::cyan),
  QColor(Qt::magenta),
  QColor(Qt::yellow),
  QColor(Qt::darkYellow),
  QColor(Qt::darkBlue),
  QColor(Qt::darkMagenta),
  QColor(Qt::darkRed),
  QColor(Qt::darkGreen),
  QColor(Qt::darkCyan),
  QColor("#0000A0"),
  QColor("#FF8000"),
  QColor("#8000FF"),
  QColor("#FF0080"),
  QColor(Qt::white),
  QColor(Qt::lightGray),
  QColor(Qt::gray),
  QColor("#FFFF80"),
  QColor("#80FFFF"),
  QColor("#FF80FF"),
  QColor(Qt::darkGray),
};

ColorBox::ColorBox(QWidget *parent) : QComboBox(parent)
{
    setEditable(false);
	init();
}

void ColorBox::init()
{
	QList<QColor> indexedColors = colorList();
	QStringList color_names = colorNames();

	QPixmap icon = QPixmap(28, 16);
	QRect r = QRect(0, 0, 27, 15);

	QPainter p;
	p.begin(&icon);

	for (int i = 0; i < indexedColors.size(); i++){
		p.setBrush(QBrush(indexedColors[i]));
		p.drawRect(r);
		this->addItem(icon, color_names[i]);
	}
	p.end();
}

void ColorBox::setColor(const QColor& c)
{
	setCurrentIndex(colorIndex(c));
}

QColor ColorBox::color() const
{
	return color(this->currentIndex());
}

int ColorBox::colorIndex(const QColor& c)
{
	if (!isValidColor(c))
		return 0;

	return colorList().indexOf(c);
}

QColor ColorBox::color(int colorIndex)
{
	QList<QColor> colorsList = colorList();
	if (colorIndex >= 0 && colorIndex < colorsList.size())
		return colorsList[colorIndex];

	return Qt::black; // default color is black.
}

QList<QColor> ColorBox::colorList()
{
#ifdef Q_OS_MAC
	QSettings settings(QSettings::IniFormat,QSettings::UserScope, "ProIndependent", "QtiPlot");
#else
	QSettings settings(QSettings::NativeFormat,QSettings::UserScope, "ProIndependent", "QtiPlot");
#endif
	settings.beginGroup("/General");

	QList<QColor> indexedColors;
	QStringList lst = settings.value("/IndexedColors").toStringList();
	if (!lst.isEmpty()){
		for (int i = 0; i < lst.size(); i++)
			indexedColors << QColor(lst[i]);
	} else {
		for (int i = 0; i < colors_count; i++)
			indexedColors << colors[i];
	}
	settings.endGroup();

	return indexedColors;
}

QStringList ColorBox::colorNames()
{
#ifdef Q_OS_MAC
	QSettings settings(QSettings::IniFormat,QSettings::UserScope, "ProIndependent", "QtiPlot");
#else
	QSettings settings(QSettings::NativeFormat,QSettings::UserScope, "ProIndependent", "QtiPlot");
#endif
	settings.beginGroup("/General");
	QStringList color_names = settings.value("/IndexedColorNames", defaultColorNames()).toStringList();
	settings.endGroup();
	return color_names;
}

QColor ColorBox::defaultColor(int colorIndex)
{
	if (colorIndex >= 0 && colorIndex < (int)sizeof(colors))
		return colors[colorIndex];

	return Qt::black; // default color is black.
}

bool ColorBox::isValidColor(const QColor& color)
{
	return colorList().contains(color);
}

int ColorBox::numPredefinedColors()
{
	return colors_count;
}

QStringList ColorBox::defaultColorNames()
{
	QStringList color_names = QStringList() << tr( "black" );
	color_names << tr( "red" );
	color_names << tr( "green" );
	color_names << tr( "blue" );
	color_names << tr( "cyan" );
	color_names << tr( "magenta" );
	color_names << tr( "yellow" );
	color_names << tr( "dark yellow" );
	color_names << tr( "navy" );
	color_names << tr( "purple" );
	color_names << tr( "wine" );
	color_names << tr( "olive" );
	color_names << tr( "dark cyan" );
	color_names << tr( "royal" );
	color_names << tr( "orange" );
	color_names << tr( "violet" );
	color_names << tr( "pink" );
	color_names << tr( "white" );
	color_names << tr( "light gray" );
	color_names << tr( "gray" );
	color_names << tr( "light yellow" );
	color_names << tr( "light cyan" );
	color_names << tr( "light magenta" );
	color_names << tr( "dark gray" );
	return color_names;
}

QList<QColor> ColorBox::defaultColors()
{
	QList<QColor> lst;
	for (int i = 0; i < colors_count; i++)
		lst << colors[i];

	return lst;
}
