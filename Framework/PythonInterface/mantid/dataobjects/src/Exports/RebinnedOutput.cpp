// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"

#include <boost/python/class.hpp>

using Mantid::DataObjects::RebinnedOutput;
using Mantid::DataObjects::Workspace2D;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(RebinnedOutput)

void export_RebinnedOutput() {
  class_<RebinnedOutput, bases<Workspace2D>, boost::noncopyable>(
      "RebinnedOutput", no_init);

  // register pointers
  RegisterWorkspacePtrToPython<RebinnedOutput>();
}
