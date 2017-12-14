#include "MantidKernel/ConfigPropertyObserver.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/System.h"
#include <iostream>

namespace Mantid {
namespace Kernel {
ConfigPropertyObserver::ConfigPropertyObserver(std::string propertyName)
    : m_propertyName(std::move(propertyName)) {}

void ConfigPropertyObserver::onValueChanged(const std::string &name,
                                            const std::string &newValue,
                                            const std::string &prevValue) {
  if (name == m_propertyName)
    onPropertyValueChanged(newValue, prevValue);
}
}
}
