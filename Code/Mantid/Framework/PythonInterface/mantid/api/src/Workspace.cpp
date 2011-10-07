#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidPythonInterface/kernel/PropertyWithValue.h"
#include "MantidPythonInterface/kernel/PropertyMarshal.h"

#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/args.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using namespace Mantid::API;
using Mantid::Kernel::PropertyWithValue;
using Mantid::Kernel::DataItem;
using Mantid::Kernel::DataItem_sptr;
using namespace boost::python;

namespace
{
  ///@cond
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Workspace_isDirtyOverloads, Workspace::isDirty, 0, 1);
  ///@endcond
}

void export_Workspace()
{
  register_ptr_to_python<Workspace_sptr>();

  object cls = class_<Workspace, bases<DataItem>, boost::noncopyable>("Workspace", no_init)
    .def("id", &Workspace::id, "Returns the string ID of the workspace type")
    .def("get_title", &Workspace::getTitle, "Returns the title of the workspace")
    .def("get_comment", &Workspace::getComment, return_value_policy<copy_const_reference>(), "Returns the comment field on the workspace")
    .def("get_name", &Workspace::getName, return_value_policy<copy_const_reference>(), "Returns the workspace name")
    .def("is_dirty", &Workspace::isDirty, Workspace_isDirtyOverloads(args("n"), "True if the workspace has run more than n algorithms (Default=1)"))
    .def("get_memory_size", &Workspace::getMemorySize, "Returns the memory footprint of the workspace in KB")
    .def("threadsafe", &Workspace::threadSafe, "Returns true if the workspace is safe to access from multiple threads")
    .def("get_history", &Workspace::getHistory, return_value_policy<copy_const_reference>(),
         "Return read-only access to the workspace history")
    ;

  // @todo: Simplify this stuff with a Macro
  Mantid::PythonInterface::PropertyMarshal::insert((PyTypeObject*)cls.ptr(), new Mantid::PythonInterface::TypedHandler<DataItem_sptr>());

}

void export_WorkspaceProperty()
{
  // Interface
  class_<IWorkspaceProperty, boost::noncopyable>("IWorkspaceProperty", no_init)
      ;

  // Export the WorkspaceProperty hierarchy so that the plain Property* return from getPointerToProperty is automatically upcast to a WorkspaceProperty*
  EXPORT_PROP_W_VALUE(Workspace_sptr);
  register_ptr_to_python<WorkspaceProperty<Workspace>*>();
  class_<WorkspaceProperty<Workspace>, bases<PropertyWithValue<Workspace_sptr>,IWorkspaceProperty>, boost::noncopyable>("WorkspaceProperty_Workspace", no_init)
      ;
}

