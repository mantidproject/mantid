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
 * false otherwise.  False if any value is NaN.
 */
template <typename T> bool equals(T const x, T const y) { return std::abs(x - y) <= std::numeric_limits<T>::epsilon(); }

/**
 * Compare two floating-point numbers as to whether they satisfy x<=y within
 * machine precision
 * @param x :: LHS comparator
 * @param y :: RHS comparator
 * @returns True if the numbers are considered <= within the machine tolerance,
 * false otherwise
 */
template <typename T> MANTID_KERNEL_DLL bool ltEquals(T const x, T const y) { return (equals(x, y) || x < y); }

/**
 * Compare two floating-point numbers as to whether they satisfy x>=y within
 * machine precision
 * @param x :: LHS comparator
 * @param y :: RHS comparator
 * @returns True if the numbers are considered <= within the machine tolerance,
 * false otherwise
 */
template <typename T> MANTID_KERNEL_DLL bool gtEquals(T const x, T const y) { return (equals(x, y) || x > y); }

/// ABSOLUTE AND RELATIVE DIFFERENCE

/**
 * Calculate the absolute difference of two floating-point numbers
 * @param x :: first value
 * @param y :: second value
 * @returns the value of the absolute difference
 */
template <typename T> T absoluteDifference(T const x, T const y) { return std::abs(x - y); }

/**
 * Calculate the relative difference of two floating-point numbers
 * @param x :: first value
 * @param y :: second value
 * @returns the value of the relative difference.  Do NOT use this
 * to compare the result to a tolerance; for that, use withinRelativeDifference
 * instead, as it will be more efficient at the comparison.
 */
template <typename T> T relativeDifference(T const x, T const y) {
  // calculate numerator |x-y|
  T const num = absoluteDifference<T>(x, y);
  if (num <= std::numeric_limits<T>::epsilon()) {
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
 * false otherwise.  False if either value is NaN.
 */
template <typename T> bool withinAbsoluteDifference(T const x, T const y, T const tolerance) {
  return ltEquals(absoluteDifference<T>(x, y), tolerance);
}

/**
 * Compare floating point numbers for relative difference to
 * within the given tolerance
 * @param x :: first value
 * @param y :: second value
 * @param tolerance :: the tolerance
 * @returns True if the numbers are considered equal within the given tolerance,
 * false otherwise.  False if either value is NaN.
 */
template <typename T> bool withinRelativeDifference(T const x, T const y, T const tolerance) {
  // handle the case of NaNs
  if (std::isnan(x) || std::isnan(y)) {
    return false;
  }
  T const num = absoluteDifference<T>(x, y);
  if (!(num > std::numeric_limits<T>::epsilon())) {
    // if |x-y| == 0.0 (within machine tolerance), this test passes
    return true;
  } else {
    // otherwise we have to calculate the denominator
    T const denom = static_cast<T>(0.5 * (std::abs(x) + std::abs(y)));
    // if denom <= 1, then |x-y| > tol implies |x-y|/denom > tol, can return early
    // NOTE can only return early if BOTH denom > tol AND |x-y| > tol.
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
template DLLExport bool equals<double>(double const, double const);
template DLLExport bool equals<float>(float const, float const);
template DLLExport bool ltEquals<double>(double const, double const);
template DLLExport bool ltEquals<float>(float const, float const);
template DLLExport bool gtEquals<double>(double const, double const);
template DLLExport bool gtEquals<float>(float const, float const);
//
template DLLExport double absoluteDifference<double>(double const, double const);
template DLLExport float absoluteDifference<float>(float const, float const);
template DLLExport double relativeDifference<double>(double const, double const);
template DLLExport float relativeDifference<float>(float const, float const);
//
template DLLExport bool withinAbsoluteDifference<double>(double const, double const, double const);
template DLLExport bool withinAbsoluteDifference<float>(float const, float const, float const);
template DLLExport bool withinRelativeDifference<double>(double const, double const, double const);
template DLLExport bool withinRelativeDifference<float>(float const, float const, float const);
///@endcond

} // namespace Mantid::Kernel
