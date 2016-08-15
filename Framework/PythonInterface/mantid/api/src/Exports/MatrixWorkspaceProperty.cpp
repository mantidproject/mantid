#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidAPI/MatrixWorkspace.h"

using Mantid::API::MatrixWorkspace;
using Mantid::API::WorkspaceProperty;

GET_POINTER_SPECIALIZATION(WorkspaceProperty<MatrixWorkspace>)

void export_MatrixWorkspaceProperty() {
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<MatrixWorkspace>::define("MatrixWorkspaceProperty");
}
