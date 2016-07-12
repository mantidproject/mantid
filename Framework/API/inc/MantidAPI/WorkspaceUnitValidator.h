#ifndef MANTID_API_WORKSPACEUNITVALIDATOR_H_
#define MANTID_API_WORKSPACEUNITVALIDATOR_H_

#include "MantidAPI/MatrixWorkspaceValidator.h"

namespace Mantid {
namespace API {

/**
  A validator which checks that the unit of the workspace referred to
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
class MANTID_API_DLL WorkspaceUnitValidator : public MatrixWorkspaceValidator {
public:
  explicit WorkspaceUnitValidator(const std::string &unitID = "");
  /// Gets the type of the validator
  std::string getType() const { return "workspaceunit"; }
  /// Clone the current state
  Kernel::IValidator_sptr clone() const override;

private:
  /// Check for validity.
  std::string checkValidity(const MatrixWorkspace_sptr &value) const override;

  /// The name of the required unit
  const std::string m_unitID;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_WORKSPACEUNITVALIDATOR_H_ */