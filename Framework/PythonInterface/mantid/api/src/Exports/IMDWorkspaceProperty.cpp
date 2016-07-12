#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidAPI/IMDWorkspace.h"

using Mantid::API::IMDWorkspace;
using Mantid::API::WorkspaceProperty;

GET_POINTER_SPECIALIZATION(WorkspaceProperty<IMDWorkspace>)

void export_IMDWorkspaceProperty() {
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<IMDWorkspace>::define("IMDWorkspaceProperty");
}
