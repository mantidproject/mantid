#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidAPI/IMDWorkspace.h"

void export_IMDWorkspaceProperty()
{
  using Mantid::API::IMDWorkspace;
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<IMDWorkspace>::define("IMDWorkspaceProperty");
}
