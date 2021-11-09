// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/MagneticOrderParameter.h"
#include "MantidAPI/FunctionFactory.h"

#include <cmath>

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

DECLARE_FUNCTION(MagneticOrderParameter)

void MagneticOrderParameter::init() {
  declareParameter("A0", 1.0, "amplitude");
  declareParameter("Alpha", 2.0, "balance parameter");
  declareParameter("Beta", 0.5, "critical exponent");
  declareParameter("CriticalTemp", 1.0, "critical temperature");
}

void MagneticOrderParameter::function1D(double *out, const double *xValues, const size_t nData) const {
  const double Amp = getParameter("A0");
  const double Alpha = getParameter("Alpha");
  const double Beta = getParameter("Beta");
  const double Tc = getParameter("CriticalTemp");

  for (size_t i = 0; i < nData; i++) {
    out[i] = Amp * pow((1 - pow(xValues[i] / Tc, Alpha)), Beta);
  }
}

void MagneticOrderParameter::functionDeriv1D(Jacobian *out, const double *xValues, const size_t nData) {
  const double Amp = getParameter("A0");
  const double Alpha = getParameter("Alpha");
  const double Beta = getParameter("Beta");
  const double Tc = getParameter("CriticalTemp");

  for (size_t i = 0; i < nData; i++) {
    double xCalc = (xValues[i] / Tc);
    double xCalcAlpha = pow(xCalc, Alpha);

    double diffAmp = pow((1 - xCalcAlpha), Beta);
    double diffAmpMin = pow((1 - xCalcAlpha), Beta - 1);
    double diffAlpha = -Amp * Beta * xCalcAlpha * log(xCalc) * diffAmpMin;
    double diffBeta = Amp * diffAmp * log(1 - xCalcAlpha);
    double diffTc = (Amp * Alpha * Beta * xCalcAlpha * diffAmpMin) / Tc;

    out->set(i, 0, diffAmp);
    out->set(i, 1, diffAlpha);
    out->set(i, 2, diffBeta);
    out->set(i, 3, diffTc);
  }
}

} // namespace Mantid::CurveFitting::Functions