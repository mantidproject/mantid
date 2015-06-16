#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidAPI/IEventWorkspace.h"

void export_IEventWorkspaceProperty() {
  using Mantid::API::IEventWorkspace;
  using Mantid::PythonInterface::WorkspacePropertyExporter;

  WorkspacePropertyExporter<IEventWorkspace>::define("IEventWorkspaceProperty");
}
