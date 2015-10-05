//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Quadratic.h"
#include "MantidAPI/FunctionFactory.h"

namespace Mantid {
namespace CurveFitting {

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(Quadratic)

/** \relates Quadratic
 *Initialise function to define parameters
 */
void Quadratic::init() {
  declareParameter("A0", 0.0, "coefficient for constant term");
  declareParameter("A1", 0.0, "coefficient for linear term");
  declareParameter("A2", 0.0, "coefficient for quadratic term");
}

/** \relates Quadratic
 * Implements the quadratic function
 * @param out :: The result of evaluating the function
 * @param xValues :: function domain values
 * @param nData :: size of the function domain
 */
void Quadratic::function1D(double *out, const double *xValues,
                           const size_t nData) const {
  const double a0 = getParameter("A0");
  const double a1 = getParameter("A1");
  const double a2 = getParameter("A2");

  for (size_t i = 0; i < nData; i++) {
    out[i] = a0 + a1 * xValues[i] + a2 * xValues[i] * xValues[i];
  }
}

/** \relates Quadratic
 * Calculates the derivatives of the quadratic
 * @param out :: The resulting jacobian
 * @param xValues :: function domain values
 * @param nData :: size of the function domain
 */
void Quadratic::functionDeriv1D(API::Jacobian *out, const double *xValues,
                                const size_t nData) {
  for (size_t i = 0; i < nData; i++) {
    out->set(i, 0, 1);
    out->set(i, 1, xValues[i]);
    out->set(i, 2, xValues[i] * xValues[i]);
  }
}

} // namespace CurveFitting
} // namespace Mantid
