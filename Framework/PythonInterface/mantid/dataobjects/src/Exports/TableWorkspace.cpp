#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"
#include <boost/python/class.hpp>
#include <boost/python/make_constructor.hpp>

using Mantid::API::ITableWorkspace;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::WorkspaceFactory;
using Mantid::DataObjects::TableWorkspace;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(TableWorkspace)

namespace {
Mantid::API::Workspace_sptr makeTableWorkspace() {
  return WorkspaceFactory::Instance().createTable();
}
} // namespace

void export_TableWorkspace() {

  class_<TableWorkspace, bases<ITableWorkspace>, boost::noncopyable>(
      "TableWorkspace", no_init)
      .def("__init__", make_constructor(&makeTableWorkspace));

  // register pointers
  RegisterWorkspacePtrToPython<TableWorkspace>();
}
