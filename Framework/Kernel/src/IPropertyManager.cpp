// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/OptionalBool.h"

///@cond
DEFINE_IPROPERTYMANAGER_GETVALUE(int16_t)
DEFINE_IPROPERTYMANAGER_GETVALUE(uint16_t)
DEFINE_IPROPERTYMANAGER_GETVALUE(int32_t)
DEFINE_IPROPERTYMANAGER_GETVALUE(uint32_t)
DEFINE_IPROPERTYMANAGER_GETVALUE(int64_t)
DEFINE_IPROPERTYMANAGER_GETVALUE(uint64_t)
DEFINE_IPROPERTYMANAGER_GETVALUE(bool)
DEFINE_IPROPERTYMANAGER_GETVALUE(double)
DEFINE_IPROPERTYMANAGER_GETVALUE(OptionalBool)
DEFINE_IPROPERTYMANAGER_GETVALUE(std::vector<int16_t>)
DEFINE_IPROPERTYMANAGER_GETVALUE(std::vector<uint16_t>)
DEFINE_IPROPERTYMANAGER_GETVALUE(std::vector<int32_t>)
DEFINE_IPROPERTYMANAGER_GETVALUE(std::vector<uint32_t>)
DEFINE_IPROPERTYMANAGER_GETVALUE(std::vector<int64_t>)
DEFINE_IPROPERTYMANAGER_GETVALUE(std::vector<uint64_t>)
DEFINE_IPROPERTYMANAGER_GETVALUE(std::vector<double>)
DEFINE_IPROPERTYMANAGER_GETVALUE(std::vector<std::string>)
DEFINE_IPROPERTYMANAGER_GETVALUE(std::vector<std::vector<std::string>>)

namespace Mantid::Kernel {
// This template implementation has been left in because although you can't
// assign to an existing string
// via the getProperty() method, you can construct a local variable by saying,
// e.g.: std::string s = getProperty("myProperty")
template <> MANTID_KERNEL_DLL std::string IPropertyManager::getValue<std::string>(const std::string &name) const {
  return getPropertyValue(name);
}

template <> MANTID_KERNEL_DLL Property *IPropertyManager::getValue<Property *>(const std::string &name) const {
  return getPointerToProperty(name);
}

// If a string is given in the argument, we can be more flexible
template <>
IPropertyManager *IPropertyManager::setProperty<std::string>(const std::string &name, const std::string &value) {
  this->setPropertyValue(name, value);
  return this;
}

/**
 * @param name The name of the property being looked for.
 * @return True if the property is managed by this.
 */
bool IPropertyManager::existsProperty(const std::string &name) const {
  auto const &props = this->getProperties();
  return std::any_of(props.cbegin(), props.cend(), [&name](const auto prop) { return name == prop->name(); });
}

/**
 * Set values of the properties existing in this manager to the values of
 * properties with the same name in another manger.
 * @param other A property manager to copy property values from.
 */
void IPropertyManager::updatePropertyValues(const IPropertyManager &other) {
  auto props = this->getProperties();
  for (auto &prop : props) {
    const std::string propName = (*prop).name();
    if (other.existsProperty(propName)) {
      (*prop).setValueFromProperty(*other.getPointerToProperty(propName));
    }
  }
}

/** Give settings to a property to determine when it gets enabled/hidden.
 * Passes ownership of the given IPropertySettings object to the named
 * property
 * @param name :: property name
 * @param settings :: IPropertySettings     */
void IPropertyManager::setPropertySettings(const std::string &name, std::unique_ptr<IPropertySettings> settings) {
  Property *prop = getPointerToProperty(name);
  if (prop)
    prop->setSettings(std::move(settings));
}

/**
 * Get all properties in a group.
 * @param group Name of a group.
 */
std::vector<Property *> IPropertyManager::getPropertiesInGroup(const std::string &group) const {
  auto props = getProperties();
  for (auto prop = props.begin(); prop != props.end();) {
    if ((**prop).getGroup() == group) {
      ++prop;
    } else {
      prop = props.erase(prop);
    }
  }
  return props;
}

// Definitions for TypedValue cast operators
// Have to come after getValue definitions above to keep MSVS2010 happy
IPropertyManager::TypedValue::operator int16_t() { return pm.getValue<int16_t>(prop); }
IPropertyManager::TypedValue::operator uint16_t() { return pm.getValue<uint16_t>(prop); }
IPropertyManager::TypedValue::operator int32_t() { return pm.getValue<int32_t>(prop); }
IPropertyManager::TypedValue::operator uint32_t() { return pm.getValue<uint32_t>(prop); }
IPropertyManager::TypedValue::operator int64_t() { return pm.getValue<int64_t>(prop); }
IPropertyManager::TypedValue::operator uint64_t() { return pm.getValue<uint64_t>(prop); }
IPropertyManager::TypedValue::operator bool() { return pm.getValue<bool>(prop); }
IPropertyManager::TypedValue::operator double() { return pm.getValue<double>(prop); }
IPropertyManager::TypedValue::operator std::string() { return pm.getPropertyValue(prop); }
IPropertyManager::TypedValue::operator OptionalBool() { return pm.getValue<OptionalBool>(prop); }
IPropertyManager::TypedValue::operator Property *() { return pm.getPointerToProperty(prop); }

#ifdef __APPLE__
// These must precede the operator() declaration, temporarily switch batch to global namespace
}
DEFINE_IPROPERTYMANAGER_GETVALUE(unsigned long);
DEFINE_IPROPERTYMANAGER_GETVALUE(std::vector<unsigned long>);
// Intel 64-bit size_t
namespace Mantid::Kernel {
IPropertyManager::TypedValue::operator unsigned long() { return pm.getValue<unsigned long>(prop); }
#endif
} // namespace Mantid::Kernel
/// @endcond
