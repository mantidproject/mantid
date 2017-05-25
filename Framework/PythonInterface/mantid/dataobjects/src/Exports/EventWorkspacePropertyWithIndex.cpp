#include "MantidDataObjects/EventWorkspace.h"
#include "MantidPythonInterface/api/WorkspacePropertyWithIndexExporter.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include <boost/python/enum.hpp>

using Mantid::DataObjects::EventWorkspace;
using Mantid::API::WorkspacePropertyWithIndex;

GET_POINTER_SPECIALIZATION(WorkspacePropertyWithIndex<EventWorkspace>)

void export_EventWorkspacePropertyWithIndex() {
  using Mantid::PythonInterface::WorkspacePropertyWithIndexExporter;
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<EventWorkspace>::define("EventWorkspaceProperty");
  WorkspacePropertyWithIndexExporter<EventWorkspace>::define(
      "EventWorkspacePropertyWithIndex");
}
