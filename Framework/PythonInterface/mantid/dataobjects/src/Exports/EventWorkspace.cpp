// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"

#include <boost/python/class.hpp>
#include <boost/python/object/inheritance.hpp>

using Mantid::API::IEventWorkspace;
using Mantid::DataObjects::EventWorkspace;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(EventWorkspace)

void export_EventWorkspace() {
  class_<EventWorkspace, bases<IEventWorkspace>, boost::noncopyable>(
      "EventWorkspace", no_init);

  // register pointers
  RegisterWorkspacePtrToPython<EventWorkspace>();
}
