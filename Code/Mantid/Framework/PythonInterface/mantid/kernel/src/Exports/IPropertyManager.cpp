#include "MantidKernel/IPropertyManager.h"
#include "MantidPythonInterface/kernel/Registry/TypeRegistry.h"
#include "MantidPythonInterface/kernel/Registry/PropertyValueHandler.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/copy_const_reference.hpp>

using namespace Mantid::Kernel;
namespace Registry = Mantid::PythonInterface::Registry;
using namespace boost::python;

namespace
{
  /**
   * Set the value of a property from the value within the
   * boost::python object
   * It is equivalent to a python method that starts with 'self'
   * @param self :: A reference to the calling object
   * @param name :: The name of the property
   * @param value :: The value of the property as a bpl object
   */
  void setProperty(IPropertyManager &self, const std::string & name,
                   const boost::python::object & value)
  {
    if( PyString_Check(value.ptr()) ) // String values can be set directly
    {
      self.setPropertyValue(name, boost::python::extract<std::string>(value));
    }
    else
    {
      try {
        Mantid::Kernel::Property *p = self.getProperty(name);
        const auto &entry  = Registry::TypeRegistry::retrieve(*(p->type_info()));
        entry.set(&self, name, value);
      }
      catch (std::invalid_argument &e)
      {
        throw std::invalid_argument("When converting parameter \"" + name + "\": " + e.what());
      }
    }
  }

  /**
   * Clones the given settingsManager and passes it on to the calling object as it takes ownership
   * of the IPropertySettings object
   * @param self The calling object
   * @param propName A property name that will pick up the settings manager
   * @param settingsManager The actual settings object
   */
  void setPropertySettings(IPropertyManager &self, const std::string & propName,
                           IPropertySettings *settingsManager)
  {
    self.setPropertySettings(propName, settingsManager->clone());
  }
}

void export_IPropertyManager()
{
  register_ptr_to_python<IPropertyManager*>();

  class_<IPropertyManager, boost::noncopyable>("IPropertyManager", no_init)
    .def("propertyCount", &IPropertyManager::propertyCount, "Returns the number of properties being managed")

    .def("getProperty", &IPropertyManager::getPointerToProperty, return_value_policy<return_by_value>(),
        "Returns the property of the given name. Use .value to give the value")

    .def("getPropertyValue", &IPropertyManager::getPropertyValue, 
         "Returns a string representation of the named property's value")

    .def("getProperties", &IPropertyManager::getProperties, return_value_policy<copy_const_reference>(),
         "Returns the list of properties managed by this object")

    .def("setPropertyValue", &IPropertyManager::setPropertyValue, 
         "Set the value of the named property via a string")

    .def("setProperty", &setProperty, "Set the value of the named property")

    .def("setPropertySettings", &setPropertySettings,
         "Assign the given IPropertySettings object to the  named property")

    .def("setPropertyGroup", &IPropertyManager::setPropertyGroup, "Set the group for a given property")

    .def("existsProperty", &IPropertyManager::existsProperty,
        "Returns whether a property exists")

        // Special methods so that IPropertyManager acts like a dictionary
    .def("__len__", &IPropertyManager::propertyCount)
    .def("__contains__", &IPropertyManager::existsProperty)
    .def("__getitem__", &IPropertyManager::getProperty)
    ;
  }

