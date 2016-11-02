#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/TypeRegistry.h"
#include "MantidPythonInterface/kernel/Registry/PropertyValueHandler.h"
#include "MantidPythonInterface/kernel/Registry/PropertyWithValueFactory.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/iterator.hpp>
#include <boost/python/list.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_internal_reference.hpp>

using namespace Mantid::Kernel;
namespace Registry = Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(IPropertyManager)

namespace {
/**
 * Set the value of a property from the value within the
 * boost::python object
 * It is equivalent to a python method that starts with 'self'
 * @param self :: A reference to the calling object
 * @param name :: The name of the property
 * @param value :: The value of the property as a bpl object
 */
void setProperty(IPropertyManager &self, const std::string &name,
                 const boost::python::object &value) {
  typedef extract<std::string> from_pystr;
  // String values can be set directly
  from_pystr cppstr(value);
  if (cppstr.check()) {
    self.setPropertyValue(name, cppstr());
  } else {
    try {
      Property *p = self.getProperty(name);
      const auto &entry = Registry::TypeRegistry::retrieve(*(p->type_info()));
      entry.set(&self, name, value);
    } catch (std::invalid_argument &e) {
      throw std::invalid_argument("When converting parameter \"" + name +
                                  "\": " + e.what());
    }
  }
}

/**
 * Create a new property from the value within the boost::python object
 * It is equivalent to a python method that starts with 'self'
 * @param self :: A reference to the calling object
 * @param name :: The name of the property
 * @param value :: The value of the property as a bpl object
 */
void declareProperty(IPropertyManager &self, const std::string &name,
                     boost::python::object value) {
  auto p = std::unique_ptr<Property>(
      Registry::PropertyWithValueFactory::create(name, value, 0));
  self.declareProperty(std::move(p));
}

/**
 * Create or set a property from the value within the boost::python object
 * It is equivalent to a python method that starts with 'self' and allows
 * python dictionary type usage.
 * @param self :: A reference to the calling object
 * @param name :: The name of the property
 * @param value :: The value of the property as a bpl object
 */
void declareOrSetProperty(IPropertyManager &self, const std::string &name,
                          boost::python::object value) {
  bool propExists = self.existsProperty(name);
  if (propExists) {
    setProperty(self, name, value);
  } else {
    declareProperty(self, name, value);
  }
}

/**
 * Clones the given settingsManager and passes it on to the calling object as it
 * takes ownership
 * of the IPropertySettings object
 * @param self The calling object
 * @param propName A property name that will pick up the settings manager
 * @param settingsManager The actual settings object
 */
void setPropertySettings(IPropertyManager &self, const std::string &propName,
                         IPropertySettings *settingsManager) {
  self.setPropertySettings(
      propName, std::unique_ptr<IPropertySettings>(settingsManager->clone()));
}

void deleteProperty(IPropertyManager &self, const std::string &propName) {
  self.removeProperty(propName);
}

/**
 * Return a PyList of all the keys in the IPropertyManager.
 * @param self The calling object
 * @return The list of keys.
 */
boost::python::list getKeys(IPropertyManager &self) {
  const std::vector<Property *> &props = self.getProperties();
  const size_t numProps = props.size();

  boost::python::list result;
  for (size_t i = 0; i < numProps; ++i) {
    result.append(props[i]->name());
  }

  return result;
}
}

void export_IPropertyManager() {
  register_ptr_to_python<IPropertyManager *>();

  class_<IPropertyManager, boost::noncopyable>("IPropertyManager", no_init)
      .def("propertyCount", &IPropertyManager::propertyCount, arg("self"),
           "Returns the number of properties being managed")

      .def("getProperty", &IPropertyManager::getPointerToProperty,
           (arg("self"), arg("name")), return_value_policy<return_by_value>(),
           "Returns the property of the given name. Use .value to give the "
           "value")

      .def("getPropertyValue", &IPropertyManager::getPropertyValue,
           (arg("self"), arg("name")),
           "Returns a string representation of the named property's value")

      .def("getProperties", &IPropertyManager::getProperties, arg("self"),
           return_value_policy<copy_const_reference>(),
           "Returns the list of properties managed by this object")

      .def("declareProperty", &declareProperty,
           (arg("self"), arg("name"), arg("value")),
           "Create a new named property")

      .def("setPropertyValue", &IPropertyManager::setPropertyValue,
           (arg("self"), arg("name"), arg("value")),
           "Set the value of the named property via a string")

      .def("setProperty", &setProperty,
           (arg("self"), arg("name"), arg("value")),
           "Set the value of the named property")

      .def("setPropertySettings", &setPropertySettings,
           (arg("self"), arg("name"), arg("settingsManager")),
           "Assign the given IPropertySettings object to the  named property")

      .def("setPropertyGroup", &IPropertyManager::setPropertyGroup,
           (arg("self"), arg("name"), arg("group")),
           "Set the group for a given property")

      .def("existsProperty", &IPropertyManager::existsProperty,
           (arg("self"), arg("name")), "Returns whether a property exists")

      // Special methods so that IPropertyManager acts like a dictionary
      // __len__, __getitem__, __setitem__, __delitem__, __iter__ and
      // __contains__
      .def("__len__", &IPropertyManager::propertyCount, arg("self"),
           "Returns the number of properties being managed")
      .def("__getitem__", &IPropertyManager::getPointerToProperty,
           (arg("self"), arg("name")), return_value_policy<return_by_value>(),
           "Returns the property of the given name. Use .value to give the "
           "value")
      .def("__setitem__", &declareOrSetProperty,
           (arg("self"), arg("name"), arg("value")),
           "Set the value of the named property or create it if it doesn't "
           "exist")
      .def("__delitem__", &deleteProperty, (arg("self"), arg("name")),
           "Delete the named property")
      // TODO   .def("__iter__", iterator<std::vector<std::string> > ())
      .def("__contains__", &IPropertyManager::existsProperty,
           (arg("self"), arg("name")), "Returns whether a property exists")

      // Bonus methods to be even more like a dict
      .def("has_key", &IPropertyManager::existsProperty,
           (arg("self"), arg("name")), "Returns whether a property exists")
      .def("keys", &getKeys, arg("self"))
      .def("values", &IPropertyManager::getProperties, arg("self"),
           return_value_policy<copy_const_reference>(),
           "Returns the list of properties managed by this object");
}
