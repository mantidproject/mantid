#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"
#include <boost/python/class.hpp>

using Mantid::DataObjects::SpecialWorkspace2D;
using Mantid::DataObjects::Workspace2D;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(SpecialWorkspace2D)

void export_SpecialWorkspace2D() {
  class_<SpecialWorkspace2D, bases<Workspace2D>, boost::noncopyable>(
      "SpecialWorkspace2D", no_init);

  // register pointers
  RegisterWorkspacePtrToPython<SpecialWorkspace2D>();
}
