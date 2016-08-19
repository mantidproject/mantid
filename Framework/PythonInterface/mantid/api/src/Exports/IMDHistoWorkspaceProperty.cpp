#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidAPI/IMDHistoWorkspace.h"

using Mantid::API::IMDHistoWorkspace;
using Mantid::API::WorkspaceProperty;

GET_POINTER_SPECIALIZATION(WorkspaceProperty<IMDHistoWorkspace>)

void export_IMDHistoWorkspaceProperty() {
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<IMDHistoWorkspace>::define(
      "IMDHistoWorkspaceProperty");
}
