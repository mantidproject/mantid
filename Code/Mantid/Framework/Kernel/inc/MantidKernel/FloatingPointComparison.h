#ifndef MANTID_KERNEL_FLOATINGPOINTCOMPARISON_H_
#define MANTID_KERNEL_FLOATINGPOINTCOMPARISON_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"

namespace Mantid {
namespace Kernel {
/// Test for equality of doubles using compiler-defined precision
template <typename T> MANTID_KERNEL_DLL bool equals(const T x, const T y);
/// Test whether x<=y within machine precision
template <typename T> MANTID_KERNEL_DLL bool ltEquals(const T x, const T y);
/// Test whether x>=y within machine precision
template <typename T> MANTID_KERNEL_DLL bool gtEquals(const T x, const T y);
}
}

#endif // MANTID_KERNEL_FLOATINGPOINTCOMPARISON_H_