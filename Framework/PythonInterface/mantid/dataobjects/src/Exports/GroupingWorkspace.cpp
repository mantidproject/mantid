// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidPythonInterface/api/RegisterWorkspacePtrToPython.h"
#include "MantidPythonInterface/core/Policies/VectorToNumpy.h"

#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/return_value_policy.hpp>

#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::DataObjects::GroupingWorkspace;
using Mantid::DataObjects::SpecialWorkspace2D;
namespace Policies = Mantid::PythonInterface::Policies;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(GroupingWorkspace)

namespace {
GroupingWorkspace *createGroupingWorkspaceWithWS() { return new GroupingWorkspace(); }
} // namespace

void export_GroupingWorkspace() {
  class_<GroupingWorkspace, bases<SpecialWorkspace2D>, boost::noncopyable>("GroupingWorkspace")
      .def("__init__", make_constructor(&createGroupingWorkspaceWithWS, default_call_policies()))
      .def("getTotalGroups", &GroupingWorkspace::getTotalGroups, (arg("self")))
      .def("getGroupIDs", &GroupingWorkspace::getGroupIDs, return_value_policy<Policies::VectorToNumpy>(),
           (arg("self")))
      .def("getDetectorIDsOfGroup", &GroupingWorkspace::getDetectorIDsOfGroup,
           return_value_policy<Policies::VectorToNumpy>(), (arg("self,"), arg("groupID")));

  // register pointers
  // cppcheck-suppress unusedScopedObject
  RegisterWorkspacePtrToPython<GroupingWorkspace>();
}
