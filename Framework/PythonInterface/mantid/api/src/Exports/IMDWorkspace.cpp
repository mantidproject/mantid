// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/IMDWorkspace.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"

#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/self.hpp>

using namespace Mantid::API;
using Mantid::PythonInterface::Registry::RegisterWorkspacePtrToPython;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(IMDWorkspace)

void export_IMDWorkspace() {
  boost::python::enum_<Mantid::API::MDNormalization>("MDNormalization")
      .value("NoNormalization", Mantid::API::NoNormalization)
      .value("VolumeNormalization", Mantid::API::VolumeNormalization)
      .value("NumEventsNormalization", Mantid::API::NumEventsNormalization);

  boost::python::enum_<Mantid::Kernel::SpecialCoordinateSystem>(
      "SpecialCoordinateSystem")
      .value("None", Mantid::Kernel::None)
      .value("QLab", Mantid::Kernel::QLab)
      .value("QSample", Mantid::Kernel::QSample)
      .value("HKL", Mantid::Kernel::HKL);

  // EventWorkspace class
  class_<IMDWorkspace, bases<Workspace, MDGeometry>, boost::noncopyable>(
      "IMDWorkspace", no_init)
      .def("getNPoints", &IMDWorkspace::getNPoints, arg("self"),
           "Returns the total number of points within the workspace")
      .def("getNEvents", &IMDWorkspace::getNEvents, arg("self"),
           "Returns the total number of events, contributed to the workspace")
      .def("getSpecialCoordinateSystem",
           &IMDWorkspace::getSpecialCoordinateSystem, arg("self"),
           "Returns the special coordinate system of the workspace")
      .def("isMDHistoWorkspace", &IMDWorkspace::isMDHistoWorkspace, arg("self"),
           "Returns True if this is considered to be binned data.")
      .def("displayNormalization", &IMDWorkspace::displayNormalization,
           args("self"),
           "Returns the visual "
           ":class:`~mantid.api.MDNormalization` of the "
           "workspace.")
      .def("displayNormalizationHisto",
           &IMDWorkspace::displayNormalizationHisto, arg("self"),
           "For MDEventWorkspaces returns the visual "
           ":class:`~mantid.api.MDNormalization` of derived "
           "MDHistoWorkspaces. For all others returns the same as "
           "displayNormalization.");

  RegisterWorkspacePtrToPython<IMDWorkspace>();
}
