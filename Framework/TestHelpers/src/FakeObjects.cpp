#include "MantidTestHelpers/FakeObjects.h"

// Property implementations
#include "MantidAPI/WorkspaceProperty.tcc"
#include "MantidKernel/PropertyWithValue.tcc"

namespace Mantid {
namespace Kernel {
///@cond TEMPLATE
template class DLLExport PropertyWithValue<boost::shared_ptr<WorkspaceTester>>;
template class DLLExport
    PropertyWithValue<boost::shared_ptr<TableWorkspaceTester>>;
///@endcond TEMPLATE
} // namespace Kernel
namespace API {
///@cond TEMPLATE
template class DLLExport Mantid::API::WorkspaceProperty<WorkspaceTester>;
template class DLLExport Mantid::API::WorkspaceProperty<TableWorkspaceTester>;
///@endcond TEMPLATE
} // namespace API
} // namespace Mantid
