#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/ISplittersWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"

// WorkspacePropertyWithIndex implementation
#include "MantidAPI/WorkspacePropertyWithIndex.tcc"

namespace Mantid {
namespace API {
///@cond TEMPLATE
template class MANTID_API_DLL
    Mantid::API::WorkspacePropertyWithIndex<Mantid::API::MatrixWorkspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspacePropertyWithIndex<Mantid::API::IEventWorkspace>;
///@endcond TEMPLATE
} // namespace API
} // namespace Mantid