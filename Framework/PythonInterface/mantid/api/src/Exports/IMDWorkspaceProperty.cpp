// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/IMDWorkspace.h"
#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidPythonInterface/kernel/GetPointer.h"

using Mantid::API::IMDWorkspace;
using Mantid::API::WorkspaceProperty; // NOLINT

GET_POINTER_SPECIALIZATION(WorkspaceProperty<IMDWorkspace>)

void export_IMDWorkspaceProperty() {
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<IMDWorkspace>::define("IMDWorkspaceProperty");
}
