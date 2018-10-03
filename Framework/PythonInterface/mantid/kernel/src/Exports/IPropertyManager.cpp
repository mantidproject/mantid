// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidPythonInterface/kernel/Converters/PyObjectToString.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/PropertyValueHandler.h"
#include "MantidPythonInterface/kernel/Registry/PropertyWithValueFactory.h"
#include "MantidPythonInterface/kernel/Registry/TypeRegistry.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/iterator.hpp>
#include <boost/python/list.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_internal_reference.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/python/str.hpp>

using namespace Mantid::Kernel;
namespace Registry = Mantid::PythonInterface::Registry;
namespace Converters = Mantid::PythonInterface::Converters;
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
void setProperty(IPropertyManager &self, const boost::python::object &name,
                 const boost::python::object &value) {
  std::string namestr;
  try {
    namestr = Converters::pyObjToStr(name);
  } catch (std::invalid_argument &) {
    throw std::invalid_argument("Failed to convert property name to a string");
  }

  extract<std::string> valuecpp(value);
  if (valuecpp.check()) {
    self.setPropertyValue(namestr, valuecpp());
#if PY_VERSION_HEX < 0x03000000
  } else if (PyUnicode_Check(value.ptr())) {
    self.setPropertyValue(namestr,
                          extract<std::string>(str(value).encode("utf-8"))());
#endif
  } else {
    try {
      Property *p = self.getProperty(namestr);
      const auto &entry = Registry::TypeRegistry::retrieve(*(p->type_info()));
      entry.set(&self, namestr, value);
    } catch (std::invalid_argument &e) {
      throw std::invalid_argument("When converting parameter \"" + namestr +
                                  "\": " + e.what());
    }
  }
}

void setProperties(IPropertyManager &self, const boost::python::dict &kwargs) {
#if PY_MAJOR_VERSION >= 3
  const object view = kwargs.attr("items")();
  const object objectItems(handle<>(PyObject_GetIter(view.ptr())));
#else
  const object objectItems = kwargs.iteritems();
#endif
  auto begin = stl_input_iterator<object>(objectItems);
  auto end = stl_input_iterator<object>();
  for (auto it = begin; it != end; ++it) {
    setProperty(self, (*it)[0], (*it)[1]);
  }
}

/**
 * Create a new property from the value within the boost::python object
 * It is equivalent to a python method that starts with 'self'
 * @param self :: A reference to the calling object
 * @param name :: The name of the property
 * @param value :: The value of the property as a bpl object
 */
void declareProperty(IPropertyManager &self, const boost::python::object &name,
                     boost::python::object value) {
  std::string nameStr = Converters::pyObjToStr(name);
  auto p = std::unique_ptr<Property>(
      Registry::PropertyWithValueFactory::create(nameStr, value, 0));
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
void declareOrSetProperty(IPropertyManager &self,
                          const boost::python::object &name,
                          boost::python::object value) {
  std::string nameStr = Converters::pyObjToStr(name);
  bool propExists = self.existsProperty(nameStr);
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

/**
 * Retrieve the property with the specified name (key) in the
 * IPropertyManager. If no property exists with the specified
 * name, return the specified default value.
 *
 * @param self  The calling IPropertyManager object
 * @param name  The name (key) of the property to retrieve
 * @param value The default value to return if no property
 *              exists with the specified key.
 * @return      The property with the specified key. If no
 *              such property exists, return the default value.
 */
Property *get(IPropertyManager &self, const std::string &name,
              const boost::python::object &value) {
  try {
    return self.getPointerToProperty(name);
  } catch (Exception::NotFoundError &) {
    return Registry::PropertyWithValueFactory::create(name, value, 0).release();
  }
}
} // namespace

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
      .def("setProperties", &setProperties, (arg("self"), arg("kwargs")),
           "Set a collection of properties from a dict")

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
           "Returns the list of properties managed by this object")
      .def("get", &get, (arg("self"), arg("name"), arg("value")),
           return_value_policy<return_by_value>(),
           "Returns the property of the given name. Use .value to give the "
           "value. If property with given name does not exist, returns given "
           "default value.");
}
