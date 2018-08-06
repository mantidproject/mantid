#ifndef MANTID_API_ADSVALIDATOR_H_
#define MANTID_API_ADSVALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/TypedValidator.h"

namespace Mantid {
namespace API {

/** ADSValidator : a validator that requires the value of a property to be
    present in the ADS.  The  type must be std::string

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_API_DLL ADSValidator
    : public Kernel::TypedValidator<std::vector<std::string>> {
public:
  /// Default constructor. Sets up an empty list of valid values.
  ADSValidator(const bool allowMultiSelection = true,
               const bool isOptional = false);

  /// Clone the validator
  Kernel::IValidator_sptr clone() const override;

  bool isMultipleSelectionAllowed() override;

  void setMultipleSelectionAllowed(const bool isMultiSelectionAllowed);

  bool isOptional() const;

  std::vector<std::string> allowedValues() const override;

  void setOptional(const bool setOptional);

protected:
  /** Checks if the string passed is in the ADS, or if all members are in the
   * ADS
   *  @param value :: The value to test
   *  @return "" if the value is on the list, or "The workspace is not in the
   * workspace list"
   */
  std::string
  checkValidity(const std::vector<std::string> &value) const override;

private:
  /// if the validator should allow multiple selection
  bool m_AllowMultiSelection;
  /// if the validator should an empty selection
  bool m_isOptional;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_ADSVALIDATOR_H_ */