#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IEventList.h"
#include "MantidPythonInterface/kernel/SingleValueTypeHandler.h"

#include <boost/python/class.hpp>

using Mantid::API::IEventWorkspace;
using Mantid::API::IEventList;
using namespace boost::python;

void export_IEventWorkspace()
{
  class_<IEventWorkspace, bases<Mantid::API::MatrixWorkspace>, boost::noncopyable>("IEventWorkspace", no_init)
    .def("get_number_events", &IEventWorkspace::getNumberEvents, "Returns the number of events in the workspace")
    .def("get_tof_min", &IEventWorkspace::getTofMin, "Returns the minimum TOF value (in microseconds) held by the workspace")
    .def("get_tof_max", &IEventWorkspace::getTofMax, "Returns the maximum TOF value (in microseconds) held by the workspace")
    .def("get_event_list", (IEventList*(IEventWorkspace::*)(const int) ) &IEventWorkspace::getEventListPtr,
        return_internal_reference<>(), "Return the event list managing the events at the given workspace index")
    .def("clear_mru", &IEventWorkspace::clearMRU, "Clear the most-recently-used lists")

    ;

  DECLARE_SINGLEVALUETYPEHANDLER(IEventWorkspace, Mantid::Kernel::DataItem_sptr);
}
