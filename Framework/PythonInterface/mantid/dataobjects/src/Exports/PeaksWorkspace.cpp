// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>

#include "MantidPythonInterface/api/RegisterWorkspacePtrToPython.h"

using Mantid::API::IPeaksWorkspace;
using Mantid::DataObjects::PeaksWorkspace;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(PeaksWorkspace)

void export_PeaksWorkspace() {

  class_<PeaksWorkspace, bases<IPeaksWorkspace>, boost::noncopyable>("PeaksWorkspace", no_init);

  // register pointers
  // cppcheck-suppress unusedScopedObject
  RegisterWorkspacePtrToPython<PeaksWorkspace>();
}
