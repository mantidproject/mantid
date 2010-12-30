/***************************************************************************
    File                 : Correlation.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2007 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : Numerical correlation of data sets

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
#include "Correlation.h"
#include "MultiLayer.h"
#include "Plot.h"
#include "PlotCurve.h"
#include "ColorBox.h"
#include <QMessageBox>
#include <QLocale>

#include <gsl/gsl_fft_halfcomplex.h>

Correlation::Correlation(ApplicationWindow *parent, Table *t, const QString& colName1, const QString& colName2, int startRow, int endRow)
: Filter(parent, t)
{
	setObjectName(tr("Correlation"));
    setDataFromTable(t, colName1, colName2, startRow, endRow);
}

bool Correlation::setDataFromTable(Table *t, const QString& colName1, const QString& colName2, int startRow, int endRow)
{
    if (!t)
        return false;

    d_table = t;

    int col1 = d_table->colIndex(colName1);
	int col2 = d_table->colIndex(colName2);

	if (col1 < 0){
		QMessageBox::warning((ApplicationWindow *)parent(), tr("MantidPlot") + " - " + tr("Error"),
		tr("The data set %1 does not exist!").arg(colName1));
		d_init_err = true;
		return false;
	} else if (col2 < 0){
		QMessageBox::warning((ApplicationWindow *)parent(), tr("MantidPlot") + " - " + tr("Error"),
		tr("The data set %1 does not exist!").arg(colName2));
		d_init_err = true;
		return false;
	}

    if (d_n > 0){//delete previousely allocated memory
		delete[] d_x;
		delete[] d_y;
	}

    startRow--; endRow--;
	if (startRow < 0 || startRow >= t->numRows())
		startRow = 0;
	if (endRow < 0 || endRow >= t->numRows())
		endRow = t->numRows() - 1;

    int from = QMIN(startRow, endRow);
    int to = QMAX(startRow, endRow);

	int rows = abs(to - from) + 1;
	d_n = 16; // tmp number of points
	while (d_n < rows)
		d_n *= 2;

    d_x = new double[d_n];
	d_y = new double[d_n];

    if(d_y && d_x){
		memset( d_x, 0, d_n * sizeof( double ) ); // zero-pad the two arrays...
		memset( d_y, 0, d_n * sizeof( double ) );
		for(int i = 0; i< d_n; i++){
		    int j = i + from;
			d_x[i] = d_table->cell(j, col1);
			d_y[i] = d_table->cell(j, col2);
		}
	} else {
		QMessageBox::critical((ApplicationWindow *)parent(), tr("MantidPlot") + " - " + tr("Error"),
                        tr("Could not allocate memory, operation aborted!"));
        d_init_err = true;
		d_n = 0;
		return false;
	}
	return true;
}

void Correlation::output()
{
    // calculate the FFTs of the two functions
	if( gsl_fft_real_radix2_transform( d_x, 1, d_n ) == 0 &&
        gsl_fft_real_radix2_transform( d_y, 1, d_n ) == 0)
	{
		for(int i=0; i<d_n/2; i++ ){// multiply the FFT by its complex conjugate
			if( i==0 || i==(d_n/2)-1 )
				d_x[i] *= d_x[i];
			else{
				int ni = d_n-i;
				double dReal = d_x[i] * d_y[i] + d_x[ni] * d_y[ni];
				double dImag = d_x[i] * d_y[ni] - d_x[ni] * d_y[i];
				d_x[i] = dReal;
				d_x[ni] = dImag;
			}
		}
	} else {
		QMessageBox::warning((ApplicationWindow *)parent(), tr("MantidPlot") + " - " + tr("Error"),
                             tr("Error in GSL forward FFT operation!"));
		return;
	}

	gsl_fft_halfcomplex_radix2_inverse(d_x, 1, d_n );	//inverse FFT

	addResultCurve();
    d_result_table = d_table;
}

void Correlation::addResultCurve()
{
    ApplicationWindow *app = (ApplicationWindow *)parent();
    if (!app)
        return;

    QLocale locale = app->locale();

    if (d_n > d_table->numRows())
        d_table->setNumRows(d_n);

	int cols = d_table->numCols();
	int cols2 = cols+1;
	d_table->addCol();
	d_table->addCol();
	int n = d_n/2;

	QVarLengthArray<double> x_temp(d_n), y_temp(d_n);//double x_temp[d_n], y_temp[d_n];
	for (int i = 0; i<d_n; i++){
	    double x = i - n;
        x_temp[i] = x;

        double y;
        if(i < n)
			y = d_x[n + i];
		else
			y = d_x[i - n];
        y_temp[i] = y;

		d_table->setText(i, cols, QString::number(x));
		d_table->setText(i, cols2, locale.toString(y, 'g', app->d_decimal_digits));
	}

	QStringList l = d_table->colNames().grep(tr("Lag"));
	QString id = QString::number((int)l.size()+1);
	QString label = objectName() + id;

	d_table->setColName(cols, tr("Lag") + id);
	d_table->setColName(cols2, label);
	d_table->setColPlotDesignation(cols, Table::X);
	d_table->setHeaderColType();

	if (d_graphics_display){
		if (!d_output_graph)
			d_output_graph = createOutputGraph()->activeGraph();

    	DataCurve *c = new DataCurve(d_table, d_table->colName(cols), d_table->colName(cols2));
		c->setData(x_temp.data(), y_temp.data(), d_n);//c->setData(x_temp, y_temp, d_n);
    	c->setPen(QPen(ColorBox::color(d_curveColorIndex), 1));
		d_output_graph->insertPlotItem(c, Graph::Line);
		d_output_graph->updatePlot();
	}
}
