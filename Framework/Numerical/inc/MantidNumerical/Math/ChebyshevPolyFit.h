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
#include <memory>
#include <vector>

namespace Mantid {
namespace Kernel {
// Keep the implementation out of here
class ChebyshevPolyFitImpl;

/**
  Compute a weighted least-squares polynomial approximations
  to an arbitrary set of data points. Each polynomial is represented in
  Chebyshev-series form. Equivalent to the NAG routine E02ADF -
  http://www.nag.co.uk/numeric/fl/manual/pdf/E02/e02adf.pdf
*/
class MANTID_KERNEL_DLL ChebyshevPolyFit {
public:
  ChebyshevPolyFit(const size_t n);
  // Implemented in cpp so a unique_ptr member doesn't need to
  // see full ChebyshevPolyFitPrivate implementation
  ~ChebyshevPolyFit();

  std::vector<double> operator()(const std::vector<double> &xs, const std::vector<double> &ys,
                                 const std::vector<double> &wgts);

private:
  std::unique_ptr<ChebyshevPolyFitImpl> m_impl;
};

} // namespace Kernel
} // namespace Mantid
