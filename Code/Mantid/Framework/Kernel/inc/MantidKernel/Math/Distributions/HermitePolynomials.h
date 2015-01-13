#ifndef MANTID_KERNEL_HERMITEPOLYNOMIALS_H_
#define MANTID_KERNEL_HERMITEPOLYNOMIALS_H_

#include "MantidKernel/DllConfig.h"
#include <vector>

namespace Mantid {
namespace Kernel {
/**
  Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
namespace Math {
/// Calculate the value of the nth Hermite polynomial for a given x value
MANTID_KERNEL_DLL double hermitePoly(const unsigned int n, const double x);

/// Calculates the value of the nth Hermite polynomial for each given x values
MANTID_KERNEL_DLL std::vector<double>
hermitePoly(const unsigned int n, const std::vector<double> &xaxis);
} // namespace Math
} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_HERMITEPOLYNOMIALS_H_ */
