#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"

// WorkspaceProperty implementation
#include "MantidAPI/WorkspaceProperty.tcc"

namespace Mantid {
// Note that this file is part of DataObjects, but we are injecting explicit
// instantiations into API. This does not extend or modify API.
namespace API {
///@cond TEMPLATE
template class MANTID_DATAOBJECTS_DLL
    Mantid::API::WorkspaceProperty<Mantid::DataObjects::EventWorkspace>;
template class MANTID_DATAOBJECTS_DLL
    Mantid::API::WorkspaceProperty<Mantid::DataObjects::SpecialWorkspace2D>;
template class MANTID_DATAOBJECTS_DLL
    Mantid::API::WorkspaceProperty<Mantid::DataObjects::SplittersWorkspace>;
template class MANTID_DATAOBJECTS_DLL
    Mantid::API::WorkspaceProperty<Mantid::DataObjects::TableWorkspace>;
template class MANTID_DATAOBJECTS_DLL
    Mantid::API::WorkspaceProperty<Mantid::DataObjects::Workspace2D>;
template class MANTID_DATAOBJECTS_DLL
    Mantid::API::WorkspaceProperty<Mantid::DataObjects::WorkspaceSingleValue>;
template class MANTID_DATAOBJECTS_DLL
    Mantid::API::WorkspaceProperty<Mantid::DataObjects::GroupingWorkspace>;
template class MANTID_DATAOBJECTS_DLL
    Mantid::API::WorkspaceProperty<Mantid::DataObjects::PeaksWorkspace>;
template class MANTID_DATAOBJECTS_DLL
    Mantid::API::WorkspaceProperty<Mantid::DataObjects::MaskWorkspace>;
template class MANTID_DATAOBJECTS_DLL
    Mantid::API::WorkspaceProperty<Mantid::DataObjects::OffsetsWorkspace>;
///@endcond TEMPLATE
} // namespace API
} // namespace Mantid
