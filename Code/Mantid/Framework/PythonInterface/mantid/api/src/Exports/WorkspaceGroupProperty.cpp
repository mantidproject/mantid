#include "MantidPythonInterface/api/WorkspacePropertyMacro.h"
#include "MantidAPI/WorkspaceGroup.h"

void export_WorkspaceGroupProperty()
{
  using Mantid::API::WorkspaceGroup;
  EXPORT_WORKSPACE_PROPERTY(WorkspaceGroup, "WorkspaceGroupProperty");
}
