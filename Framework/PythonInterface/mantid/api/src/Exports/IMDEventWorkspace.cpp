// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidPythonInterface/api/RegisterWorkspacePtrToPython.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using namespace Mantid::API;
using Mantid::PythonInterface::Registry::RegisterWorkspacePtrToPython;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(IMDEventWorkspace)

void export_IMDEventWorkspace() {
  // IMDEventWorkspace class
  class_<IMDEventWorkspace, bases<IMDWorkspace, MultipleExperimentInfos>, boost::noncopyable>("IMDEventWorkspace",
                                                                                              no_init)
      .def("getNPoints", &IMDEventWorkspace::getNPoints, arg("self"),
           "Returns the total number of points (events) in this "
           ":class:`~mantid.api.Workspace`")

      .def("getNumDims", &IMDEventWorkspace::getNumDims, arg("self"),
           "Returns the number of dimensions in this "
           ":class:`~mantid.api.Workspace`")

      .def("getBoxController", (BoxController_sptr (IMDEventWorkspace::*)())&IMDEventWorkspace::getBoxController,
           arg("self"),
           "Returns the :class:`~mantid.api.BoxController` used in this "
           ":class:`~mantid.api.Workspace`")
      .def("setDisplayNormalization", &IMDEventWorkspace::setDisplayNormalization, (arg("self"), arg("normalization")),
           "Sets the visual normalization of"
           " the :class:`~mantid.api.Workspace`.")
      .def("setDisplayNormalizationHisto", &IMDEventWorkspace::setDisplayNormalizationHisto,
           (arg("self"), arg("normalization")),
           "For :class:`~mantid.api.IMDEventWorkspace` s sets"
           " the visual normalization of dervied "
           ":class:`~mantid.api.IMDHistoWorkspace` s.");

  RegisterWorkspacePtrToPython<IMDEventWorkspace>();
}
