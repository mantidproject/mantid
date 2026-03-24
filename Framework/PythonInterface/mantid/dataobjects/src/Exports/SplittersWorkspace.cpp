// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidPythonInterface/api/RegisterWorkspacePtrToPython.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/make_constructor.hpp>

using Mantid::API::ISplittersWorkspace;
using Mantid::API::WorkspaceFactory;
using Mantid::DataObjects::SplittersWorkspace;
using Mantid::DataObjects::TableWorkspace;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(SplittersWorkspace)

namespace {
Mantid::API::Workspace_sptr makeSplittersWorkspace() {
  return WorkspaceFactory::Instance().createTable("SplittersWorkspace");
}
} // namespace

void export_SplittersWorkspace() {
  class_<SplittersWorkspace, bases<TableWorkspace, ISplittersWorkspace>, boost::noncopyable>("SplittersWorkspace",
                                                                                             no_init)
      .def("__init__", make_constructor(&makeSplittersWorkspace));

  // register pointers
  RegisterWorkspacePtrToPython<SplittersWorkspace>();
}
