#include "MantidDataObjects/Workspace2D.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"

#include <boost/python/class.hpp>

using Mantid::API::MatrixWorkspace;
using Mantid::DataObjects::Workspace2D;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(Workspace2D)

void export_Workspace2D() {
  class_<Workspace2D, bases<MatrixWorkspace>, boost::noncopyable>("Workspace2D",
                                                                  no_init);

  // register pointers
  RegisterWorkspacePtrToPython<Workspace2D>();
}
