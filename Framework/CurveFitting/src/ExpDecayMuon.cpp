//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/ExpDecayMuon.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>

namespace Mantid {
namespace CurveFitting {

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(ExpDecayMuon)

void ExpDecayMuon::init() {
  declareParameter("A", 0.2, "Amplitude at time 0");
  declareParameter("Lambda", 0.2, "Decay rate");
}

void ExpDecayMuon::function1D(double *out, const double *xValues,
                              const size_t nData) const {
  const double gA0 = getParameter("A");
  const double gs = getParameter("Lambda");

  for (size_t i = 0; i < nData; i++) {
    out[i] = gA0 * exp(-gs * (xValues[i]));
  }
}

void ExpDecayMuon::functionDeriv1D(Jacobian *out, const double *xValues,
                                   const size_t nData) {
  const double gA0 = getParameter("A");
  const double gs = getParameter("Lambda");

  for (size_t i = 0; i < nData; i++) {
    double x = xValues[i];
    double e = exp(-gs * x);
    out->set(i, 0, e);            // derivative w.r.t. A (gA0)
    out->set(i, 1, -gA0 * x * e); // derivative w.r.t  Lambda (gs)
  }
}

} // namespace CurveFitting
} // namespace Mantid
