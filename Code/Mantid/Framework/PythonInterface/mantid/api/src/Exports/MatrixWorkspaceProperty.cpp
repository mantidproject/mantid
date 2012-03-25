#include "MantidPythonInterface/api/WorkspacePropertyMacro.h"
#include "MantidAPI/MatrixWorkspace.h"

void export_MatrixWorkspaceProperty()
{
  using Mantid::API::MatrixWorkspace;
  EXPORT_WORKSPACE_PROPERTY(MatrixWorkspace, "MatrixWorkspaceProperty");
}
