#include "MantidPythonInterface/api/WorkspacePropertyMacro.h"
#include "MantidAPI/IMDHistoWorkspace.h"

void export_IMDHistoWorkspaceProperty()
{
  using Mantid::API::IMDHistoWorkspace;
  EXPORT_WORKSPACE_PROPERTY(IMDHistoWorkspace, "IMDHistoWorkspaceProperty");
}