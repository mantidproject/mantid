#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"
#include <boost/python/class.hpp>

using Mantid::API::IMaskWorkspace;
using Mantid::DataObjects::MaskWorkspace;
using Mantid::DataObjects::SpecialWorkspace2D;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

void export_MaskWorkspace() {
  class_<MaskWorkspace, bases<SpecialWorkspace2D, IMaskWorkspace>,
         boost::noncopyable>("MaskWorkspace", no_init);

  // register pointers
  RegisterWorkspacePtrToPython<MaskWorkspace>();
}
