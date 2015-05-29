#include "MantidDataObjects/TableWorkspace.h"
#include <boost/python/class.hpp>

#include "MantidPythonInterface/kernel/Registry/DataItemInterface.h"

using Mantid::API::Workspace;
using Mantid::DataObjects::TableWorkspace;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

void export_TableWorkspace()
{
class_<TableWorkspace, bases<Workspace>,
       boost::noncopyable>("TableWorkspace", no_init)
    ;

  // register pointers
  RegisterWorkspacePtrToPython<TableWorkspace>();
}
