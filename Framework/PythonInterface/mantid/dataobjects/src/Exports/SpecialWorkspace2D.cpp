// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/api/RegisterWorkspacePtrToPython.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/default_call_policies.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/overloads.hpp>

using Mantid::detid_t;
using Mantid::DataObjects::SpecialWorkspace2D;
using Mantid::DataObjects::Workspace2D;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(SpecialWorkspace2D)

GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(SpecialWorkspace2D_setValue, SpecialWorkspace2D::setValue, 2, 3)
// cppcheck-suppress unknownMacro
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(SpecialWorkspace2D_getValue, SpecialWorkspace2D::getValue, 1, 2)

GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")

namespace {
SpecialWorkspace2D *createSWS2DWithWS(const boost::python::object &workspace) {
  Mantid::API::MatrixWorkspace_sptr matWS = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
      Mantid::PythonInterface::ExtractSharedPtr<Mantid::API::Workspace>(workspace)());
  return new SpecialWorkspace2D(matWS);
}
} // namespace

void export_SpecialWorkspace2D() {
  class_<SpecialWorkspace2D, bases<Workspace2D>, boost::noncopyable>("SpecialWorkspace2D")
      .def("__init__", make_constructor(&createSWS2DWithWS, default_call_policies(), (arg("workspace"))))
      .def("getValue",
           (double (SpecialWorkspace2D::*)(const detid_t, const double) const) & SpecialWorkspace2D::getValue,
           SpecialWorkspace2D_getValue((arg("self"), arg("detectorID"))))
      .def("setValue",
           (void (SpecialWorkspace2D::*)(const detid_t, const double, const double))&SpecialWorkspace2D::setValue,
           SpecialWorkspace2D_setValue((arg("self"), arg("detectorID"), arg("value"), arg("error")),
                                       "Set the value of the data for a given detector ID"))
      .def("getDetectorIDs", &SpecialWorkspace2D::getDetectorIDs, (arg("self,"), arg("workspaceIndex")));

  // register pointers
  RegisterWorkspacePtrToPython<SpecialWorkspace2D>();
}
