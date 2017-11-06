#ifndef MANTID_KERNEL_DATETIMEVALIDATOR_H_
#define MANTID_KERNEL_DATETIMEVALIDATOR_H_

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/TypedValidator.h"
#include "MantidKernel/IValidator.h"
#include <string>

namespace Mantid {
namespace Kernel {
/**
  Checks that a string contains a timestamp in ISO 8601 format
  (YYYY-MM-DDTHH:MM:SS.mmmmmm)

  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_KERNEL_DLL DateTimeValidator : public TypedValidator<std::string> {
public:
  DateTimeValidator();

  /// Clone the current state
  IValidator_sptr clone() const override;

  void allowEmpty(const bool &);

private:
  /// Checks the value is valid
  std::string checkValidity(const std::string &value) const override;

  /// Allows for an empty string to be accepted as input
  bool m_allowedEmpty;
};
}
}

#endif /** DATETIMEVALIDATOR */
