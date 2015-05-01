#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IEventList.h"
#include "MantidPythonInterface/kernel/Registry/DataItemInterface.h"

#include <boost/python/class.hpp>
#include <boost/python/object.hpp>

using namespace Mantid::API;
using Mantid::PythonInterface::Registry::DataItemInterface;
using namespace boost::python;

/**
 * Python exports of the Mantid::API::IEventWorkspace class.
 */
// clang-format off
void export_IEventWorkspace()
// clang-format on
{
  class_<IEventWorkspace, bases<Mantid::API::MatrixWorkspace>, boost::noncopyable>("IEventWorkspace", no_init)
    .def("getNumberEvents", &IEventWorkspace::getNumberEvents, args("self"),
         "Returns the number of events in the workspace")
    .def("getTofMin", &IEventWorkspace::getTofMin, args("self"),
         "Returns the minimum TOF value (in microseconds) held by the workspace")
    .def("getTofMax", &IEventWorkspace::getTofMax, args("self"),
         "Returns the maximum TOF value (in microseconds) held by the workspace")
    .def("getEventList", (IEventList*(IEventWorkspace::*)(const int) ) &IEventWorkspace::getEventListPtr,
         return_internal_reference<>(), args("self", "workspace_index"), "Return the event list managing the events at the given workspace index")
    .def("clearMRU", &IEventWorkspace::clearMRU, args("self"), "Clear the most-recently-used lists")
    ;

  DataItemInterface<IEventWorkspace>()
    // map IDs to this interface
    .castFromID("EventWorkspace")
  ;
}

