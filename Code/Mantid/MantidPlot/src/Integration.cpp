/***************************************************************************
    File                 : Integration.cpp
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
#include "Integration.h"
#include "nrutil.h"
#include "MultiLayer.h"
#include "MyParser.h"
#include "FunctionCurve.h"

#include <QMessageBox>
#include <QDateTime>
#include <QLocale>

#include <gsl/gsl_vector.h>

Integration::Integration(const QString& formula, const QString& var, ApplicationWindow *parent, Graph *g, double start, double end)
: Filter(parent, g),
d_formula(formula),
d_variable(var)
{
	d_init_err = false;
	d_n = 0;
	d_from = start;
	d_to = end;
	if (d_to == d_from)
		d_init_err = true;

	MyParser parser;
	double x = 0.0;
	parser.DefineVar(d_variable.ascii(), &x);
	parser.SetExpr(d_formula.ascii());
	try {
		parser.Eval();
	} catch(mu::ParserError &e) {
		QMessageBox::critical(parent, tr("MantidPlot - Input error"), QString::fromStdString(e.GetMsg()));
		d_init_err = true;
	}

	setObjectName(tr("Integration"));
	d_integrand = AnalyticalFunction;
	d_method = 1;
    d_max_iterations = 20;
    d_sort_data = false;
}

Integration::Integration(ApplicationWindow *parent, Graph *g)
: Filter(parent, g)
{
	init();
}

Integration::Integration(ApplicationWindow *parent, Graph *g, const QString& curveTitle)
: Filter(parent, g)
{
	init();
	setDataFromCurve(curveTitle);
}

Integration::Integration(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end)
: Filter(parent, g)
{
	init();
	setDataFromCurve(curveTitle, start, end);
}

Integration::Integration(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int start, int end)
: Filter(parent, t)
{
	init();
	setDataFromTable(t, xCol, yCol, start, end);
}

void Integration::init()
{
	setObjectName(tr("Integration"));
	d_integrand = DataSet;
	d_method = 1;
    d_max_iterations = 1;
    d_sort_data = true;
}

double Integration::trapez()
{
	double sum = 0.0;
	int size = d_n - 1;
	for(int i=0; i < size; i++){
		int j = i + 1;
		sum += 0.5*(d_y[j] + d_y[i])*(d_x[j] - d_x[i]);
	}
    return sum;
}

double Integration::trapezf(int n)
{
    MyParser parser;
	double x = d_from;
	parser.DefineVar(d_variable.ascii(), &x);
	parser.SetExpr(d_formula.ascii());

    static double s;
    if (n == 1){
		x = d_from;
		double aux = parser.Eval();
		x = d_to;
        return (s = 0.5*(d_to - d_from)*(aux + parser.Eval()));
    } else {
        int it = 1;
        for(int j=1; j < n-1; j++)
            it<<=1;

        double tnm = it;
        double del = (d_to - d_from)/tnm;
        x = d_from + 0.5*del;
        double sum = 0.0;
        for(int j=1; j <= it; j++, x += del)
            sum += parser.Eval();

        s = 0.5*(s + (d_to - d_from)*sum/tnm);
        return s;
    }
}

// Using Numerical Recipes. This is Romberg Integration method.
int Integration::romberg()
{
    d_area = 0.0;
	double *s = new double[d_max_iterations + 1];
	double *h = new double[d_max_iterations + 2];
	h[1] = 1.0;
	int j;
	for(j = 1; j <= d_max_iterations; j++){
        s[j] = trapezf(j);
		if(j > d_method){
		    double ss, dss;
			polint(&h[j-d_method], &s[j-d_method], d_method, 0.0, &ss, &dss);
			if (fabs(dss) <= d_tolerance * fabs(ss)){
                d_area = ss;
                break;
			}
		}
		h[j+1] = 0.25*h[j];
	}
    delete[] s;
    delete[] h;
    return j;
}

QString Integration::logInfo()
{
	ApplicationWindow *app = (ApplicationWindow *)parent();
    QLocale locale = app->locale();
    int prec = app->d_decimal_digits;

	QString logInfo = "[" + QDateTime::currentDateTime().toString(Qt::LocalDate);
	if (d_integrand == AnalyticalFunction){
		logInfo += "\n" + tr("Numerical integration of") + " f(" + d_variable + ") = " + d_formula + " ";
		logInfo += tr("using a %1 order method").arg(d_method) + "\n";
		logInfo += tr("From") + " x = " + locale.toString(d_from, 'g', prec) + " ";
		logInfo += tr("to") + " x = " + locale.toString(d_to, 'g', prec) + "\n";
		logInfo += tr("Tolerance") + " = " + locale.toString(d_tolerance, 'g', prec) + "\n";
		logInfo += tr("Iterations") + ": " + QString::number(romberg()) + "\n";
	} else if (d_integrand == DataSet){
		if (d_graph)
			logInfo += tr("\tPlot")+ ": ''" + d_graph->multiLayer()->objectName() + "'']\n";
		else
			logInfo += "\n";
		QString dataSet;
		if (d_curve)
			dataSet = d_curve->title().text();
		else
			dataSet = d_y_col_name;
		logInfo += "\n" + tr("Numerical integration of") + ": " + dataSet + " ";
		logInfo += tr("using the Trapezoidal Rule") + "\n";
		logInfo += tr("Points") + ": " + QString::number(d_n) + " " + tr("from") + " x = " + locale.toString(d_from, 'g', prec) + " ";
    	logInfo += tr("to") + " x = " + locale.toString(d_to, 'g', prec) + "\n";

		// use GSL to find maximum value of data set
		gsl_vector *aux = gsl_vector_alloc(d_n);
		for(int i=0; i < d_n; i++)
			gsl_vector_set (aux, i, fabs(d_y[i]));
		int maxID = static_cast<int>(gsl_vector_max_index (aux));
		gsl_vector_free(aux);

    	logInfo += tr("Peak at") + " x = " + locale.toString(d_x[maxID], 'g', prec)+"\t";
		logInfo += "y = " + locale.toString(d_y[maxID], 'g', prec)+"\n";
		d_area = trapez();
	}

	logInfo += tr("Area") + "=" + locale.toString(d_area, 'g', prec);
	logInfo += "\n-------------------------------------------------------------\n";
    return logInfo;
}

void Integration::setMethodOrder(int n)
{
    if (n < 1 || n > 5){
        QMessageBox::critical((ApplicationWindow *)parent(), tr("MantidPlot - Error"),
        tr("Unknown integration method. Valid values must be in the range: 1 (Trapezoidal Method) to 5."));
        return;
    }
    d_method = n;
}

void Integration::output()
{
    if(d_integrand != AnalyticalFunction || d_init_err)
        return;

    if (!d_output_graph)
        return;

    FunctionCurve* c = d_output_graph->addFunction(QStringList(d_formula), d_from, d_to, d_points,
                    d_variable, FunctionCurve::Normal);
    if (c){
        c->setBrush(QBrush(c->pen().color(), Qt::BDiagPattern));
        d_output_graph->replot();
    }
}
