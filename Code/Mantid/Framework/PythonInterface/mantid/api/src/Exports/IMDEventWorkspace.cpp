#include "MantidAPI/IMDEventWorkspace.h"

#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using namespace Mantid::API;
using Mantid::PythonInterface::Registry::RegisterWorkspacePtrToPython;
using namespace boost::python;

// clang-format off
void export_IMDEventWorkspace()
// clang-format on
{
  // MDEventWorkspace class
  class_< IMDEventWorkspace, bases<IMDWorkspace, MultipleExperimentInfos>,
          boost::noncopyable >("IMDEventWorkspace", no_init)
    .def("getNPoints", &IMDEventWorkspace::getNPoints,
         "Returns the total number of points (events) in this workspace")

    .def("getNumDims", &IMDEventWorkspace::getNumDims,
         "Returns the number of dimensions in this workspace")

    .def("getBoxController", (BoxController_sptr(IMDEventWorkspace::*)() )&IMDEventWorkspace::getBoxController,
         "Returns the BoxController used in this workspace")
  ;

  //-----------------------------------------------------------------------------------------------
  RegisterWorkspacePtrToPython<IMDEventWorkspace>();
}

