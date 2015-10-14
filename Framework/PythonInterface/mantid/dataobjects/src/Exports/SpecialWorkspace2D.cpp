#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"
#include <boost/python/class.hpp>

using Mantid::DataObjects::Workspace2D;
using Mantid::DataObjects::SpecialWorkspace2D;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

void export_SpecialWorkspace2D() {
  class_<SpecialWorkspace2D, bases<Workspace2D>, boost::noncopyable>(
      "SpecialWorkspace2D", no_init);

  // register pointers
  RegisterWorkspacePtrToPython<SpecialWorkspace2D>();
}
