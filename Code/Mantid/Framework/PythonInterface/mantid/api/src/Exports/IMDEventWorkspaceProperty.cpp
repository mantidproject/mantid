#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidAPI/IMDEventWorkspace.h"

void export_IMDEventWorkspaceProperty()
{
  using Mantid::API::IMDEventWorkspace;
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<IMDEventWorkspace>::define("IMDEventWorkspaceProperty");
}
