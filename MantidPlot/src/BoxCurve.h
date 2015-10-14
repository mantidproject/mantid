/***************************************************************************
    File                 : BoxCurve.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Box curve

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
#ifndef BOXCURVE_H
#define BOXCURVE_H

#include "PlotCurve.h"
#include <qwt_plot.h>
#include <qwt_symbol.h>

//! Box curve
class BoxCurve: public DataCurve
{
public:
	enum BoxStyle{NoBox, Rect, Diamond, WindBox, Notch};
	enum Range{None, SD, SE, r25_75, r10_90, r5_95, r1_99, MinMax, UserDef};

	BoxCurve(Table *t, const QString& name, int startRow = 0, int endRow = -1);

	void copy(const BoxCurve *b);

	virtual QwtDoubleRect boundingRect() const;

	QwtSymbol::Style minStyle(){return min_style;};
	void setMinStyle(QwtSymbol::Style s){min_style = s;};

	QwtSymbol::Style maxStyle(){return max_style;};
	void setMaxStyle(QwtSymbol::Style s){max_style = s;};

	void setMeanStyle(QwtSymbol::Style s){mean_style = s;};
	QwtSymbol::Style meanStyle(){return mean_style;};

	void setP99Style(QwtSymbol::Style s){p99_style = s;};
	QwtSymbol::Style p99Style(){return p99_style;};

	void setP1Style(QwtSymbol::Style s){p1_style = s;};
	QwtSymbol::Style p1Style(){return p1_style;};

	int boxStyle(){return b_style;};
	void setBoxStyle(int style);

	int boxWidth(){return b_width;};
	void setBoxWidth(int width){b_width=width;};

	double boxRange(){return b_coeff;};
	int boxRangeType(){return b_range;};
	void setBoxRange(int type, double coeff);

	double whiskersRange(){return w_coeff;};
	int whiskersRangeType(){return w_range;};
	void setWhiskersRange(int type, double coeff);

    void loadData();

private:
	using DataCurve::draw; // Unhide base class method (avoids Intel compiler warning)
	void draw(QPainter *painter,const QwtScaleMap &xMap,
		const QwtScaleMap &yMap, int from, int to) const;
	void drawBox(QPainter *painter, const QwtScaleMap &xMap,
				const QwtScaleMap &yMap, double *dat, int size) const;
	using DataCurve::drawSymbols; // Unhide base class method (avoids Intel compiler warning)
	void drawSymbols(QPainter *painter, const QwtScaleMap &xMap,
				const QwtScaleMap &yMap, double *dat, int size) const;

	QwtSymbol::Style min_style, max_style, mean_style, p99_style, p1_style;
	double b_coeff, w_coeff;
	int b_style, b_width, b_range, w_range;
};


//! Single array data (extension to QwtData)
class QwtSingleArrayData: public QwtData
{
public:
    QwtSingleArrayData(const double x, QwtArray<double> y, size_t)
	{
		d_y = y;
		d_x = x;
	};

    virtual QwtData *copy() const{return new QwtSingleArrayData(d_x, d_y, size());};

    virtual size_t size() const{return d_y.size();};
    virtual double x(size_t) const{return d_x;};
    virtual double y(size_t i) const{return d_y[static_cast<int>(i)];};

private:
    QwtArray<double> d_y;
	double d_x;
};

#endif
