#include "MantidAPI/WorkspaceGroup.h"
#include "MantidPythonInterface/kernel/SingleValueTypeHandler.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/weak_ptr.hpp>

using Mantid::API::WorkspaceGroup;
using Mantid::API::WorkspaceGroup_sptr;
using Mantid::API::Workspace;
using Mantid::API::Workspace_sptr;
using Mantid::Kernel::DataItem_sptr;
using namespace boost::python;

namespace
{
  namespace bpl = boost::python;

  /**
   * Get a workspace at the given index
   * @param self A reference to the calling workspace group
   * @param index An index into the group
   */
  boost::weak_ptr<Workspace> getItemAsWorkspace(WorkspaceGroup & self, const size_t index)
  {
    return boost::weak_ptr<Workspace>(self.getItem(index));
  }

  /**
   * Get a workspace at the given index that is cast up to the appropriate interface
   * @param self A reference to the calling workspace group
   * @param index An index into the group
   */
  bpl::object getItemUpcasted(bpl::object self, const size_t index)
  {
    bpl::object workspace = self.attr("getItemAsWorkspace")(index);
    Mantid::PythonInterface::PropertyMarshal::upcastFromDataItem(workspace);
    return workspace;
  }

}

void export_WorkspaceGroup()
{
  register_ptr_to_python<WorkspaceGroup_sptr>();

  class_< WorkspaceGroup, bases<Workspace>, boost::noncopyable >("WorkspaceGroup", no_init)
    .def("getNumberOfEntries", &WorkspaceGroup::getNumberOfEntries, "Returns the number of entries in the group")
    .def("getNames", &WorkspaceGroup::getNames, "Returns the names of the entries in the group")
    .def("contains", &WorkspaceGroup::contains, "Returns true if the given name is in the group")
    .def("add", &WorkspaceGroup::add, "Add a name to the group")
    .def("remove", &WorkspaceGroup::remove, "Remove a name from the group")
    .def("getItemAsWorkspace",&getItemAsWorkspace, "Return the item at the given index, not casted up automatically. For internal use.")
    // ------------ Operators --------------------------------
    .def("__len__", &WorkspaceGroup::getNumberOfEntries)
    .def("__contains__", &WorkspaceGroup::contains)
    .def("__getitem__",&getItemUpcasted)
  ;

  DECLARE_SINGLEVALUETYPEHANDLER(WorkspaceGroup, DataItem_sptr);
}

