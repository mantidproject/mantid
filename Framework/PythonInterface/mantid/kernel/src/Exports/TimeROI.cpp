// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/TimeROI.h"
#include "MantidTypes/Core/DateAndTime.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/list.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/tuple.hpp>

using Mantid::Kernel::TimeROI;
using Mantid::Types::Core::DateAndTime;
using namespace boost::python;

boost::python::list getTimeIntervals(const TimeROI &self) {
  boost::python::list times;
  for (const auto &splitter : self.toTimeIntervals()) {
    times.append(make_tuple(splitter.start(), splitter.stop()));
  }
  return times;
}

void export_TimeROI() {
  register_ptr_to_python<TimeROI *>();

  class_<TimeROI>("TimeROI", no_init)
      .def("durationInSeconds", (double (TimeROI::*)() const) & TimeROI::durationInSeconds, arg("self"),
           "Duration of the whole TimeROI")
      .def("durationInSeconds",
           (double (TimeROI::*)(const DateAndTime &, const DateAndTime &) const) & TimeROI::durationInSeconds,
           (arg("self"), arg("startTime"), arg("stopTime")), "Duration of the TimeROI between startTime and stopTime")
      .def("update_union", &TimeROI::update_union, (arg("self"), arg("other")),
           return_value_policy<copy_const_reference>(),
           "Updates the TimeROI values with the union with another TimeROI."
           "See https://en.wikipedia.org/wiki/Union_(set_theory) for more details")
      .def("update_intersection", &TimeROI::update_intersection, (arg("self"), arg("other")),
           return_value_policy<copy_const_reference>(),
           "Updates the TimeROI values with the intersection with another TimeROI."
           "See https://en.wikipedia.org/wiki/Intersection for more details")
      .def("useAll", &TimeROI::useAll, "True if all times are use")
      .def("useNone", &TimeROI::useNone, "True if all times are ignore")
      .def("numberOfRegions", &TimeROI::numberOfRegions, arg("self"), "Number of regions of interest")
      .def("firstTime", &TimeROI::firstTime,
           "First time in the object. Will generate an execption if useAll() is True.")
      .def("lastTime", &TimeROI::lastTime, "Last time in the object. Will generate an execption if useAll() is True.")
      .def("toTimeIntervals", &getTimeIntervals, arg("self"),
           "Returns a list of start and stop times for all time intervals");
}
