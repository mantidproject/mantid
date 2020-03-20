// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/TypedValidator.h"
#include <string>

namespace Mantid {
namespace Kernel {
/** DateValidator is a validator that validates date, format of valid date is
   "DD/MM/YYYY"
    At present, this validator is only available for properties of type
   std::string
    This class has written for validating  start and end dates of  ICat
   interface.

    @author Sofia Antony, STFC Rutherford Appleton Laboratory
    @date 03/09/2010
*/
class MANTID_KERNEL_DLL DateValidator : public TypedValidator<std::string> {
public:
  /// Clone the current state
  IValidator_sptr clone() const override;

private:
  /// Checks the value is valid
  std::string checkValidity(const std::string &value) const override;
};
} // namespace Kernel
} // namespace Mantid
