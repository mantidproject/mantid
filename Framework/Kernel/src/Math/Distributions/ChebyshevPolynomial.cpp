//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/Math/Distributions/ChebyshevPolynomial.h"

#include <cassert>
#include <cmath>

using std::acos;
using std::cos;

namespace Mantid {
namespace Kernel {

//-----------------------------------------------------------------------------
// Public members
//-----------------------------------------------------------------------------
/**
 * Return the value of the nth Chebyshev polynomial of the first kind
 * \f$T_n(x) = cos(ncos^-1x)\f$
 * @param n Order of the required polynomial
 * @param x X value to evaluate the polynomial in the range [-1,1]. No checking
 * is performed.
 * @return The value of the nth polynomial at the specified value
 */
double ChebyshevPolynomial::operator()(const size_t n, const double x) {
  assert(x >= -1.0 && x <= 1.0);
  return cos(static_cast<double>(n) * acos(x));
}

} // namespace Kernel
} // namespace Mantid
