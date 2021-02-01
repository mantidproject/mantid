// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Functions/StaticKuboToyabeTimesStretchExp.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

DECLARE_FUNCTION(StaticKuboToyabeTimesStretchExp)

void StaticKuboToyabeTimesStretchExp::init() {
  declareParameter("A", 0.2, "Amplitude at time 0");
  declareParameter("Delta", 0.2, "StaticKuboToyabe decay rate");
  declareParameter("Lambda", 0.2, "Exponential decay rate");
  declareParameter("Beta", 0.2, "Stretching Exponent");
}

void StaticKuboToyabeTimesStretchExp::function1D(double *out, const double *xValues, const size_t nData) const {
  const double A = getParameter("A");
  const double D = getParameter("Delta");
  const double L = getParameter("Lambda");
  const double B = getParameter("Beta");

  const double C1 = 2.0 / 3;
  const double C2 = 1.0 / 3;

  for (size_t i = 0; i < nData; i++) {
    double x = xValues[i];
    double DXSquared = pow(D * x, 2);
    double stretchExp = exp(-pow(L * x, B));
    out[i] = A * (exp(-DXSquared / 2) * (1 - DXSquared) * C1 + C2) * stretchExp;
  }
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
