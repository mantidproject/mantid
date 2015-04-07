#include "MantidAPI/Workspace.h"
#include "MantidPythonInterface/kernel/Registry/DataItemInterface.h"

#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/copy_const_reference.hpp>

using namespace Mantid::API;
using Mantid::Kernel::DataItem;
using Mantid::PythonInterface::Registry::DataItemInterface;
using namespace boost::python;

namespace
{
  ///@cond
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Workspace_isDirtyOverloads, Workspace::isDirty, 0, 1)
  ///@endcond
}

void export_Workspace()
{
  class_<Workspace, bases<DataItem>, boost::noncopyable>("Workspace", no_init)
    .def("getName", &Workspace::getName, return_value_policy<copy_const_reference>(), 
         args("self"), "Returns the name of the workspace. This could be an empty string")
    .def("getTitle", &Workspace::getTitle, args("self"), "Returns the title of the workspace")
    .def("setTitle", &Workspace::setTitle, args("self", "title"))
    .def("getComment", &Workspace::getComment, return_value_policy<copy_const_reference>(), "Returns the comment field on the workspace")
    .def("setComment", &Workspace::setComment, args("self", "comment"))
    .def("isDirty", &Workspace::isDirty, Workspace_isDirtyOverloads(arg("n"), "True if the workspace has run more than n algorithms (Default=1)"))
    .def("getMemorySize", &Workspace::getMemorySize, args("self"), "Returns the memory footprint of the workspace in KB")
    .def("getHistory", (const WorkspaceHistory &(Workspace::*)() const)&Workspace::getHistory, return_value_policy<reference_existing_object>(),
         args("self"), "Return read-only access to the workspace history")
    ;

  //-------------------------------------------------------------------------------------------------

  DataItemInterface<Workspace>();
}
