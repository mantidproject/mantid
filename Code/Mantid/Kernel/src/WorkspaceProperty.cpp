#include "MantidKernel/WorkspaceProperty.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{
namespace Kernel
{

/// @cond

template DLLExport class WorkspaceProperty<API::Workspace>;
template DLLExport class WorkspaceProperty<DataObjects::Workspace1D>;
template DLLExport class WorkspaceProperty<DataObjects::Workspace2D>;

/// @endcond

} // namespace Kernel
} // namespace Mantid
