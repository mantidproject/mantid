#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"

#include <boost/python/class.hpp>

using Mantid::API::IMDHistoWorkspace;
using Mantid::DataObjects::MDHistoWorkspace;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(MDHistoWorkspace)

void export_MDHistoWorkspace() {
  class_<MDHistoWorkspace, bases<IMDHistoWorkspace>, boost::noncopyable>(
      "MDHistoWorkspace", no_init);

  // register pointers
  RegisterWorkspacePtrToPython<MDHistoWorkspace>();
}
