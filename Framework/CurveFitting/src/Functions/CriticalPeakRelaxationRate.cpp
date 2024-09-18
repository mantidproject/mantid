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
  declareParameter("Background1", 0.0, "coefficient for non-critical background when x < Critical Temperature");
  declareParameter("Background2", 0.0, "coefficient for non-critical background when x > Critical Temperature");
  declareAttribute("Delta", Attribute(0.01));
}

void CriticalPeakRelaxationRate::function1D(double *out, const double *xValues, const size_t nData) const {

  const double Scale = getParameter("Scaling");
  const double Tc = getParameter("CriticalTemp");
  const double Exp = getParameter("Exponent");
  const double Bg1 = getParameter("Background1");
  const double Bg2 = getParameter("Background2");
  const double Delta = getAttribute("Delta").asDouble();

  for (size_t i = 0; i < nData; i++) {
    if (xValues[i] + Delta < Tc || xValues[i] - Delta > Tc) {
      auto denom = pow(fabs(xValues[i] - Tc), Exp);
      if (xValues[i] < Tc) {
        out[i] = Bg1 + Scale / denom;
      } else {
        out[i] = Bg2 + Scale / denom;
      }
    } else {
      out[i] = 1e6;
    }
  }
}

} // namespace Mantid::CurveFitting::Functions