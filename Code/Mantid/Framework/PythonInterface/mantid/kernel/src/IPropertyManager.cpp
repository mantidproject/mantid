#include "MantidKernel/IPropertyManager.h"
#include "MantidPythonInterface/kernel/PropertyMarshal.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Kernel::IPropertyManager;
namespace PropertyMarshal = Mantid::PythonInterface::PropertyMarshal;
using namespace boost::python;

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
    .def("setProperty", &PropertyMarshal::setProperty, "Set the value of the named property")
    // Special methods to act like dicctionary
    .def("__contains__", &IPropertyManager::existsProperty)
    ;
}

