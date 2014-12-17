#include "MantidCurveFitting/SpecialFunctionSupport.h"
#include <limits>
#include <gsl/gsl_math.h>

namespace Mantid {
namespace CurveFitting {
namespace SpecialFunctionSupport {

using std::complex;

/** Implement Exponential Integral function, E1(z), based on formulaes in
 *Abramowitz and Stegun (A&S)
 *  In fact this implementation returns exp(z)*E1(z) where z is a complex number
 *
 *  @param z :: input
 *  @return exp(z)*E1(z)
 */
complex<double> exponentialIntegral(const complex<double> &z) {
  double z_abs = abs(z);

  if (z_abs == 0.0) {
    // Basically function is infinite in along the real axis for this case
    return complex<double>(std::numeric_limits<double>::max(), 0.0);
  } else if (z_abs < 10.0) // 10.0 is a guess based on formula 5.1.55 (no idea
                           // how good it really is)
  {
    // use formula 5.1.11 in A&S. rewrite last term in 5.1.11 as
    // x*sum_n=0 (-1)^n x^n / (n+1)*(n+1)! and then calculate the
    // terms in the sum recursively
    complex<double> z1(1.0, 0.0);
    complex<double> z2(1.0, 0.0);
    for (int i = 1; i <= 100; i++) // is max of 100 here best?
    {
      z2 = -static_cast<double>(i) * (z2 * z) / ((i + 1.0) * (i + 1.0));
      z1 += z2;
      if (abs(z2) < 1.0E-10 * abs(z1))
        break; // i.e. if break loop if little change to term added
    }
    return exp(z) * (-log(z) - M_EULER + z * z1);
  } else {
    // use formula 5.1.22 in A&S. See discussion page 231
    complex<double> z1(0.0, 0.0);
    for (int i = 20; i >= 1; i--) // not sure if 20 is always sensible?
      z1 = static_cast<double>(i) / (1.0 + static_cast<double>(i) / (z + z1));
    complex<double> retVal = 1.0 / (z + z1);
    if (z.real() <= 0.0 && z.imag() == 0.0)
      retVal -= exp(z) * complex<double>(0.0, 1.0) * M_PI; // see formula 5.1.7
    return retVal;
  }
}

} // End namespace SpecialFunctionSupport
} // End namespace CurveFitting
} // End namespace Mantid
