/***************************************************************************
    File                 : LogisticFit.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2007 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : Logistic Fit class

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
#include "LogisticFit.h"
#include "fit_gsl.h"

LogisticFit::LogisticFit(ApplicationWindow *parent, Graph *g)
: Fit(parent, g)
{
	init();
}

LogisticFit::LogisticFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle)
: Fit(parent, g)
{
	init();
	setDataFromCurve(curveTitle);
}

LogisticFit::LogisticFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end)
: Fit(parent, g)
{
	init();
	setDataFromCurve(curveTitle, start, end);
}

LogisticFit::LogisticFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow, int endRow)
: Fit(parent, t)
{
	init();
	setDataFromTable(t, xCol, yCol, startRow, endRow);
}

void LogisticFit::init()
{
	setObjectName(tr("Logistic"));
    d_f = logistic_f;
    d_df = logistic_df;
	d_fdf = logistic_fdf;
	d_fsimplex = logistic_d;
	d_param_explain << tr("init value") << tr("final value") << tr("center") << tr("power");
	d_param_names << "A1" << "A2" << "x0" << "p";
	d_explanation = tr("Logistic Fit");
	d_formula = "A2+(A1-A2)/(1+(x/x0)^p))";
	d_p = 4;    
	initWorkspace(d_p);
}

void LogisticFit::calculateFitCurveData(double *X, double *Y)
{
	double a1 = d_results[0];
	double a2 = d_results[1];
	double x0 = d_results[2];
	double p = d_results[3];

	if (d_gen_function){
		double X0 = d_x[0];
		double step = (d_x[d_n-1]-X0)/(d_points-1);
        for (int i=0; i<d_points; i++){
			double x = X0+i*step;
        	X[i] = x;
        	Y[i] = (a1 - a2)/(1 + pow(x/x0, p)) + a2;
		} 
	} else {
      	for (int i=0; i<d_points; i++){
			double x = d_x[i];
        	X[i] = x;
        	Y[i] = (a1 - a2)/(1 + pow(x/x0, p)) + a2;
		}
	}
}

void LogisticFit::guessInitialValues()
{
	gsl_vector_view x = gsl_vector_view_array (d_x, d_n);
	gsl_vector_view y = gsl_vector_view_array (d_y, d_n);

	double min_out, max_out;
	gsl_vector_minmax (&y.vector, &min_out, &max_out);

	gsl_vector_set(d_param_init, 0, min_out);
	gsl_vector_set(d_param_init, 1, max_out);
	gsl_vector_set(d_param_init, 2, gsl_vector_get (&x.vector, d_n/2));
	gsl_vector_set(d_param_init, 3, 1.0);
}
