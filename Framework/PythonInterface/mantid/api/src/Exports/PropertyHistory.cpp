#include "MantidKernel/PropertyHistory.h"
#include "MantidAPI/IAlgorithm.h"

#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/self.hpp>

using Mantid::API::IAlgorithm;
using Mantid::Kernel::PropertyHistory;
using namespace boost::python;

void export_PropertyHistory() {
  register_ptr_to_python<PropertyHistory *>();

  class_<PropertyHistory, boost::noncopyable>("PropertyHistory", no_init)
      .def("name", &PropertyHistory::name, "Returns the name of the property.")
      .def("value", &PropertyHistory::value,
           "Returns the value of the property.")
      .def("type", &PropertyHistory::type, "Returns the type of the property.")
      .def("isDefault", &PropertyHistory::isDefault,
           "Returns if the property value is the default value.")
      .def("direction", &PropertyHistory::direction,
           "Returns the direction of the property.")
      // ----------------- Operators --------------------------------------
      .def(self_ns::str(self));
}
