#include "MantidKernel/DateAndTime.h"
#include <boost/python/class.hpp>
#include <boost/python/operators.hpp> // Also provides self

using Mantid::Kernel::DateAndTime;
using namespace boost::python;

void export_DateAndTime()
{

  class_<DateAndTime>("DateAndTime", no_init)
    .def("total_nanoseconds", &DateAndTime::total_nanoseconds)
    .def("__str__", &Mantid::Kernel::DateAndTime::to_ISO8601_string)
    // cppcheck-suppress duplicateExpression
    .def(self == self)
    // cppcheck-suppress duplicateExpression
    .def(self != self)
    // cppcheck-suppress duplicateExpression
    .def(self < self)
    .def(self + int64_t())
    .def(self += int64_t())
    .def(self - int64_t())
    .def(self -= int64_t());

}

