#include "MantidAPI/IMDWorkspace.h"
#include "MantidPythonInterface/kernel/RegisterSingleValueHandler.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/self.hpp>

using Mantid::API::IMDWorkspace;
using Mantid::API::IMDWorkspace_sptr;
using Mantid::API::Workspace;
using namespace boost::python;

void export_IMDWorkspace()
{
  register_ptr_to_python<IMDWorkspace_sptr>();

  // EventWorkspace class
  class_< IMDWorkspace, bases<Workspace>, boost::noncopyable >("IMDWorkspace", no_init)
    .def("getNPoints", &IMDWorkspace::getNPoints, "Returns the total number of points within the workspace")
    .def("getNumDims", &IMDWorkspace::getNumDims, "Returns the number of dimensions in the workspace")
    .def("getDimension", &IMDWorkspace::getDimension, "Return the chosen dimension of the workspace")
    ;

  REGISTER_SINGLEVALUE_HANDLER(IMDWorkspace_sptr);

}

