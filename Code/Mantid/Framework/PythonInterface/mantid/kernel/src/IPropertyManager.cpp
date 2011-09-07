#include "MantidKernel/IPropertyManager.h"
#include "MantidPythonInterface/kernel/PropertyMarshal.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Kernel::IPropertyManager;
using Mantid::PythonInterface::PropertyMarshal;
using namespace boost::python;

void export_IPropertyManager()
{
  register_ptr_to_python<IPropertyManager*>();
  class_<IPropertyManager, boost::noncopyable>("IPropertyManager", no_init)
    .def("get_property", &IPropertyManager::getPointerToProperty, return_value_policy<return_by_value>(),
        "Returns the property of the given name. Use .value to give the value")
    .def("set_property_value", &IPropertyManager::setPropertyValue, "Set the value of the named property via a string")
    .def("set_property", &PropertyMarshal::setProperty, "Set the value of the named property")
    ;
}

