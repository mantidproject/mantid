// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/PropertyManagerProperty.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidKernel/PropertyWithValueJSON.h"

#include <json/value.h>

#include <sstream>

namespace Mantid::Kernel {

// -----------------------------------------------------------------------------
// Public Methods
// -----------------------------------------------------------------------------
/**
 * Constructor with a default empty pointer
 * @param name The name of the property
 * @param direction Direction of the property
 */
PropertyManagerProperty::PropertyManagerProperty(const std::string &name, unsigned int direction)
    : PropertyManagerProperty(name, ValueType(), direction) {}

/**
 * Constructor taking an initial value
 * @param name The name of the property
 * @param defaultValue A default value for the property
 * @param direction Direction of the property
 */
PropertyManagerProperty::PropertyManagerProperty(const std::string &name, const ValueType &defaultValue,
                                                 unsigned int direction)
    : BaseClass(name, defaultValue, direction), m_dataServiceKey(), m_defaultAsStr() {
  if (name.empty()) {
    throw std::invalid_argument("PropertyManagerProperty() requires a name");
  }
  if (defaultValue)
    m_defaultAsStr = defaultValue->asString(true);
}

/**
 * @return The value of the property represented as a string
 */
std::string PropertyManagerProperty::value() const {
  if (m_dataServiceKey.empty()) {
    auto mgr = (*this)();
    return (mgr ? mgr->asString(true) : "");
  } else
    return m_dataServiceKey;
}

/**
 * Create a Json::Value objectValue from the PropertyManager
 */
Json::Value PropertyManagerProperty::valueAsJson() const { return (*this)()->asJson(true); }

/**
 *
 * @return The string representation of the default value
 */
std::string PropertyManagerProperty::getDefault() const { return m_defaultAsStr; }

/**
 * Overwrite the current value. The string is expected to contain either:
 *   - the key to a PropertyManager stored in the PropertyManagerDataService
 *   - or json-serialized data, where the properties must already exist on the
 * containing PropertyManager
 * @param strValue A string assumed to contain serialized Json
 * @return If an error occurred then this contains an error message, otherwise
 * an empty string
 */
std::string PropertyManagerProperty::setValue(const std::string &strValue) {
  auto &globalPropMgrs = PropertyManagerDataService::Instance();
  try {
    (*this) = globalPropMgrs.retrieve(strValue);
    m_dataServiceKey = strValue;
    return "";
  } catch (Exception::NotFoundError &) {
    // try the string as json
  }

  auto pmValue = (*this)();
  if (!pmValue) {
    pmValue = std::make_shared<PropertyManager>();
    (*this) = pmValue;
  }
  std::string strValueToSet = strValue.empty() ? "{}" : strValue;
  std::ostringstream msg;
  try {
    const std::unordered_set<std::string> ignored;
    bool createMissing{true};
    pmValue->resetProperties();
    pmValue->setProperties(strValueToSet, ignored, createMissing);
    m_dataServiceKey.clear();
  } catch (std::invalid_argument &exc) {
    msg << "Error setting value from string.\n"
           "String is expected to contain either the name of a global "
           "PropertyManager or a json-formatted object.\n"
           "Parser error: "
        << exc.what();
  } catch (std::exception &exc) {
    msg << "Error setting value from string.\n" << exc.what();
  }
  return msg.str();
}

/**
 * Sets the value of the property from a Json object
 * @param value A Json objectValue
 * @return An empty string indicating success otherwise a help message
 * indicating what error has occurred
 */
std::string PropertyManagerProperty::setValueFromJson(const Json::Value &value) {
  if (value.type() == Json::objectValue) {
    try {
      *this = createPropertyManager(value);
      return "";
    } catch (std::exception &exc) {
      return std::string("Attempt to set Json value to property failed: ") + exc.what();
    }
  } else {
    return std::string("Attempt to set incorrect Json type to "
                       "PropertyManager. Expected objectValue, found " +
                       std::to_string(value.type()));
  }
}

// -----------------------------------------------------------------------------
// IPropertyManager::getValue instantiations
// -----------------------------------------------------------------------------
template <>
MANTID_KERNEL_DLL PropertyManager_sptr IPropertyManager::getValue<PropertyManager_sptr>(const std::string &name) const {
  auto *prop = dynamic_cast<PropertyWithValue<PropertyManager_sptr> *>(getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    throw std::runtime_error(std::string("Attempt to assign property ") + name +
                             " to incorrect type. Expected shared_ptr<PropertyManager>.");
  }
}

template <>
MANTID_KERNEL_DLL PropertyManager_const_sptr
IPropertyManager::getValue<PropertyManager_const_sptr>(const std::string &name) const {
  auto *prop = dynamic_cast<PropertyWithValue<PropertyManager_sptr> *>(getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name + " to incorrect type. Expected const shared_ptr<PropertyManager>.";
    throw std::runtime_error(message);
  }
}

} // namespace Mantid::Kernel
