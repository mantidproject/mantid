#include "MantidAPI/WorkspaceGroup.h"
#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"
#include "MantidPythonInterface/kernel/Registry/RegisterSingleValueHandler.h"
#include "MantidPythonInterface/kernel/Policies/DowncastingPolicies.h"

#include <boost/python/class.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/weak_ptr.hpp>

using Mantid::API::WorkspaceGroup;
using Mantid::API::WorkspaceGroup_sptr;
using Mantid::API::Workspace;
using Mantid::API::Workspace_sptr;
namespace Policies = Mantid::PythonInterface::Policies;
using namespace boost::python;

void export_WorkspaceGroup() 
{
  REGISTER_SHARED_PTR_TO_PYTHON(WorkspaceGroup);

  class_< WorkspaceGroup, bases<Workspace>, boost::noncopyable >("WorkspaceGroup", no_init)
    .def("getNumberOfEntries", &WorkspaceGroup::getNumberOfEntries, "Returns the number of entries in the group")
    .def("getNames", &WorkspaceGroup::getNames, "Returns the names of the entries in the group")
    .def("contains", (bool (WorkspaceGroup::*)(const std::string & wsName) const)&WorkspaceGroup::contains, "Returns true if the given name is in the group")
    .def("add", &WorkspaceGroup::add, "Add a name to the group")
    .def("size", &WorkspaceGroup::size, "Returns the number of workspaces contained in the group")
    .def("remove", &WorkspaceGroup::remove, "Remove a name from the group")
    .def("getItem", (Workspace_sptr (WorkspaceGroup::*)(const size_t) const)&WorkspaceGroup::getItem, 
         return_value_policy<Policies::ToWeakPtrWithDowncast>(), "Returns the item at the given index")
    .def("isMultiPeriod", &WorkspaceGroup::isMultiperiod, "Retuns true if the workspace group is multi-period")
    // ------------ Operators --------------------------------
    .def("__len__", &WorkspaceGroup::getNumberOfEntries)
    .def("__contains__", (bool (WorkspaceGroup::*)(const std::string & wsName) const)&WorkspaceGroup::contains)
    .def("__getitem__", (Workspace_sptr (WorkspaceGroup::*)(const size_t) const)&WorkspaceGroup::getItem, 
         return_value_policy<Policies::ToWeakPtrWithDowncast>())
  ;

  REGISTER_SINGLEVALUE_HANDLER(WorkspaceGroup_sptr);
}

