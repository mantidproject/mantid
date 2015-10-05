//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/ExpDecay.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>

namespace Mantid {
namespace CurveFitting {

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(ExpDecay)

ExpDecay::ExpDecay() {
  declareParameter("Height", 1.0, "Height at time 0");
  declareParameter("Lifetime", 1.0, "Lifetime of the process");
}

void ExpDecay::function1D(double *out, const double *xValues,
                          const size_t nData) const {
  const double h = getParameter("Height");
  const double t = getParameter("Lifetime");

  for (size_t i = 0; i < nData; i++) {
    out[i] = h * exp(-(xValues[i]) / t);
  }
}

void ExpDecay::functionDeriv1D(Jacobian *out, const double *xValues,
                               const size_t nData) {
  const double h = getParameter("Height");
  const double t = getParameter("Lifetime");

  for (size_t i = 0; i < nData; i++) {
    double x = xValues[i];
    double e = exp(-x / t);
    out->set(i, 0, e);
    out->set(i, 1, h * e * x / t / t);
  }
}

} // namespace CurveFitting
} // namespace Mantid
