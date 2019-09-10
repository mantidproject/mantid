// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidKernel/WarningSuppressions.h"

#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/overloads.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::PythonInterface;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(Workspace)

namespace {
///@cond
GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Workspace_isDirtyOverloads,
                                       Workspace::isDirty, 0, 1)
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")

///@endcond
} // namespace

/**
 * DEPRECATED. Use DataItem.name()
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
      .def("getHistory",
           (const WorkspaceHistory &(Workspace::*)() const) &
               Workspace::getHistory,
           arg("self"), return_value_policy<reference_existing_object>(),
           "Return read-only access to the "
           ":class:`~mantid.api.WorkspaceHistory`");

  // register pointers
  RegisterWorkspacePtrToPython<Workspace>();
}
