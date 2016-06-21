#include "MantidDataObjects/TableWorkspace.h"
#include <boost/python/class.hpp>

#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"

using Mantid::API::ITableWorkspace;
using Mantid::DataObjects::TableWorkspace;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

void export_TableWorkspace() {

  class_<TableWorkspace, bases<ITableWorkspace>, boost::noncopyable>(
      "TableWorkspace", no_init);

  // register pointers
  RegisterWorkspacePtrToPython<TableWorkspace>();
}
