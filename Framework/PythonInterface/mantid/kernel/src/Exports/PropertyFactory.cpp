#include "MantidPythonInterface/kernel/Registry/PropertyWithValueFactory.h"
#include "MantidPythonInterface/kernel/Registry/SequenceTypeHandler.h"
#include "MantidPythonInterface/kernel/Registry/TypedPropertyValueHandler.h"

#include <boost/python/class.hpp>
#include <boost/python/manage_new_object.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::Kernel::Property;
using Mantid::PythonInterface::Registry::PropertyWithValueFactory;
using namespace boost::python;

// Empty class definition
class PropertyFactory {};

// Helper namespace
namespace propertyFactoryHelper {

// Helper function to remove unique pointer and return a raw pointer
Property *removeUniquePointer(const std::string &name,
                              const boost::python::list &defaultValue) {

  // Get the unique pointer from the factory and convert it into a raw pointer
  auto ptr = PropertyWithValueFactory::createTimeSeries(name, defaultValue);

  return ptr.release();
}
} // namespace propertyFactoryHelper

// Export the PropertyFactory
void export_PropertyFactory() {
  class_<PropertyFactory, boost::noncopyable>("PropertyFactory", no_init)
      .def("createTimeSeries", &propertyFactoryHelper::removeUniquePointer,
           arg("log_name"), arg("log_values"),
           return_value_policy<manage_new_object>())
      .staticmethod("createTimeSeries");
}
