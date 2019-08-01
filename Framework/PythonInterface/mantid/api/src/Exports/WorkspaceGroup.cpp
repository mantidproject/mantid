// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidPythonInterface/core/DataServiceExporter.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/core/Policies/ToWeakPtr.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_non_const_reference.hpp>
#include <boost/python/iterator.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/return_value_policy.hpp>

using namespace Mantid::API;
using namespace Mantid::PythonInterface;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(WorkspaceGroup)

/**
 * Returns an iterator pointing to the first element in the group.
 *
 * @param self :: handle to the workspace group.
 * @return A non-const iterator pointing at start of workspace group,
 *         for use in python.
 */
std::vector<Workspace_sptr>::iterator group_begin(WorkspaceGroup &self) {
  return self.begin();
}

/**
 * Returns an iterator pointing to the past-the-end element in the group.
 *
 * @param self :: handle to the workspace group.
 * @return A non-const iterator pointing at end of workspace group,
 *         for use in python.
 */
std::vector<Workspace_sptr>::iterator group_end(WorkspaceGroup &self) {
  return self.end();
}

/** Constructor function for WorkspaceGroup */
Workspace_sptr makeWorkspaceGroup() {
  Workspace_sptr wsGroup = boost::make_shared<WorkspaceGroup>();
  return wsGroup;
}

void addWorkspace(WorkspaceGroup &self, const boost::python::object &pyobj) {
  self.addWorkspace(
      DataServiceExporter<AnalysisDataServiceImpl,
                          Workspace_sptr>::extractCppValue(pyobj));
}

Workspace_sptr getItem(WorkspaceGroup &self, const int &index) {
  if (index < 0) {
    if (static_cast<size_t>(-index) > self.size())
      self.throwIndexOutOfRangeError(index);
    return self.getItem(self.size() + index);
  }
  return self.getItem(index);
}

void export_WorkspaceGroup() {
  class_<WorkspaceGroup, bases<Workspace>, boost::noncopyable>("WorkspaceGroup",
                                                               no_init)
      .def("__init__", make_constructor(&makeWorkspaceGroup))
      .def("getNumberOfEntries", &WorkspaceGroup::getNumberOfEntries,
           arg("self"), "Returns the number of entries in the group")
      .def("getNames", &WorkspaceGroup::getNames, arg("self"),
           "Returns the names of the entries in the group")
      .def("contains",
           (bool (WorkspaceGroup::*)(const std::string &wsName) const) &
               WorkspaceGroup::contains,
           (arg("self"), arg("workspace")),
           "Returns true if the given name is in the group")
      .def("sortByName", &WorkspaceGroup::sortByName, (arg("self")),
           "Sort members by name")
      .def("add", &WorkspaceGroup::add, (arg("self"), arg("workspace_name")),
           "Add a name to the group")
      .def("addWorkspace", addWorkspace, (arg("self"), arg("workspace")),
           "Add a workspace to the group.")
      .def("size", &WorkspaceGroup::size, arg("self"),
           "Returns the number of workspaces contained in the group")
      .def("remove", &WorkspaceGroup::remove,
           (arg("self"), arg("workspace_name")), "Remove a name from the group")
      .def("getItem", getItem, (arg("self"), arg("index")),
           return_value_policy<Policies::ToWeakPtr>(),
           "Returns the item at the given index")
      .def("isMultiPeriod", &WorkspaceGroup::isMultiperiod, arg("self"),
           "Returns true if the workspace group is multi-period")
      // ------------ Operators --------------------------------
      .def("__len__", &WorkspaceGroup::getNumberOfEntries, arg("self"),
           "Gets the number of entries in the workspace group")
      .def("__contains__",
           (bool (WorkspaceGroup::*)(const std::string &wsName) const) &
               WorkspaceGroup::contains,
           (arg("self"), arg("workspace name")),
           "Does this group contain the named workspace?")
      .def("__getitem__", getItem, (arg("self"), arg("index")),
           return_value_policy<Policies::ToWeakPtr>())
      .def("__iter__", range<return_value_policy<copy_non_const_reference>>(
                           &group_begin, &group_end));

  Registry::RegisterWorkspacePtrToPython<WorkspaceGroup>();
}
