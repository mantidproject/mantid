#include "MantidDataObjects/EventWorkspace.h"
#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidPythonInterface/kernel/GetPointer.h"

using Mantid::API::WorkspaceProperty; // NOLINT
using Mantid::DataObjects::EventWorkspace;

GET_POINTER_SPECIALIZATION(WorkspaceProperty<EventWorkspace>)

void export_EventWorkspaceProperty() {
  using Mantid::PythonInterface::WorkspacePropertyExporter;

  WorkspacePropertyExporter<EventWorkspace>::define("EventWorkspaceProperty");
}
