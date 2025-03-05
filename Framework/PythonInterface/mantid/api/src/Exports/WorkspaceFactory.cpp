// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/core/Policies/AsType.h"

#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>

using namespace boost::python;
using namespace Mantid::API;
using namespace Mantid::PythonInterface::Policies;

GET_POINTER_SPECIALIZATION(WorkspaceFactoryImpl)

namespace {
/**
 * Creates from workspace using an existing one as a template.
 * The C++ implementation accepts a std::shared<const MatrixWorkspace> which
 * we cannot currently handle. This allows a std::shared<MatrixWorkspace> to
 *be passed
 * in an converted. See MantidPythonInterface/core/Policies/RemoveConst for
 *the full
 * explanation of why this is necessary
 *
 *  @param  parent    A shared pointer to the parent workspace
 *  @param  NVectors  (Optional) The number of vectors/histograms/detectors in
 *the workspace
 *  @param  XLength   (Optional) The number of X data points/bin boundaries in
 *each vector (must all be the same)
 *  @param  YLength   (Optional) The number of data/error points in each vector
 *(must all be the same)
 *  @return A shared pointer to the newly created instance
 *  @throw  std::out_of_range If invalid (0 or less) size arguments are given
 *  @throw  NotFoundException If the class is not registered in the factory
 **/
Workspace_sptr createFromParentPtr(WorkspaceFactoryImpl const *const self, const MatrixWorkspace_sptr &parent,
                                   size_t NVectors = size_t(-1), size_t XLength = size_t(-1),
                                   size_t YLength = size_t(-1)) {
  return self->create(parent, NVectors, XLength, YLength);
}

/// Overload generator for create
GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")

// cppcheck-suppress unknownMacro
BOOST_PYTHON_FUNCTION_OVERLOADS(createFromParent_Overload, createFromParentPtr, 2, 5)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(createTable_Overload, createTable, 0, 1)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(createPeaks_Overload, createPeaks, 0, 1)

GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")
} // namespace

void export_WorkspaceFactory() {
  const char *createFromParentDoc = "Create a workspace based on the given "
                                    "one. The meta-data, instrument etc are "
                                    "copied from the input"
                                    "If the size parameters are passed then "
                                    "the workspace will be a different size.";

  const char *createFromScratchDoc = "Create a clean new worksapce of the given size.";
  using createFromScratchPtr = MatrixWorkspace_sptr (WorkspaceFactoryImpl::*)(const std::string &, const size_t &,
                                                                              const size_t &, const size_t &) const;

  class_<WorkspaceFactoryImpl, boost::noncopyable>("WorkspaceFactoryImpl", no_init)
      .def("create", &createFromParentPtr,
           createFromParent_Overload(createFromParentDoc, (arg("self"), arg("parent"), arg("NVectors") = -1,
                                                           arg("XLength") = -1, arg("YLength") = -1)))

      .def("create", (createFromScratchPtr)&WorkspaceFactoryImpl::create, createFromScratchDoc,
           return_value_policy<AsType<Workspace_sptr>>(),
           (arg("self"), arg("className"), arg("NVectors"), arg("XLength"), arg("YLength")))

      .def("createTable", &WorkspaceFactoryImpl::createTable,
           createTable_Overload(
               "Creates an empty TableWorkspace",
               (arg("self"), arg("className") = "TableWorkspace"))[return_value_policy<AsType<Workspace_sptr>>()])

      .def("createPeaks", &WorkspaceFactoryImpl::createPeaks,
           createPeaks_Overload(
               "Creates an empty PeaksWorkspace",
               (arg("self"), arg("className") = "PeaksWorkspace"))[return_value_policy<AsType<Workspace_sptr>>()])

      .def("Instance", &WorkspaceFactory::Instance, return_value_policy<reference_existing_object>(),
           "Returns the single instance of this class.")
      .staticmethod("Instance");
}
