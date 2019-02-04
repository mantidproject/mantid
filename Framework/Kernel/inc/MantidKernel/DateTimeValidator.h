// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_DATETIMEVALIDATOR_H_
#define MANTID_KERNEL_DATETIMEVALIDATOR_H_

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/TypedValidator.h"
#include <string>

namespace Mantid {
namespace Kernel {
/**
  Checks that a string contains a timestamp in ISO 8601 format
  (YYYY-MM-DDTHH:MM:SS.mmmmmm)
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
} // namespace Kernel
} // namespace Mantid

#endif /** DATETIMEVALIDATOR */
