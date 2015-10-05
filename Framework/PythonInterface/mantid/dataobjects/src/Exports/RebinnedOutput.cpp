#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"

#include <boost/python/class.hpp>

using Mantid::DataObjects::Workspace2D;
using Mantid::DataObjects::RebinnedOutput;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

void export_RebinnedOutput() {
  class_<RebinnedOutput, bases<Workspace2D>, boost::noncopyable>(
      "RebinnedOutput", no_init);

  // register pointers
  RegisterWorkspacePtrToPython<RebinnedOutput>();
}
