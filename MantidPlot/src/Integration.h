/***************************************************************************
    File                 : Integration.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2007 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : Numerical integration of data sets

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
#ifndef INTEGRATION_H
#define INTEGRATION_H

#include "Filter.h"

class Integration : public Filter
{
Q_OBJECT

public:
	enum Integrand{DataSet, AnalyticalFunction};

	Integration(ApplicationWindow *parent, Graph *g);
	Integration(ApplicationWindow *parent, Graph *g, const QString& curveTitle);
	Integration(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end);
	Integration(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int start, int end);
	Integration(const QString& formula, const QString& var, ApplicationWindow *parent,
                Graph *g, double start, double end);

    int method(){return d_method;};
    void setMethodOrder(int n);

    double area(){return d_area;};

private:
    void init();
    QString logInfo();

    void output();

	double trapez();
	double trapezf(int n);
    //! Returns the number of iterations used to calculate the area if d_integrand = AnalyticalFunction.
    int romberg();

    //! the integration method: 1 = trapezoidal, max = 5!
    int d_method;

    //! the value of the integral
    double d_area;

	//! the type of the integrand
	Integrand d_integrand;
	//! Analytical function to be integrated
	QString d_formula;
	//! Variable name for the function to be integrated
	QString d_variable;
};

#endif
