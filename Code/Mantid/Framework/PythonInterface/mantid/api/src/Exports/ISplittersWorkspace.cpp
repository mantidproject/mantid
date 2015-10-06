#include "MantidAPI/ISplittersWorkspace.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"
#include <boost/python/class.hpp>
#include <boost/python/return_internal_reference.hpp>

using Mantid::API::ISplittersWorkspace;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

void export_ISplittersWorkspace() {
  class_<ISplittersWorkspace, boost::noncopyable>("ISplittersWorkspace",
                                                  no_init)
      .def("getNumberSplitters", &ISplittersWorkspace::getNumberSplitters,
           "Returns the number of splitters within the workspace");

  // register pointers
  RegisterWorkspacePtrToPython<ISplittersWorkspace>();
}
