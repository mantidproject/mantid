#include "MantidDataObjects/EventList.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

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
           "Create TofEvent and add to EventList.");
}
