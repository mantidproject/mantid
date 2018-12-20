#ifndef MANTID_API_MDFRAMEVALIDATOR_H
#define MANTID_API_MDFRAMEVALIDATOR_H

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/TypedValidator.h"

/**
  A validator which checks that the frame of the MDWorkspace referred to
  by a WorkspaceProperty is the expected one.

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
namespace Mantid {
namespace API {
class MANTID_API_DLL MDFrameValidator
    : public Kernel::TypedValidator<IMDWorkspace_sptr> {
public:
  explicit MDFrameValidator(const std::string &frameName);
  /// Gets the type of the validator
  std::string getType() const { return "mdframe"; }
  /// Clone the current state
  Kernel::IValidator_sptr clone() const override;

private:
  /// Check for validity.
  std::string checkValidity(const IMDWorkspace_sptr &workspace) const override;

  /// The name of the required frame
  const std::string m_frameID;
};

} // namespace API
} // namespace Mantid

#endif // MANTID_API_MDFRAMEVALIDATOR_H
