// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidPythonInterface/api/RegisterWorkspacePtrToPython.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/class.hpp>

using Mantid::API::MatrixWorkspace;
using Mantid::DataObjects::WorkspaceSingleValue;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(WorkspaceSingleValue)

void export_WorkspaceSingleValue() {
  class_<WorkspaceSingleValue, bases<MatrixWorkspace>, boost::noncopyable>("WorkspaceSingleValue", no_init);

  // register pointers
  // cppcheck-suppress unusedScopedObject
  RegisterWorkspacePtrToPython<WorkspaceSingleValue>();
}
