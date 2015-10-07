#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidAPI/IMDHistoWorkspace.h"

void export_IMDHistoWorkspaceProperty() {
  using Mantid::API::IMDHistoWorkspace;
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<IMDHistoWorkspace>::define(
      "IMDHistoWorkspaceProperty");
}
