#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"
#include <boost/python/class.hpp>

using Mantid::API::ISplittersWorkspace;
using Mantid::DataObjects::SplittersWorkspace;
using Mantid::DataObjects::TableWorkspace;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(SplittersWorkspace)

void export_SplittersWorkspace() {
  class_<SplittersWorkspace, bases<TableWorkspace, ISplittersWorkspace>,
         boost::noncopyable>("SplittersWorkspace", no_init);

  // register pointers
  RegisterWorkspacePtrToPython<SplittersWorkspace>();
}
