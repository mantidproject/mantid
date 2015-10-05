#include "MantidDataObjects/PeaksWorkspace.h"
#include <boost/python/class.hpp>

#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"

using Mantid::API::IPeaksWorkspace;
using Mantid::DataObjects::PeaksWorkspace;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

void export_PeaksWorkspace() {

  class_<PeaksWorkspace, bases<IPeaksWorkspace>, boost::noncopyable>(
      "PeaksWorkspace", no_init);

  // register pointers
  RegisterWorkspacePtrToPython<PeaksWorkspace>();
}
