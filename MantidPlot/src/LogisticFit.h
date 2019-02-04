// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : LogisticFit.h
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef LOGISTICFIT_H
#define LOGISTICFIT_H

#include "Fit.h"

class LogisticFit : public Fit {
  Q_OBJECT

public:
  LogisticFit(ApplicationWindow *parent, Graph *g);
  LogisticFit(ApplicationWindow *parent, Graph *g, const QString &curveTitle);
  LogisticFit(ApplicationWindow *parent, Graph *g, const QString &curveTitle,
              double start, double end);
  LogisticFit(ApplicationWindow *parent, Table *t, const QString &xCol,
              const QString &yCol, int startRow = 1, int endRow = -1);

  void guessInitialValues() override;
  double eval(double *par, double x) override {
    return (par[0] - par[1]) / (1 + pow(x / par[2], par[3])) + par[1];
  };

private:
  void init();
  void calculateFitCurveData(double *X, double *Y) override;
};

#endif
