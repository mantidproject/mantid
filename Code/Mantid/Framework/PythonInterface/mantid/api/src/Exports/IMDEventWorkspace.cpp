#include "MantidAPI/IMDEventWorkspace.h"

#include "MantidPythonInterface/kernel/Registry/RegisterSingleValueHandler.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using namespace Mantid::API;
using namespace boost::python;

void export_IMDEventWorkspace()
{
  register_ptr_to_python<boost::shared_ptr<IMDEventWorkspace>>();

  // MDEventWorkspace class
  class_< IMDEventWorkspace, bases<IMDWorkspace, MultipleExperimentInfos>, boost::noncopyable >("IMDEventWorkspace", no_init)
    .def("getNPoints", &IMDEventWorkspace::getNPoints, "Returns the total number of points (events) in this workspace")
    .def("getNumDims", &IMDEventWorkspace::getNumDims, "Returns the number of dimensions in this workspace")
    .def("getBoxController", (BoxController_sptr(IMDEventWorkspace::*)() )&IMDEventWorkspace::getBoxController,
         "Returns the BoxController used in this workspace")
  ;

  REGISTER_SINGLEVALUE_HANDLER(IMDEventWorkspace_sptr);

}

