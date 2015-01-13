//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/StaticKuboToyabe.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>

namespace Mantid {
namespace CurveFitting {

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(StaticKuboToyabe)

void StaticKuboToyabe::init() {
  declareParameter("A", 0.2, "Amplitude at time 0");
  declareParameter("Delta", 0.2, "Decay rate");
}

void StaticKuboToyabe::function1D(double *out, const double *xValues,
                                  const size_t nData) const {
  const double A = getParameter("A");
  const double G = getParameter("Delta");

  for (size_t i = 0; i < nData; i++) {
    out[i] = A * (exp(-pow(G * xValues[i], 2) / 2) *
                      (1 - pow(G * xValues[i], 2)) * 2.0 / 3 +
                  1.0 / 3);
  }
}

} // namespace CurveFitting
} // namespace Mantid
