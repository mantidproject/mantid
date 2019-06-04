/***************************************************************************
    File                 : FunctionCurve.cpp
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
#include "FunctionCurve.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MyParser.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"

FunctionCurve::FunctionCurve(const QString &name)
    : PlotCurve(name), d_function_type(Normal), d_variable("x"), d_formulas(),
      d_from(0.0), d_to(0.0), m_identifier(nullptr) {
  setType(GraphOptions::Function);
}

FunctionCurve::FunctionCurve(const FunctionType &t, const QString &name)
    : PlotCurve(name), d_function_type(t), d_variable("x"), d_formulas(),
      d_from(0.0), d_to(0.0), m_identifier(nullptr) {
  // d_variable = "x";
  setType(GraphOptions::Function);
}

/**
 * This constructor creates a function curve from a Mantid IFunction and uses a
 * workspace for x values
 * @param fun :: A pointer to a Mantid function
 * @param wsName :: A name of a workspace to provide x values and to be passed
 * to the function
 * @param wsIndex :: An index in the workspace
 * @param name :: A name of the curve
 */
FunctionCurve::FunctionCurve(const Mantid::API::IFunction *fun,
                             const QString &wsName, int wsIndex,
                             const QString &name)
    : PlotCurve(name), d_function_type(FunctionCurve::Normal),
      d_variable(""), // This indicates that mu::Parser is not used
      d_from(0), d_to(0), m_identifier(fun) {
  setType(GraphOptions::Function);

  // Save construction information in d_formulas
  d_formulas << "Mantid" << QString::fromStdString(fun->asString()) << wsName
             << QString::number(wsIndex);
}

FunctionCurve::FunctionCurve(const FunctionCurve &c)
    : PlotCurve(c.title().text()), d_function_type(c.d_function_type),
      d_variable(c.d_variable), d_formulas(c.d_formulas), d_from(c.d_from),
      d_to(c.d_to), m_identifier(c.m_identifier) {}

FunctionCurve::~FunctionCurve() {}

void FunctionCurve::setRange(double from, double to) {
  d_from = from;
  d_to = to;
}

void FunctionCurve::copy(FunctionCurve *f) {
  d_function_type = f->functionType();
  d_variable = f->variable();
  d_formulas = f->formulas();
  d_from = f->startRange();
  d_to = f->endRange();
}

QString FunctionCurve::saveToString() {
  QString s = "<Function>\n";
  s += "<Type>" + QString::number(d_function_type) + "</Type>\n";
  s += "<Title>" + title().text() + "</Title>\n";
  s += "<Expression>" + d_formulas.join("\t") + "</Expression>\n";
  s += "<Variable>" + d_variable + "</Variable>\n";
  s += "<Range>" + QString::number(d_from, 'g', 15) + "\t" +
       QString::number(d_to, 'g', 15) + "</Range>\n";
  s += "<Points>" + QString::number(dataSize()) + "</Points>\n";
  s += saveCurveLayout();
  s += "</Function>\n";
  return s;
}

QString FunctionCurve::legend() {
  QString label = title().text() + ": ";
  if (d_function_type == Normal)
    label += d_formulas[0];
  else if (d_function_type == Parametric) {
    label += "X(" + d_variable + ")=" + d_formulas[0];
    label += ", Y(" + d_variable + ")=" + d_formulas[1];
  } else if (d_function_type == Polar) {
    label += "R(" + d_variable + ")=" + d_formulas[0];
    label += ", Theta(" + d_variable + ")=" + d_formulas[1];
  }
  return label;
}

void FunctionCurve::loadData(int points) {
  if (d_variable.isEmpty() && !d_formulas.isEmpty() &&
      d_formulas[0].compare("Mantid") ==
          0) { // Mantid::API::IFunction is used to calculate the data points
    if (d_formulas.size() < 4)
      return;

    QString fnInput = d_formulas[1];
    QString wsName = d_formulas[2];
    int wsIndex = d_formulas[3].toInt();

    try {
      Mantid::API::MatrixWorkspace_const_sptr ws =
          boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
              Mantid::API::AnalysisDataService::Instance().retrieve(
                  wsName.toStdString()));

      if (wsIndex >= static_cast<int>(ws->getNumberHistograms()))
        return;

      const auto &wsXPoints = ws->points(wsIndex);

      if (d_from < wsXPoints.front())
        d_from = wsXPoints.front();
      if (d_to > wsXPoints.back())
        d_to = wsXPoints.back();

      const auto numBins = wsXPoints.size();

      std::vector<double> X;
      X.reserve(numBins);

      for (size_t i = 0; i < numBins; i++) {
        const double x = wsXPoints[i];
        if (x < d_from)
          continue;
        if (x > d_to)
          break;
        X.push_back(x);
      }

      // Create the function and initialize it using fnInput which was saved in
      // d_formulas[1]
      auto f = Mantid::API::FunctionFactory::Instance().createInitialized(
          fnInput.toStdString());
      f->setMatrixWorkspace(ws, wsIndex, d_from, d_to);
      f->applyTies();
      Mantid::API::FunctionDomain1DVector domain(X);
      Mantid::API::FunctionValues Y(domain);
      f->function(domain, Y);

      setData(&X[0], Y.getPointerToCalculated(0), static_cast<int>(X.size()));
    } catch (...) {
      return;
    }
  }      // Mantid
  else { // mu::Parser is used to calculate the data points
    if (!points)
      points = dataSize();

    QVarLengthArray<double> X(points), Y(points); // double X[points],
                                                  // Y[points];
    double step = (d_to - d_from) / (double)(points - 1);
    bool error = false;

    if (d_function_type == Normal) {
      MyParser parser;
      double x;
      try {
        parser.DefineVar(d_variable.toAscii().constData(), &x);
        parser.SetExpr(d_formulas[0].toAscii().constData());

        X[0] = d_from;
        x = d_from;
        Y[0] = parser.Eval();
        for (int i = 1; i < points; i++) {
          x += step;
          X[i] = x;
          Y[i] = parser.Eval();
        }
      } catch (mu::ParserError &) {
        error = true;
      }
    } else if (d_function_type == Parametric || d_function_type == Polar) {
      QStringList aux = d_formulas;
      MyParser xparser;
      MyParser yparser;
      double par;
      if (d_function_type == Polar) {
        QString swap = aux[0];
        aux[0] = "(" + swap + ")*cos(" + aux[1] + ")";
        aux[1] = "(" + swap + ")*sin(" + aux[1] + ")";
      }

      try {
        xparser.DefineVar(d_variable.toAscii().constData(), &par);
        yparser.DefineVar(d_variable.toAscii().constData(), &par);
        xparser.SetExpr(aux[0].toAscii().constData());
        yparser.SetExpr(aux[1].toAscii().constData());
        par = d_from;
        for (int i = 0; i < points; i++) {
          X[i] = xparser.Eval();
          Y[i] = yparser.Eval();

          par += step;
        }
      } catch (mu::ParserError &) {
        error = true;
      }
    }

    if (error)
      return;

    setData(X.data(), Y.data(), points);
  }
}

/**
 * Load the data from a MatrixWorkspace if it is a Mantid-type FunctionCurve.
 * @param ws :: A workspace to load the data from.
 * @param wi :: An index of a histogram with the data.
 * @param peakRadius :: A peak radius to pass to the domain.
 */
void FunctionCurve::loadMantidData(Mantid::API::MatrixWorkspace_const_sptr ws,
                                   size_t wi, int peakRadius) {
  if (!d_variable.isEmpty() || d_formulas.isEmpty() ||
      d_formulas[0] != "Mantid")
    return;
  if (d_formulas.size() < 2)
    return;

  QString fnInput = d_formulas[1];

  try {

    if (wi >= ws->getNumberHistograms())
      return;

    const auto &wsXPoints = ws->points(wi);

    if (d_from < wsXPoints.front())
      d_from = wsXPoints.front();
    if (d_to > wsXPoints.back())
      d_to = wsXPoints.back();

    const size_t numBins = wsXPoints.size();

    std::vector<double> X;
    X.reserve(numBins);

    for (size_t i = 0; i < numBins; i++) {
      const double x = wsXPoints[i];
      if (x < d_from)
        continue;
      if (x > d_to)
        break;
      X.push_back(x);
    }

    // Create the function and initialize it using fnInput which was saved in
    // d_formulas[1]
    auto f = Mantid::API::FunctionFactory::Instance().createInitialized(
        fnInput.toStdString());
    f->setMatrixWorkspace(ws, wi, d_from, d_to);
    f->applyTies();
    Mantid::API::FunctionDomain1DVector domain(X);
    Mantid::API::FunctionValues Y(domain);
    if (peakRadius > 0) {
      domain.setPeakRadius(peakRadius);
    }
    f->function(domain, Y);

    setData(&X[0], Y.getPointerToCalculated(0), static_cast<int>(X.size()));
  } catch (...) {
    return;
  }
}
