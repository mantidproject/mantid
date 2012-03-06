#include "MantidPythonInterface/api/WorkspacePropertyMacro.h"

void export_MatrixWorkspaceProperty()
{
  using Mantid::API::MatrixWorkspace;
  EXPORT_WORKSPACE_PROPERTY(MatrixWorkspace, "MatrixWorkspaceProperty");
}
