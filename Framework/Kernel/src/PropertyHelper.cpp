// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidKernel/PropertyHelper.h"
#include "MantidKernel/Strings.h"

namespace Mantid::Kernel {

/** Helper functions for setting the value of an OptionalBool property */
template <> void MANTID_KERNEL_DLL toValue(const std::string &strValue, OptionalBool &value) {
  const auto normalizedStr = Mantid::Kernel::Strings::toLower(strValue);
  if (normalizedStr == "0" || normalizedStr == "false") {
    value = OptionalBool::False;
  } else if (normalizedStr == "1" || normalizedStr == "true") {
    value = OptionalBool::True;
  } else {
    value = strValue;
  }
}

} // namespace Mantid::Kernel
