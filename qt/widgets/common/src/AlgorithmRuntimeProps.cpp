// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/AlgorithmRuntimeProps.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidQtWidgets/Common/IAlgorithmRuntimeProps.h"

#include <string>

namespace MantidQt::API {
Mantid::Kernel::IPropertyManager::TypedValue
AlgorithmRuntimeProps::getProperty(const std::string &name) const noexcept {
  return Mantid::Kernel::PropertyManager::getProperty(name);
}
void AlgorithmRuntimeProps::setPropertyValue(const std::string &name, const std::string &value) {
  if (!existsProperty(name)) {
    declareProperty(name, value);
  } else {
    Mantid::Kernel::PropertyManager::setPropertyValue(name, value);
  }
}
} // namespace MantidQt::API
