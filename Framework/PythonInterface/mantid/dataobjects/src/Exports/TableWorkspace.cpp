#include "MantidDataObjects/TableWorkspace.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"
#include <boost/python/class.hpp>

using Mantid::API::ITableWorkspace;
using Mantid::DataObjects::TableWorkspace;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(TableWorkspace)

void export_TableWorkspace() {

  class_<TableWorkspace, bases<ITableWorkspace>, boost::noncopyable>(
      "TableWorkspace", no_init);

  // register pointers
  RegisterWorkspacePtrToPython<TableWorkspace>();
}
