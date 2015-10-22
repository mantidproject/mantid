#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidAPI/ITableWorkspace.h"

void export_ITableWorkspaceProperty() {
  using Mantid::API::ITableWorkspace;
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<ITableWorkspace>::define("ITableWorkspaceProperty");
}
