// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_MANTIDWIDGETS_PROJECTSAVEMODEL_H
#define MANTIDQT_MANTIDWIDGETS_PROJECTSAVEMODEL_H

#include "MantidAPI/Workspace.h"
#include "MantidQtWidgets/Common/IProjectSerialisable.h"
#include "MantidQtWidgets/Common/pixmaps.h"

#include "DllOption.h"
#include <unordered_map>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

// POD structs to pass information to the view
//==============================================================================

struct WorkspaceInfo {
  std::string name;
  std::string type;
  std::string size;
  std::string icon_id;
  size_t numWindows;
  std::vector<WorkspaceInfo> subWorkspaces;

  bool operator==(const WorkspaceInfo &b) const { return name == b.name; }
};

struct WindowInfo {
  std::string name;
  std::string type;
  std::string icon_id;

  bool operator==(const WindowInfo &b) const { return name == b.name; }
};

// Model definition
//==============================================================================

class EXPORT_OPT_MANTIDQT_COMMON ProjectSaveModel {
public:
  /// Construct a new model instance with vector of window handles
  ProjectSaveModel(std::vector<MantidQt::API::IProjectSerialisable *> windows,
                   std::vector<std::string> activePythonInterfaces =
                       std::vector<std::string>());

  /// Check if a workspace has any windows attached to it
  bool hasWindows(const std::string &ws) const;
  /// Get all window names for a collection of workspace names
  std::vector<std::string>
  getWindowNames(const std::vector<std::string> &wsNames) const;
  /// Get all workspace names
  std::vector<std::string> getWorkspaceNames() const;
  /// Return the list of python interfaces that can be saved
  std::vector<std::string> getAllPythonInterfaces() const;
  /// Get all window information for a collection of workspaces
  std::vector<WindowInfo>
  getWindowInformation(const std::vector<std::string> &wsNames,
                       bool includeUnattached = false) const;
  /// Get all workspace information
  std::vector<WorkspaceInfo> getWorkspaceInformation() const;
  /// Get all window handles for this workspace
  std::vector<MantidQt::API::IProjectSerialisable *>
  getWindows(const std::string &wsName) const;
  /// Get all window handles for a collection of workspace names
  std::vector<MantidQt::API::IProjectSerialisable *>
  getUniqueWindows(const std::vector<std::string> &wsNames) const;
  /// Get all workspaces from the ADS
  std::vector<Mantid::API::Workspace_sptr> getWorkspaces() const;
  /// Check if the size of the project is > than the warning size.
  bool needsSizeWarning(const std::vector<std::string> &wsNames);
  /// Find the size of a project from a list of workspace names.
  virtual size_t getProjectSize(const std::vector<std::string> &wsNames);

private:
  /// Create a workspace info object for this workspace
  WorkspaceInfo
  makeWorkspaceInfoObject(Mantid::API::Workspace_const_sptr ws) const;

  WindowInfo
  makeWindowInfoObject(MantidQt::API::IProjectSerialisable *window) const;

  // Instance variables

  /// Map to hold which windows are associated with a workspace
  std::unordered_map<std::string,
                     std::vector<MantidQt::API::IProjectSerialisable *>>
      m_workspaceWindows;

  std::vector<MantidQt::API::IProjectSerialisable *> m_unattachedWindows;
  std::vector<std::string> m_activePythonInterfaces;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQT_MANTIDWIDGETS_PROJECTSAVEMODEL_H
