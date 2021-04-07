// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/TypedValidator.h"

#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {

/** ArrayLenghtValidator : Validate length of an array property
 */
template <typename TYPE> class MANTID_KERNEL_DLL ArrayLengthValidator : public TypedValidator<std::vector<TYPE>> {
public:
  ArrayLengthValidator();
  ArrayLengthValidator(const size_t len);
  ArrayLengthValidator(const size_t lenmin, const size_t lenmax);
  ~ArrayLengthValidator() override;

  IValidator_sptr clone() const override;

  /// Return if it has a length
  bool hasLength() const;
  /// Return if it has a length
  bool hasMinLength() const;
  /// Return if it has a length
  bool hasMaxLength() const;
  /// Return the length
  const size_t &getLength() const;
  /// Return the minimum length
  const size_t &getMinLength() const;
  /// Return the maximum length
  const size_t &getMaxLength() const;
  /// Set length
  void setLength(const size_t &value);
  /// Clear the length
  void clearLength();
  /// Set length min
  void setLengthMin(const size_t &value);
  /// Set length max
  void setLengthMax(const size_t &value);
  /// Clear minimum
  void clearLengthMin();
  /// Clear maximum
  void clearLengthMax();

private:
  std::string checkValidity(const std::vector<TYPE> &value) const override;
  /// private variable containing the size of the array
  size_t m_arraySize;
  /// private variable, true if size is set, false if not
  bool m_hasArraySize;
  /// private variable containing the minimum size of the array
  size_t m_arraySizeMin;
  /// private variable, true if min size is set, false if not
  bool m_hasArraySizeMin;
  /// private variable containing the size max of the array
  size_t m_arraySizeMax;
  /// private variable, true if size max is set, false if not
  bool m_hasArraySizeMax;
};

} // namespace Kernel
} // namespace Mantid
