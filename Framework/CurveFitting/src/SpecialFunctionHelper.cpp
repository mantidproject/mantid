// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/SpecialFunctionSupport.h"
#include <gsl/gsl_math.h>
#include <limits>

namespace Mantid {
namespace CurveFitting {
namespace SpecialFunctionSupport {

using std::complex;

/** Implementation of complex integral E_1
 *
 *  The algorithm originates from the following reference,
 *
 *  Shanjie Zhang, Jianming Jin,
 *  Computation of Special Functions,
 *  Wiley, 1996,
 *  ISBN: 0-471-11963-6,
 *  LC: QA351.C45.
 *
 *  Also, refre to the following link for its implementation in Fortran,
 *
 *  https://people.sc.fsu.edu/~jburkardt/f_src/special_functions/special_functions.f90
 *
 */
complex<double> exponentialIntegral(const complex<double> &z) {
  complex<double> e1;

  double rz = real(z);
  double az = abs(z);
  double eL = 0.5772156649015328;

  if (fabs(az) < 1.0E-8) {
    // If z = 0, then the result is infinity... diverge!
    complex<double> r(1.0E300, 0.0);
    e1 = r;
  } else if (az <= 10.0 || (rz < 0.0 && az < 20.0)) {
    // Some interesting region, equal to integrate to infinity, converged
    complex<double> r(1.0, 0.0);
    e1 = r;
    complex<double> cr = r;

    for (size_t k = 1; k <= 150; ++k) {
      auto dk = double(k);
      cr = -cr * dk * z / ((dk + 1.0) * (dk + 1.0));
      e1 += cr;
      if (abs(cr) < abs(e1) * 1.0E-15) {
        // cr is converged to zero
        break;
      }
    } // ENDFOR k

    e1 = -eL - log(z) + (z * e1);
  } else {
    complex<double> ct0(0.0, 0.0);
    for (int k = 120; k > 0; --k) {
      complex<double> dk(double(k), 0.0);
      ct0 = dk / (1.0 + dk / (z + ct0));
    } // ENDFOR k

    e1 = 1.0 / (z + ct0);
    e1 = e1 * exp(-z);
    if (rz < 0.0 && fabs(imag(z)) < 1.0E-10) {
      complex<double> u(0.0, 1.0);
      e1 = e1 - (M_PI * u);
    }
  }
  return e1;
}

} // End namespace SpecialFunctionSupport
} // End namespace CurveFitting
} // End namespace Mantid
