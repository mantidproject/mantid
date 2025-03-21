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
#include "MantidKernel/V3D.h"

#include <cmath>
#include <limits>

namespace {
typedef std::true_type MADE_FOR_INTEGERS;
typedef std::false_type MADE_FOR_FLOATS;
} // namespace

namespace Mantid::Kernel {

/**
 * Compare numbers for equality to within machine epsilon.
 * @param x :: LHS comparator
 * @param y :: RHS comparator
 * @returns True if the numbers are considered equal within machine precision.
 * Machine precision is a 1 at the least significant bit scaled to the same power as the numbers compared.
 * E.g. for 1,       it is usually 1x2^{-52}
 * E.g. for 1x2^100, it is usually 1x2^{-52} x 1x2^100 = 1x2^{48}
 * False if any value is NaN.  False if comparing opposite infinities.
 */
template <typename T> bool equals(T const x, T const y) { return equals(x, y, std::is_integral<T>()); }

/**
 * Compare integer numbers
 * @param x :: LHS comparator
 * @param y :: RHS comparator
 * @returns True if the numbers are considered equal
 */
template <typename T> inline bool equals(T const x, T const y, MADE_FOR_INTEGERS) { return x == y; }

/**
 * Compare floating point numbers for equality to within
 * std::numeric_limits<TYPE>::epsilon precision
 * @param x :: LHS comparator
 * @param y :: RHS comparator
 * @returns True if the numbers are considered equal within machine precision.
 * Machine precision is a 1 at the least significant bit scaled to the same power as the numbers compared.
 * E.g. for 1,       it is usually 1x2^{-52}
 * E.g. for 1x2^100, it is usually 1x2^{-52} x 1x2^100 = 1x2^{48}
 * False if any value is NaN.  False if comparing opposite infinities.
 */
template <typename T> inline bool equals(T const x, T const y, MADE_FOR_FLOATS) {
  // handle infinities
  if (std::isinf(x) && std::isinf(y)) {
    // if x,y both +inf, x-y=NaN; if x,y both -inf, x-y=NaN; else not an NaN
    return std::isnan(x - y);
  } else {
    // produce a scale for comparison
    // in general can use either value, but use the second to work better with near differences,
    // which call as equals(difference, tolerance), since tolerance will more often be finite
    int const exp = y < std::numeric_limits<T>::min() ? std::numeric_limits<T>::min_exponent - 1 : std::ilogb(y);
    // compare to within machine epsilon
    return std::abs(x - y) <= std::ldexp(std::numeric_limits<T>::epsilon(), exp);
  }
}

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

#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wabsolute-value"
#endif
/**
 * Calculate the absolute difference of two floating-point numbers
 * @param x :: first value
 * @param y :: second value
 * @returns the value of the absolute difference
 */
template <typename T> T absoluteDifference(T const x, T const y) { return std::abs(x - y); }
#ifdef __APPLE__
#pragma clang diagnostic pop
#endif

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
    return static_cast<T>(0);
  } else {
    // otherwise we have to calculate the denominator
    T const denom = static_cast<T>((std::abs(x) + std::abs(y)) / static_cast<T>(2));
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
template <typename T, typename S> bool withinAbsoluteDifference(T const x, T const y, S const tolerance) {
  return withinAbsoluteDifference(x, y, tolerance, std::is_integral<T>());
}

#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wabsolute-value"
#endif
// specialization for integer types
template <typename T, typename S>
inline bool withinAbsoluteDifference(T const x, T const y, S const tolerance, MADE_FOR_INTEGERS) {
  return ltEquals(static_cast<S>(std::llabs(x - y)), tolerance);
}
#ifdef __APPLE__
#pragma clang diagnostic pop
#endif

/**
 * Compare floating point numbers for absolute difference to
 * within the given tolerance
 * @param x :: first value
 * @param y :: second value
 * @param tolerance :: the tolerance
 * @returns True if the numbers are considered equal within the given tolerance,
 * false otherwise.  False if either value is NaN.
 */
template <typename T, typename S>
inline bool withinAbsoluteDifference(T const x, T const y, S const tolerance, MADE_FOR_FLOATS) {
  // handle the case of infinities
  if (std::isinf(x) && std::isinf(y))
    // if both are +inf, return true; if both -inf, return true; else false
    return std::isnan(static_cast<S>(x - y));
  return ltEquals(static_cast<S>(absoluteDifference<T>(x, y)), tolerance);
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
template <typename T, typename S> bool withinRelativeDifference(T const x, T const y, S const tolerance) {
  return withinRelativeDifference(x, y, tolerance, std::is_integral<T>());
}

#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wabsolute-value"
#endif
template <typename T, typename S>
inline bool withinRelativeDifference(T const x, T const y, S const tolerance, MADE_FOR_INTEGERS) {
  S const num = static_cast<S>(std::llabs(x - y));
  if (num == static_cast<S>(0)) {
    return true;
  } else {
    S const denom = static_cast<S>(std::llabs(x) + std::llabs(y));
    return num <= static_cast<S>(2) * denom * tolerance;
  }
}
#ifdef __APPLE__
#pragma clang diagnostic pop
#endif

/**
 * Compare floating point numbers for relative difference to
 * within the given tolerance
 * @param x :: first value
 * @param y :: second value
 * @param tolerance :: the tolerance
 * @returns True if the numbers are considered equal within the given tolerance,
 * false otherwise.  False if either value is NaN.
 */
template <typename T, typename S>
inline bool withinRelativeDifference(T const x, T const y, S const tolerance, MADE_FOR_FLOATS) {
  // handles the case of infinities
  if (std::isinf(x) && std::isinf(y))
    // if both are +inf, return true; if both -inf, return true; else false
    return isnan(static_cast<S>(x - y));
  S const num = static_cast<S>(absoluteDifference<T>(x, y));
  if (num <= std::numeric_limits<S>::epsilon()) {
    // if |x-y| == 0.0 (within machine tolerance), this test passes
    return true;
  } else {
    // otherwise we have to calculate the denominator
    S const denom = static_cast<S>(0.5) * static_cast<S>(std::abs(x) + std::abs(y));
    // if denom <= 1, then |x-y| > tol implies |x-y|/denom > tol, can return early
    // NOTE can only return early if BOTH denom > tol AND |x-y| > tol.
    if (denom <= static_cast<S>(1) && !ltEquals(num, tolerance)) {
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
// difference methods
template DLLExport double absoluteDifference<double>(double const, double const);
template DLLExport float absoluteDifference<float>(float const, float const);
template DLLExport double relativeDifference<double>(double const, double const);
template DLLExport float relativeDifference<float>(float const, float const);
// within difference methods -- object and tolerance same
template DLLExport bool withinAbsoluteDifference<double>(double const, double const, double const);
template DLLExport bool withinAbsoluteDifference<float>(float const, float const, float const);
template DLLExport bool withinAbsoluteDifference<int>(int const, int const, int const);
template DLLExport bool withinAbsoluteDifference<unsigned int>(unsigned int const, unsigned int const,
                                                               unsigned int const);
template DLLExport bool withinAbsoluteDifference<long>(long const, long const, long const);
template DLLExport bool withinAbsoluteDifference<long long>(long long const, long long const, long long const);
//
template DLLExport bool withinRelativeDifference<double>(double const, double const, double const);
template DLLExport bool withinRelativeDifference<float>(float const, float const, float const);
template DLLExport bool withinRelativeDifference<int>(int const, int const, int const);
template DLLExport bool withinRelativeDifference<unsigned int>(unsigned int const, unsigned int const,
                                                               unsigned int const);
template DLLExport bool withinRelativeDifference<long>(long const, long const, long const);
template DLLExport bool withinRelativeDifference<long long>(long long const, long long const, long long const);
// within difference methods -- tolerance is double
template DLLExport bool withinAbsoluteDifference<float, double>(float const, float const, double const);
template DLLExport bool withinAbsoluteDifference<int, double>(int const, int const, double const);
template DLLExport bool withinAbsoluteDifference<unsigned int, double>(unsigned int const, unsigned int const,
                                                                       double const);
template DLLExport bool withinAbsoluteDifference<long, double>(long const, long const, double const);
template DLLExport bool withinAbsoluteDifference<unsigned long, double>(unsigned long const, unsigned long const,
                                                                        double const);
template DLLExport bool withinAbsoluteDifference<long long, double>(long long const, long long const, double const);
template DLLExport bool withinAbsoluteDifference<unsigned long long, double>(unsigned long long const,
                                                                             unsigned long long const, double const);
//
template DLLExport bool withinRelativeDifference<float, double>(float const, float const, double const);
template DLLExport bool withinRelativeDifference<int, double>(int const, int const, double const);
template DLLExport bool withinRelativeDifference<unsigned int, double>(unsigned int const, unsigned int const,
                                                                       double const);
template DLLExport bool withinRelativeDifference<long, double>(long const, long const, double const);
template DLLExport bool withinRelativeDifference<unsigned long, double>(unsigned long const, unsigned long const,
                                                                        double const);
template DLLExport bool withinRelativeDifference<long long, double>(long long const, long long const, double const);
template DLLExport bool withinRelativeDifference<unsigned long long, double>(unsigned long long const,
                                                                             unsigned long long const, double const);
///@endcond

/// Template specialization for V3D

/**
 * Compare 3D vectors (class V3D) for absolute difference to
 * within the given tolerance.  The ansolute difference is calculated as
 *   abs diff = ||V1 - V2||
 * consistent with other normalized vector binary operations.
 * @param V1 :: first value
 * @param V2 :: second value
 * @param tolerance :: the tolerance
 * @returns True if the vectorsare considered equal within the given tolerance,
 * false otherwise.  False if any value in either vector is an NaN.
 */
template <> DLLExport bool withinAbsoluteDifference<V3D, double>(V3D const V1, V3D const V2, double const tolerance) {
  // we want ||V1-V2|| < tol
  // NOTE ||V1-V2||^2 < tol^2 --> ||V1-V2|| < tol, since both are non-negative
  return ltEquals((V1 - V2).norm2(), tolerance * tolerance);
}

/**
 * Compare 3D vectors (class V3D) for relative difference to
 * within the given tolerance.  The relative difference is calculated as
 *   rel diff = ||V1 - V2||/sqrt(||V1||*||V2||)
 * consistent with other normalized vector binary operations.
 * @param V1 :: first value
 * @param V2 :: second value
 * @param tolerance :: the tolerance
 * @returns True if the vectorsare considered equal within the given tolerance,
 * false otherwise.  False if any value in either vector is an NaN.
 */
template <> DLLExport bool withinRelativeDifference<V3D, double>(V3D const V1, V3D const V2, double const tolerance) {
  // NOTE we must avoid sqrt as much as possible
  double tol2 = tolerance * tolerance;
  double diff2 = (V1 - V2).norm2(), denom4;
  if (diff2 < 1 && diff2 <= std::numeric_limits<double>::epsilon()) {
    // make sure not due to underflow
    double const scale = std::max({V1.X(), V1.Y(), V1.Z()});
    if (scale < 1 && scale * scale < std::numeric_limits<double>::epsilon()) {
      V3D const v1 = V1 / scale, v2 = V2 / scale;
      diff2 = (v1 - v2).norm();
      denom4 = std::sqrt(v1.norm()) * sqrt(v2.norm());
      return ltEquals(diff2, denom4 * tolerance);
    }
    // if the difference has zero length, the vectors are equal, this test passes
    return true;
  } else {
    // otherwise we must calculate the denominator
    denom4 = static_cast<double>(V1.norm2() * V2.norm2());
    // NOTE for large numbers, risk of overflow -- normalize by max(V1)
    // ||V1-V2||/sqrt(||V1||*||V2||) * (a/a) = ||(aV1)-(aV2)||/sqrt(||aV1||*||aV2||)
    if (std::isinf(denom4)) {
      double const scale = 1.0 / std::max({V1.X(), V1.Y(), V1.Z(), V2.X(), V2.Y(), V2.Z()});
      V3D const v1 = V1 * scale, v2 = V2 * scale;
      diff2 = (v1 - v2).norm();
      denom4 = std::sqrt(v1.norm()) * sqrt(v2.norm());
      tol2 = tolerance;
    }
    // if denom <= 1, then |x-y| > tol implies |x-y|/denom > tol, can return early
    // NOTE can only return early if BOTH denom <= 1. AND |x-y| > tol.
    if (denom4 <= 1. && !ltEquals(diff2, tol2)) { // NOTE ||V1||^2.||V2||^2 <= 1 --> sqrt(||V1||.||V2||) <= 1
      return false;
    } else {
      // avoid division and sqrt for improved performance
      // we want ||V1-V2||/sqrt(||V1||*||V2||) < tol
      // NOTE ||V1-V2||^4 < ||V1||^2.||V2||^2 * tol^4 --> ||V1-V2||/sqrt(||V1||*||V2||) < tol
      return ltEquals(diff2 * diff2, denom4 * tol2 * tol2);
    }
  }
}

} // namespace Mantid::Kernel
