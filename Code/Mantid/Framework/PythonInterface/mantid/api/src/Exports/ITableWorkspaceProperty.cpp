#include "MantidPythonInterface/api/WorkspacePropertyMacro.h"
#include "MantidAPI/ITableWorkspace.h"

void export_ITableWorkspaceProperty()
{
  using Mantid::API::ITableWorkspace;
  EXPORT_WORKSPACE_PROPERTY(ITableWorkspace, "ITableWorkspaceProperty");
}
