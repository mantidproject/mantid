#include "MantidVatesAPI/SingleWorkspaceProvider.h"
#include "MantidAPI/Workspace.h"

namespace Mantid {
namespace VATES {

SingleWorkspaceProvider::SingleWorkspaceProvider(
    Mantid::API::Workspace_sptr workspace)
    : m_workspace(workspace) {}

/**
 * SingleWorkspaceProvider::canProvideWorkspace
 * @return true if the pointer the workspace is not nullptr else false
 */
bool SingleWorkspaceProvider::canProvideWorkspace(std::string) const {
  return m_workspace != nullptr;
}

/**
 * SingleWorkspaceProvider::fetchWorkspace
 * @returns the underlying workspace
 */
Mantid::API::Workspace_sptr
SingleWorkspaceProvider::fetchWorkspace(std::string) const {
  return m_workspace;
}

/**
 * SingleWorkspaceProvider::disposeWorkspace: this is left blank, as we don't
 * want
 *        to be able to dispose of the workspace
 */
void SingleWorkspaceProvider::disposeWorkspace(std::string) const {}
} // namespace VATES
} // namespace Mantid
