#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidAPI/WorkspaceGroup.h"

void export_WorkspaceGroupProperty() {
  using Mantid::API::WorkspaceGroup;
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<WorkspaceGroup>::define("WorkspaceGroupProperty");
}
