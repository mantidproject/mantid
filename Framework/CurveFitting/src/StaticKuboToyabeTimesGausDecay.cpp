#include "MantidCurveFitting/StaticKuboToyabeTimesGausDecay.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>

namespace Mantid {
namespace CurveFitting {
using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(StaticKuboToyabeTimesGausDecay)

void StaticKuboToyabeTimesGausDecay::init() {
  declareParameter("A", 1.0, "Amplitude at time 0");
  declareParameter("Delta", 0.2, "StaticKuboToyabe decay rate");
  declareParameter("Sigma", 0.2, "Gaus decay rate");
}

void StaticKuboToyabeTimesGausDecay::function1D(double *out,
                                                const double *xValues,
                                                const size_t nData) const {
  const double A = getParameter("A");
  const double D = getParameter("Delta");
  const double S = getParameter("Sigma");

  // Precalculate squares
  const double D2 = pow(D, 2);
  const double S2 = pow(S, 2);

  // Precalculate constants
  const double C1 = 2.0 / 3;
  const double C2 = 1.0 / 3;

  for (size_t i = 0; i < nData; i++) {
    double x2 = pow(xValues[i], 2);
    out[i] =
        A * (exp(-(x2 * D2) / 2) * (1 - x2 * D2) * C1 + C2) * exp(-S2 * x2);
  }
}
} // namespace CurveFitting
} // namespace Mantid
