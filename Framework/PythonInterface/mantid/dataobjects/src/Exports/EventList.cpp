#include "MantidDataObjects/EventList.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using namespace boost::python;
using namespace Mantid::DataObjects;

namespace {
void addEventToEventList(EventList &self, double tof,
                         Mantid::Kernel::DateAndTime pulsetime) {
  self.addEventQuickly(Mantid::DataObjects::TofEvent(tof, pulsetime));
}
}

void export_EventList() {
  register_ptr_to_python<EventList *>();

  class_<EventList, bases<Mantid::API::IEventList>, boost::noncopyable>(
      "EventList")
      .def("addEventQuickly", &addEventToEventList,
           args("self", "tof", "pulsetime"),
           "Create TofEvent and add to EventList.");
}
