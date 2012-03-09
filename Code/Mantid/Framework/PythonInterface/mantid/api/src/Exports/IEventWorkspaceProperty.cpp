#include "MantidPythonInterface/api/WorkspacePropertyMacro.h"
#include "MantidAPI/IEventWorkspace.h"

void export_IEventWorkspaceProperty()
{
  using Mantid::API::IEventWorkspace;
  EXPORT_WORKSPACE_PROPERTY(IEventWorkspace, "IEventWorkspaceProperty");
}
