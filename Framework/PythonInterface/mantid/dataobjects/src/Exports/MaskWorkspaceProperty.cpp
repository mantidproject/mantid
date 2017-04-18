#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidDataObjects/MaskWorkspace.h"

using Mantid::DataObjects::MaskWorkspace;
using Mantid::API::WorkspaceProperty;

GET_POINTER_SPECIALIZATION(WorkspaceProperty<MaskWorkspace>)

void export_MaskWorkspaceProperty() {
  using Mantid::PythonInterface::WorkspacePropertyExporter;

  WorkspacePropertyExporter<MaskWorkspace>::define("MaskWorkspaceProperty");
}