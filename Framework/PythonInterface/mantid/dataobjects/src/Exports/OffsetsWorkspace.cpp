// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidPythonInterface/api/RegisterWorkspacePtrToPython.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>

using Mantid::DataObjects::OffsetsWorkspace;
using Mantid::DataObjects::SpecialWorkspace2D;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(OffsetsWorkspace)

void export_OffsetsWorkspace() {
  class_<OffsetsWorkspace, bases<SpecialWorkspace2D>, boost::noncopyable>("OffsetsWorkspace", no_init);

  // register pointers
  // cppcheck-suppress unusedScopedObject
  RegisterWorkspacePtrToPython<OffsetsWorkspace>();
}
