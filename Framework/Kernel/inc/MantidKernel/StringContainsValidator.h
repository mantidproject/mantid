#ifndef MANTID_KERNEL_STRINGCONTAINSVALIDATOR_H_
#define MANTID_KERNEL_STRINGCONTAINSVALIDATOR_H_

#include "MantidKernel/System.h"
#include "MantidKernel/TypedValidator.h"

namespace Mantid {
namespace Kernel {

/** StringContainsValidator : A validator designed to ensure that a string input
  contain a given sub string or a set of sub strings. The sub strings should be
  case sensitive

  @author Elliot Oram, ISIS, RAL
  @date 05/08/2015

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
class DLLExport StringContainsValidator : public TypedValidator<std::string> {
public:
  StringContainsValidator();
  StringContainsValidator(const std::vector<std::string> &);

  /// Clone the current state
  IValidator_sptr clone() const override;

  /// Allows a for a vector of required strings to be passed to the validator
  void setRequiredStrings(const std::vector<std::string> &);

private:
  /// Checks the value is valid
  std::string checkValidity(const std::string &value) const override;

  /// A vector of the sub strings the string must contain in order to pass
  /// validation
  std::vector<std::string> m_requiredStrings;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_STRINGCONTAINSVALIDATOR_H_ */
