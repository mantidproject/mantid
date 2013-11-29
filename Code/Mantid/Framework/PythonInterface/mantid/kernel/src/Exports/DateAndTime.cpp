#include "MantidKernel/DateAndTime.h"
#include <boost/python/class.hpp>
#include <boost/python/operators.hpp> // Also provides self

using Mantid::Kernel::DateAndTime;
using Mantid::Kernel::time_duration;
using namespace boost::python;

/** Circumvent a bug in IPython 1.1, which chokes on nanosecond precision datetimes.
 *  Adding a space to the string returned by the C++ method avoids it being given
 *  the special treatment that leads to the problem.
 */
std::string ISO8601StringPlusSpace(DateAndTime & self)
{
  return self.toISO8601String()+" ";
}

void export_DateAndTime()
{
  class_<DateAndTime>("DateAndTime", no_init)
    // Constructors
    .def(init<const std::string>("Construct from an ISO8601 string"))
    .def(init<double,double>("Construct using a number of seconds and nanoseconds (floats)"))
    .def(init<int64_t,int64_t>("Construct using a number of seconds and nanoseconds (integers)"))
    .def(init<int64_t>("Construct a total number of nanoseconds"))
    .def("total_nanoseconds", &DateAndTime::totalNanoseconds)
    .def("totalNanoseconds", &DateAndTime::totalNanoseconds)
    .def("setToMinimum", &DateAndTime::setToMinimum)
    .def("__str__", &ISO8601StringPlusSpace)
    // cppcheck-suppress duplicateExpression
    .def(self == self)
    // cppcheck-suppress duplicateExpression
    .def(self != self)
    // cppcheck-suppress duplicateExpression
    .def(self < self)
    .def(self + int64_t())
    .def(self += int64_t())
    .def(self - int64_t())
    .def(self -= int64_t())
    // cppcheck-suppress duplicateExpression
    .def(self - self)
  ;
}

void export_time_duration()
{
  class_<time_duration>("time_duration", no_init)
    .def("hours", &time_duration::hours, "Returns the normalized number of hours")
    .def("minutes", &time_duration::minutes, "Returns the normalized number of minutes +/-(0..59)")
    .def("seconds", &time_duration::seconds, "Returns the normalized number of seconds +/-(0..59)")
    .def("total_seconds", &time_duration::total_seconds, "Get the total number of seconds truncating any fractional seconds")
    .def("total_milliseconds", &time_duration::total_milliseconds, "Get the total number of milliseconds truncating any remaining digits")
    .def("total_microseconds", &time_duration::total_microseconds, "Get the total number of microseconds truncating any remaining digits")
    .def("total_nanoseconds", &time_duration::total_nanoseconds, "Get the total number of nanoseconds truncating any remaining digits")

    ;
}
