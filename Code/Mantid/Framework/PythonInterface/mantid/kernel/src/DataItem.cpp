#include "MantidKernel/DataItem.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Kernel::DataItem;
using Mantid::Kernel::DataItem_sptr;
using boost::python::class_;
using boost::python::no_init;

void export_DataItem()
{
  boost::python::register_ptr_to_python<DataItem_sptr>();

  class_<DataItem,boost::noncopyable>("DataItem", no_init)
    .def("id", &DataItem::id, "The string ID of the class")
    .def("name", &DataItem::name, "The name of the object")
    .def("threadSafe", &DataItem::threadSafe, "Returns true if the object can be accessed safely from multiple threads")
    .def("__str__", &DataItem::toString, "Returns a serialised version of the object")
  ;
}

