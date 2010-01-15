/***************************************************************************
    File                 : NonLinearFit.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : NonLinearFit class

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
#include "NonLinearFit.h"
#include "MyParser.h"
#include "fit_gsl.h"

#include <QMessageBox>

NonLinearFit::NonLinearFit(ApplicationWindow *parent, Graph *g)
: Fit(parent, g)
{
	init();
}

NonLinearFit::NonLinearFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle)
: Fit(parent, g)
{
	init();
	setDataFromCurve(curveTitle);
}

NonLinearFit::NonLinearFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end)
: Fit(parent, g)
{
	init();
	setDataFromCurve(curveTitle, start, end);
}

NonLinearFit::NonLinearFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow, int endRow)
: Fit(parent, t)
{
	init();
	setDataFromTable(t, xCol, yCol, startRow, endRow);
}

void NonLinearFit::init()
{
    if (objectName().isEmpty())
        setObjectName(tr("NonLinear"));
	d_formula = QString::null;
	d_f = user_f;
	d_df = user_df;
	d_fdf = user_fdf;
	d_fsimplex = user_d;
	d_explanation = tr("Non-linear Fit");
    d_fit_type = User;
}

void NonLinearFit::setFormula(const QString& s)
{
	if (s.isEmpty()){
		QMessageBox::critical((ApplicationWindow *)parent(),  tr("MantidPlot - Input function error"),
				tr("Please enter a valid non-empty expression! Operation aborted!"));
		d_init_err = true;
		return;
	}

	if (!d_p){
		QMessageBox::critical((ApplicationWindow *)parent(), tr("MantidPlot - Fit Error"),
				tr("There are no parameters specified for this fit operation. Please define a list of parameters first!"));
		d_init_err = true;
		return;
	}

	if (d_formula == s)
		return;

	try
	{
		double *param = new double[d_p];
		MyParser parser;
		double xvar;
		parser.DefineVar("x", &xvar);
		for (int k=0; k<(int)d_p; k++){
			param[k]=gsl_vector_get(d_param_init, k);
			parser.DefineVar(d_param_names[k].ascii(), &param[k]);
		}
		parser.SetExpr(s.ascii());
		parser.Eval() ;
		delete[] param;
	}
	catch(mu::ParserError &e)
	{
		QMessageBox::critical((ApplicationWindow *)parent(),  tr("MantidPlot - Input function error"), QString::fromStdString(e.GetMsg()));
		d_init_err = true;
		return;
	}

	d_init_err = false;
	d_formula = s;
}

void NonLinearFit::setParametersList(const QStringList& lst)
{
	if (lst.count() < 1){
		QMessageBox::critical((ApplicationWindow *)parent(), tr("MantidPlot - Fit Error"),
				tr("You must provide a list containing at least one parameter for this type of fit. Operation aborted!"));
		d_init_err = true;
		return;
	}

	d_init_err = false;
	d_param_names = lst;

	if (d_p > 0)
		freeWorkspace();
	d_p = (int)lst.count();
	initWorkspace(d_p);

	for (int i=0; i<d_p; i++)
		d_param_explain << "";
}

void NonLinearFit::calculateFitCurveData(double *X, double *Y)
{
	MyParser parser;
	for (int i=0; i<d_p; i++)
		parser.DefineVar(d_param_names[i].ascii(), &d_results[i]);

	double x;
	parser.DefineVar("x", &x);
	parser.SetExpr(d_formula.ascii());

	if (d_gen_function){
		double X0 = d_x[0];
		double step = (d_x[d_n-1]-X0)/(d_points-1);
		for (int i=0; i<d_points; i++){
		    x = X0 + i*step;
			X[i] = x;
			Y[i] = parser.Eval();
		}
	} else {
		for (int i=0; i<d_points; i++) {
		    x = d_x[i];
			X[i] = x;
			Y[i] = parser.Eval();
		}
	}
}

double NonLinearFit::eval(double *par, double x)
{
	MyParser parser;
	for (int i=0; i<d_p; i++)
		parser.DefineVar(d_param_names[i].ascii(), &par[i]);
	parser.DefineVar("x", &x);
	parser.SetExpr(d_formula.ascii());
    return parser.Eval();
}
