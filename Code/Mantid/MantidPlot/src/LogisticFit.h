/***************************************************************************
    File                 : LogisticFit.h
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
#ifndef LOGISTICFIT_H
#define LOGISTICFIT_H

#include "Fit.h"

class LogisticFit : public Fit
{
	Q_OBJECT

	public:
		LogisticFit(ApplicationWindow *parent, Graph *g);
		LogisticFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle);
		LogisticFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end);
		LogisticFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow = 1, int endRow = -1);

		void guessInitialValues();
        double eval(double *par, double x){return (par[0]-par[1])/(1+pow(x/par[2], par[3]))+par[1];};

	private:
		void init();
		void calculateFitCurveData(double *X, double *Y);
};

#endif
