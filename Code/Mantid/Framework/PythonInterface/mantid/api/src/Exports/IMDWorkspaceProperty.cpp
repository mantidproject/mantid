#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidAPI/IMDWorkspace.h"

// clang-format off
void export_IMDWorkspaceProperty()
// clang-format on
{
  using Mantid::API::IMDWorkspace;
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<IMDWorkspace>::define("IMDWorkspaceProperty");
}
