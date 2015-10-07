#ifndef MANTID_KERNEL_UNITCONVERSION_H_
#define MANTID_KERNEL_UNITCONVERSION_H_
/**
  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
#include "MantidKernel/DeltaEMode.h"
#include "MantidKernel/DllConfig.h"
#include <string>

namespace Mantid {
namespace Kernel {
//-------------------------------------------------------------------------
// Forward declarations
//-------------------------------------------------------------------------
class Unit;

/**
 * A set of static helper methods to perform conversions between units
 */
class MANTID_KERNEL_DLL UnitConversion {
public:
  /// Convert a single value between the given units (as strings)
  static double run(const std::string &src, const std::string &dest,
                    const double srcValue, const double l1, const double l2,
                    const double twoTheta, const DeltaEMode::Type emode,
                    const double efixed);
  /// Convert a single value between the given units
  static double run(Unit &srcUnit, Unit &destUnit, const double srcValue,
                    const double l1, const double l2, const double twoTheta,
                    const DeltaEMode::Type emode, const double efixed);

  /// Convert to ElasticQ
  static double run(const double twoTheta, const double efixed);

private:
  /// Perform a quick conversion
  static double convertQuickly(const double srcValue, const double factor,
                               const double power);
  /// Convert through TOF
  static double convertViaTOF(Unit &srcUnit, Unit &destUnit,
                              const double srcValue, const double l1,
                              const double l2, const double twoTheta,
                              const DeltaEMode::Type emode,
                              const double efixed);

  /// Convert to ElasticQ from Energy
  static double convertToElasticQ(const double twoTheta, const double efixed);
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_UNITCONVERSION_H_ */
