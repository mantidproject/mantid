#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceHistory.h"

#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"

#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/copy_const_reference.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::PythonInterface;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(Workspace)

namespace {
///@cond
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Workspace_isDirtyOverloads,
                                       Workspace::isDirty, 0, 1)
#ifdef __clang__
#pragma clang diagnostic pop
#endif
///@endcond
}

//--------------------------------------------------------------------------------------
// Deprecated function
//--------------------------------------------------------------------------------------
/**
 * @param self Reference to the calling object
 * @return name of the workspace.
 */
std::string getName(Workspace &self) {
  PyErr_Warn(PyExc_DeprecationWarning,
             ".getName() is deprecated. Use .name() instead.");
  return self.getName();
}

void export_Workspace() {
  class_<Workspace, bases<DataItem>, boost::noncopyable>("Workspace", no_init)
      .def("getName", &getName, arg("self"),
           "Returns the name of the workspace. This could be an empty string")
      .def("getTitle", &Workspace::getTitle, arg("self"),
           "Returns the title of the workspace")
      .def("setTitle", &Workspace::setTitle, (arg("self"), arg("title")),
           "Set the title of the workspace")
      .def("getComment", &Workspace::getComment, arg("self"),
           return_value_policy<copy_const_reference>(),
           "Returns the comment field on the workspace")
      .def("setComment", &Workspace::setComment, (arg("self"), arg("comment")),
           "Set the comment field of the workspace")
      .def("isDirty", &Workspace::isDirty,
           Workspace_isDirtyOverloads((arg("self"), arg("n")),
                                      "True if the workspace has run "
                                      "more than n algorithms "
                                      "(Default=1)"))
      .def("getMemorySize", &Workspace::getMemorySize, arg("self"),
           "Returns the memory footprint of the workspace in KB")
      .def("getHistory", (const WorkspaceHistory &(Workspace::*)() const) &
                             Workspace::getHistory,
           arg("self"), return_value_policy<reference_existing_object>(),
           "Return read-only access to the workspace history");

  // register pointers
  RegisterWorkspacePtrToPython<Workspace>();
}
