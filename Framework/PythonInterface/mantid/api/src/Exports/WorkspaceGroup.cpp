#include "MantidAPI/WorkspaceGroup.h"
#include "MantidPythonInterface/kernel/Policies/ToWeakPtr.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"

#include <boost/python/class.hpp>
#include <boost/python/return_value_policy.hpp>

using namespace Mantid::API;
using namespace Mantid::PythonInterface;
using namespace boost::python;

void export_WorkspaceGroup() {
  class_<WorkspaceGroup, bases<Workspace>, boost::noncopyable>("WorkspaceGroup",
                                                               no_init)
      .def("getNumberOfEntries", &WorkspaceGroup::getNumberOfEntries,
           "Returns the number of entries in the group")
      .def("getNames", &WorkspaceGroup::getNames,
           "Returns the names of the entries in the group")
      .def("contains",
           (bool (WorkspaceGroup::*)(const std::string &wsName) const) &
               WorkspaceGroup::contains,
           "Returns true if the given name is in the group")
      .def("add", &WorkspaceGroup::add, "Add a name to the group")
      .def("size", &WorkspaceGroup::size,
           "Returns the number of workspaces contained in the group")
      .def("remove", &WorkspaceGroup::remove, "Remove a name from the group")
      .def("getItem", (Workspace_sptr (WorkspaceGroup::*)(const size_t) const) &
                          WorkspaceGroup::getItem,
           return_value_policy<Policies::ToWeakPtr>(),
           "Returns the item at the given index")
      .def("isMultiPeriod", &WorkspaceGroup::isMultiperiod,
           "Retuns true if the workspace group is multi-period")
      // ------------ Operators --------------------------------
      .def("__len__", &WorkspaceGroup::getNumberOfEntries)
      .def("__contains__",
           (bool (WorkspaceGroup::*)(const std::string &wsName) const) &
               WorkspaceGroup::contains)
      .def("__getitem__",
           (Workspace_sptr (WorkspaceGroup::*)(const size_t) const) &
               WorkspaceGroup::getItem,
           return_value_policy<Policies::ToWeakPtr>());

  Registry::RegisterWorkspacePtrToPython<WorkspaceGroup>();
}
