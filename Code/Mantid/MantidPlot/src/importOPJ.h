/***************************************************************************
    File                 : importOPJ.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006-2007 by Ion Vasilief, Alex Kargovsky
    Email (use @ for *)  : ion_vasilief*yahoo.fr, kargovsky*yumr.phys.msu.su
    Description          : Origin project import class

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
#ifndef IMPORTOPJ_H
#define IMPORTOPJ_H

#include "ApplicationWindow.h"
#include "origin/OPJFile.h"

//! Origin project import class
class ImportOPJ
{
public:
	ImportOPJ(ApplicationWindow *app, const QString& filename);

	bool createProjectTree(const OPJFile& opj);
	bool importTables(const OPJFile& opj);
	bool importGraphs(const OPJFile& opj);
	bool importNotes(const OPJFile& opj);
	int error(){return parse_error;};

private:
    int arrowAngle(double length, double width){return ceil(45*atan(0.5*width/length)/atan(1.0));};
	int translateOrigin2QtiplotLineStyle(int linestyle);
	QString parseOriginText(const QString &str);
	QString parseOriginTags(const QString &str);
	void addText(const text& _text, Graph* graph, LegendWidget* txt, const rect& layerRect, double fFontScaleFactor, double fXScale, double fYScale);
	int parse_error;
	int xoffset;
	ApplicationWindow *mw;
};

#endif //IMPORTOPJ_H
