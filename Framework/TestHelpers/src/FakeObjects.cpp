#include "MantidTestHelpers/FakeObjects.h"

// WorkspaceProperty implementation
#include "MantidAPI/WorkspaceProperty.tcc"

namespace Mantid {
// Note that this file is part of DataObjects, but we are injecting explicit
// instantiations into API. This does not extend or modify API.
namespace API {
///@cond TEMPLATE
template class MANTID_API_DLL Mantid::API::WorkspaceProperty<WorkspaceTester>;
template class MANTID_API_DLL
    Mantid::API::WorkspaceProperty<TableWorkspaceTester>;
///@endcond TEMPLATE
} // namespace API
} // namespace Mantid
