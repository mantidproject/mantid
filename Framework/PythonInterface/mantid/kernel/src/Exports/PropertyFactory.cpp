// Import the header for the PropertyWithValueFactory so that we can access 
// the create() functions 
#include "MantidPythonInterface/kernel/Registry/PropertyWithValueFactory.h"

//#include "MantidPythonInterface/kernel/Registry/MappingTypeHandler.h"
#include "MantidPythonInterface/kernel/Registry/TypedPropertyValueHandler.h"
#include "MantidPythonInterface/kernel/Registry/SequenceTypeHandler.h"
//#include "MantidPythonInterface/kernel/GetPointer.h"

// For exporting to the Python side
#include <boost/python/class.hpp>

#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/manage_new_object.hpp>

// Get the Classes to use
using Mantid::PythonInterface::Registry::PropertyWithValueFactory;
using Mantid::Kernel::Property;

// Boost
using namespace boost::python;

// Dummy Class
class DLLExport PropertyFactory {};

//GET_POINTER_SPECIALIZATION(PropertyFactory)

// Dummy helper namespace
namespace helper {
  std::string testFunction() {
    return "Hello!";
  }

  Property* removeUniquePointer(const std::string &name, 
                                const boost::python::object &defaultValue) {

    std::unique_ptr<Property> ptr = PropertyWithValueFactory::createTimeSeries(name, defaultValue);

    Property* raw_ptr = ptr.release();

    return raw_ptr;
  }
}

// Export the PropertyFactory
void export_PropertyFactory() {
  
  /*
  // Try a dummy export
  class_<PropertyFactory>("PropertyFactory")
    .def("testFunction", &helper::testFunction)
    .staticmethod("testFunction");
  */
  //register_ptr_to_python<PropertyFactory *>();

  class_<PropertyFactory, boost::noncopyable>("PropertyFactory", no_init)
    .def("createTimeSeries", &helper::removeUniquePointer,
      arg("log_name"), arg("log_values"), return_value_policy<manage_new_object>())
    .staticmethod("createTimeSeries");
}
