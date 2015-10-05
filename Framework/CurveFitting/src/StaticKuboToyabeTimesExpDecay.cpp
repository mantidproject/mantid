#include "MantidCurveFitting/StaticKuboToyabeTimesExpDecay.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>

namespace Mantid {
namespace CurveFitting {

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(StaticKuboToyabeTimesExpDecay)

void StaticKuboToyabeTimesExpDecay::init() {
  declareParameter("A", 0.2, "Amplitude at time 0");
  declareParameter("Delta", 0.2, "StaticKuboToyabe decay rate");
  declareParameter("Lambda", 0.2, "Exponential decay rate");
}

void StaticKuboToyabeTimesExpDecay::function1D(double *out,
                                               const double *xValues,
                                               const size_t nData) const {
  const double A = getParameter("A");
  const double D = getParameter("Delta");
  const double L = getParameter("Lambda");

  const double C1 = 2.0 / 3;
  const double C2 = 1.0 / 3;

  for (size_t i = 0; i < nData; i++) {
    double x = xValues[i];
    double DXSquared = pow(D * x, 2);
    out[i] =
        A * (exp(-DXSquared / 2) * (1 - DXSquared) * C1 + C2) * exp(-L * x);
  }
}

} // namespace CurveFitting
} // namespace Mantid
