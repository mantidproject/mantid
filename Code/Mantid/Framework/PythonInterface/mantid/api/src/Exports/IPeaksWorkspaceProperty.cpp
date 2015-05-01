#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidAPI/IPeaksWorkspace.h"

// clang-format off
void export_IPeaksWorkspaceProperty()
// clang-format on
{
  using Mantid::API::IPeaksWorkspace;
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<IPeaksWorkspace>::define("IPeaksWorkspaceProperty");
}
