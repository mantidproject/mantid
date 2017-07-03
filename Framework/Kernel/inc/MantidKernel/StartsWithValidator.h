#ifndef MANTID_KERNEL_STARTSWITHVALIDATOR_H_
#define MANTID_KERNEL_STARTSWITHVALIDATOR_H_

#include "MantidKernel/IValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/System.h"
#include <set>
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {
/** StartsWithValidator is a validator that requires the value of a property to
   start with one
    of the strings in a defined list of possibilities.

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport StartsWithValidator : public Kernel::StringListValidator {
public:
  StartsWithValidator() = default;
  StartsWithValidator(const std::vector<std::string> &values);
  StartsWithValidator(const std::set<std::string> &values);
  IValidator_sptr clone() const override;

protected:
  std::string checkValidity(const std::string &value) const override;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_STARTSWITHVALIDATOR_H_*/
