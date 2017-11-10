#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"
#include <boost/python/class.hpp>

using Mantid::DataObjects::OffsetsWorkspace;
using Mantid::DataObjects::SpecialWorkspace2D;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(OffsetsWorkspace)

void export_OffsetsWorkspace() {
  class_<OffsetsWorkspace, bases<SpecialWorkspace2D>, boost::noncopyable>(
      "OffsetsWorkspace", no_init);

  // register pointers
  RegisterWorkspacePtrToPython<OffsetsWorkspace>();
}
