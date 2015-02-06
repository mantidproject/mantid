//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/EndErfc.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>
#include <gsl/gsl_sf_erf.h>

namespace Mantid {
namespace CurveFitting {

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(EndErfc)

void EndErfc::init() {
  declareParameter(
      "A", 2000.0,
      "Half value at minus infinity minus half value at plus infinity");
  declareParameter("B", 50.0, "Mid x value");
  declareParameter("C", 6.0, "Width parameter");
  declareParameter("D", 0.0, "Minimum value - must not be negative");
}

void EndErfc::function1D(double *out, const double *xValues,
                         const size_t nData) const {
  const double gA = getParameter("A");
  const double gB = getParameter("B");
  const double gC = getParameter("C");
  const double gD = getParameter("D");

  for (size_t i = 0; i < nData; i++) {
    double x = xValues[i];
    out[i] = gA * gsl_sf_erfc((gB - x) / gC) + gD;
  }

  if (gA < 0) {
    for (size_t i = 0; i < nData; i++) {
      out[i] = -2 * gA;
    }
  }
}

void EndErfc::setActiveParameter(size_t i, double value) {
  size_t j = i;

  if (parameterName(j) == "D" && value < 0.0)
    setParameter(j, 0.0, false); // Don't let D become negative
  else
    setParameter(j, value, false);
}

} // namespace CurveFitting
} // namespace Mantid
