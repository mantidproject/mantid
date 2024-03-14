// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidPythonInterface/core/GetPointer.h"

using Mantid::DataObjects::OffsetsWorkspace;

GET_POINTER_SPECIALIZATION(Mantid::API::WorkspaceProperty<OffsetsWorkspace>)

void export_OffsetsWorkspaceProperty() {
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<OffsetsWorkspace>::define("OffsetsWorkspaceProperty");
}
