/***************************************************************************
    File                 : PluginFit.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Plugin Fit class

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
#include "PluginFit.h"

#include <QLibrary>
#include <QMessageBox>

PluginFit::PluginFit(ApplicationWindow *parent, Graph *g)
: Fit(parent, g), f_eval(NULL)
{
	init();
}

PluginFit::PluginFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle)
: Fit(parent, g), f_eval(NULL)
{
	init();
	setDataFromCurve(curveTitle);
}

PluginFit::PluginFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end)
: Fit(parent, g), f_eval(NULL)
{
	init();
	setDataFromCurve(curveTitle, start, end);
}

PluginFit::PluginFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow, int endRow)
: Fit(parent, t), f_eval(NULL)
{
	init();
	setDataFromTable(t, xCol, yCol, startRow, endRow);
}

void PluginFit::init()
{
	d_explanation = tr("Plugin Fit");
    d_fit_type = Plugin;
}


namespace{
typedef union {
  double (*func)(const gsl_vector *, void *);
  void* ptr;
} simplex_union;

typedef union {
  int (*func)(const gsl_vector *, void *, gsl_vector *);
  void* ptr;
} f_union;

typedef union {
  int (*func)(const gsl_vector *, void *,gsl_matrix *);
  void* ptr;
} df_union;

typedef union {
  int (*func)(const gsl_vector *, void *, gsl_vector *, gsl_matrix *);
  void* ptr;
} fdf_union;

typedef union {
  double (*func)(double, double *);
  void* ptr;
} ffe_union;

typedef union {
  char* (*func)();
  void* ptr;
} ff_union;
}

bool PluginFit::load(const QString& pluginName)
{
	if (!QFile::exists (pluginName)){
		QMessageBox::critical(static_cast<ApplicationWindow*>(parent()), tr("MantidPlot - File not found"),
				tr("Plugin file: <p><b> %1 </b> <p>not found. Operation aborted!").arg(pluginName));
		return false;
	}

	QLibrary lib(pluginName);
	lib.setAutoUnload(false);

	simplex_union simplex;
        simplex.ptr = lib.resolve( "function_d" );
        d_fsimplex = simplex.func;
	if (!d_fsimplex){
		QMessageBox::critical(static_cast<ApplicationWindow*>(parent()), tr("MantidPlot - Plugin Error"),
				tr("The plugin does not implement a %1 method necessary for simplex fitting.").arg("function_d"));
		return false;
	}

        f_union f;
        f.ptr = lib.resolve( "function_f" );
        d_f = f.func;
	if (!d_f){
		QMessageBox::critical(static_cast<ApplicationWindow*>(parent()), tr("MantidPlot - Plugin Error"),
				tr("The plugin does not implement a %1 method necessary for Levenberg-Marquardt fitting.").arg("function_f"));
		return false;
	}

        df_union df;
        df.ptr = lib.resolve( "function_df" );
	d_df = df.func;
	if (!(df.ptr)){
		QMessageBox::critical(static_cast<ApplicationWindow*>(parent()), tr("MantidPlot - Plugin Error"),
				tr("The plugin does not implement a %1 method necessary for Levenberg-Marquardt fitting.").arg("function_df"));
		return false;
	}

        fdf_union fdf;
        fdf.ptr = lib.resolve( "function_fdf" );
	d_fdf = fdf.func;
	if (!d_fdf){
		QMessageBox::critical(static_cast<ApplicationWindow*>(parent()), tr("MantidPlot - Plugin Error"),
				tr("The plugin does not implement a %1 method necessary for Levenberg-Marquardt fitting.").arg("function_fdf"));
		return false;
	}

        ffe_union ffe;
        ffe.ptr = lib.resolve("function_eval");
	f_eval = ffe.func;
	if (!f_eval)
		return false;

	typedef char* (*fitFunc)();
        ff_union ff;
        ff.ptr = lib.resolve("parameters");
	fitFunc fitFunction = ff.func;
	if (fitFunction){
		d_param_names = QString(fitFunction()).split(",", QString::SkipEmptyParts);
		d_p = (int)d_param_names.count();
        initWorkspace(d_p);
	} else
		return false;

        ff.ptr = lib.resolve("explanations");
	fitFunc fitExplain = ff.func;
	if (fitExplain)
		d_param_explain = QString(fitExplain()).split(",", QString::SkipEmptyParts);
	else
		for (int i=0; i<d_p; i++)
			d_param_explain << "";

        ff.ptr = lib.resolve("name");
	fitFunction = ff.func;
	setObjectName(QString(fitFunction()));

        ff.ptr = lib.resolve("function");
	fitFunction = ff.func;
	if (fitFunction)
		d_formula = QString(fitFunction());
	else
		return false;

	return true;
}

void PluginFit::calculateFitCurveData(double *X, double *Y)
{
	if (d_gen_function && f_eval)
	{
		double X0 = d_x[0];
		double step = (d_x[d_n-1]-X0)/(d_points-1);
		for (int i=0; i<d_points; i++){
		    double x = X0+i*step;
			X[i] = x;
			Y[i]= f_eval(x, d_results);
		}
	} else {
		for (int i=0; i<d_points; i++) {
		    double x = d_x[i];
			X[i] = x;
			Y[i]= f_eval(x, d_results);
		}
	}
}
