#include "MantidAPI/IMDWorkspace.h"
#include "MantidPythonInterface/kernel/Registry/DataItemInterface.h"

#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/self.hpp>

using namespace Mantid::API;
using Mantid::PythonInterface::Registry::DataItemInterface;
using namespace boost::python;

void export_IMDWorkspace()
{
  boost::python::enum_<Mantid::API::MDNormalization>("MDNormalization")
          .value("NoNormalization", Mantid::API::NoNormalization)
          .value("VolumeNormalization", Mantid::API::VolumeNormalization)
          .value("NumEventsNormalization", Mantid::API::NumEventsNormalization);

  boost::python::enum_<Mantid::Kernel::SpecialCoordinateSystem>("SpecialCoordinateSystem")
          .value("None", Mantid::Kernel::None)
          .value("QLab", Mantid::Kernel::QLab)
          .value("QSample", Mantid::Kernel::QSample)
          .value("HKL", Mantid::Kernel::HKL);

  // EventWorkspace class
  class_< IMDWorkspace, bases<Workspace, MDGeometry>, boost::noncopyable >("IMDWorkspace", no_init)
    .def("getNPoints", &IMDWorkspace::getNPoints, args("self"), "Returns the total number of points within the workspace")
    .def("getNEvents", &IMDWorkspace::getNEvents, args("self"), "Returns the total number of events, contributed to the workspace")
    .def("getSpecialCoordinateSystem", &IMDWorkspace::getSpecialCoordinateSystem, args("self"), "Returns the special coordinate system of the workspace");

  DataItemInterface<IMDWorkspace>();
}

