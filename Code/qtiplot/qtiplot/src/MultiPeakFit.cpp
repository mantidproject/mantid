/***************************************************************************
    File                 : fitclasses.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : MultiPeakFit module with Lorentz and Gauss peak shapes

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
#include "MultiPeakFit.h"
#include "fit_gsl.h"
#include "PlotCurve.h"
#include "FunctionCurve.h"
#include "ColorBox.h"
#include "MultiLayer.h"

#include <gsl/gsl_statistics.h>

#include <QLocale>
#include <QMessageBox>

MultiPeakFit::MultiPeakFit(ApplicationWindow *parent, Graph *g, PeakProfile profile, int peaks)
: Fit(parent, g), d_profile(profile)
{
init(peaks);
}

MultiPeakFit::MultiPeakFit(ApplicationWindow *parent, Table *t, const QString&, const QString&,
		  				  int, int, PeakProfile profile, int peaks)
: Fit(parent, t), d_profile(profile)
{
init(peaks);
}

void MultiPeakFit::init(int peaks)
{
	if (d_profile == Gauss){
		setObjectName(tr("Gauss"));
		d_f = gauss_multi_peak_f;
		d_df = gauss_multi_peak_df;
		d_fdf = gauss_multi_peak_fdf;
		d_fsimplex = gauss_multi_peak_d;
	} else {
		setObjectName(tr("Lorentz"));
		d_f = lorentz_multi_peak_f;
		d_df = lorentz_multi_peak_df;
		d_fdf = lorentz_multi_peak_fdf;
		d_fsimplex = lorentz_multi_peak_d;
	}

	d_param_init = NULL;
	covar = NULL;
	d_results = NULL;

	setNumPeaks(peaks);

	generate_peak_curves = true;
	d_peaks_color = 2;//green
}

void MultiPeakFit::setNumPeaks(int n)
{
	d_peaks = n;
	if (d_profile == Gauss)
		d_explanation = tr("Gauss Fit");
	else
		d_explanation = tr("Lorentz Fit");
	if (d_peaks > 1)
		d_explanation += "(" + QString::number(d_peaks) +") " + tr("multi-peak");

	freeWorkspace();
	d_p = 3*d_peaks + 1;
	initWorkspace(d_p);

	d_param_names = generateParameterList(d_peaks);
	d_param_explain = generateExplanationList(d_peaks);
	d_formula = generateFormula(d_peaks, d_profile);
}

QStringList MultiPeakFit::generateParameterList(int peaks)
{
	if (peaks == 1)
		return QStringList() << "A" << "xc" << "w" << "y0";

	QStringList lst;
	for (int i = 0; i<peaks; i++){
		QString index = QString::number(i+1);
		lst << "A" + index;
		lst << "xc" + index;
		lst << "w" + index;
	}
	lst << "y0";
	return lst;
}

QStringList MultiPeakFit::generateExplanationList(int peaks)
{
	if (peaks == 1)
		return QStringList() << tr("area") << tr("center") <<  tr("width") << tr("offset");

	QStringList lst;
	for (int i = 0; i<peaks; i++){
		QString index = QString::number(i+1);
		lst << tr("area") + " " + index;
		lst << tr("center") + " " + index;
		lst << tr("width") + " " + index;
	}
	lst << tr("offset");
	return lst;
}

QString MultiPeakFit::generateFormula(int peaks, PeakProfile profile)
{
	if (peaks == 1){
		switch (profile){
			case Gauss:
				return "y0+A*sqrt(2/PI)/w*exp(-2*((x-xc)/w)^2)";
				break;

			case Lorentz:
				return "y0+2*A/PI*w/(4*(x-xc)^2+w^2)";
				break;
		}
	}

	QString formula = "y0+";
	for (int i = 0; i<peaks; i++){
		formula += peakFormula(i+1, profile);
		if (i < peaks - 1)
			formula += "+";
	}
	return formula;
}

QString MultiPeakFit::peakFormula(int peakIndex, PeakProfile profile)
{
	QString formula;
	QString index = QString::number(peakIndex);
	switch (profile)
	{
		case Gauss:
			formula += "sqrt(2/PI)*A" + index + "/w" + index;
			formula += "*exp(-2*(x-xc" + index + ")^2/w" + index + "^2)";
			break;
		case Lorentz:
			formula += "2*A"+index+"/PI*w"+index+"/(4*(x-xc"+index+")^2+w"+index+"^2)";
			break;
	}
	return formula;
}

void MultiPeakFit::guessInitialValues()
{
	if (!d_n || d_peaks > 1)
		return;

	size_t imin, imax;
	gsl_stats_minmax_index(&imin, &imax, d_y, 1, d_n);

	double min_out = d_y[imin];
	double max_out = d_y[imax];

	QVarLengthArray<double> temp(d_n);//double temp[d_n];
	for (int i = 0; i < d_n; i++)
		temp[i] = fabs(d_y[i]);
	size_t imax_temp = gsl_stats_max_index(temp.data(), 1, d_n);//size_t imax_temp = gsl_stats_max_index(temp, 1, d_n);

	double offset, area;
	if (imax_temp == imax)
		offset = min_out;
	else //reversed bell
		offset = max_out;

	double xc = d_x[imax_temp];
	double width = 2*gsl_stats_sd(d_x, 1, d_n);

	if (d_profile == Lorentz)
		area = M_PI_2*width*fabs(max_out - min_out);
	else
		area = sqrt(M_PI_2)*width*fabs(max_out - min_out);

	gsl_vector_set(d_param_init, 0, area);
	gsl_vector_set(d_param_init, 1, xc);
	gsl_vector_set(d_param_init, 2, width);
	gsl_vector_set(d_param_init, 3, offset);
}

void MultiPeakFit::customizeFitResults()
{
	for (int j=0; j<d_peaks; j++){
	    d_results[3*j] = fabs(d_results[3*j]);
        d_results[3*j + 2] = fabs(d_results[3*j + 2]);
	}
}

void MultiPeakFit::insertPeakFunctionCurve(double *x, double *y, int peak)
{
	QStringList curves = d_output_graph->curvesList();
	int index = 0;
	for (int i = 0; i<(int)curves.count(); i++ ){
		if (curves[i].startsWith(tr("Peak")))
			index++;
	}
	QString title = tr("Peak") + QString::number(++index);

	FunctionCurve *c = new FunctionCurve(FunctionCurve::Normal, title);
	c->setPen(QPen(ColorBox::color(d_peaks_color), 1));
	c->setData(x, y, d_points);
	c->setRange(d_x[0], d_x[d_n-1]);

	QString formula = "y0+"+peakFormula(peak + 1, d_profile);
	QString parameter = QString::number(d_results[d_p-1], 'e', d_prec);
	formula.replace(d_param_names[d_p-1], parameter);
	for (int j=0; j<3; j++){
		int p = 3*peak + j;
		parameter = QString::number(d_results[p], 'e', d_prec);
		formula.replace(d_param_names[p], parameter);
	}
	c->setFormula(formula.replace("--", "+").replace("-+", "-").replace("+-", "-"));
	d_output_graph->insertPlotItem(c, Graph::Line);
	d_output_graph->addFitCurve(c);
}

void MultiPeakFit::generateFitCurve()
{
	ApplicationWindow *app = (ApplicationWindow *)parent();
	if (!d_gen_function)
		d_points = d_n;

	gsl_matrix * m = gsl_matrix_alloc (d_points, d_peaks);
	if (!m){
		QMessageBox::warning(app, tr("MantidPlot - Fit Error"), tr("Could not allocate enough memory for the fit curves!"));
		return;
	}

	QVarLengthArray<double> X(d_points), Y(d_points);//double X[d_points], Y[d_points];
	int i, j;
	int peaks_aux = d_peaks;
	if (d_peaks == 1)
		peaks_aux--;

	if (d_gen_function){
		double step = (d_x[d_n-1] - d_x[0])/(d_points-1);
		for (i = 0; i<d_points; i++){
		    double x = d_x[0] + i*step;
			X[i] = x;
			double yi = 0;
			for (j=0; j<d_peaks; j++){
                double y = evalPeak(d_results, x, j);
				gsl_matrix_set(m, i, j, y + d_results[d_p - 1]);
				yi += y;
			}
            Y[i] = yi + d_results[d_p - 1];//add offset
		}

        customizeFitResults();

		if (d_graphics_display){
			if (!d_output_graph)
				d_output_graph = createOutputGraph()->activeGraph();

			if (d_peaks > 1)
				insertFitFunctionCurve(QString(objectName()) + tr("Fit"), X.data(), Y.data(), 2);//insertFitFunctionCurve(QString(objectName()) + tr("Fit"), X, Y, 2);
			else
				insertFitFunctionCurve(QString(objectName()) + tr("Fit"), X.data(), Y.data());//insertFitFunctionCurve(QString(objectName()) + tr("Fit"), X, Y);

			if (generate_peak_curves){
				for (i=0; i<peaks_aux; i++){//add the peak curves
					for (j=0; j<d_points; j++)
						Y[j] = gsl_matrix_get (m, j, i);

				insertPeakFunctionCurve(X.data(), Y.data(), i);//insertPeakFunctionCurve(X, Y, i);
				}
			}
			d_output_graph->replot();
		}
	} else {
		QString tableName = app->generateUniqueName(tr("Fit"));
		QString dataSet;
		if (d_curve)
			dataSet = d_curve->title().text();
		else
			dataSet = d_y_col_name;
		QString label = d_explanation + " " + tr("fit of") + " " + dataSet;

		d_result_table = app->newHiddenTable(tableName, label, d_points, peaks_aux + 2);
		QStringList header = QStringList() << "1";
		for (i = 0; i<peaks_aux; i++)
			header << tr("peak") + QString::number(i+1);
		header << "2";
		d_result_table->setHeader(header);

        QLocale locale = app->locale();
		for (i = 0; i<d_points; i++){
			X[i] = d_x[i];
			d_result_table->setText(i, 0, locale.toString(X[i], 'e', d_prec));

			double yi=0;
			for (j=0; j<d_peaks; j++){
				double diff = X[i] - d_results[3*j + 1];
				double w = d_results[3*j + 2];
				double y_aux = 0;
				if (d_profile == Gauss)
					y_aux += sqrt(M_2_PI)*d_results[3*j]/w*exp(-2*diff*diff/(w*w));
				else
					y_aux += M_2_PI*d_results[3*j]*w/(4*diff*diff+w*w);

				yi += y_aux;
				y_aux += d_results[d_p - 1];
				d_result_table->setText(i, j+1, locale.toString(y_aux, 'e', d_prec));
				gsl_matrix_set(m, i, j, y_aux);
			}
			Y[i] = yi + d_results[d_p - 1];//add offset
			if (d_peaks > 1)
				d_result_table->setText(i, d_peaks+1, locale.toString(Y[i], 'e', d_prec));
		}

		customizeFitResults();

		if (d_graphics_display){
			if (!d_output_graph)
				d_output_graph = createOutputGraph()->activeGraph();

			label = tableName + "_2";
			DataCurve *c = new DataCurve(d_result_table, tableName + "_1", label);
			if (d_peaks > 1)
				c->setPen(QPen(ColorBox::color(d_curveColorIndex), 2));
			else
				c->setPen(QPen(ColorBox::color(d_curveColorIndex), 1));
			c->setData(X.data(), Y.data(), d_points);//c->setData(X, Y, d_points);
			d_output_graph->insertPlotItem(c, Graph::Line);
			d_output_graph->addFitCurve(c);

			if (generate_peak_curves){
				for (i=0; i<peaks_aux; i++){//add the peak curves
					for (j=0; j<d_points; j++)
						Y[j] = gsl_matrix_get (m, j, i);

					label = tableName + "_" + tr("peak") + QString::number(i+1);
					c = new DataCurve(d_result_table, tableName + "_1", label);
					c->setPen(QPen(ColorBox::color(d_peaks_color), 1));
					c->setData(X.data(), Y.data(), d_points);//c->setData(X, Y, d_points);
					d_output_graph->insertPlotItem(c, Graph::Line);
					d_output_graph->addFitCurve(c);
				}
			}
			d_output_graph->replot();
		}
	}
	gsl_matrix_free(m);
}

double MultiPeakFit::eval(double *par, double x)
{
	double y = 0;
	for (int i=0; i<d_peaks; i++)
		y += evalPeak(par, x, i);

	return y + par[d_p - 1];//add offset
}

double MultiPeakFit::evalPeak(double *par, double x, int peak)
{
	int aux = 3*peak;
    double diff = x - par[aux + 1];
    double w = par[aux + 2];
    if (d_profile == Gauss)
        return sqrt(M_2_PI)*par[aux]/w*exp(-2*diff*diff/(w*w));
    else
        return M_2_PI*par[aux]*w/(4*diff*diff+w*w);
}

QString MultiPeakFit::logFitInfo(int iterations, int status)
{
	QString info = Fit::logFitInfo(iterations, status);
	if (d_peaks == 1)
		return info;

    ApplicationWindow *app = (ApplicationWindow *)parent();
    QLocale locale = app->locale();

	info += tr("Peak") + "\t" + tr("Area") + "\t";
	info += tr("Center") + "\t" + tr("Width") + "\t" + tr("Height") + "\n";
	info += "---------------------------------------------------------------------------------------\n";
	for (int j=0; j<d_peaks; j++){
		info += QString::number(j+1) + "\t";
		info += locale.toString(d_results[3*j], 'e', d_prec) + "\t";
		info += locale.toString(d_results[3*j+1], 'e', d_prec) + "\t";
		info += locale.toString(d_results[3*j+2], 'e', d_prec) + "\t";

		if (d_profile == Lorentz)
			info += locale.toString(M_2_PI*d_results[3*j]/d_results[3*j+2], 'e', d_prec) + "\n";
		else
			info += locale.toString(sqrt(M_2_PI)*d_results[3*j]/d_results[3*j+2], 'e', d_prec) + "\n";
	}
	info += "---------------------------------------------------------------------------------------\n";
	return info;
}

/*****************************************************************************
 *
 * Class LorentzFit
 *
 *****************************************************************************/

LorentzFit::LorentzFit(ApplicationWindow *parent, Graph *g)
: MultiPeakFit(parent, g, MultiPeakFit::Lorentz, 1)
{
	init();
}

LorentzFit::LorentzFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle)
: MultiPeakFit(parent, g, MultiPeakFit::Lorentz, 1)
{
	init();
	setDataFromCurve(curveTitle);
}

LorentzFit::LorentzFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end)
: MultiPeakFit(parent, g, MultiPeakFit::Lorentz, 1)
{
	init();
	setDataFromCurve(curveTitle, start, end);
}

LorentzFit::LorentzFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow, int endRow)
: MultiPeakFit(parent, t, xCol, yCol, startRow, endRow, MultiPeakFit::Lorentz, 1)
{
	init();
	setDataFromTable(t, xCol, yCol, startRow, endRow);
}

void LorentzFit::init()
{
	setObjectName(tr("Lorentz"));
	d_explanation = tr("Lorentz Fit");
	d_param_explain << tr("area") << tr("center") << tr("width") << tr("offset");
}

/*****************************************************************************
 *
 * Class GaussFit
 *
 *****************************************************************************/

GaussFit::GaussFit(ApplicationWindow *parent, Graph *g)
: MultiPeakFit(parent, g, MultiPeakFit::Gauss, 1)
{
	init();
}

GaussFit::GaussFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle)
: MultiPeakFit(parent, g, MultiPeakFit::Gauss, 1)
{
	init();
	setDataFromCurve(curveTitle);
}

GaussFit::GaussFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end)
: MultiPeakFit(parent, g, MultiPeakFit::Gauss, 1)
{
	init();
	setDataFromCurve(curveTitle, start, end);
}

GaussFit::GaussFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow, int endRow)
: MultiPeakFit(parent, t, xCol, yCol, startRow, endRow, MultiPeakFit::Gauss, 1)
{
	init();
	setDataFromTable(t, xCol, yCol, startRow, endRow);
}

void GaussFit::init()
{
	setObjectName(tr("Gauss"));
	d_explanation = tr("Gauss Fit");
	d_param_explain << tr("area") << tr("center") << tr("width") << tr("offset");
}

/*****************************************************************************
 *
 * Class GaussAmpFit
 *
 *****************************************************************************/

GaussAmpFit::GaussAmpFit(ApplicationWindow *parent, Graph *g)
: Fit(parent, g)
{
	init();
}

GaussAmpFit::GaussAmpFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle)
: Fit(parent, g)
{
	init();
	setDataFromCurve(curveTitle);
}

GaussAmpFit::GaussAmpFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end)
: Fit(parent, g)
{
	init();
	setDataFromCurve(curveTitle, start, end);
}

GaussAmpFit::GaussAmpFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow, int endRow)
: Fit(parent, t)
{
	init();
	setDataFromTable(t, xCol, yCol, startRow, endRow);
}

void GaussAmpFit::init()
{
	setObjectName(tr("GaussAmp"));
	d_f = gauss_f;
	d_df = gauss_df;
	d_fdf = gauss_fdf;
	d_fsimplex = gauss_d;
	d_p = 4;
    initWorkspace(d_p);
	d_param_explain << tr("offset") << tr("amplitude") << tr("center") << tr("width");
	d_param_names << "y0" << "A" << "xc" << "w";
	d_explanation = tr("GaussAmp Fit");
	d_formula = "y0+A*exp(-(x-xc)^2/(2*w^2))";
}

void GaussAmpFit::calculateFitCurveData(double *X, double *Y)
{
	double y0 = d_results[0];
	double a = d_results[1];
	double xc = d_results[2];
	double w2 = d_results[3]*d_results[3];
	if (d_gen_function){
		double X0 = d_x[0];
		double step = (d_x[d_n-1] - X0)/(d_points - 1);
		for (int i=0; i<d_points; i++){
            double x = X0 + i*step;
			X[i] = x;
			double diff = x - xc;
			Y[i] = a*exp(-0.5*diff*diff/w2) + y0;
		}
	}else{
		for (int i=0; i<d_points; i++){
		    double x = d_x[i];
			X[i] = x;
			double diff = x - xc;
			Y[i] = a*exp(-0.5*diff*diff/w2) + y0;
		}
	}
}

double GaussAmpFit::eval(double *par, double x)
{
    double diff = x - par[2];
    return par[1]*exp(-0.5*diff*diff/(par[3]*par[3])) + par[0];
}

void GaussAmpFit::guessInitialValues()
{
	size_t imin, imax;
	gsl_stats_minmax_index(&imin, &imax, d_y, 1, d_n);

	double min_out = d_y[imin];
	double max_out = d_y[imax];

	gsl_vector_set(d_param_init, 1, fabs(max_out - min_out));

	QVarLengthArray<double> temp(d_n);//double temp[d_n];
	for (int i = 0; i < d_n; i++)
		temp[i] = fabs(d_y[i]);
	size_t imax_temp = gsl_stats_max_index(temp.data(), 1, d_n);//size_t imax_temp = gsl_stats_max_index(temp, 1, d_n);

	gsl_vector_set(d_param_init, 2, d_x[imax_temp]);
	gsl_vector_set(d_param_init, 3, gsl_stats_sd(d_x, 1, d_n));

	if (imax_temp == imax)
		gsl_vector_set(d_param_init, 0, min_out);
	else //reversed bell
		gsl_vector_set(d_param_init, 0, max_out);
}
