#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidAPI/MatrixWorkspace.h"

// clang-format off
void export_MatrixWorkspaceProperty()
// clang-format on
{
  using Mantid::API::MatrixWorkspace;
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<MatrixWorkspace>::define("MatrixWorkspaceProperty");
}
