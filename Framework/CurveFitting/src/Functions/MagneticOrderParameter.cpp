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
    double formula = Amp * pow((1 - pow(xValues[i] / Tc, Alpha)), Beta);
    // check whether the formula has returned an nan and if so return 0 instead
    if (!std::isfinite(formula)) {
      formula = 0;
    }
    out[i] = formula;
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
    // if diffAmp is NaN or inf we should use 0 instead.
    if (!std::isfinite(diffAmp)) {
      diffAmp = 0;
    }

    double diffAmpMin = pow((1 - xCalcAlpha), Beta - 1);
    // if diffAmpMin is NaN or inf we should use 0 instead.
    if (!std::isfinite(diffAmpMin)) {
      diffAmpMin = 0;
    }

    double diffAlpha = -Amp * Beta * xCalcAlpha * log(xCalc) * diffAmpMin;
    double diffBeta = Amp * diffAmp * log(1 - xCalcAlpha);
    // we can only take a log of a positive number. If thats not possible the diffBeta should be 0.
    if ((1 - xCalcAlpha) <= 0.0) {
      diffBeta = 0;
    }
    double diffTc = (Amp * Alpha * Beta * xCalcAlpha * diffAmpMin) / Tc;

    out->set(i, 0, diffAmp);
    out->set(i, 1, diffAlpha);
    out->set(i, 2, diffBeta);
    out->set(i, 3, diffTc);
  }
}

} // namespace Mantid::CurveFitting::Functions
