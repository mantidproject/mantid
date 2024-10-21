// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidPythonInterface/api/RegisterWorkspacePtrToPython.h"
#include "MantidPythonInterface/core/DataServiceExporter.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/core/ReleaseGlobalInterpreterLock.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_non_const_reference.hpp>
#include <boost/python/iterator.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/return_value_policy.hpp>

using namespace Mantid::API;
using namespace Mantid::PythonInterface;
using namespace boost::python;

namespace {

PyObject *convertWsToObj(const Workspace_sptr &ws) {
  if (Mantid ::API::AnalysisDataService::Instance().doesExist(ws->getName())) {
    // Decay to weak ptr so the ADS manages lifetime
    using PtrT = std::weak_ptr<Workspace>;
    PtrT weak_ptr = ws;
    return boost::python::to_python_value<PtrT>()(weak_ptr);
  } else {
    return boost::python::to_python_value<Workspace_sptr>()(ws);
  }
}
} // namespace

GET_POINTER_SPECIALIZATION(WorkspaceGroup)

/**
 * Returns an iterator pointing to the first element in the group.
 *
 * @param self :: handle to the workspace group.
 * @return A non-const iterator pointing at start of workspace group,
 *         for use in python.
 */
std::vector<Workspace_sptr>::iterator group_begin(WorkspaceGroup &self) { return self.begin(); }

/**
 * Returns an iterator pointing to the past-the-end element in the group.
 *
 * @param self :: handle to the workspace group.
 * @return A non-const iterator pointing at end of workspace group,
 *         for use in python.
 */
std::vector<Workspace_sptr>::iterator group_end(WorkspaceGroup &self) { return self.end(); }

/** Constructor function for WorkspaceGroup */
Workspace_sptr makeWorkspaceGroup() {
  Workspace_sptr wsGroup = std::make_shared<WorkspaceGroup>();
  return wsGroup;
}

void addItem(WorkspaceGroup &self, const std::string &name) {
  ReleaseGlobalInterpreterLock releaseGIL;
  self.add(name);
}

void addWorkspace(WorkspaceGroup &self, const boost::python::object &pyobj) {
  ReleaseGlobalInterpreterLock releaseGIL;
  self.addWorkspace(DataServiceExporter<AnalysisDataServiceImpl, Workspace_sptr>::extractCppValue(pyobj));
}

void removeItem(WorkspaceGroup &self, const std::string &name) {
  ReleaseGlobalInterpreterLock releaseGIL;
  self.remove(name);
}

PyObject *getItem(WorkspaceGroup &self, const int &index) {
  if (index < 0) {
    if (static_cast<size_t>(-index) > self.size())
      self.throwIndexOutOfRangeError(index);
    return convertWsToObj(self.getItem(self.size() + index));
  }
  return convertWsToObj(self.getItem(index));
}

void export_WorkspaceGroup() {
  class_<WorkspaceGroup, bases<Workspace>, boost::noncopyable>("WorkspaceGroup", no_init)
      .def("__init__", make_constructor(&makeWorkspaceGroup))
      .def("getNumberOfEntries", &WorkspaceGroup::getNumberOfEntries, arg("self"),
           "Returns the number of entries in the group")
      .def("getNames", &WorkspaceGroup::getNames, arg("self"), "Returns the names of the entries in the group")
      .def("contains", (bool(WorkspaceGroup::*)(const std::string &wsName) const) & WorkspaceGroup::contains,
           (arg("self"), arg("workspace")), "Returns true if the given name is in the group")
      .def("sortByName", &WorkspaceGroup::sortByName, (arg("self")), "Sort members by name")
      .def("add", addItem, (arg("self"), arg("workspace_name")), "Add a name to the group")
      .def("addWorkspace", addWorkspace, (arg("self"), arg("workspace")), "Add a workspace to the group.")
      .def("size", &WorkspaceGroup::size, arg("self"), "Returns the number of workspaces contained in the group")
      .def("remove", removeItem, (arg("self"), arg("workspace_name")), "Remove a name from the group")
      .def("getItem", getItem, (arg("self"), arg("index")), "Returns the item at the given index")
      .def("isMultiPeriod", &WorkspaceGroup::isMultiperiod, arg("self"),
           "Returns true if the workspace group is multi-period")
      // ------------ Operators --------------------------------
      .def("__len__", &WorkspaceGroup::getNumberOfEntries, arg("self"),
           "Gets the number of entries in the workspace group")
      .def("__contains__", (bool(WorkspaceGroup::*)(const std::string &wsName) const) & WorkspaceGroup::contains,
           (arg("self"), arg("workspace name")), "Does this group contain the named workspace?")
      .def("__getitem__", getItem, (arg("self"), arg("index")))
      .def("__iter__", range<return_value_policy<copy_non_const_reference>>(&group_begin, &group_end));

  Registry::RegisterWorkspacePtrToPython<WorkspaceGroup>();
}
