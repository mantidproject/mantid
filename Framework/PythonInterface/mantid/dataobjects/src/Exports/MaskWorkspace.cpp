// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidPythonInterface/api/RegisterWorkspacePtrToPython.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>

using Mantid::API::IMaskWorkspace;
using Mantid::DataObjects::MaskWorkspace;
using Mantid::DataObjects::SpecialWorkspace2D;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(MaskWorkspace)

void export_MaskWorkspace() {
  class_<MaskWorkspace, bases<SpecialWorkspace2D, IMaskWorkspace>, boost::noncopyable>("MaskWorkspace", no_init)
      .def("getMaskedDetectors", &MaskWorkspace::getMaskedDetectors, arg("self"), "Returns all masked detector IDs.");

  // register pointers
  // cppcheck-suppress unusedScopedObject
  RegisterWorkspacePtrToPython<MaskWorkspace>();
}
