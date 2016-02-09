#ifndef VATES_API_SINGLE_WORKSPACE_PROVIDER_H_
#define VATES_API_SINGLE_WORKSPACE_PROVIDER_H_

#include "WorkspaceProvider.h"
#include "MantidAPI/Workspace_fwd.h"

namespace Mantid
{
namespace VATES
{

/**
 * The SingleWorkspaceProvider class holds a reference to a single
 * IMDWorkspace. Note that this
 *        means that the workspace does not have to live in the ADS.
 */
class DLLExport SingleWorkspaceProvider : public WorkspaceProvider
{
public:
    SingleWorkspaceProvider(Mantid::API::Workspace_sptr workspace);
    bool canProvideWorkspace(std::string wsName) const override;
    Mantid::API::Workspace_sptr fetchWorkspace(std::string wsName) const override;
    void disposeWorkspace(std::string wsName) const override;

private:
    Mantid::API::Workspace_sptr m_workspace;
};
}
}

#endif
