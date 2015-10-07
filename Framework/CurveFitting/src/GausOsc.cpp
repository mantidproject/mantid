//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/GausOsc.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>

namespace Mantid {
namespace CurveFitting {

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(GausOsc)

void GausOsc::init() {
  declareParameter("A", 10.0, "Amplitude at time 0");
  declareParameter("Sigma", 0.2, "Decay rate");
  declareParameter("Frequency", 0.1, "Frequency of oscillation");
  declareParameter("Phi", 0.0, "Frequency of oscillation");
}

void GausOsc::function1D(double *out, const double *xValues,
                         const size_t nData) const {
  const double A = getParameter("A");
  const double G = getParameter("Sigma");
  const double gf = getParameter("Frequency");
  const double gphi = getParameter("Phi");

  for (size_t i = 0; i < nData; i++) {
    double x = xValues[i];
    out[i] = A * exp(-G * G * x * x) * cos(2 * M_PI * gf * x + gphi);
  }
}

void GausOsc::functionDeriv1D(Jacobian *out, const double *xValues,
                              const size_t nData) {
  const double A = getParameter("A");
  const double G = getParameter("Sigma");
  const double gf = getParameter("Frequency");
  const double gphi = getParameter("Phi");

  for (size_t i = 0; i < nData; i++) {
    double x = xValues[i];
    double g = exp(-G * G * x * x);
    double c = cos(2 * M_PI * gf * x + gphi);
    double s = sin(2 * M_PI * gf * x + gphi);
    out->set(i, 0, g * c);
    out->set(i, 1, -2 * G * x * x * A * g * c);
    out->set(i, 2, -A * g * 2 * M_PI * x * s);
    out->set(i, 3, -A * g * s);
  }
}

void GausOsc::setActiveParameter(size_t i, double value) {
  size_t j = i;

  if (parameterName(j) == "Sigma")
    setParameter(j, fabs(value), false); // Make sigma positive
  else if (parameterName(j) == "Phi") {
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
