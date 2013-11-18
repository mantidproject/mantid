#include "MantidAPI/IMDWorkspace.h"
#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"
#include "MantidPythonInterface/kernel/Registry/RegisterSingleValueHandler.h"
#include <boost/python/class.hpp>
#include <boost/python/self.hpp>
#include <boost/python/enum.hpp>

using Mantid::API::IMDWorkspace;
using Mantid::API::IMDWorkspace_sptr;
using Mantid::API::MDGeometry;
using Mantid::API::Workspace;
using namespace boost::python;

void export_IMDWorkspace()
{
  boost::python::enum_<Mantid::API::MDNormalization>("MDNormalization")
          .value("NoNormalization", Mantid::API::NoNormalization)
          .value("VolumeNormalization", Mantid::API::VolumeNormalization)
          .value("NumEventsNormalization", Mantid::API::NumEventsNormalization);

  REGISTER_SHARED_PTR_TO_PYTHON(IMDWorkspace);

  // EventWorkspace class
  class_< IMDWorkspace, bases<Workspace, MDGeometry>, boost::noncopyable >("IMDWorkspace", no_init)
    .def("getNPoints", &IMDWorkspace::getNPoints, "Returns the total number of points within the workspace")
    .def("getNEvents", &IMDWorkspace::getNEvents, "Returns the total number of events, contributed to the workspace")
    ;

  REGISTER_SINGLEVALUE_HANDLER(IMDWorkspace_sptr);

}

