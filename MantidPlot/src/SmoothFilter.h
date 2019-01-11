// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : SmoothFilter.h
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef SMOOTHFILTER_H
#define SMOOTHFILTER_H

#include "Filter.h"

class SmoothFilter : public Filter {
  Q_OBJECT

public:
  SmoothFilter(ApplicationWindow *parent, Graph *g, const QString &curveTitle,
               int m = 3);
  SmoothFilter(ApplicationWindow *parent, Graph *g, const QString &curveTitle,
               double start, double end, int m = 3);
  SmoothFilter(ApplicationWindow *parent, Table *t, const QString &xCol,
               const QString &yCol, int start = 0, int end = -1, int m = 3);

  enum SmoothMethod { SavitzkyGolay = 1, FFT = 2, Average = 3 };

  int method() { return (int)d_method; };
  void setMethod(int m);

  void setSmoothPoints(int points, int left_points = 0);
  //! Sets the polynomial order in the Savitky-Golay algorithm.
  void setPolynomOrder(int order);

private:
  void init(int m);
  void calculateOutputData(double *x, double *y) override;
  void smoothFFT(double *x, double *y);
  void smoothAverage(double *x, double *y);
  void smoothSavGol(double *x, double *y);

  //! The smooth method.
  SmoothMethod d_method;

  //! The number of adjacents points used to smooth the data set.
  int d_smooth_points;

  //! The number of left adjacents points used by the Savitky-Golay algorithm.
  int d_sav_gol_points;

  //! Polynomial order in the Savitky-Golay algorithm (see Numerical Receipes in
  // C for details).
  int d_polynom_order;
};

#endif
