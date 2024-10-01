// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"

#include <memory>
#include <string>

namespace Mantid {
namespace Kernel {
namespace DynamicPointerCastHelper {

/*
Will cast a shared_ptr of type U to one of type T using std::dynamic_pointer_cast. If the cast
is invalid then it will throw an exception. This is useful for avoiding warnings about
potential null objects coming out of std::dynamic_pointer_cast.
*/
template <typename T, typename U>
std::shared_ptr<T> MANTID_KERNEL_DLL dynamicPointerCastWithCheck(std::shared_ptr<U> sharedPtr,
                                                                 const std::string &error = "") {
  auto result = std::dynamic_pointer_cast<T>(sharedPtr);
  if (result == nullptr) {
    throw std::invalid_argument(error.empty() ? "Invalid cast" : error);
  }
  return result;
}

} // namespace DynamicPointerCastHelper
} // namespace Kernel
} // namespace Mantid
