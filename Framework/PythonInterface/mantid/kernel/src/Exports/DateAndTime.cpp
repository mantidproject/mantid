// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/DateAndTime.h"
#include "MantidPythonInterface/kernel/Converters/DateAndTime.h"
#include <boost/make_shared.hpp>
#include <boost/python/class.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/operators.hpp> // Also provides self
#include <numpy/arrayobject.h>
#include <numpy/arrayscalars.h>

using namespace Mantid::Types::Core;
using namespace Mantid::PythonInterface;
using namespace boost::python;
using boost::python::arg;

/** Circumvent a bug in IPython 1.1, which chokes on nanosecond precision
 * datetimes.
 *  Adding a space to the string returned by the C++ method avoids it being
 * given
 *  the special treatment that leads to the problem.
 */
std::string ISO8601StringPlusSpace(DateAndTime &self) {
  // TODO this should cmake check and turn off the behavior
  return self.toISO8601String() + " ";
}

int64_t total_nanoseconds(DateAndTime &self) {
  PyErr_Warn(
      PyExc_DeprecationWarning,
      ".total_nanoseconds() is deprecated. Use .totalNanoseconds() instead.");
  return self.totalNanoseconds();
}

void export_DateAndTime() {
  class_<DateAndTime>("DateAndTime", no_init)
      // Constructors
      .def(init<const std::string>((arg("self"), arg("ISO8601 string")),
                                   "Construct from an ISO8601 string"))
      .def(init<double, double>(
          (arg("self"), arg("seconds"), arg("nanoseconds")),
          "Construct using a number of seconds and nanoseconds (floats)since "
          "1990-01-01T00:00"))
      .def(init<int64_t, int64_t>(
          (arg("self"), arg("seconds"), arg("nanoseconds")),
          "Construct using a number of seconds and nanoseconds (integers) "
          "since 1990-01-01T00:00"))
      .def(init<int64_t>(
          (arg("self"), arg("total_nanoseconds")),
          "Construct a total number of nanoseconds since 1990-01-01T00:00"))
      .def("__init__",
           make_constructor(Converters::to_dateandtime, default_call_policies(),
                            (arg("other"))),
           "Construct from numpy.datetime64")
      .def("total_nanoseconds", &total_nanoseconds, arg("self"),
           "Since 1990-01-01T00:00")
      .def("totalNanoseconds", &DateAndTime::totalNanoseconds, arg("self"),
           "Since 1990-01-01T00:00")
      .def("toISO8601String", &DateAndTime::toISO8601String, arg("self"),
           "Converts the time into ISO8601Standard and returns the string")
      .def("setToMinimum", &DateAndTime::setToMinimum, arg("self"))
      .def("to_datetime64", &Mantid::PythonInterface::Converters::to_datetime64,
           arg("self"),
           "Convert to numpy.datetime64") // this is panda's name for the
                                          // function
      .def("__str__", &ISO8601StringPlusSpace, arg("self"))
      .def("__long__", &DateAndTime::totalNanoseconds, arg("self"))
      .def("__int__", &DateAndTime::totalNanoseconds, arg("self"))
      .def(self == self)
      .def(self != self)
      // cppcheck-suppress duplicateExpression
      .def(self < self)
      .def(self + int64_t())
      .def(self += int64_t())
      .def(self - int64_t())
      .def(self -= int64_t())
      .def(self - self);
}

long time_duration_total_nanoseconds(time_duration &self) {
  PyErr_Warn(
      PyExc_DeprecationWarning,
      ".total_nanoseconds() is deprecated. Use .totalNanoseconds() instead.");
  return self.total_nanoseconds();
}

void export_time_duration() {
  class_<time_duration>("time_duration", no_init)
      .def("hours", &time_duration::hours, arg("self"),
           "Returns the normalized number of hours")
      .def("minutes", &time_duration::minutes, arg("self"),
           "Returns the normalized number of minutes +/-(0..59)")
      .def("seconds", &time_duration::seconds, arg("self"),
           "Returns the normalized number of seconds +/-(0..59)")
      .def("total_seconds", &time_duration::total_seconds, arg("self"),
           "Get the total number of seconds truncating any fractional seconds")
      .def("total_milliseconds", &time_duration::total_milliseconds,
           arg("self"),
           "Get the total number of milliseconds truncating any remaining "
           "digits")
      .def("total_microseconds", &time_duration::total_microseconds,
           arg("self"),
           "Get the total number of microseconds truncating any remaining "
           "digits")
      .def("total_nanoseconds", &time_duration_total_nanoseconds, arg("self"),
           "Get the total number of nanoseconds truncating any remaining "
           "digits")
      .def("totalNanoseconds", &time_duration::total_nanoseconds, arg("self"),
           "Get the total number of nanoseconds truncating any remaining "
           "digits");
}
