#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidAPI/IPeaksWorkspace.h"

using Mantid::API::IPeaksWorkspace;
using Mantid::API::WorkspaceProperty;

GET_POINTER_SPECIALIZATION(WorkspaceProperty<IPeaksWorkspace>)

void export_IPeaksWorkspaceProperty() {
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<IPeaksWorkspace>::define("IPeaksWorkspaceProperty");
}
