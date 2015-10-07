//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/LogNormal.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>

namespace Mantid {
namespace CurveFitting {

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(LogNormal)

LogNormal::LogNormal() {
  declareParameter("Height", 1.0, "Overall scaling factor");
  declareParameter("Location", 1.0, "Natural logarithm of the geometric mean");
  declareParameter("Scale", 1.0,
                   "Natural logarithm of the geometric standard deviation");
}

/** \relates LogNormal
 * Implements the LogNormal function
 * @param out :: The result of evaluating the function
 * @param xValues :: function domain values
 * @param nData :: size of the function domain
 */
void LogNormal::function1D(double *out, const double *xValues,
                           const size_t nData) const {
  const double h = getParameter("Height");
  const double t = getParameter("Location");
  const double b = getParameter("Scale");

  for (size_t i = 0; i < nData; i++) {
    double x = xValues[i];
    if (x == 0.0) {
      out[i] = 0.0; // limit of the distribution as x approaches to zero
    } else {
      double c = (log(x) - t) / b;
      out[i] = h / x * exp(-c * c / 2);
    }
  }
}

/** \relates LogNormal
 * Calculates the derivatives of the LogNormal
 * @param out :: The resulting jacobian
 * @param xValues :: function domain values
 * @param nData :: size of the function domain
 */
void LogNormal::functionDeriv1D(API::Jacobian *out, const double *xValues,
                                const size_t nData) {
  const double h = getParameter("Height");
  const double t = getParameter("Location");
  const double b = getParameter("Scale");

  for (size_t i = 0; i < nData; i++) {
    double x = xValues[i];
    if (x == 0.0) {
      out->set(i, 0,
               0.0); // all partial derivatives approach to 0 as x goes to 0
      out->set(i, 1, 0.0);
      out->set(i, 2, 0.0);
    } else {
      double c = (log(x) - t) / b;
      double e = exp(-c * c / 2) / x;
      out->set(i, 0, e); // partial derivative with respect to Height
      out->set(
          i, 1,
          h * e *
              (c / b)); // partial derivative with respect to Location parameter
      out->set(i, 2,
               h * e *
                   (c * c /
                    b)); // partial derivative with respect to Scale parameter
    }
  }
}

} // namespace CurveFitting
} // namespace Mantid
