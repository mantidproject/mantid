// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"
#include <boost/python/class.hpp>

using Mantid::DataObjects::GroupingWorkspace;
using Mantid::DataObjects::SpecialWorkspace2D;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(GroupingWorkspace)

void export_GroupingWorkspace() {
  class_<GroupingWorkspace, bases<SpecialWorkspace2D>, boost::noncopyable>(
      "GroupingWorkspace", no_init);

  // register pointers
  RegisterWorkspacePtrToPython<GroupingWorkspace>();
}
