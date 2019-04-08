// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_ARRAYBOUNDEDVALIDATOR_H_
#define MANTID_KERNEL_ARRAYBOUNDEDVALIDATOR_H_

#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Kernel {
/** @class ArrayBoundedValidator ArrayBoundedValidator.h
   Kernel/ArrayBoundedValidator.h

    ArrayBoundedValidator is a validator that requires all values in an array
    to be between upper or lower bounds, or both.

    @author Michael Reuter, NScD Oak Ridge National Laboratory
    @date 09/11/2010
*/
template <typename TYPE>
class MANTID_KERNEL_DLL ArrayBoundedValidator final
    : public TypedValidator<std::vector<TYPE>> {
public:
  ArrayBoundedValidator() = default;
  ArrayBoundedValidator(const ArrayBoundedValidator<TYPE> &abv) noexcept;
  ArrayBoundedValidator(const TYPE lowerBound, const TYPE upperBound) noexcept;
  ArrayBoundedValidator(TYPE lowerBound, TYPE upperBound,
                        bool exclusive) noexcept;
  ArrayBoundedValidator(BoundedValidator<TYPE> &bv) noexcept;
  /// Clone the current state
  IValidator_sptr clone() const override;
  /// Return if it has a lower bound
  bool hasLower() const noexcept;
  /// Return if it has a lower bound
  bool hasUpper() const noexcept;
  /// Return the lower bound value
  TYPE lower() const noexcept;
  /// Return the upper bound value
  TYPE upper() const noexcept;
  bool isLowerExclusive() const noexcept;
  /// Check if upper bound is exclusive
  bool isUpperExclusive() const noexcept;
  /// Set the lower bound to be exclusive
  void setLowerExclusive(const bool exclusive) noexcept;
  /// Set the upper bound to be exclusive
  void setUpperExclusive(const bool exclusive) noexcept;

  /// Set both the upper and lower bounds to be exclusive
  void setExclusive(const bool exclusive) noexcept;

  /// Set lower bound value
  void setLower(const TYPE &value) noexcept;
  /// Set upper bound value
  void setUpper(const TYPE &value) noexcept;
  /// Clear lower bound value
  void clearLower() noexcept;
  /// Clear upper bound value
  void clearUpper() noexcept;

private:
  std::string checkValidity(const std::vector<TYPE> &value) const override;

  BoundedValidator<TYPE> m_actualValidator;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_ARRAYBOUNDEDVALIDATOR_H_ */
