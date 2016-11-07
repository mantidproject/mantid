#ifndef MANTIDQTCUSTOMINTERFACES_PROJECTSAVEMODEL_H
#define MANTIDQTCUSTOMINTERFACES_PROJECTSAVEMODEL_H

#include "MantidAPI/Workspace.h"
#include "MantidQtAPI/IProjectSerialisable.h"

#include <vector>
#include <unordered_map>

namespace  MantidQt {
namespace CustomInterfaces {

class ProjectSaveModel
{
public:
  ProjectSaveModel(std::vector<MantidQt::API::IProjectSerialisable*> windows);

  /// Check if a workspace has any windows attached to it
  bool hasWindows(const std::string& ws) const;
  /// Get all window handles for this workspace
  std::vector<MantidQt::API::IProjectSerialisable*> getWindows(const std::string& wsName) const;
  /// Get all workspace names
  std::set<std::string> getWorkspaceNames() const;

private:
  std::vector<Mantid::API::Workspace_sptr> getWorkspaces() const;

  // Instance variables
  /// Map to hold which windows are associated with a workspace
  std::unordered_map<std::string, std::vector<MantidQt::API::IProjectSerialisable*>> m_workspaceWindows;
};

} // CustomInterfaces
} // MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_PROJECTSAVEMODEL_H
