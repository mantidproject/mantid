// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/LeanPeaksWorkspace.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>

#include "MantidPythonInterface/api/RegisterWorkspacePtrToPython.h"

using Mantid::API::IPeaksWorkspace;
using Mantid::DataObjects::LeanPeaksWorkspace;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(LeanPeaksWorkspace)

void export_LeanPeaksWorkspace() {

  class_<LeanPeaksWorkspace, bases<IPeaksWorkspace>, boost::noncopyable>(
      "LeanPeaksWorkspace", no_init);

  // register pointers
  RegisterWorkspacePtrToPython<LeanPeaksWorkspace>();
}
