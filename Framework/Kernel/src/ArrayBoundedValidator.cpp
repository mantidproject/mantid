// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/ArrayBoundedValidator.h"

#include <boost/make_shared.hpp>
#include <sstream>

namespace Mantid {
namespace Kernel {

/**
 * Copy constructor
 * @param abv :: the ArrayBoundedValidator to copy
 */
template <typename TYPE>
ArrayBoundedValidator<TYPE>::ArrayBoundedValidator(
    const ArrayBoundedValidator<TYPE> &abv) noexcept
    : TypedValidator<std::vector<TYPE>>(),
      m_actualValidator(abv.m_actualValidator) {}

/**
 * Constructor via bounds parameters
 * @param lowerBound :: the lower bound value to validate
 * @param upperBound :: the upper bound value to validate
 */
template <typename TYPE>
ArrayBoundedValidator<TYPE>::ArrayBoundedValidator(
    const TYPE lowerBound, const TYPE upperBound) noexcept
    : TypedValidator<std::vector<TYPE>>(),
      m_actualValidator(lowerBound, upperBound) {}

template <typename TYPE>
ArrayBoundedValidator<TYPE>::ArrayBoundedValidator(TYPE lowerBound,
                                                   TYPE upperBound,
                                                   bool exclusive) noexcept
    : TypedValidator<std::vector<TYPE>>(),
      m_actualValidator(lowerBound, upperBound, exclusive) {}

/**
 * Constructor via a BoundedValidator
 * @param bv :: the BoundedValidator object to use
 */
template <typename TYPE>
ArrayBoundedValidator<TYPE>::ArrayBoundedValidator(
    BoundedValidator<TYPE> &bv) noexcept
    : TypedValidator<std::vector<TYPE>>(), m_actualValidator(bv) {}

/**
 * Create a clone of the current ArrayBoundedValidator
 * @return the cloned object
 */
template <typename TYPE>
IValidator_sptr ArrayBoundedValidator<TYPE>::clone() const {
  return boost::make_shared<ArrayBoundedValidator<TYPE>>(*this);
}

/**
 * Function that actually does the work of checking the validity of the
 * array elements.
 * @param value :: the array to be checked
 * @return a listing of the indicies that fail the bounds checks
 */
template <typename TYPE>
std::string ArrayBoundedValidator<TYPE>::checkValidity(
    const std::vector<TYPE> &value) const {
  // declare a class that can do conversions to string
  std::ostringstream error;
  // load in the "no error" condition
  error << "";
  typename std::vector<TYPE>::const_iterator it;
  std::size_t index = 0;
  for (it = value.begin(); it != value.end(); ++it) {
    std::string retval = m_actualValidator.isValid(*it);
    if (!retval.empty()) {
      error << "At index " << index << ": " << retval;
    }
    index++;
  }

  return error.str();
}

template <typename TYPE>
bool ArrayBoundedValidator<TYPE>::hasLower() const noexcept {
  return m_actualValidator.hasLower();
}

template <typename TYPE>
bool ArrayBoundedValidator<TYPE>::hasUpper() const noexcept {
  return m_actualValidator.hasUpper();
}

template <typename TYPE>
TYPE ArrayBoundedValidator<TYPE>::lower() const noexcept {
  return m_actualValidator.lower();
}

template <typename TYPE>
TYPE ArrayBoundedValidator<TYPE>::upper() const noexcept {
  return m_actualValidator.upper();
}

template <typename TYPE>
bool ArrayBoundedValidator<TYPE>::isLowerExclusive() const noexcept {
  return m_actualValidator.isLowerExclusive();
}
/// Check if upper bound is exclusive
template <typename TYPE>
bool ArrayBoundedValidator<TYPE>::isUpperExclusive() const noexcept {
  return m_actualValidator.isUpperExclusive();
}
/// Set the lower bound to be exclusive
template <typename TYPE>
void ArrayBoundedValidator<TYPE>::setLowerExclusive(
    const bool exclusive) noexcept {
  m_actualValidator.setLowerExclusive(exclusive);
}
/// Set the upper bound to be exclusive
template <typename TYPE>
void ArrayBoundedValidator<TYPE>::setUpperExclusive(
    const bool exclusive) noexcept {
  m_actualValidator.setUpperExclusive(exclusive);
}

/// Set both the upper and lower bounds to be exclusive
template <typename TYPE>
void ArrayBoundedValidator<TYPE>::setExclusive(const bool exclusive) noexcept {
  m_actualValidator.setLowerExclusive(exclusive);
  m_actualValidator.setUpperExclusive(exclusive);
}

template <typename TYPE>
void ArrayBoundedValidator<TYPE>::setLower(const TYPE &value) noexcept {
  m_actualValidator.setLower(value);
}

template <typename TYPE>
void ArrayBoundedValidator<TYPE>::setUpper(const TYPE &value) noexcept {
  m_actualValidator.setUpper(value);
}

template <typename TYPE>
void ArrayBoundedValidator<TYPE>::clearLower() noexcept {
  m_actualValidator.clearLower();
}

template <typename TYPE>
void ArrayBoundedValidator<TYPE>::clearUpper() noexcept {
  m_actualValidator.clearUpper();
}

// Required explicit instantiations
template class ArrayBoundedValidator<double>;
template class ArrayBoundedValidator<int32_t>;
template class ArrayBoundedValidator<int64_t>;
#if defined(_WIN32) || defined(__clang__) && defined(__APPLE__)
template class ArrayBoundedValidator<long>;
#endif

} // namespace Kernel
} // namespace Mantid
