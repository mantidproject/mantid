#ifndef MANTID_KERNEL_DIFFRACTION_H_
#define MANTID_KERNEL_DIFFRACTION_H_

#include "MantidKernel/DllConfig.h"
#include <functional>

namespace Mantid {
namespace Kernel {
namespace Diffraction {

/** Diffraction : Collection of functions useful in diffraction scattering

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

MANTID_KERNEL_DLL double calcTofMin(const double difc, const double difa,
                                    const double tzero,
                                    const double tofmin = 0.);

MANTID_KERNEL_DLL double calcTofMax(const double difc, const double difa,
                                    const double tzero, const double tofmax);

MANTID_KERNEL_DLL std::function<double(double)>
getTofToDConversionFunc(const double difc, const double difa,
                        const double tzero);

MANTID_KERNEL_DLL std::function<double(double)>
getDToTofConversionFunc(const double difc, const double difa,
                        const double tzero);

} // namespace Diffraction
} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_DIFFRACTION_H_ */
