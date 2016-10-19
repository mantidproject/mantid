#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"

// WorkspaceProperty implementation
#include "MantidAPI/WorkspaceProperty.tcc"

namespace Mantid {
// Note that this file is part of DataObjects, but we are extending API
namespace API {
///@cond TEMPLATE
template class MANTID_API_DLL
    Mantid::API::WorkspaceProperty<Mantid::DataObjects::EventWorkspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspaceProperty<Mantid::DataObjects::Workspace2D>;
template class MANTID_API_DLL
    Mantid::API::WorkspaceProperty<Mantid::DataObjects::WorkspaceSingleValue>;
///@endcond TEMPLATE
} // namespace API
} // namespace Mantid
