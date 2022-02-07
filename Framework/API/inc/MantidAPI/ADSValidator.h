// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/TypedValidator.h"

namespace Mantid {
namespace API {

/** ADSValidator : a validator that requires the value of a property to be
    present in the ADS.  The  type must be std::string
*/
class MANTID_API_DLL ADSValidator : public Kernel::TypedValidator<std::vector<std::string>> {
public:
  /// Default constructor. Sets up an empty list of valid values.
  ADSValidator(const bool allowMultiSelection = true, const bool isOptional = false);

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
  std::string checkValidity(const std::vector<std::string> &value) const override;

private:
  /// if the validator should allow multiple selection
  bool m_AllowMultiSelection;
  /// if the validator should an empty selection
  bool m_isOptional;
};

} // namespace API
} // namespace Mantid
