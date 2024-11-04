// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/IProjectSerialisable.h"

#include <QMainWindow>
#include <QWidget>
#include <set>
#include <string>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

struct WindowInfo;
struct WorkspaceInfo;

/** @class IProjectSaveView

IProjectSaveView is the interface for defining the functions that the project
save view needs to implement.
*/
class IProjectSaveView {
public:
  /// Get all window handles passed to the view
  virtual std::vector<MantidQt::API::IProjectSerialisable *> getWindows() = 0;
  /// Get all active python interfaces names passed to the view
  virtual std::vector<std::string> getAllPythonInterfaces() = 0;

  /// Get the names of all checked workspaces
  virtual std::vector<std::string> getCheckedWorkspaceNames() = 0;
  /// Get the names of all unchecked workspaces
  virtual std::vector<std::string> getUncheckedWorkspaceNames() = 0;
  /// Get any checked interface names on the view
  virtual std::vector<std::string> getCheckedPythonInterfaces() = 0;
  /// Get any unchecked interface names on the view
  virtual std::vector<std::string> getUncheckedPythonInterfaces() = 0;
  /// Get the project path
  virtual QString getProjectPath() = 0;
  /// Set the project path
  virtual void setProjectPath(const QString &path) = 0;
  /// Update the workspaces list with a collection of workspace info items
  virtual void updateWorkspacesList(const std::vector<WorkspaceInfo> &workspaces) = 0;
  /// Update the list of interfaces
  virtual void updateInterfacesList(const std::vector<std::string> &interfaces) = 0;
  /// Update the included windows list with a collection of window info items
  virtual void updateIncludedWindowsList(const std::vector<WindowInfo> &windows) = 0;
  /// Update the excluded windows list with a collection of window info items
  virtual void updateExcludedWindowsList(const std::vector<WindowInfo> &windows) = 0;
  /// Remove items from the included window list
  virtual void removeFromIncludedWindowsList(const std::vector<std::string> &windows) = 0;
  /// Remove items from the excluded window list
  virtual void removeFromExcludedWindowsList(const std::vector<std::string> &windows) = 0;
};
} // namespace MantidWidgets
} // namespace MantidQt
