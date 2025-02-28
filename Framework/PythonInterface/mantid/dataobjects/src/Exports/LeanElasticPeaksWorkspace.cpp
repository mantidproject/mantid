// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>

#include "MantidPythonInterface/api/RegisterWorkspacePtrToPython.h"

using Mantid::API::IPeaksWorkspace;
using Mantid::DataObjects::LeanElasticPeaksWorkspace;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(LeanElasticPeaksWorkspace)

void export_LeanElasticPeaksWorkspace() {

  class_<LeanElasticPeaksWorkspace, bases<IPeaksWorkspace>, boost::noncopyable>("LeanElasticPeaksWorkspace", no_init);

  // register pointers
  // cppcheck-suppress unusedScopedObject
  RegisterWorkspacePtrToPython<LeanElasticPeaksWorkspace>();
}
