#include "MantidKernel/DataItem.h"
#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Kernel::DataItem;
using Mantid::Kernel::DataItem_sptr;
using boost::python::class_;
using boost::python::no_init;

void export_DataItem()
{
  REGISTER_SHARED_PTR_TO_PYTHON(DataItem);

  class_<DataItem,boost::noncopyable>("DataItem", no_init)
    .def("id", &DataItem::id, "The string ID of the class")
    .def("name", &DataItem::name, "The name of the object")
    .def("threadSafe", &DataItem::threadSafe, "Returns true if the object can be accessed safely from multiple threads")
    .def("__str__", &DataItem::name, "Returns the string name of the object if it has been stored")
    .def("__repr__", &DataItem::toString, "Returns a description of the object")
  ;
}

