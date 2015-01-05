//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Abragam.h"
#include "MantidAPI//FunctionFactory.h"
#include <cmath>

namespace Mantid {
namespace CurveFitting {

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(Abragam)

void Abragam::init() {
  declareParameter("A", 0.2, "Amplitude");
  declareParameter("Omega", 0.5, "Angular Frequency of oscillation");
  declareParameter("Phi", 0, "Phase of oscillation at 0 (in Radians)");
  declareParameter("Sigma", 1, "Decay rate?");
  declareParameter("Tau", 1, "?");
}

void Abragam::function1D(double *out, const double *xValues,
                         const size_t nData) const {
  const double A = getParameter("A");
  const double w = getParameter("Omega");
  const double phi = getParameter("Phi");
  const double sig = getParameter("Sigma");
  const double t = getParameter("Tau");

  for (size_t i = 0; i < nData; i++) {
    double A1 = A * cos(w * xValues[i] + phi);
    double A2 =
        -(sig * sig * t * t) * (exp(-xValues[i] / t) - 1 + (xValues[i] / t));
    double A3 = exp(A2);

    out[i] = A1 * A3;
  }
}

void Abragam::functionDeriv(const API::FunctionDomain &domain,
                            API::Jacobian &jacobian) {
  calNumericalDeriv(domain, jacobian);
}

void Abragam::setActiveParameter(size_t i, double value) {
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
