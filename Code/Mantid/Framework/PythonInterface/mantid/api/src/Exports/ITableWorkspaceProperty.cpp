#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidAPI/ITableWorkspace.h"

// clang-format off
void export_ITableWorkspaceProperty()
// clang-format on
{
  using Mantid::API::ITableWorkspace;
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<ITableWorkspace>::define("ITableWorkspaceProperty");
}
