#ifdef _MSC_VER
  #pragma warning( disable: 4250 ) // Disable warning regarding inheritance via dominance, we have no way around it with the design
#endif

#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidPythonInterface/kernel/Registry/TypeRegistry.h"
#include "MantidPythonInterface/kernel/Registry/PropertyValueHandler.h"

#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/copy_const_reference.hpp>

using Mantid::Kernel::IPropertyManager;
using Mantid::Kernel::PropertyManager;
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
  void setProperty(PropertyManager &self, const std::string & name,
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

void export_PropertyManager()
{
  REGISTER_SHARED_PTR_TO_PYTHON(PropertyManager);
  class_<PropertyManager, bases<IPropertyManager>, boost::noncopyable>("PropertyManager")
		  .def("propertyCount", &PropertyManager::propertyCount, "Returns the number of properties being managed")
    	  .def("getProperty", &PropertyManager::getProperty, return_value_policy<return_by_value>(),
    	  		"Returns the property of the given name. Use .value to give the value")
    	  .def("getPropertyValue", &PropertyManager::getPropertyValue,
    	  		"Returns a string representation of the named property's value")
    	  .def("getProperties", &PropertyManager::getProperties, return_value_policy<copy_const_reference>(),
    	  		"Returns the list of properties managed by this object")
    	  .def("setPropertyValue", &PropertyManager::setPropertyValue,
    	  		"Set the value of the named property via a string")
    	  .def("setProperty", &setProperty, "Set the value of the named property")
    	  // Special methods to act like a dictionary
    	  .def("__len__", &PropertyManager::propertyCount)
    	  .def("__contains__", &PropertyManager::existsProperty)
    	  .def("__getitem__", &PropertyManager::getProperty)
    	  ;
}

#ifdef _MSC_VER
  #pragma warning( default: 4250 )
#endif
