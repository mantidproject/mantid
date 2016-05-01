#include "MantidKernel/PropertyManagerProperty.h"
#include "MantidKernel/IPropertyManager.h"

namespace Mantid {
namespace Kernel {

// -----------------------------------------------------------------------------
// Public Methods
// -----------------------------------------------------------------------------
/**
 * Constructor with a default empty pointer
 * @param name The name of the property
 * @param direction Direction of the property
 */
PropertyManagerProperty::PropertyManagerProperty(const std::string &name,
                                                 unsigned int direction)
    : PropertyManagerProperty(name, ValueType(), direction) {}

/**
 * Constructor taking an initial value
 * @param name The name of the property
 * @param defaultValue A default value for the property
 * @param direction Direction of the property
 */
PropertyManagerProperty::PropertyManagerProperty(const std::string &name,
                                                 const ValueType &defaultValue,
                                                 unsigned int direction)
    : BaseClass(name, defaultValue, direction) {
  if (name.empty()) {
    throw std::invalid_argument("PropertyManagerProperty() requires a name");
  }
}

// -----------------------------------------------------------------------------
// IPropertyManager::getValue instantiations
// -----------------------------------------------------------------------------

template <>
MANTID_KERNEL_DLL PropertyManager_sptr
IPropertyManager::getValue<PropertyManager_sptr>(
    const std::string &name) const {
  auto *prop = dynamic_cast<PropertyWithValue<PropertyManager_sptr> *>(
      getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected shared_ptr<PropertyManager>.";
    throw std::runtime_error(message);
  }
}

template <>
MANTID_KERNEL_DLL PropertyManager_const_sptr
IPropertyManager::getValue<PropertyManager_const_sptr>(
    const std::string &name) const {
  auto *prop = dynamic_cast<PropertyWithValue<PropertyManager_sptr> *>(
      getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected const shared_ptr<PropertyManager>.";
    throw std::runtime_error(message);
  }
}

} // namespace Kernel
} // namespace Mantid
