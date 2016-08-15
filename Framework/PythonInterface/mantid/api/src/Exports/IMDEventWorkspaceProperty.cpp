#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidAPI/IMDEventWorkspace.h"

using Mantid::API::IMDEventWorkspace;
using Mantid::API::WorkspaceProperty;

GET_POINTER_SPECIALIZATION(WorkspaceProperty<IMDEventWorkspace>)

void export_IMDEventWorkspaceProperty() {
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<IMDEventWorkspace>::define(
      "IMDEventWorkspaceProperty");
}
