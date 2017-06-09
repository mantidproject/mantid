#include "MantidPythonInterface/api/WorkspacePropertyWithIndexExporter.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidAPI/MatrixWorkspace.h"
#include <boost/python/enum.hpp>

using Mantid::API::MatrixWorkspace;
using Mantid::API::WorkspacePropertyWithIndex;

GET_POINTER_SPECIALIZATION(WorkspacePropertyWithIndex<MatrixWorkspace>)

void export_WorkspacePropertyWithIndex() {
  using Mantid::API::IndexType;
  // Property and Lock mode enums
  boost::python::enum_<IndexType>("IndexType")
      .value("SpectrumNumber", IndexType::SpectrumNumber)
      .value("WorkspaceIndex", IndexType::WorkspaceIndex);

  using Mantid::PythonInterface::WorkspacePropertyWithIndexExporter;
  WorkspacePropertyWithIndexExporter<MatrixWorkspace>::define(
      "WorkspacePropertyWithIndex");
}
