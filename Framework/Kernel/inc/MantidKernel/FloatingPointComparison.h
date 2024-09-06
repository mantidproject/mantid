// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
/// Calculate absolute difference between x, y
template <typename T> MANTID_KERNEL_DLL T absoluteDifference(const T x, const T y);
/// Calculate relative difference between x, y
template <typename T> MANTID_KERNEL_DLL T relativeDifference(const T x, const T y);
/// Test whether x, y are within absolute tolerance tol
template <typename T> MANTID_KERNEL_DLL bool withinAbsoluteDifference(const T x, const T y, const T tolerance);
/// Test whether x, y are within relative tolerance tol
template <typename T> MANTID_KERNEL_DLL bool withinRelativeDifference(const T x, const T y, const T tolerance);
} // namespace Kernel
} // namespace Mantid
