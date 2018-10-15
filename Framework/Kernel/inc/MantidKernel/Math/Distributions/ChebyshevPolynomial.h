// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_CHEBYSHEVPOLYNOMIAL_H_
#define MANTID_KERNEL_CHEBYSHEVPOLYNOMIAL_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"

namespace Mantid {
namespace Kernel {

/**
  Evaluates a single Chebyshev polynomial (first kind) for x in range [-1,1].
  See http://mathworld.wolfram.com/ChebyshevPolynomialoftheFirstKind.html
  for more information
*/
struct MANTID_KERNEL_DLL ChebyshevPolynomial {
  double operator()(const size_t n, const double x);
};
} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_CHEBYSHEVPOLYNOMIAL_H_ */
