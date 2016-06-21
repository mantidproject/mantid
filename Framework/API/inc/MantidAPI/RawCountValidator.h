#ifndef MANTID_API_RAWCOUNTVALIDATOR_H_
#define MANTID_API_RAWCOUNTVALIDATOR_H_

#include "MantidAPI/MatrixWorkspaceValidator.h"

namespace Mantid {
namespace API {

/** A validator which checks that a workspace contains raw counts in its bins.

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
class MANTID_API_DLL RawCountValidator : public MatrixWorkspaceValidator {
public:
  explicit RawCountValidator(const bool &mustNotBeDistribution = true);

  /// Gets the type of the validator
  std::string getType() const { return "rawcount"; }
  /// Clone the current state
  Kernel::IValidator_sptr clone() const override;

private:
  /// Check for validity
  std::string checkValidity(const MatrixWorkspace_sptr &value) const override;

  /// A flag indicating whether this validator requires that the workspace must
  /// be a distribution (false) or not (true, the default)
  const bool m_mustNotBeDistribution;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_RAWCOUNTVALIDATOR_H_ */
