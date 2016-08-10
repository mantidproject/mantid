#include "MantidAPI/WorkspaceListProperty.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/ISplittersWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/IPropertyManager.h"

namespace Mantid {
namespace API {
///@cond TEMPLATE
template class MANTID_API_DLL
    Mantid::API::WorkspaceListProperty<Mantid::API::Workspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspaceListProperty<Mantid::API::MatrixWorkspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspaceListProperty<Mantid::API::ITableWorkspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspaceListProperty<Mantid::API::IEventWorkspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspaceListProperty<Mantid::API::WorkspaceGroup>;
///@endcond TEMPLATE
} // namespace API
} // namespace Mantid