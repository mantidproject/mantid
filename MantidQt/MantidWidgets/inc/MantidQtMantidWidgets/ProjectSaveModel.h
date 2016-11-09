#ifndef MANTIDQT_MANTIDWIDGETS_PROJECTSAVEMODEL_H
#define MANTIDQT_MANTIDWIDGETS_PROJECTSAVEMODEL_H

#include "MantidAPI/Workspace.h"
#include "MantidQtAPI/IProjectSerialisable.h"

#include <vector>
#include <unordered_map>

namespace  MantidQt {
namespace MantidWidgets {

class ProjectSaveModel
{
public:
  ProjectSaveModel(std::vector<MantidQt::API::IProjectSerialisable*> windows);

  /// Check if a workspace has any windows attached to it
  bool hasWindows(const std::string& ws) const;
  /// Get all window handles for this workspace
  std::vector<MantidQt::API::IProjectSerialisable*> getWindows(const std::string& wsName) const;
  /// Get all window handles for a collection of workspace names
  std::vector<MantidQt::API::IProjectSerialisable *> getUniqueWindows(const std::vector<std::string> &wsNames) const;
  /// Get all window names for a collection of workspace names
  std::vector<std::string> getWindowNames(const std::vector<std::string> &wsNames) const;
  /// Get all workspace names
  std::vector<std::string> getWorkspaceNames() const;

private:
  std::vector<Mantid::API::Workspace_sptr> getWorkspaces() const;

  // Instance variables
  /// Map to hold which windows are associated with a workspace
  std::unordered_map<std::string, std::vector<MantidQt::API::IProjectSerialisable*>> m_workspaceWindows;
};

} // CustomInterfaces
} // MantidQt

#endif // MANTIDQT_MANTIDWIDGETS_PROJECTSAVEMODEL_H
