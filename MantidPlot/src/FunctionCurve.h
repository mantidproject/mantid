/***************************************************************************
    File                 : FunctionCurve.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu
 Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Function curve class

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
#ifndef FUNCTIONCURVE_H
#define FUNCTIONCURVE_H

#include "PlotCurve.h"

#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace API {
class IFunction;
class MatrixWorkspace;
}
}

// Function curve class
class FunctionCurve : public PlotCurve {
public:
  enum FunctionType { Normal = 0, Parametric = 1, Polar = 2 };

  FunctionCurve(const FunctionType &t, const QString &name = QString());
  explicit FunctionCurve(const QString &name = QString());
  FunctionCurve(const Mantid::API::IFunction *fun, const QString &wsName,
                int wsIndex = 0, const QString &name = QString());
  FunctionCurve(const FunctionCurve &c);
  ~FunctionCurve() override;

  FunctionCurve &operator=(const FunctionCurve &rhs) = delete;

  PlotCurve *clone(const Graph *) const override {
    return new FunctionCurve(*this);
  }

  double startRange() { return d_from; };
  double endRange() { return d_to; };
  void setRange(double from, double to);

  QStringList formulas() { return d_formulas; };
  void setFormulas(const QStringList &lst) { d_formulas = lst; };

  //! Provided for convenience when dealing with normal functions
  void setFormula(const QString &s) { d_formulas = {s}; };

  QString variable() { return d_variable; };
  void setVariable(const QString &s) { d_variable = s; };

  FunctionType functionType() { return d_function_type; };
  void setFunctionType(const FunctionType &t) { d_function_type = t; };

  void copy(FunctionCurve *f);

  //! Returns a string used when saving to a project file
  QString saveToString();

  //! Returns a string that can be displayed in a plot legend
  QString legend();

  void loadData(int points = 0);

  void loadMantidData(boost::shared_ptr<const Mantid::API::MatrixWorkspace> ws,
                      size_t wi, int peakRadius = 0);

  /// No error bars on this curve: Always return an empty list.
  QList<ErrorBarSettings *> errorBarSettingsList() const override {
    return QList<ErrorBarSettings *>();
  }

  /// returns identifier where this curve plots a IFunction
  const Mantid::API::IFunction *getIFunctionIdentifier() const {
    return m_identifier;
  };

private:
  FunctionType d_function_type;
  QString d_variable;
  QStringList d_formulas;
  double d_from, d_to;

  /// Used to identify which IFunction it is plotting
  /// Equal null where the curve is not plotting an IFunction
  const Mantid::API::IFunction *m_identifier;
};

#endif
