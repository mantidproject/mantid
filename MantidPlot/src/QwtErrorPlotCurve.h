/***************************************************************************
    File                 : QwtErrorPlotCurve.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu
 Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Error bars curve

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
#ifndef ERRORBARS_H
#define ERRORBARS_H

#include "Mantid/ErrorBarSettings.h"
#include "PlotCurve.h"
#include <qwt_plot.h>

//! Error bars curve
class QwtErrorPlotCurve : public DataCurve, public ErrorBarSettings {
public:
  enum Orientation { Horizontal = 0, Vertical = 1 };

  QwtErrorPlotCurve(int orientation, Table *t, const QString &name);
  QwtErrorPlotCurve(Table *t, const QString &name);

  void copy(const QwtErrorPlotCurve *e);

  QwtDoubleRect boundingRect() const override;

  double errorValue(int i);
  QwtArray<double> errors() { return err; };
  void setErrors(const QwtArray<double> &data) { err = data; };

  double width() const override { return pen().widthF(); };
  void setWidth(double w) override;

  QColor color() const override { return pen().color(); };
  void setColor(const QColor &c) override;

  int direction() { return type; };
  void setDirection(int o) { type = o; };

  bool xErrors();
  void setXErrors(bool yes);

  //! Returns the master curve to which this error bars curve is attached.
  DataCurve *masterCurve() { return d_master_curve; };
  void setMasterCurve(DataCurve *c);

  //! Causes the master curve to delete this curve from its managed error bars
  // list.
  void detachFromMasterCurve() { d_master_curve->removeErrorBars(this); };

  QString plotAssociation() const override;

  bool updateData(Table *t, const QString &colName) override;
  void loadData() override;

private:
  using DataCurve::draw; // Unhide base class method (avoids Intel compiler
                         // warning)
  void draw(QPainter *painter, const QwtScaleMap &xMap, const QwtScaleMap &yMap,
            int from, int to) const override;

  void drawErrorBars(QPainter *painter, const QwtScaleMap &xMap,
                     const QwtScaleMap &yMap, int from, int to) const;

  //! Stores the error bar values
  QwtArray<double> err;

  //! Orientation of the bars: Horizontal or Vertical
  int type;

  //! Reference to the master curve to which this error bars curve is attached.
  DataCurve *d_master_curve;
};

#endif
