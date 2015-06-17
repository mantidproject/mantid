#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidAPI/ISplittersWorkspace.h"

void export_ISplittersWorkspaceProperty() {
  using Mantid::API::ISplittersWorkspace;
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<ISplittersWorkspace>::define("ISplittersWorkspaceProperty");
}
