//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/FloatingPointComparison.h"
#include <limits>
#include <cmath>

namespace Mantid {
namespace Kernel {

/**
 * Compare floating point numbers for equality to within
 * std::numeric_limits<TYPE>::epsilon precision
 * @param x :: LHS comparator
 * @param y :: RHS comparator
 * @returns True if the numbers are considered equal within the given tolerance,
 * false otherwise
 */
template <typename TYPE> bool equals(const TYPE x, const TYPE y) {
  return !(std::fabs(x - y) > std::numeric_limits<TYPE>::epsilon());
}

/**
 * Compare two floating-point numbers as to whether they satisfy x<=y within
 * machine precision
 * @param x :: LHS comparator
 * @param y :: RHS comparator
 * @returns True if the numbers are considered <= within the machine tolerance,
 * false otherwise
 */
template <typename T> MANTID_KERNEL_DLL bool ltEquals(const T x, const T y) {
  return (equals(x, y) || x < y);
}

/**
 * Compare two floating-point numbers as to whether they satisfy x>=y within
 * machine precision
 * @param x :: LHS comparator
 * @param y :: RHS comparator
 * @returns True if the numbers are considered <= within the machine tolerance,
 * false otherwise
 */
template <typename T> MANTID_KERNEL_DLL bool gtEquals(const T x, const T y) {
  return (equals(x, y) || x > y);
}

///@cond
// Concrete instantiations
template DLLExport bool equals<double>(const double, const double);
template DLLExport bool equals<float>(const float, const float);
template DLLExport bool ltEquals<double>(const double, const double);
template DLLExport bool ltEquals<float>(const float, const float);
template DLLExport bool gtEquals<double>(const double, const double);
template DLLExport bool gtEquals<float>(const float, const float);
///@endcond
}
}
