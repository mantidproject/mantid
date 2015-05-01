#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidAPI/IEventWorkspace.h"

// clang-format off
void export_IEventWorkspaceProperty()
// clang-format on
{
  using Mantid::API::IEventWorkspace;
  using Mantid::PythonInterface::WorkspacePropertyExporter;

  WorkspacePropertyExporter<IEventWorkspace>::define("IEventWorkspaceProperty");
}
