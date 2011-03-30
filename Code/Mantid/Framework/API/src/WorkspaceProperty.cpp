#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"

namespace Mantid
{
namespace API
{
///@cond TEMPLATE
template DLLExport class Mantid::API::WorkspaceProperty<Mantid::API::Workspace>;
template DLLExport class Mantid::API::WorkspaceProperty<Mantid::API::IEventWorkspace>;
template DLLExport class Mantid::API::WorkspaceProperty<Mantid::API::IMDEventWorkspace>;
template DLLExport class Mantid::API::WorkspaceProperty<Mantid::API::IMDWorkspace>;
template DLLExport class Mantid::API::WorkspaceProperty<Mantid::API::MatrixWorkspace>;
///@endcond TEMPLATE
} // namespace API
} // namespace Mantid
