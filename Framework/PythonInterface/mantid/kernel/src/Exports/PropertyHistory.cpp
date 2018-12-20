#include "MantidKernel/PropertyHistory.h"
#include "MantidPythonInterface/kernel/GetPointer.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/self.hpp>

using Mantid::Kernel::PropertyHistory;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(PropertyHistory)

void export_PropertyHistory() {
  register_ptr_to_python<Mantid::Kernel::PropertyHistory_sptr>();

  class_<PropertyHistory>("PropertyHistory", no_init)

      .def("name", &PropertyHistory::name, arg("self"),
           return_value_policy<copy_const_reference>(),
           "Returns the name of the property.")

      .def("value", &PropertyHistory::value, arg("self"),
           return_value_policy<copy_const_reference>(),
           "Returns the value of the property.")

      .def("type", &PropertyHistory::type, arg("self"),
           return_value_policy<copy_const_reference>(),
           "Returns the type of the property.")

      .def("isDefault", &PropertyHistory::isDefault, arg("self"),
           "Returns if the property value is the default value.")

      .def("direction", &PropertyHistory::direction, arg("self"),
           "Returns the direction of the property.")
      // ----------------- Operators --------------------------------------
      .def(self_ns::str(self));
}
