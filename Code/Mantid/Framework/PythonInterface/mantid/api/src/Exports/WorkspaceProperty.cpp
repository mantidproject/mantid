#include "MantidPythonInterface/api/WorkspacePropertyMacro.h"
#include "MantidAPI/Workspace.h"
#include <boost/python/enum.hpp>

void export_WorkspaceProperty()
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
  EXPORT_WORKSPACE_PROPERTY(Workspace, "WorkspaceProperty");
}
