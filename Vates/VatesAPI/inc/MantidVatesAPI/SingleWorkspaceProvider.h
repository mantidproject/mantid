#ifndef VATES_API_SINGLE_WORKSPACE_PROVIDER_H_
#define VATES_API_SINGLE_WORKSPACE_PROVIDER_H_

#include "WorkspaceProvider.h"
#include "MantidAPI/IMDWorkspace.h"

namespace Mantid {
namespace VATES {

/**
 * @brief The SingleWorkspaceProvider class holds a reference to a single IMDWorkspace. Note that this
 *        means that the workspace does not have to live in the ADS.
 */
class DLLExport SingleWorkspaceProvider : public WorkspaceProvider{
public:
  SingleWorkspaceProvider(Mantid::API::IMDWorkspace_sptr workspace);
  bool canProvideWorkspace(std::string wsName) const;
  Mantid::API::Workspace_sptr fetchWorkspace(std::string wsName) const;
  void disposeWorkspace(std::string wsName) const;
private:
  Mantid::API::IMDWorkspace_sptr m_workspace = nullptr;
};

}
}

#endif
