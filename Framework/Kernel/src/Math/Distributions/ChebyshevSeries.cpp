// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/Math/Distributions/ChebyshevSeries.h"
#include <algorithm>
#include <cassert>

//-----------------------------------------------------------------------------
// Anonmouys static helpers
//-----------------------------------------------------------------------------

namespace Mantid {
namespace Kernel {

//-----------------------------------------------------------------------------
// Public members
//-----------------------------------------------------------------------------
/**
 * Constructor for an n-th order polynomial
 * @param degree Degree of polynomial required. It will require degree+1
 * coefficients to evaluate.
 */
ChebyshevSeries::ChebyshevSeries(const size_t degree) : m_bk(degree + 3, 0.0) {
  // The algorithm requires computing upto n+2 terms so space is
  // reserved for (n+1)+2 values.
}

/**
 * @param x X value to evaluate the polynomial in the range [-1,1]. No checking
 * is performed.
 * @param c Vector of (n+1) coefficients for the polynomial. They should be
 * ordered from 0->n. Providing more coefficients that required is not
 * considered an error.
 * @return Value of the polynomial. The value is undefined if x or n are
 * out of range
 */
double ChebyshevSeries::operator()(const std::vector<double> &c, const double x) {
  const size_t degree(m_bk.size() - 3);
  assert(c.size() >= degree + 1);

  m_bk.resize(degree + 3);
  std::fill(m_bk.begin(), m_bk.end(), 0.0);
  for (size_t i = 0; i <= degree; ++i) {
    const size_t k = degree - i;
    m_bk[k] = c[k] + 2. * x * m_bk[k + 1] - m_bk[k + 2];
  }
  return m_bk[0] - x * m_bk[1];
}

} // namespace Kernel
} // namespace Mantid
