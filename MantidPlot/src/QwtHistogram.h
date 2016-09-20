/***************************************************************************
    File                 : QwtHistogram.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu
 Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Histogram class

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
#include "QwtBarCurve.h"

class Matrix;

//! Histogram class
class QwtHistogram : public QwtBarCurve {
public:
  QwtHistogram(Table *t, const QString &xColName, const QString &name,
               int startRow, int endRow);
  explicit QwtHistogram(Matrix *m);

  void copy(const QwtHistogram *h);

  QwtDoubleRect boundingRect() const override;

  void setBinning(bool autoBin, double size, double begin, double end);
  bool autoBinning() { return d_autoBin; };
  double begin() { return d_begin; };
  double end() { return d_end; };
  double binSize() { return d_bin_size; };

  void loadData() override;
  void initData(double *Y, int size);

  double mean() { return d_mean; };
  double standardDeviation() { return d_standard_deviation; };
  double minimum() { return d_min; };
  double maximum() { return d_max; };

  Matrix *matrix() { return d_matrix; };

private:
  void draw(QPainter *painter, const QwtScaleMap &xMap, const QwtScaleMap &yMap,
            int from, int to) const override;
  // Implement overloaded virtual method to just pass up to the base class to
  // avoid
  // an Intel compiler warning
  void draw(QPainter *p, const QwtScaleMap &xMap, const QwtScaleMap &yMap,
            const QRect &rect) const override {
    DataCurve::draw(p, xMap, yMap, rect);
  }

  void loadDataFromMatrix();
  Matrix *d_matrix;

  bool d_autoBin;
  double d_bin_size, d_begin, d_end;

  //! Variables storing statistical information
  double d_mean, d_standard_deviation, d_min, d_max;
};
