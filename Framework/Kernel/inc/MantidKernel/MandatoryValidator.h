// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/TypedValidator.h"
#include <memory>
#include <vector>

namespace Mantid {
namespace Kernel {
namespace Detail {
/// Forward declare checking function
template <typename T> MANTID_KERNEL_DLL bool checkIsEmpty(const T &);

/// Specialization for any vector type
template <typename T> bool checkIsEmpty(const std::vector<T> &value) { return value.empty(); }

/// Defines the concept of emptiness
template <typename T> struct IsEmpty {
  /**
   * Returns true if the value is considered empty
   * @param value: to be checked
   * @return
   */
  static bool check(const T &value) { return checkIsEmpty(value); }
};
} // namespace Detail

/** @class MandatoryValidator MandatoryValidator.h Kernel/MandatoryValidator.h

    Validator to check that a property is not left empty.
    MandatoryValidator is a validator that requires a string to be set to a
   non-blank value
    or a vector (i.e. ArrayProperty) is not empty.

    @author Nick Draper, Tessella Support Services plc
    @date 28/11/2007
*/
template <typename TYPE> class MANTID_KERNEL_DLL MandatoryValidator : public TypedValidator<TYPE> {
public:
  IValidator_sptr clone() const override { return std::make_shared<MandatoryValidator>(); }

private:
  /**
   * Check if a value has been provided
   *  @param value :: the string to test
   *  @return "A value must be entered for this parameter" if empty or ""
   */
  std::string checkValidity(const TYPE &value) const override {
    if (Detail::IsEmpty<TYPE>::check(value))
      return "A value must be entered for this parameter";
    else
      return "";
  }
};

} // namespace Kernel
} // namespace Mantid
