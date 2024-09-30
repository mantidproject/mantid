// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/EnumStringProperty.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/PropertyWithValue.h"
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {
// Example implementation of the constructor (usually in a source file)
template <typename T, const std::vector<std::string> *Names> EnumStringProperty<T, Names>::EnumStringProperty() {
  // Constructor implementation...
}

template <typename T, const std::vector<std::string> *Names> std::string EnumStringProperty<T, Names>::value() {
  // Return value implementation...
  return {};
}

template <typename T, const std::vector<std::string> *Names>
std::string EnumStringProperty<T, Names>::setValue(const std::string &value) {
  // Set value implementation...
  return {};
}
} // namespace Kernel
} // namespace Mantid
