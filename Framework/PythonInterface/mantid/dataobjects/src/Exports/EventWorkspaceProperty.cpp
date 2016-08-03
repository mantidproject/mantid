#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidDataObjects/EventWorkspace.h"

using Mantid::DataObjects::EventWorkspace;
using Mantid::API::WorkspaceProperty;

GET_POINTER_SPECIALIZATION(WiorkspaceProperty<IEventWorkspace>)

void export_EventWorkspaceProperty() {
  using Mantid::PythonInterface::WorkspacePropertyExporter;

  WorkspacePropertyExporter<EventWorkspace>::define("EventWorkspaceProperty");
}
