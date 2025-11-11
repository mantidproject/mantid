// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
  double operator()(const std::size_t n, const double x);
};
} // namespace Kernel
} // namespace Mantid
