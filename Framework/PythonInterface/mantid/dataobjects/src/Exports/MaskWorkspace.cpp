#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"
#include <boost/python/class.hpp>

using Mantid::API::IMaskWorkspace;
using Mantid::DataObjects::MaskWorkspace;
using Mantid::DataObjects::SpecialWorkspace2D;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(MaskWorkspace)

void export_MaskWorkspace() {
  class_<MaskWorkspace, bases<SpecialWorkspace2D, IMaskWorkspace>,
         boost::noncopyable>("MaskWorkspace", no_init)
      .def("getMaskedDetectors", &MaskWorkspace::getMaskedDetectors,
           arg("self"), "Returns all masked detector IDs.");

  // register pointers
  RegisterWorkspacePtrToPython<MaskWorkspace>();
}
