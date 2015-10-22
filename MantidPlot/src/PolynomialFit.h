/***************************************************************************
    File                 : PolynomialFit.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : Polynomial Fit and Linear Fit classes

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
#ifndef POLYNOMIALFIT_H
#define POLYNOMIALFIT_H

#include "Fit.h"

class PolynomialFit : public Fit
{
	Q_OBJECT

	public:
		PolynomialFit(ApplicationWindow *parent, Graph *g, int order = 2, bool legend = false);
		PolynomialFit(ApplicationWindow *parent, Graph *g, QString& curveTitle, int order = 2, bool legend = false);
		PolynomialFit(ApplicationWindow *parent, Graph *g, QString& curveTitle, double start, double end, int order = 2, bool legend = false);
		PolynomialFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow = 1, int endRow = -1, int order = 2, bool legend = false);

		virtual QString legendInfo();
		void fit();

		int order(){return d_order;};
		void setOrder(int order);

		static QString generateFormula(int order);
		static QStringList generateParameterList(int order);

		virtual double eval(double *par, double x);

	private:
		void init();
		void calculateFitCurveData(double *X, double *Y);

		int d_order;
		bool show_legend;
};

class LinearFit : public Fit
{
	Q_OBJECT

	public:
		LinearFit(ApplicationWindow *parent, Graph *g);
		LinearFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle);
		LinearFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end);
		LinearFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow = 1, int endRow = -1);

		void fit();
	    virtual double eval(double *par, double x){return par[0] + par[1]*x;};

	private:
		void init();
		void calculateFitCurveData(double *X, double *Y);
};

class LinearSlopeFit : public Fit
{
	Q_OBJECT

	public:
		LinearSlopeFit(ApplicationWindow *parent, Graph *g);
		LinearSlopeFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle);
		LinearSlopeFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end);
		LinearSlopeFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow = 1, int endRow = -1);

		void fit();
	    virtual double eval(double *par, double x){return par[0]*x;};

	private:
		void init();
		void calculateFitCurveData(double *X, double *Y);
};
#endif
