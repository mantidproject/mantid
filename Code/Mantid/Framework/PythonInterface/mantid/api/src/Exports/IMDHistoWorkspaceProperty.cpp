#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidAPI/IMDHistoWorkspace.h"

// clang-format off
void export_IMDHistoWorkspaceProperty()
// clang-format on
{
  using Mantid::API::IMDHistoWorkspace;
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<IMDHistoWorkspace>::define("IMDHistoWorkspaceProperty");
}
