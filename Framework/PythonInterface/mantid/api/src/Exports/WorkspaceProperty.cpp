// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/Workspace.h"
#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/enum.hpp>

using Mantid::API::Workspace;
using Mantid::API::WorkspaceProperty; // NOLINT

GET_POINTER_SPECIALIZATION(WorkspaceProperty<Workspace>)

void export_WorkspaceProperty() {
  using Mantid::API::PropertyMode;
  // Property and Lock mode enums
  boost::python::enum_<PropertyMode::Type>("PropertyMode")
      .value("Optional", PropertyMode::Optional)
      .value("Mandatory", PropertyMode::Mandatory);

  using Mantid::API::LockMode;
  // Property and Lock mode enums
  boost::python::enum_<LockMode::Type>("LockMode")
      .value("Lock", LockMode::Lock)
      .value("NoLock", LockMode::NoLock);

  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<Workspace>::define("WorkspaceProperty");
}
