#include "MantidKernel/ConfigObserver.h"
#include "MantidKernel/ConfigService.h"
#include <iostream>

namespace Mantid {
namespace Kernel {
ConfigObserver::ConfigObserver()
    : m_valueChangeListener(*this, &ConfigObserver::notifyValueChanged) {
  ConfigService::Instance().addObserver(m_valueChangeListener);
}

ConfigObserver::ConfigObserver(const ConfigObserver &other)
    : m_valueChangeListener(other.m_valueChangeListener) {
  ConfigService::Instance().addObserver(m_valueChangeListener);
}

ConfigObserver::ConfigObserver(ConfigObserver &&other) noexcept
    : m_valueChangeListener(other.m_valueChangeListener) {
  ConfigService::Instance().removeObserver(other.m_valueChangeListener);
  ConfigService::Instance().addObserver(m_valueChangeListener);
}

ConfigObserver &ConfigObserver::operator=(const ConfigObserver &other) {
  m_valueChangeListener = other.m_valueChangeListener;
  ConfigService::Instance().addObserver(m_valueChangeListener);
  return *this;
}

ConfigObserver &ConfigObserver::operator=(ConfigObserver &&other) noexcept {
  m_valueChangeListener = other.m_valueChangeListener;
  ConfigService::Instance().removeObserver(other.m_valueChangeListener);
  ConfigService::Instance().addObserver(m_valueChangeListener);
  return *this;
}

ConfigObserver::~ConfigObserver() noexcept {
  ConfigService::Instance().removeObserver(m_valueChangeListener);
}

void ConfigObserver::notifyValueChanged(const std::string &name,
                                        const std::string &newValue,
                                        const std::string &prevValue) {
  onValueChanged(name, newValue, prevValue);
}

void ConfigObserver::notifyValueChanged(
    ConfigValChangeNotification_ptr notification) {
  notifyValueChanged(notification->key(), notification->curValue(),
                     notification->preValue());
}
}
}
