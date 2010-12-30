/***************************************************************************
    File                 : Differentiation.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2007 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : Numerical differentiation of data sets

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
#include "Differentiation.h"
#include "MultiLayer.h"
#include "LegendWidget.h"

#include <QLocale>

Differentiation::Differentiation(ApplicationWindow *parent, Graph *g)
: Filter(parent, g)
{
	init();
}

Differentiation::Differentiation(ApplicationWindow *parent, Graph *g, const QString& curveTitle)
: Filter(parent, g)
{
	init();
	setDataFromCurve(curveTitle);
}

Differentiation::Differentiation(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end)
: Filter(parent, g)
{
	init();
	setDataFromCurve(curveTitle, start, end);
}

Differentiation::Differentiation(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int start, int end)
: Filter(parent, t)
{
	init();
	setDataFromTable(t, xCol, yCol, start, end);
}

void Differentiation::init()
{
	setObjectName(tr("Derivative"));
    d_min_points = 4;
}

void Differentiation::output()
{
    double *result = new double[d_n-1];
	for (int i = 1; i < d_n-1; i++)
		result[i]=0.5*((d_y[i+1]-d_y[i])/(d_x[i+1]-d_x[i]) + (d_y[i]-d_y[i-1])/(d_x[i]-d_x[i-1]));

    ApplicationWindow *app = (ApplicationWindow *)parent();
    QLocale locale = app->locale();
    QString tableName = app->generateUniqueName(QString(objectName()));
    QString dataSet;
	if (d_curve)
		dataSet = d_curve->title().text();
	else
		dataSet = d_y_col_name;

    d_result_table = app->newHiddenTable(tableName, tr("Derivative") + " " + tr("of","Derivative of")  + " " + dataSet, d_n-2, 2);
	for (int i = 1; i < d_n-1; i++) {
		d_result_table->setText(i-1, 0, locale.toString(d_x[i], 'g', app->d_decimal_digits));
		d_result_table->setText(i-1, 1, locale.toString(result[i], 'g', app->d_decimal_digits));
	}
    delete[] result;

	if (d_graphics_display){
		if (!d_output_graph)
			d_output_graph = createOutputGraph()->activeGraph();

    	d_output_graph->insertCurve(d_result_table, tableName + "_2", 0);
    	QString legend = "\\l(1)" + tr("Derivative") + " " + tr("of","Derivative of") + " " + dataSet;
    	LegendWidget *l = d_output_graph->legend();
		if (l){
    		l->setText(legend);
    		l->repaint();
        } else
            d_output_graph->newLegend(legend);
	}
}
