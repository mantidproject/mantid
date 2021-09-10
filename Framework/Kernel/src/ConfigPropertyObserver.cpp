// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/ConfigPropertyObserver.h"
#include "MantidKernel/ConfigService.h"

namespace Mantid {
namespace Kernel {

/**
 * Begins listening for change notifications from the global ConfigService
 * concerning the property with the specified name.
 */
ConfigPropertyObserver::ConfigPropertyObserver(std::string propertyName) : m_propertyName(std::move(propertyName)) {}

/**
 * Filters out change notifications concerning other properties.
 */
void ConfigPropertyObserver::onValueChanged(const std::string &name, const std::string &newValue,
                                            const std::string &prevValue) {
  if (name == m_propertyName)
    onPropertyValueChanged(newValue, prevValue);
}
} // namespace Kernel
} // namespace Mantid
