#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidPythonInterface/kernel/GetPointer.h"

using Mantid::API::WorkspaceProperty; // NOLINT
using Mantid::DataObjects::PeaksWorkspace;

GET_POINTER_SPECIALIZATION(WorkspaceProperty<PeaksWorkspace>)

void export_IPeaksWorkspaceProperty() {
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<PeaksWorkspace>::define("PeaksWorkspaceProperty");
}
