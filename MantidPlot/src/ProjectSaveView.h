#ifndef MANTIDQT_MANTIDWIDGETS_PROJECTSAVEVIEW_H
#define MANTIDQT_MANTIDWIDGETS_PROJECTSAVEVIEW_H

#include "MantidQtWidgets/Common/IProjectSaveView.h"
#include "MantidQtWidgets/Common/IProjectSerialisable.h"
#include "MantidQtWidgets/Common/ProjectSavePresenter.h"
#include "ProjectSerialiser.h"
#include "ui_ProjectSave.h"

#include <QMainWindow>
#include <QWidget>
#include <set>
#include <string>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

/** @class ProjectSaveView

ProjectSaveView is the interaces for defining the functions that the project
save view needs to implement.

Copyright &copy; 2011-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class ProjectSaveView : public QDialog, IProjectSaveView {
  Q_OBJECT
public:
  ProjectSaveView(
      const QString &projectName, MantidQt::API::ProjectSerialiser &serialiser,
      const std::vector<MantidQt::API::IProjectSerialisable *> &windows,
      QWidget *parent = nullptr);

  /// Get all of the window handles passed to the view
  std::vector<MantidQt::API::IProjectSerialisable *> getWindows() override;
  /// Get any checked workspace names on the view
  std::vector<std::string> getCheckedWorkspaceNames() override;
  /// Get any unchecked workspace names on the view
  std::vector<std::string> getUncheckedWorkspaceNames() override;
  /// Get the current path of the project
  QString getProjectPath() override;
  /// Set the current path of the project
  void setProjectPath(const QString &path) override;
  /// Update the list of workspaces with a collection of workspace info
  void
  updateWorkspacesList(const std::vector<WorkspaceInfo> &workspaces) override;
  /// Update the list of included windows with a collection of window info
  void
  updateIncludedWindowsList(const std::vector<WindowInfo> &windows) override;
  /// Update the list of excluded windows with a collection of window info
  void
  updateExcludedWindowsList(const std::vector<WindowInfo> &windows) override;
  /// Remove a collection of windows from the included window list
  void removeFromIncludedWindowsList(
      const std::vector<std::string> &windows) override;
  /// Remove a collection of windows from the excluded window list
  void removeFromExcludedWindowsList(
      const std::vector<std::string> &windows) override;

signals:
  /// Signal emitted when the ProjectSerialiser has finished writing
  void projectSaved();

private slots:
  /// Slot to browse for a new project path
  void findFilePath();
  /// Slot to save the project
  void save(bool checked);
  /// Slot to move windows when workspaces are checked/unchecked
  void workspaceItemChanged(QTreeWidgetItem *item, int column);

private:
  /// Get a list of included windows names to be saved
  std::vector<std::string> getIncludedWindowNames() const;
  /// Get the name value of all items with a given check state
  std::vector<std::string>
  getItemsWithCheckState(const Qt::CheckState state) const;
  /// Remove an item from a QTreeWidget
  void removeItem(QTreeWidget *widget, const std::string &name);
  /// Add an new window item QTreeWidget
  void addWindowItem(QTreeWidget *widget, const WindowInfo &info);
  /// Add an new workspace item QTreeWidget
  void addWorkspaceItem(const WorkspaceInfo &info);
  /// Make a new tree widget item from a workspace info object
  QTreeWidgetItem *makeWorkspaceItem(const WorkspaceInfo &info) const;
  /// Check if the project path already existed
  bool checkIfNewProject(const QString &projectName) const;
  /// Resize a QTreeWidgets columns to fit text correctly
  void resizeWidgetColumns(QTreeWidget *widget);
  /// Connect up signals to the interface on initilisation
  void connectSignals();
  /// Update the checked state of the tree when an item is updated
  void updateWorkspaceListCheckState(QTreeWidgetItem *item);

  // Instance variables

  /// List of windows to be serialised
  std::vector<MantidQt::API::IProjectSerialisable *> m_serialisableWindows;
  /// Handle to the presenter for this view
  std::unique_ptr<ProjectSavePresenter> m_presenter;
  /// Handle to the project serialiser
  MantidQt::API::ProjectSerialiser &m_serialiser;
  /// Handle to the UI
  Ui::ProjectSave m_ui;
};
} // namespace MantidWidgets
} // namespace MantidQt
#endif /* MANTIDQT_MANTIDWIDGETS_PROJECTSAVEVIEW_H */
