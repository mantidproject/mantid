// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/WorkspaceNearestNeighbourInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/V3D.h"
#include "MantidPythonInterface/api/RegisterWorkspacePtrToPython.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/make_constructor.hpp>

using namespace boost::python;
using namespace Mantid::API;
using namespace Mantid::PythonInterface::Registry;

GET_POINTER_SPECIALIZATION(WorkspaceNearestNeighbourInfo)

namespace {
WorkspaceNearestNeighbourInfo *createWorkspaceNearestNeighbourInfo(const boost::python::object &workspace,
                                                                   const bool ignoreMaskedDetectors,
                                                                   const int nNeighbours = 8) {
  Mantid::API::MatrixWorkspace_sptr matrixWorkspace = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
      Mantid::PythonInterface::ExtractSharedPtr<Mantid::API::Workspace>(workspace)());

  return new WorkspaceNearestNeighbourInfo(*matrixWorkspace, ignoreMaskedDetectors, nNeighbours);
}

boost::python::dict mapToDict(const std::map<Mantid::specnum_t, Mantid::Kernel::V3D> &inputMap) {
  boost::python::dict result;
  for (const auto &pair : inputMap) {
    result[pair.first] = pair.second;
  }
  return result;
}

boost::python::dict getNeighboursByDetector(const WorkspaceNearestNeighbourInfo &self,
                                            const Mantid::Geometry::IDetector *comp, double radius = 0.0) {
  auto result = self.getNeighbours(comp, radius);
  return mapToDict(result);
}

boost::python::dict getNeighboursBySpec(const WorkspaceNearestNeighbourInfo &self, Mantid::specnum_t spec,
                                        double radius) {
  auto result = self.getNeighbours(spec, radius);
  return mapToDict(result);
}

boost::python::dict getNeighboursExact(const WorkspaceNearestNeighbourInfo &self, Mantid::specnum_t spec) {
  auto result = self.getNeighboursExact(spec);
  return mapToDict(result);
}

} // namespace

void export_WorkspaceNearestNeighbourInfo() {
  class_<WorkspaceNearestNeighbourInfo, boost::noncopyable>("WorkspaceNearestNeighbourInfo", no_init)
      .def("__init__", make_constructor(&createWorkspaceNearestNeighbourInfo, default_call_policies(),
                                        (arg("workspace"), arg("ignoreMaskedDetectors"), arg("nNeighbours") = 8)))
      .def("getNeighbours", &getNeighboursByDetector, (arg("self"), arg("comp"), arg("radius") = 0.0))
      .def("getNeighbours", &getNeighboursBySpec, (arg("self"), arg("spec_num"), arg("radius")))
      .def("getNeighboursExact", &getNeighboursExact, (arg("self"), arg("spec_num")));
}
