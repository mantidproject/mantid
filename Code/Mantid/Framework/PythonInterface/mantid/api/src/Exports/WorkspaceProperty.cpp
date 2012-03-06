#include "MantidPythonInterface/api/WorkspacePropertyMacro.h"
#include "MantidAPI/Workspace.h"

void export_WorkspaceProperty()
{
  using Mantid::API::Workspace;
  EXPORT_WORKSPACE_PROPERTY(Workspace, "WorkspaceProperty");
}
