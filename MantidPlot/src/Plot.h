/***************************************************************************
    File                 : Plot.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Plot window class

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
#ifndef PLOT_H
#define PLOT_H

#include <QObject>
#include <QMap>
#include <QLocale>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>

class Grid;

/// Helper class to delay deletion of curves
struct Detacher : public QObject
{
  Detacher(QwtPlotItem* plotItem);
  ~Detacher();
private:
  Detacher();
  QwtPlotItem *m_plotItem;
};

//! Plot window class
class Plot: public QwtPlot
{
    Q_OBJECT

public:
	Plot(int width = 500, int height = 400, QWidget *parent = 0, const char *name = 0);

	Grid *grid(){return static_cast<Grid *>(d_grid);};
	QList<int> curveKeys(){return d_curves.keys();};
	QList<QwtPlotItem *> curvesList(){return d_curves.values();};

	int insertCurve(QwtPlotItem *c);
	void removeCurve(int index);

	int closestCurve(int xpos, int ypos, int &dist, int &point);
	QwtPlotCurve* curve(int index);
	QwtPlotItem* plotItem(int index){return d_curves.value(index);};
	QMap<int, QwtPlotItem*> curves(){return d_curves;};

	QwtPlotMarker* marker(int index){return d_markers.value(index);};
	QList<int> markerKeys(){return d_markers.keys();};
	int insertMarker(QwtPlotMarker *m);
	void removeMarker(int index);

	QList<int> getMajorTicksType();
	void setMajorTicksType(int axis, int type);

	QList<int> getMinorTicksType();
	void setMinorTicksType(int axis, int type);

	int minorTickLength() const;
	int majorTickLength() const;
	void setTickLength (int minLength, int majLength);

	int axesLinewidth() const;
	void setAxesLinewidth(int width);

    void axisLabelFormat(int axis, char &f, int &prec) const;

    int axisLabelFormat(int axis);
    int axisLabelPrecision(int axis);

    QColor frameColor();
    const QColor & paletteBackgroundColor() const;

    using QwtPlot::print; // Avoid Intel compiler warning
    void print(QPainter *, const QRect &rect, const QwtPlotPrintFilter & = QwtPlotPrintFilter()) const;
    void updateLayout();

    void updateCurveLabels();
    // pass through method that is public on the base class in later qwt versions
    void updateAxes() { QwtPlot::updateAxes(); }

    void reverseCurveOrder(); // Created in connection with waterfall plots. Called from Graph method of same name.

signals:
  void dragMousePress(QPoint);
  void dragMouseRelease(QPoint);
  void dragMouseMove(QPoint);

protected:
    void showEvent (QShowEvent * event);
    void printFrame(QPainter *painter, const QRect &rect) const;
    // 'Dummy' QRect argument inserted into printCanvas method to avoid Intel
    // compiler warning (about printCanvas signature not matching that in base class)
    void printCanvas(QPainter *painter, const QRect&, const QRect &canvasRect,
   			 const QwtScaleMap map[axisCnt], const QwtPlotPrintFilter &pfilter) const;
	virtual void drawItems (QPainter *painter, const QRect &rect,
			const QwtScaleMap map[axisCnt], const QwtPlotPrintFilter &pfilter) const;

	void drawInwardTicks(QPainter *painter, const QRect &rect,
							const QwtScaleMap&map, int axis, bool min, bool maj) const;
    void drawBreak(QPainter *painter, const QRect &rect, const QwtScaleMap &map, int axis) const;

  bool eventFilter(QObject *obj, QEvent *ev);

	Grid *d_grid;
	QMap<int, QwtPlotItem*> d_curves;
	QMap<int, QwtPlotMarker*> d_markers;

	int minTickLength, majTickLength;
	int marker_key;
	int curve_key;

};
#endif
