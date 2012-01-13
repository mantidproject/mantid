#include "MantidAPI/WorkspaceGroup.h"
#include "MantidPythonInterface/kernel/SingleValueTypeHandler.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::API::WorkspaceGroup;
using Mantid::API::WorkspaceGroup_sptr;
using Mantid::API::Workspace;
using Mantid::Kernel::DataItem_sptr;
using namespace boost::python;

void export_WorkspaceGroup()
{
  register_ptr_to_python<WorkspaceGroup_sptr>();

  class_< WorkspaceGroup, bases<Workspace>, boost::noncopyable >("WorkspaceGroup", no_init)
    .def("getNumberOfEntries", &WorkspaceGroup::getNumberOfEntries, "Returns the number of entries in the group")
    .def("getNames", &WorkspaceGroup::getNames, "Returns the names of the entries in the group")
    .def("contains", &WorkspaceGroup::contains, "Returns true if the given name is in the group")
    .def("add", &WorkspaceGroup::add, "Add a name to the group")
    .def("remove", &WorkspaceGroup::remove, "Remove a name from the group")
    // ------------ Operators --------------------------------
    .def("__len__", &WorkspaceGroup::getNumberOfEntries)
    .def("__contains__", &WorkspaceGroup::contains)
  ;

  DECLARE_SINGLEVALUETYPEHANDLER(WorkspaceGroup, DataItem_sptr);
}

