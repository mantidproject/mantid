#ifndef MANTID_KERNEL_CHEBYSHEVSERIES_H_
#define MANTID_KERNEL_CHEBYSHEVSERIES_H_
/*
  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
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
