// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/MultipleExperimentInfos.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/Workspace.h"
#include "MantidPythonInterface/core/ExtractSharedPtr.h"
#include <boost/python/class.hpp>

using Mantid::API::ExperimentInfo;
using Mantid::API::ExperimentInfo_sptr;
using Mantid::API::MultipleExperimentInfos;
using Mantid::API::Workspace;
using Mantid::PythonInterface::ExtractSharedPtr;
using namespace boost::python;

namespace {
void addExperimentInfo(MultipleExperimentInfos &self, const boost::python::object &item) {
  auto workspaceExtractor = ExtractSharedPtr<Workspace>(item);
  if (!workspaceExtractor.check())
    throw std::invalid_argument("Incorrect type. Expected a workspace.");

  if (auto exptInfo = std::dynamic_pointer_cast<ExperimentInfo>(workspaceExtractor()))
    self.addExperimentInfo(exptInfo);
  else
    throw std::invalid_argument("Incorrect type. Expected a workspace type derived from ExperimentInfo.");
}
} // namespace

void export_MultipleExperimentInfos() {
  class_<MultipleExperimentInfos, boost::noncopyable>("MultipleExperimentInfos", no_init)
      .def(
          "getExperimentInfo",
          (ExperimentInfo_sptr (MultipleExperimentInfos::*)(const uint16_t))&MultipleExperimentInfos::getExperimentInfo,
          (arg("self"), arg("expInfoIndex")), "Return the experiment info at the given index.")
      .def("addExperimentInfo", addExperimentInfo, (arg("self"), arg("ExperimentalInfo")),
           "Add a new :class:`~mantid.api.ExperimentInfo` to this "
           ":class:`~mantid.api.IMDWorkspace`")
      .def("getNumExperimentInfo", &MultipleExperimentInfos::getNumExperimentInfo, arg("self"),
           "Return the number of :class:`~mantid.api.ExperimentInfo` objects,")
      .def("copyExperimentInfos", &MultipleExperimentInfos::copyExperimentInfos,
           (arg("self"), arg("MultipleExperimentInfos")), "Copy the :class:`~mantid.api.ExperimentInfo` from another");
}
