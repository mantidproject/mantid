// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/ISplittersWorkspace.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"
#include <boost/python/class.hpp>
#include <boost/python/return_internal_reference.hpp>

using Mantid::API::ISplittersWorkspace;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(ISplittersWorkspace)

void export_ISplittersWorkspace() {
  class_<ISplittersWorkspace, boost::noncopyable>("ISplittersWorkspace",
                                                  no_init)
      .def("getNumberSplitters", &ISplittersWorkspace::getNumberSplitters,
           arg("self"), "Returns the number of splitters within the workspace");

  // register pointers
  RegisterWorkspacePtrToPython<ISplittersWorkspace>();
}
