#include "MantidPythonInterface/api/WorkspacePropertyExporter.h"
#include "MantidAPI/Workspace.h"
#include <boost/python/enum.hpp>

// clang-format off
void export_WorkspaceProperty()
// clang-format on
{
  using Mantid::API::PropertyMode;
  // Property and Lock mode enums
  boost::python::enum_<PropertyMode::Type>("PropertyMode")
    .value("Optional", PropertyMode::Optional)
    .value("Mandatory", PropertyMode::Mandatory)
  ;

    using Mantid::API::LockMode;
    // Property and Lock mode enums
    boost::python::enum_<LockMode::Type>("LockMode")
      .value("Lock", LockMode::Lock)
      .value("NoLock", LockMode::NoLock)
    ;

  using Mantid::API::Workspace;
  using Mantid::PythonInterface::WorkspacePropertyExporter;
  WorkspacePropertyExporter<Workspace>::define("WorkspaceProperty");
}
