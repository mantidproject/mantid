// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidFrameworkTestHelpers/FakeObjects.h"

// Property implementations
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.hxx"
#include "MantidKernel/PropertyWithValue.hxx"

DECLARE_WORKSPACE(WorkspaceTester)

namespace Mantid {
namespace Kernel {
///@cond TEMPLATE
template class DLLExport PropertyWithValue<std::shared_ptr<WorkspaceTester>>;
template class DLLExport PropertyWithValue<std::shared_ptr<TableWorkspaceTester>>;
///@endcond TEMPLATE
} // namespace Kernel
namespace API {
///@cond TEMPLATE
template class DLLExport Mantid::API::WorkspaceProperty<WorkspaceTester>;
template class DLLExport Mantid::API::WorkspaceProperty<TableWorkspaceTester>;
///@endcond TEMPLATE
} // namespace API
} // namespace Mantid
