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
#include "MantidAPI/WorkspacePropertyWithIndex.tcc"

namespace Mantid {
namespace API {
///@cond TEMPLATE
template class MANTID_API_DLL
    Mantid::API::WorkspacePropertyWithIndex<Mantid::API::Workspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspacePropertyWithIndex<Mantid::API::IEventWorkspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspacePropertyWithIndex<Mantid::API::IMDEventWorkspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspacePropertyWithIndex<Mantid::API::IMDHistoWorkspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspacePropertyWithIndex<Mantid::API::IMDWorkspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspacePropertyWithIndex<Mantid::API::MatrixWorkspace>;

///@endcond TEMPLATE
} // namespace API
} // namespace Mantid
