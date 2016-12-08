#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidAPI/WorkspaceGroup.h"

using Mantid::API::WorkspaceGroup;
using Mantid::API::WorkspaceProperty;

GET_POINTER_SPECIALIZATION(WorkspaceProperty<WorkspaceGroup>)

void export_WorkspaceGroupProperty() {
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<WorkspaceGroup>::define("WorkspaceGroupProperty");
}
