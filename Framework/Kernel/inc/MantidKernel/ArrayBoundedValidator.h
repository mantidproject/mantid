// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_ARRAYBOUNDEDVALIDATOR_H_
#define MANTID_KERNEL_ARRAYBOUNDEDVALIDATOR_H_

#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/TypedValidator.h"
#include <string>
#include <vector>

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
class MANTID_KERNEL_DLL ArrayBoundedValidator
    : public TypedValidator<std::vector<TYPE>> {
public:
  ArrayBoundedValidator();
  ArrayBoundedValidator(const ArrayBoundedValidator<TYPE> &abv);
  ArrayBoundedValidator(const TYPE lowerBound, const TYPE upperBound);
  ArrayBoundedValidator(BoundedValidator<TYPE> &bv);
  ~ArrayBoundedValidator() override;
  /// Clone the current state
  IValidator_sptr clone() const override;
  /// Return the object that checks the bounds
  boost::shared_ptr<BoundedValidator<TYPE>> getValidator() const;

  /// Return if it has a lower bound
  bool hasLower() const;
  /// Return if it has a lower bound
  bool hasUpper() const;
  /// Return the lower bound value
  const TYPE &lower() const;
  /// Return the upper bound value
  const TYPE &upper() const;

  /// Set lower bound value
  void setLower(const TYPE &value);
  /// Set upper bound value
  void setUpper(const TYPE &value);
  /// Clear lower bound value
  void clearLower();
  /// Clear upper bound value
  void clearUpper();

private:
  std::string checkValidity(const std::vector<TYPE> &value) const override;

  /// The object used to do the actual validation
  boost::shared_ptr<BoundedValidator<TYPE>> boundVal;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_ARRAYBOUNDEDVALIDATOR_H_ */
