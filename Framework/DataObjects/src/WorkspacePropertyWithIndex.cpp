#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"

// WorkspacePropertyWithIndex implementation
#include "MantidAPI/WorkspacePropertyWithIndex.tcc"

namespace Mantid {
// Note that this file is part of DataObjects, but we are injecting explicit
// instantiations into API. This does not extend or modify API.
namespace API {
///@cond TEMPLATE
template class MANTID_DATAOBJECTS_DLL Mantid::API::WorkspacePropertyWithIndex<
    Mantid::DataObjects::EventWorkspace>;
template class MANTID_DATAOBJECTS_DLL Mantid::API::WorkspacePropertyWithIndex<
    Mantid::DataObjects::SpecialWorkspace2D>;
template class MANTID_DATAOBJECTS_DLL
    Mantid::API::WorkspacePropertyWithIndex<Mantid::DataObjects::Workspace2D>;
template class MANTID_DATAOBJECTS_DLL Mantid::API::WorkspacePropertyWithIndex<
    Mantid::DataObjects::WorkspaceSingleValue>;
template class MANTID_DATAOBJECTS_DLL Mantid::API::WorkspacePropertyWithIndex<
    Mantid::DataObjects::GroupingWorkspace>;
template class MANTID_DATAOBJECTS_DLL
    Mantid::API::WorkspacePropertyWithIndex<Mantid::DataObjects::MaskWorkspace>;
template class MANTID_DATAOBJECTS_DLL Mantid::API::WorkspacePropertyWithIndex<
    Mantid::DataObjects::OffsetsWorkspace>;
///@endcond TEMPLATE
} // namespace API
} // namespace Mantid
