#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidPythonInterface/kernel/GetPointer.h"

using Mantid::API::WorkspaceProperty; // NOLINT
using Mantid::DataObjects::MaskWorkspace;

GET_POINTER_SPECIALIZATION(WorkspaceProperty<MaskWorkspace>)

void export_MaskWorkspaceProperty() {
  using Mantid::PythonInterface::WorkspacePropertyExporter;

  WorkspacePropertyExporter<MaskWorkspace>::define("MaskWorkspaceProperty");
}
