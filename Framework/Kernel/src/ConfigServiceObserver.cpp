#include "MantidKernel/ConfigServiceObserver.h"
#include "MantidKernel/ConfigService.h"
#include <iostream>

namespace Mantid {
namespace Kernel {
ConfigServiceObserver::ConfigServiceObserver()
    : m_valueChangeListener(*this, &ConfigServiceObserver::notifyValueChanged) {
  ConfigService::Instance().addObserver(m_valueChangeListener);
}

ConfigServiceObserver::ConfigServiceObserver(const ConfigServiceObserver &other)
    : m_valueChangeListener(other.m_valueChangeListener) {
  ConfigService::Instance().addObserver(m_valueChangeListener);
}

ConfigServiceObserver::ConfigServiceObserver(
    ConfigServiceObserver &&other) noexcept
    : m_valueChangeListener(other.m_valueChangeListener) {
  ConfigService::Instance().removeObserver(other.m_valueChangeListener);
  ConfigService::Instance().addObserver(m_valueChangeListener);
}

ConfigServiceObserver &ConfigServiceObserver::
operator=(const ConfigServiceObserver &other) {
  m_valueChangeListener = other.m_valueChangeListener;
  ConfigService::Instance().addObserver(m_valueChangeListener);
  return *this;
}

ConfigServiceObserver &ConfigServiceObserver::
operator=(ConfigServiceObserver &&other) noexcept {
  m_valueChangeListener = other.m_valueChangeListener;
  ConfigService::Instance().removeObserver(other.m_valueChangeListener);
  ConfigService::Instance().addObserver(m_valueChangeListener);
  return *this;
}

ConfigServiceObserver::~ConfigServiceObserver() noexcept {
  ConfigService::Instance().removeObserver(m_valueChangeListener);
}

void ConfigServiceObserver::notifyValueChanged(
    const std::string &name, const std::string &newValue,
    const std::string &prevValue) {
  onValueChanged(name, newValue, prevValue);
}

void ConfigServiceObserver::onValueChanged(const std::string &name,
                                           const std::string &newValue,
                                           const std::string &prevValue) {}

void ConfigServiceObserver::notifyValueChanged(
    ConfigValChangeNotification_ptr notification) {
  notifyValueChanged(notification->key(), notification->curValue(),
                     notification->preValue());
}
}
}
