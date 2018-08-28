#ifndef MANTID_API_ORIENTEDLATTICEVALIDATOR_H
#define MANTID_API_ORIENTEDLATTICEVALIDATOR_H

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidKernel/TypedValidator.h"

/**
  A validator which checks that a workspace has an oriented lattice attached to
  it.

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
class MANTID_API_DLL OrientedLatticeValidator
    : public Kernel::TypedValidator<ExperimentInfo_sptr> {
public:
  /// Gets the type of the validator
  std::string getType() const { return "orientedlattice"; }
  /// Clone the current state
  Kernel::IValidator_sptr clone() const override;

private:
  /// Check for validity.
  std::string
  checkValidity(const ExperimentInfo_sptr &workspace) const override;
};

} // namespace API
} // namespace Mantid

#endif // MANTID_API_ORIENTEDLATTICEVALIDATOR_H
