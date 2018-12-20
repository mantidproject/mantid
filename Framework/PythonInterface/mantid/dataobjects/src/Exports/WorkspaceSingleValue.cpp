#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"

#include <boost/python/class.hpp>

using Mantid::API::MatrixWorkspace;
using Mantid::DataObjects::WorkspaceSingleValue;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(WorkspaceSingleValue)

void export_WorkspaceSingleValue() {
  class_<WorkspaceSingleValue, bases<MatrixWorkspace>, boost::noncopyable>(
      "WorkspaceSingleValue", no_init);

  // register pointers
  RegisterWorkspacePtrToPython<WorkspaceSingleValue>();
}
