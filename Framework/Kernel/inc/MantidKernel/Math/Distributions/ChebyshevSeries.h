// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_CHEBYSHEVSERIES_H_
#define MANTID_KERNEL_CHEBYSHEVSERIES_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include <vector>

namespace Mantid {
namespace Kernel {

/**
 * Evaluate an approximation to a nth order polynomial using a Chebyshev
 * series through Crenshaw's algorithm to evaluate
 * \f$p_n(x) = \Sigma_{i=1}^{n}c_iT_i\f$
 * The evaluation is implemented using the reccurrence relations
 *   http://mathworld.wolfram.com/ClenshawRecurrenceFormula.html
 */
class MANTID_KERNEL_DLL ChebyshevSeries {
public:
  ChebyshevSeries(const size_t degree);
  double operator()(const std::vector<double> &c, const double x);

private:
  // Storage for n+3 coefficents in the recurrence relation
  std::vector<double> m_bk;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_CHEBYSHEVSERIES_H_ */
