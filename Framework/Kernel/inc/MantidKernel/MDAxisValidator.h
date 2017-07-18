#ifndef MANTID_KERNEL_MDAXISVALIDATOR_H_
#define MANTID_KERNEL_MDAXISVALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include <utility>

namespace Mantid {
namespace Kernel {

/** @class MDAxisValidator MDAxisValidator.h Kernel/MDAxisValidator.h

    MDAxisValidator is a class that checks the number of MD axes match the
    number of workspace dimensions,
    refactoring out the common validation code from several MD algorithms
    into a common class.

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
class MANTID_KERNEL_DLL MDAxisValidator {
public:
  MDAxisValidator(const std::vector<int> &axes, const size_t nDimensions,
                  const bool checkIfEmpty);
  virtual ~MDAxisValidator() = default;
  virtual std::map<std::string, std::string> validate() const;

private:
  std::vector<int> m_axes;
  size_t m_wsDimensions;
  bool m_emptyCheck;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_MDAXISVALIDATOR_H_ */
