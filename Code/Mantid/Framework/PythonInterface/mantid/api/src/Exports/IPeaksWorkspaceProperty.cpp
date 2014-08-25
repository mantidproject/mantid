#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidAPI/IPeaksWorkspace.h"

void export_IPeaksWorkspaceProperty()
{
  using Mantid::API::IPeaksWorkspace;
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<IPeaksWorkspace>::define("IPeaksWorkspaceProperty");
}
