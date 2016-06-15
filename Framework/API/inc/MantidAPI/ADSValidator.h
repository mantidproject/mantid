#ifndef MANTID_API_ADSVALIDATOR_H_
#define MANTID_API_ADSVALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/TypedValidator.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidAPI/AnalysisDataService.h"

#include <vector>


namespace Mantid {
namespace API {

using namespace Mantid::Kernel;

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
class ADSValidator : public Kernel::TypedValidator<std::string> {
public:
  /// Default constructor. Sets up an empty list of valid values.
  ADSValidator(const bool allowMultiSelection = true,
    const bool isOptional = false) : 
    TypedValidator<std::string>(), m_AllowMultiSelection(allowMultiSelection),
    m_isOptional{ isOptional };

  /// Clone the validator
  IValidator_sptr clone() const override {
    return boost::make_shared<ADSValidator>(*this);
  }

  bool isMultiSelectionAllowed() const
  {
    return m_AllowMultiSelection;
  }

  void setMultiSelectionAllowed(const bool isMultiSelectionAllowed) {
    m_AllowMultiSelection = isMultiSelectionAllowed;
  }

  bool isOptional() const {
    return m_isOptional;
  }

  void setOptional(const bool setOptional) {
    m_isOptional = setOptional;
  }


protected:
  /** Checks if the string passed is in the ADS, or if all members are in the ADS
   *  @param value :: The value to test
   *  @return "" if the value is on the list, or "The workspace is not in the workspace list"
   */
  std::string checkValidity(const std::string &value) const override {
    if (!m_isOptional && isEmpty(value))
      return "Select a value";
    std::vector<std::string> ws_to_validate;
    ws_to_validate.push_back(value);
    if (m_AllowMultiSelection)
    {
      StringTokenizer tokenizer(value, ",|;\t",
        StringTokenizer::TOK_TRIM | StringTokenizer::TOK_IGNORE_EMPTY | StringTokenizer::TOK_IGNORE_FINAL_EMPTY_TOKEN);
      ws_to_validate = tokenizer.asVector();
    }
    auto& ads = AnalysisDataService::Instance();
    std::ostringstream os;
    for (std::string wsName : ws_to_validate) {
      if (!ads.doesExist(wsName)) {
        os << "The workspace \"" << value
          << "\" is not in the workspace list";
      }
    }
    return os.str();
  }

  /**
   * Is the value considered empty. Specialized string version to use empty
   * @param value :: The value to check
   * @return True if it is considered empty
   */
  bool isEmpty(const std::string &value) const { return value.empty(); }

private:
  /// if the validator should allow multiple selection
  bool m_AllowMultiSelection;
  /// if the validator should an empty selection
  bool m_isOptional;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_ADSVALIDATOR_H_ */