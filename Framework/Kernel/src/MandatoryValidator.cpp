// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//------------------------------------------
// Includes
//------------------------------------------
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/OptionalBool.h"
#include <cmath>

namespace Mantid::Kernel::Detail {
/**
 * Specialization of checkIsEmpty for string
 * @param value :: A string object
 * @return True if the string is considered empty
 */
template <> MANTID_KERNEL_DLL bool checkIsEmpty(const std::string &value) { return value.empty(); }
/**
 * Specialization of checkIsEmpty for double values
 * @param value :: A double
 * @return True if the value is considered empty, see EmptyValues.h
 */
template <> MANTID_KERNEL_DLL bool checkIsEmpty(const double &value) {
  return std::fabs(value - Mantid::EMPTY_DBL()) < 1e-08;
}
/**
 * Specialization of checkIsEmpty for int
 * @param value :: A int value
 * @return True if the value is considered empty, see EmptyValues.h
 */
template <> MANTID_KERNEL_DLL bool checkIsEmpty(const int &value) { return (value == Mantid::EMPTY_INT()); }
/**
 * Specialization of checkIsEmpty for long
 * @param value :: A long value
 * @return True if the value is considered empty, see EmptyValues.h
 */
template <> MANTID_KERNEL_DLL bool checkIsEmpty(const long &value) {
  // 32 bit for Windows and Clang, 64 bit for GCC
  return (value == Mantid::EMPTY_LONG());
}
#if defined(_WIN32) || defined(__clang__) && defined(__APPLE__)
/**
 * Specialization of checkIsEmpty for 64 bit intiger
 * @param value :: A int64_t value
 * @return True if the value is considered empty, see EmptyValues.h
 */
template <> MANTID_KERNEL_DLL bool checkIsEmpty(const int64_t &value) { return (value == Mantid::EMPTY_INT64()); }
#endif
/**
 * Specialization of checkIsEmpty for OptionalBool
 * @param value :: A long value
 * @return True if the value is considered empty, see EmptyValues.h
 */
template <> MANTID_KERNEL_DLL bool checkIsEmpty(const OptionalBool &value) {
  return (value.getValue() == OptionalBool::Unset);
}
} // namespace Mantid::Kernel::Detail
