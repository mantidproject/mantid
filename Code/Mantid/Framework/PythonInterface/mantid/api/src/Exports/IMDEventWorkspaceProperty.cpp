#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidAPI/IMDEventWorkspace.h"

// clang-format off
void export_IMDEventWorkspaceProperty()
// clang-format on
{
  using Mantid::API::IMDEventWorkspace;
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<IMDEventWorkspace>::define("IMDEventWorkspaceProperty");
}
