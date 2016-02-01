#include "MantidDataObjects/EventWorkspace.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"

#include <boost/python/class.hpp>
#include <boost/python/object/inheritance.hpp>

using Mantid::API::IEventWorkspace;
using Mantid::DataObjects::EventWorkspace;
using Mantid::DataObjects::EventList;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

void export_EventWorkspace() {
  class_<EventWorkspace, bases<IEventWorkspace>, boost::noncopyable>(
      "EventWorkspace", no_init)
      .def("getEventList", (EventList * (EventWorkspace::*)(const int)) &
                               EventWorkspace::getEventListPtr,
           return_internal_reference<>(), args("self", "workspace_index"),
           "Return the event list managing the events at the given workspace "
           "index")
      .def("padSpectra",
           (void (EventWorkspace::*)(void)) & EventWorkspace::padSpectra,
           args("self"), "Pad pixels in the workspace using the loaded "
                         "spectra. Requires a non-empty spectra-detector map");

  // register pointers
  RegisterWorkspacePtrToPython<EventWorkspace>();
}
