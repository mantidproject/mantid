// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidPythonInterface/api/RegisterWorkspacePtrToPython.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>

using Mantid::API::ISplittersWorkspace;
using Mantid::DataObjects::SplittersWorkspace;
using Mantid::DataObjects::TableWorkspace;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(SplittersWorkspace)

void export_SplittersWorkspace() {
  class_<SplittersWorkspace, bases<TableWorkspace, ISplittersWorkspace>, boost::noncopyable>("SplittersWorkspace",
                                                                                             no_init);

  // register pointers
  // cppcheck-suppress unusedScopedObject
  RegisterWorkspacePtrToPython<SplittersWorkspace>();
}
