#include "MantidKernel/PropertyHistory.h"

#include <boost/python/class.hpp>
#include <boost/python/self.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Kernel::PropertyHistory;
using namespace boost::python;

void export_PropertyHistory() {
  register_ptr_to_python<Mantid::Kernel::PropertyHistory_sptr>();

  class_<PropertyHistory>("PropertyHistory", no_init)

      .def("name", &PropertyHistory::name,
           return_value_policy<copy_const_reference>(),
           "Returns the name of the property.")

      .def("value", &PropertyHistory::value,
           return_value_policy<copy_const_reference>(),
           "Returns the value of the property.")

      .def("type", &PropertyHistory::type,
           return_value_policy<copy_const_reference>(),
           "Returns the type of the property.")

      .def("isDefault", &PropertyHistory::isDefault,
           "Returns if the property value is the default value.")

      .def("direction", &PropertyHistory::direction,
           "Returns the direction of the property.")
      // ----------------- Operators --------------------------------------
      .def(self_ns::str(self));
}
