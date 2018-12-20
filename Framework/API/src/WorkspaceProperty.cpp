#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/ISplittersWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"

// WorkspaceProperty implementation
#include "MantidAPI/WorkspaceProperty.tcc"

namespace Mantid {
namespace API {
///@cond TEMPLATE
template class MANTID_API_DLL
    Mantid::API::WorkspaceProperty<Mantid::API::Workspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspaceProperty<Mantid::API::IEventWorkspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspaceProperty<Mantid::API::IMDEventWorkspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspaceProperty<Mantid::API::IMDHistoWorkspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspaceProperty<Mantid::API::IMDWorkspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspaceProperty<Mantid::API::MatrixWorkspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspaceProperty<Mantid::API::IPeaksWorkspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspaceProperty<Mantid::API::ITableWorkspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspaceProperty<Mantid::API::WorkspaceGroup>;
///@endcond TEMPLATE
} // namespace API
} // namespace Mantid
