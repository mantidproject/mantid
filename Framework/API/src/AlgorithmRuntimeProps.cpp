// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidKernel/IPropertyManager.h"

#include <string>

namespace Mantid::API {
Mantid::Kernel::IPropertyManager::TypedValue AlgorithmRuntimeProps::getProperty(const std::string &name) const {
  return Mantid::Kernel::PropertyManager::getProperty(name);
}
void AlgorithmRuntimeProps::setPropertyValue(const std::string &name, const std::string &value) {
  if (!existsProperty(name)) {
    declareProperty(name, value);
  } else {
    Mantid::Kernel::PropertyManager::setPropertyValue(name, value);
  }
}
} // namespace Mantid::API
