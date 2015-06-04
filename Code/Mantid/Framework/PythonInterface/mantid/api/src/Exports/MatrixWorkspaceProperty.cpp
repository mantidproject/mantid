#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidAPI/MatrixWorkspace.h"

void export_MatrixWorkspaceProperty() {
  using Mantid::API::MatrixWorkspace;
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<MatrixWorkspace>::define("MatrixWorkspaceProperty");
}
