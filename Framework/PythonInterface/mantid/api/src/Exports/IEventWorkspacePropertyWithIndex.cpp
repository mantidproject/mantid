#include "MantidAPI/IEventWorkspace.h"
#include "MantidPythonInterface/api/WorkspacePropertyWithIndexExporter.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include <boost/python/enum.hpp>

using Mantid::API::IEventWorkspace;
using Mantid::API::WorkspacePropertyWithIndex;

GET_POINTER_SPECIALIZATION(WorkspacePropertyWithIndex<IEventWorkspace>)

void export_IEventWorkspacePropertyWithIndex() {
  using Mantid::PythonInterface::WorkspacePropertyWithIndexExporter;
  WorkspacePropertyWithIndexExporter<IEventWorkspace>::define(
      "IEventWorkspacePropertyWithIndex");
}
