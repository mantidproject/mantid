#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/ISplittersWorkspace.h"

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
    Mantid::API::WorkspaceProperty<Mantid::API::IMDWorkspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspaceProperty<Mantid::API::MatrixWorkspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspaceProperty<Mantid::API::ITableWorkspace>;
///@endcond TEMPLATE
} // namespace API
} // namespace Mantid
