#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"

void export_WorkspaceGroupProperty() {
  using Mantid::API::WorkspaceGroup;
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<WorkspaceGroup>::define("WorkspaceGroupProperty");
}
