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
template MANTID_API_DLL class Mantid::API::WorkspaceListProperty<
    Mantid::API::Workspace>;
template MANTID_API_DLL class Mantid::API::WorkspaceListProperty<
    Mantid::API::MatrixWorkspace>;
template MANTID_API_DLL class Mantid::API::WorkspaceListProperty<
    Mantid::API::ITableWorkspace>;
template MANTID_API_DLL class Mantid::API::WorkspaceListProperty<
    Mantid::API::IEventWorkspace>;
template MANTID_API_DLL class Mantid::API::WorkspaceListProperty<
    Mantid::API::WorkspaceGroup>;
///@endcond TEMPLATE
} // namespace API
} // namespace Mantid