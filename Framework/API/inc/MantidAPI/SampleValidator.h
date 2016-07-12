#ifndef MANTID_API_SAMPLEVALIDATOR_H_
#define MANTID_API_SAMPLEVALIDATOR_H_

#include "MantidAPI/MatrixWorkspaceValidator.h"

namespace Mantid {
namespace API {

/**
  A validator which checks that sample has the required properties.

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
class MANTID_API_DLL SampleValidator : public MatrixWorkspaceValidator {
public:
  /// Enumeration describing requirements
  enum Requirements { Shape = 0x1, Material = 0x2 };

  SampleValidator(const unsigned int flags = (Shape | Material));
  std::string getType() const;
  Kernel::IValidator_sptr clone() const override;
  std::string checkValidity(const MatrixWorkspace_sptr &value) const override;

private:
  unsigned int m_requires;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_SAMPLEVALIDATOR_H_ */
