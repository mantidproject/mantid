#include "MantidKernel/ConfigObserver.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Kernel {
/**
 * Begins listening to notifications from the global ConfigService.
 */
ConfigObserver::ConfigObserver()
    : m_valueChangeListener(*this, &ConfigObserver::notifyValueChanged) {
  ConfigService::Instance().addObserver(m_valueChangeListener);
}

/**
 * Copying a config observer is the same as default constructing one.
 *
 * It is necessary to create a new listener object with the this pointer
 * in order for the correct object to recieve the notification.
 *
 * @param other The observer to copy.
 */
ConfigObserver::ConfigObserver(const ConfigObserver &other)
    : m_valueChangeListener(*this, &ConfigObserver::notifyValueChanged) {
  UNUSED_ARG(other);
  ConfigService::Instance().addObserver(m_valueChangeListener);
}

/**
 * Nothing to do but we must overload the = operator to prevent a
 * default copy which would produce incorrect results.
 *
 * We don't need to re-register with the ConfigService since it
 * is not possible to create a ConfigObserver which is unregistered
 * and we only ever unregister in the destructor.
 *
 * @param other The observer to copy.
 */
ConfigObserver &ConfigObserver::operator=(const ConfigObserver &other) {
  UNUSED_ARG(other);
  return *this;
}

ConfigObserver::~ConfigObserver() noexcept {
  ConfigService::Instance().removeObserver(m_valueChangeListener);
}

/**
 * Called when a config property's value is changed.
 *
 * @param name The name of the property which changed.
 * @param newValue The new value of the property.
 * @param oldValue The old value of the property.
 */
void ConfigObserver::notifyValueChanged(const std::string &name,
                                        const std::string &newValue,
                                        const std::string &oldValue) {
  onValueChanged(name, newValue, oldValue);
}

/**
 * Called when a config property's value is changed.
 *
 * @param notification The Poco notification object.
 */
void ConfigObserver::notifyValueChanged(
    ConfigValChangeNotification_ptr notification) {
  notifyValueChanged(notification->key(), notification->curValue(),
                     notification->preValue());
}
} // namespace Kernel
} // namespace Mantid
