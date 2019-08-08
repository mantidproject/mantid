// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/DataItem.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::Kernel::DataItem;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(DataItem)

void export_DataItem() {
  register_ptr_to_python<boost::shared_ptr<DataItem>>();

  class_<DataItem, boost::noncopyable>("DataItem", no_init)
      .def("id", &DataItem::id, arg("self"), "The string ID of the class")
      .def("name", &DataItem::getName, arg("self"), "The name of the object",
           return_value_policy<copy_const_reference>())
      .def("threadSafe", &DataItem::threadSafe, arg("self"),
           "Returns true if the object "
           "can be accessed safely from "
           "multiple threads")
      .def("__str__", &DataItem::getName, arg("self"),
           "Returns the string name of the object if it has been stored",
           return_value_policy<copy_const_reference>())
      .def("__repr__", &DataItem::toString, arg("self"),
           "Returns a description of the object");
}
