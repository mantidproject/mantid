#include "MantidAPI/IMDWorkspace.h"
#include "MantidPythonInterface/kernel/Registry/RegisterSingleValueHandler.h"
#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/self.hpp>

using namespace Mantid::API;
using namespace boost::python;

void export_IMDWorkspace()
{
  boost::python::enum_<Mantid::API::MDNormalization>("MDNormalization")
          .value("NoNormalization", Mantid::API::NoNormalization)
          .value("VolumeNormalization", Mantid::API::VolumeNormalization)
          .value("NumEventsNormalization", Mantid::API::NumEventsNormalization);

  register_ptr_to_python<boost::shared_ptr<IMDWorkspace>>();

  // EventWorkspace class
  class_< IMDWorkspace, bases<Workspace, MDGeometry>, boost::noncopyable >("IMDWorkspace", no_init)
    .def("getNPoints", &IMDWorkspace::getNPoints, args("self"), "Returns the total number of points within the workspace")
    .def("getNEvents", &IMDWorkspace::getNEvents, args("self"), "Returns the total number of events, contributed to the workspace")
    ;

  REGISTER_SINGLEVALUE_HANDLER(IMDWorkspace_sptr);

}

