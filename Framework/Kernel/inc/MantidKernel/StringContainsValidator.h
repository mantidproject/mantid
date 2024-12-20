// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/TypedValidator.h"

namespace Mantid {
namespace Kernel {

/** StringContainsValidator : A validator designed to ensure that a string input
  contain a given sub string or a set of sub strings. The sub strings should be
  case sensitive

  @author Elliot Oram, ISIS, RAL
  @date 05/08/2015
*/
class MANTID_KERNEL_DLL StringContainsValidator : public TypedValidator<std::string> {
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
