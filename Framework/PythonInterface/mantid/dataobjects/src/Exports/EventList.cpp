// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/EventList.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_arg.hpp>

using namespace boost::python;
using namespace Mantid::DataObjects;

GET_POINTER_SPECIALIZATION(EventList)

namespace {
void addEventToEventList(EventList &self, double tof,
                         Mantid::Types::Core::DateAndTime pulsetime) {
  self.addEventQuickly(Mantid::Types::Event::TofEvent(tof, pulsetime));
}
} // namespace

void export_EventList() {
  register_ptr_to_python<EventList *>();

  class_<EventList, bases<Mantid::API::IEventList>, boost::noncopyable>(
      "EventList")
      .def("addEventQuickly", &addEventToEventList,
           args("self", "tof", "pulsetime"),
           "Create TofEvent and add to EventList.")
      .def("__iadd__",
           (EventList & (EventList::*)(const EventList &)) &
               EventList::operator+=,
           return_self<>(), (arg("self"), arg("other")))
      .def("__isub__",
           (EventList & (EventList::*)(const EventList &)) &
               EventList::operator-=,
           return_self<>(), (arg("self"), arg("other")));
}
