#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidDataObjects/PeaksWorkspace.h"

using Mantid::DataObjects::PeaksWorkspace;
using Mantid::API::WorkspaceProperty; // NOLINT

GET_POINTER_SPECIALIZATION(WorkspaceProperty<PeaksWorkspace>)

void export_IPeaksWorkspaceProperty() {
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<PeaksWorkspace>::define("PeaksWorkspaceProperty");
}
