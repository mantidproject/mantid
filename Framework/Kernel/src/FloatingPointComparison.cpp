// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/FloatingPointComparison.h"
#include "MantidKernel/System.h"

#include <cmath>
#include <limits>

namespace Mantid::Kernel {

/**
 * Compare floating point numbers for equality to within
 * std::numeric_limits<TYPE>::epsilon precision
 * @param x :: LHS comparator
 * @param y :: RHS comparator
 * @returns True if the numbers are considered equal within the given tolerance,
 * false otherwise.  False if either is NaN
 */
template <typename TYPE> bool equals(const TYPE x, const TYPE y) {
  // bool const xnan(std::isnan(x)), ynan(std::isnan(y));
  // if (xnan || ynan){
  //   if (xnan && ynan)
  //     return true;
  //   else
  //     return false;
  // }
  // else {
  return !(std::abs(x - y) > std::numeric_limits<TYPE>::epsilon());
  // }
}

/**
 * Compare two floating-point numbers as to whether they satisfy x<=y within
 * machine precision
 * @param x :: LHS comparator
 * @param y :: RHS comparator
 * @returns True if the numbers are considered <= within the machine tolerance,
 * false otherwise
 */
template <typename T> MANTID_KERNEL_DLL bool ltEquals(const T x, const T y) { return (equals(x, y) || x < y); }

/**
 * Compare two floating-point numbers as to whether they satisfy x>=y within
 * machine precision
 * @param x :: LHS comparator
 * @param y :: RHS comparator
 * @returns True if the numbers are considered <= within the machine tolerance,
 * false otherwise
 */
template <typename T> MANTID_KERNEL_DLL bool gtEquals(const T x, const T y) { return (equals(x, y) || x > y); }

/// ABSOLUTE AND RELATIVE DIFFERENCE

/**
 * Calculate the absolute difference of two floating-point numbers
 * @param x :: first value
 * @param y :: second value
 * @returns the value of the absolute difference
 */
template <typename T> T absoluteDifference(const T x, const T y) { return std::abs(x - y); }

/**
 * Calculate the relative difference of two floating-point numbers
 * @param x :: first value
 * @param y :: second value
 * @returns True if the numbers are considered equal within the given tolerance,
 * false otherwise
 */
template <typename T> T relativeDifference(const T x, const T y) {
  // calculate numerator |x-y|
  T const num = absoluteDifference<T>(x, y);
  if (!(num > std::numeric_limits<T>::epsilon())) {
    // if |x-y| == 0.0 (within machine tolerance), relative difference is zero
    return 0.0;
  } else {
    // otherwise we have to calculate the denominator
    T const denom = static_cast<T>(0.5 * (std::abs(x) + std::abs(y)));
    // NOTE if we made it this far, at least one of x or y is nonzero, so denom will be nonzero
    return num / denom;
  }
}

/**
 * Compare floating point numbers for absolute difference to
 * within the given tolerance
 * @param x :: first value
 * @param y :: second value
 * @param tolerance :: the tolerance
 * @returns True if the numbers are considered equal within the given tolerance,
 * false otherwise
 */
template <typename T> bool withinAbsoluteDifference(const T x, const T y, const T tolerance) {
  return ltEquals(absoluteDifference<T>(x, y), tolerance);
}

/**
 * Compare floating point numbers for relative difference to
 * within the given tolerance
 * @param x :: first value
 * @param y :: second value
 * @param tolerance :: the tolerance
 * @returns True if the numbers are considered equal within the given tolerance,
 * false otherwise
 */
template <typename T> bool withinRelativeDifference(const T x, const T y, const T tolerance) {
  T const num = absoluteDifference<T>(x, y);
  if (!(num > std::numeric_limits<T>::epsilon())) {
    // if |x-y| == 0.0 (within machine tolerance), this test passes
    return true;
  } else {
    // otherwise we have to calculate the denominator
    T const denom = static_cast<T>((std::abs(x) + std::abs(y)) / 2.);
    // if denom <= 1, it will only make the numerator larger
    if (denom <= 1. && !ltEquals(num, tolerance)) {
      return false;
    } else {
      // avoid division for improved performance
      return ltEquals(num, denom * tolerance);
    }
  }
}

///@cond
// Concrete instantiations
template DLLExport bool equals<double>(const double, const double);
template DLLExport bool equals<float>(const float, const float);
template DLLExport bool ltEquals<double>(const double, const double);
template DLLExport bool ltEquals<float>(const float, const float);
template DLLExport bool gtEquals<double>(const double, const double);
template DLLExport bool gtEquals<float>(const float, const float);
//
template DLLExport double absoluteDifference<double>(const double, const double);
template DLLExport float absoluteDifference<float>(const float, const float);
template DLLExport double relativeDifference<double>(const double, const double);
template DLLExport float relativeDifference<float>(const float, const float);
//
template DLLExport bool withinAbsoluteDifference<double>(const double, const double, const double);
template DLLExport bool withinAbsoluteDifference<float>(const float, const float, const float);
template DLLExport bool withinRelativeDifference<double>(const double, const double, const double);
template DLLExport bool withinRelativeDifference<float>(const float, const float, const float);
///@endcond

} // namespace Mantid::Kernel
