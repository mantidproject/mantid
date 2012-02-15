#include "MantidKernel/IPropertyManager.h"
#include "MantidPythonInterface/kernel/TypeRegistry.h"
#include "MantidPythonInterface/kernel/PropertyValueHandler.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Kernel::IPropertyManager;
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
                   boost::python::object value)
  {
    if( PyString_Check(value.ptr()) ) // String values can be set directly
    {
      self.setPropertyValue(name, boost::python::extract<std::string>(value));
    }
    else
    {
      Mantid::Kernel::Property *p = self.getProperty(name);
      Registry::PropertyValueHandler *entry = Registry::getHandler(*(p->type_info()));
      entry->set(&self, name, value);
    }
  }
}

void export_IPropertyManager()
{
  register_ptr_to_python<IPropertyManager*>();

  class_<IPropertyManager, boost::noncopyable>("IPropertyManager", no_init)
    .def("getProperty", &IPropertyManager::getPointerToProperty, return_value_policy<return_by_value>(),
        "Returns the property of the given name. Use .value to give the value")
    .def("getPropertyValue", &IPropertyManager::getPropertyValue, 
         "Returns a string representation of the named property's value")
    .def("setPropertyValue", &IPropertyManager::setPropertyValue, 
         "Set the value of the named property via a string")
    .def("setProperty", &setProperty, "Set the value of the named property")
    // Special methods to act like dictionary
    .def("__contains__", &IPropertyManager::existsProperty)
    ;
}

