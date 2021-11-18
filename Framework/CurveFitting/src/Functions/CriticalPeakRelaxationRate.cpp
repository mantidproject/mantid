// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/CriticalPeakRelaxationRate.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/PhysicalConstants.h"

#include <cmath>

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

DECLARE_FUNCTION(CriticalPeakRelaxationRate)

void CriticalPeakRelaxationRate::init() {
  declareParameter("Scaling", 1.0, "coefficient for scaling");
  declareParameter("CriticalTemp", 0.01, "coefficient for critical temperature");
  declareParameter("Exponent", 1.0, "coefficient for critical exponent");
  declareParameter("Background", 0.0, "coefficient for non-critical background");
}

void CriticalPeakRelaxationRate::function1D(double *out, const double *xValues, const size_t nData) const {
  checkParams(xValues, nData);

  const double scale = getParameter("Scaling");
  const double tc = getParameter("CriticalTemp");
  const double exp = getParameter("Exponent");
  const double bg = getParameter("Background");

  for (size_t i = 0; i < nData; i++) {
    double expression = abs(xValues[i] - tc);
    out[i] = scale / pow(expression, exp) + bg;
  }
}

void CriticalPeakRelaxationRate::functionDeriv1D(Jacobian *out, const double *xValues, const size_t nData) {
  checkParams(xValues, nData);

  const double scale = getParameter("Scaling");
  const double tc = getParameter("CriticalTemp");
  const double exp = getParameter("Exponent");
  const double bg = getParameter("Background");

  for (size_t i = 0; i < nData; i++) {
    double expression = abs(tc- xValues[i]);

    const double diffScale = pow(expression, -exp);
    const double diffTc = scale * exp * (xValues[i] - tc) * pow(expression, (-exp-2));
    const double diffExp = scale * pow(expression, -exp) * log(expression);

    out->set(i, 0, diffScale);
    out->set(i, 1, diffTc);
    out->set(i, 2, diffExp);
    out->set(i, 3, 1);
  }
}

void CriticalPeakRelaxationRate::checkParams(const double *xValues, const size_t nData) const {
  const double tc = getParameter("CriticalTemp");
  for (size_t i = 0; i < nData; i++) {
    if (xValues[i] == tc) {
      throw std::invalid_argument("Use the exclude range option with x=" + std::to_string(xValues[i]) + " and y = inf");
    }
  }
  }
} // namespace Mantid::CurveFitting::Functions