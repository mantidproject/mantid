#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"
#include <boost/python/class.hpp>

using Mantid::DataObjects::GroupingWorkspace;
using Mantid::DataObjects::SpecialWorkspace2D;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(GroupingWorkspace)

void export_GroupingWorkspace() {
  class_<GroupingWorkspace, bases<SpecialWorkspace2D>, boost::noncopyable>(
      "GroupingWorkspace", no_init);

  // register pointers
  RegisterWorkspacePtrToPython<GroupingWorkspace>();
}
