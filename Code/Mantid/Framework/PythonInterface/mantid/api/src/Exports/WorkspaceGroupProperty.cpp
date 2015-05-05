#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidAPI/WorkspaceGroup.h"

// clang-format off
void export_WorkspaceGroupProperty()
// clang-format on
{
  using Mantid::API::WorkspaceGroup;
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<WorkspaceGroup>::define("WorkspaceGroupProperty");
}
