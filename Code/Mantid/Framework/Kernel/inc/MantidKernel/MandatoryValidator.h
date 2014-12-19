#ifndef MANTID_KERNEL_MANDATORYVALIDATOR_H_
#define MANTID_KERNEL_MANDATORYVALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/TypedValidator.h"
#include <vector>

namespace Mantid {
namespace Kernel {
namespace Detail {
/// Forward declare checking function
template <typename T> DLLExport bool checkIsEmpty(const T &);

/// Specialization for any vector type
template <typename T> bool checkIsEmpty(const std::vector<T> &value) {
  return value.empty();
}

/// Defines the concept of emptiness
template <typename T> struct IsEmpty {
  /**
   * Returns true if the value is considered empty
   * @param value: to be checked
   * @return
   */
  static bool check(const T &value) { return checkIsEmpty(value); }
};
}

/** @class MandatoryValidator MandatoryValidator.h Kernel/MandatoryValidator.h

    Validator to check that a property is not left empty.
    MandatoryValidator is a validator that requires a string to be set to a
   non-blank value
    or a vector (i.e. ArrayProperty) is not empty.

    @author Nick Draper, Tessella Support Services plc
    @date 28/11/2007

    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
template <typename TYPE>
class DLLExport MandatoryValidator : public TypedValidator<TYPE> {
public:
  IValidator_sptr clone() const {
    return boost::make_shared<MandatoryValidator>();
  }

private:
  /**
   * Check if a value has been provided
   *  @param value :: the string to test
   *  @return "A value must be entered for this parameter" if empty or ""
   */
  std::string checkValidity(const TYPE &value) const {
    if (Detail::IsEmpty<TYPE>::check(value))
      return "A value must be entered for this parameter";
    else
      return "";
  }
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_MANDATORYVALIDATOR_H_*/
