//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/ExpDecayOsc.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>

namespace Mantid {
namespace CurveFitting {

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(ExpDecayOsc)

void ExpDecayOsc::init() {
  declareParameter("A", 0.2, "Amplitude at time 0");
  declareParameter("Lambda", 0.2, "Decay rate");
  declareParameter("Frequency", 0.1, "Frequency of oscillation");
  declareParameter("Phi", 0.0, "Phase of oscillation at 0 (in Radians)");
}

void ExpDecayOsc::function1D(double *out, const double *xValues,
                             const size_t nData) const {
  const double gA0 = getParameter("A");
  const double gs = getParameter("Lambda");
  const double gf = getParameter("Frequency");
  const double gphi = getParameter("Phi");

  for (size_t i = 0; i < nData; i++) {
    double x = xValues[i];
    out[i] = gA0 * exp(-gs * x) * cos(2 * M_PI * gf * x + gphi);
  }
}

void ExpDecayOsc::functionDeriv1D(Jacobian *out, const double *xValues,
                                  const size_t nData) {
  const double gA0 = getParameter("A");
  const double gs = getParameter("Lambda");
  const double gf = getParameter("Frequency");
  const double gphi = getParameter("Phi");

  for (size_t i = 0; i < nData; i++) {
    double x = xValues[i];
    double e = exp(-gs * x);
    double c = cos(2 * M_PI * gf * x + gphi);
    double s = sin(2 * M_PI * gf * x + gphi);
    out->set(i, 0, e * c);            // derivative w.r.t. A (gA0)
    out->set(i, 1, -gA0 * x * e * c); // derivative w.r.t  Lambda (gs)
    out->set(i, 2,
             -gA0 * e * 2 * M_PI * x * s); // derivate w.r.t. Frequency (gf)
    out->set(i, 3, -gA0 * e * s);          // detivative w.r.t Phi (gphi)
  }
}

void ExpDecayOsc::setActiveParameter(size_t i, double value) {
  size_t j = i;

  if (parameterName(j) == "Phi") {
    // Put angle in range of (-180 to 180] degrees
    double a = fmod(value, 2 * M_PI);
    if (a <= -M_PI)
      a += 2 * M_PI;
    if (a > M_PI)
      a -= 2 * M_PI;
    setParameter(j, a, false);
  } else
    setParameter(j, value, false);
}

} // namespace CurveFitting
} // namespace Mantid
