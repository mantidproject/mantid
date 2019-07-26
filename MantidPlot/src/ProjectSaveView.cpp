// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/IProjectSerialisable.h"
#include "MantidQtWidgets/Common/ProjectSavePresenter.h"
#include "MantidQtWidgets/Common/WindowIcons.h"

#include "ProjectSaveView.h"

#include <QFileDialog>

using namespace MantidQt::API;

namespace MantidQt {
namespace MantidWidgets {

/**
 * Create a new instance of the view.
 *
 * @param projectName :: the existing project path
 * @param serialiser :: a ProjectSerialiser instance
 * @param windows :: vector of window handles for the open application
 * @param
 * @param parent :: parent widget for this object
 */
ProjectSaveView::ProjectSaveView(
    const QString &projectName, ProjectSerialiser &serialiser,
    const std::vector<IProjectSerialisable *> &windows,
    const std::vector<std::string> &activePythonInterfaces, QWidget *parent)
    : QDialog(parent), m_serialisableWindows(windows.cbegin(), windows.cend()),
      m_allPythonInterfaces(activePythonInterfaces), m_serialiser(serialiser) {

  m_ui.setupUi(this);
  m_presenter.reset(new ProjectSavePresenter(this));

  if (!checkIfNewProject(projectName))
    m_ui.projectPath->setText(projectName);

  m_ui.saveProgressBar->setValue(0);

  connectSignals();
}

// IProjectSaveView interface implementations
//==============================================================================

/**
 * Get all window handles passed to the view
 * @return a vector of handles to each window
 */
std::vector<API::IProjectSerialisable *> ProjectSaveView::getWindows() {
  return m_serialisableWindows;
}

/**
 * @brief Get all python interfaces that can be saved
 * @return a vector of names of launcher scripts for the interfaces
 */
std::vector<std::string> ProjectSaveView::getAllPythonInterfaces() {
  return m_allPythonInterfaces;
}

/**
 * Get all of the checked workspace names
 * @return a vector of workspace names
 */
std::vector<std::string> ProjectSaveView::getCheckedWorkspaceNames() {
  return getItemsWithCheckState(*m_ui.workspaceList, Qt::CheckState::Checked);
}

/**
 * Get all of the unchecked workspace names
 * @return a vector of workspace names
 */
std::vector<std::string> ProjectSaveView::getUncheckedWorkspaceNames() {
  return getItemsWithCheckState(*m_ui.workspaceList, Qt::CheckState::Unchecked);
}

/**
 * Get all of the checked workspace names
 * @return a vector of workspace names
 */
std::vector<std::string> ProjectSaveView::getCheckedPythonInterfaces() {
  return getItemsWithCheckState(*m_ui.interfaceList, Qt::CheckState::Checked);
}

/**
 * Get all of the unchecked workspace names
 * @return a vector of workspace names
 */
std::vector<std::string> ProjectSaveView::getUncheckedPythonInterfaces() {
  return getItemsWithCheckState(*m_ui.interfaceList, Qt::CheckState::Unchecked);
}

/**
 * Get the project path text
 *
 * This path may or may not exist yet and must be further validated.
 *
 * @return string representing the project path
 */
QString ProjectSaveView::getProjectPath() { return m_ui.projectPath->text(); }

/**
 * Set the project path
 * @param path :: path the project will be saved to
 */
void ProjectSaveView::setProjectPath(const QString &path) {
  m_ui.projectPath->setText(path);
}

/**
 * Update the list of workspaces.
 *
 * This will create one new item in the list for each workspace info
 * object passed
 *
 * @param workspaces :: vector of workspace info objects to add to the view
 */
void ProjectSaveView::updateWorkspacesList(
    const std::vector<WorkspaceInfo> &workspaces) {
  m_ui.workspaceList->clear();
  for (const auto &info : workspaces) {
    addWorkspaceItem(info);
  }
  // pad the header for longish workspace names
  m_ui.workspaceList->header()->resizeSection(0, 300);
}

/**
 * Update the list of interfaces. The names will have `_` characters
 * substituted by a single space character
 * @param workspaces :: vector of interface launcher script names
 */
void ProjectSaveView::updateInterfacesList(
    const std::vector<std::string> &interfaces) {
  m_ui.interfaceList->clear();
  for (const auto &launcherScript : interfaces) {
    const auto originalName = QString::fromStdString(launcherScript);
    auto sanitized = originalName;
    sanitized.replace("_", " ");
    QStringList columns{sanitized};
    auto item = new QTreeWidgetItem(columns);
    item->setCheckState(0, Qt::CheckState::Checked);
    item->setData(0, Qt::UserRole, originalName);
    m_ui.interfaceList->addTopLevelItem(item);
  }
  m_ui.interfaceList->header()->resizeSection(0, 300);
}

/**
 * Update the included windows list
 *
 * This will create one new item in the list for each window info object passed.
 *
 * @param windows :: vector of window info objects to add to the view
 */
void ProjectSaveView::updateIncludedWindowsList(
    const std::vector<WindowInfo> &windows) {
  m_ui.includedWindows->clear();
  for (const auto &info : windows) {
    addWindowItem(m_ui.includedWindows, info);
  }

  resizeWidgetColumns(m_ui.includedWindows);
}

/**
 * Update the excluded windows list
 *
 * This will create one new item in the list for each window info object passed.
 *
 * @param windows :: vector of window info objects to add to the view
 */
void ProjectSaveView::updateExcludedWindowsList(
    const std::vector<WindowInfo> &windows) {
  m_ui.excludedWindows->clear();
  for (const auto &info : windows) {
    addWindowItem(m_ui.excludedWindows, info);
  }

  resizeWidgetColumns(m_ui.excludedWindows);
}

/**
 * Remove a list of windows from the included windows list
 * @param windows :: vector of window names to remove from the include list
 */
void ProjectSaveView::removeFromIncludedWindowsList(
    const std::vector<std::string> &windows) {
  for (const auto &name : windows) {
    removeItem(m_ui.includedWindows, name);
  }
}

/**
 * Remove a list of windows from the excluded windows list
 * @param windows :: vector of window names to remove from the exclude list
 */
void ProjectSaveView::removeFromExcludedWindowsList(
    const std::vector<std::string> &windows) {
  for (const auto &name : windows) {
    removeItem(m_ui.excludedWindows, name);
  }
}

// Private slots
//==============================================================================

/**
 * Slot of handle when a workspace item is changed.
 *
 * When a workspace item is check or unchecked this will notify the presenter to
 * move the windows associated with this workspace to the other (included/
 * excluded) list.
 *
 * @param item :: QTreeWidget item that was modified
 * @param column :: column that was modified
 */
void ProjectSaveView::workspaceItemChanged(QTreeWidgetItem *item, int column) {
  updateWorkspaceListCheckState(item);
  if (item->checkState(column) == Qt::CheckState::Checked) {
    m_presenter->notify(ProjectSavePresenter::Notification::CheckWorkspace);
  } else if (item->checkState(column) == Qt::CheckState::Unchecked) {
    m_presenter->notify(ProjectSavePresenter::Notification::UncheckWorkspace);
  }
}

/**
 * Slot to save the project.
 *
 * This will call the project serialiser passed on construction and save the
 * current state of the project. If only certain workspaces have been selected
 * then only a subset of the workspaces/windows will be passed to the project
 * serialiser.
 *
 * @param checked :: unused argument
 */
void ProjectSaveView::save(bool checked) {
  UNUSED_ARG(checked);

  if (m_ui.projectPath->text().isEmpty()) {
    QMessageBox::warning(this, "Project Save",
                         "Please choose a valid file path", QMessageBox::Ok);
    return;
  }
  auto wsNames = getCheckedWorkspaceNames();
  if (m_serialiser.needsSizeWarning()) {
    auto result = QMessageBox::question(
        this, "Project Save",
        "This project is very large, and so may take a long "
        "time to save. Would you like to continue?",
        QMessageBox::Yes | QMessageBox::No);
    if (result == QMessageBox::No) {
      close();
      return;
    }
  }
  m_presenter->notify(ProjectSavePresenter::Notification::PrepareProjectFolder);
  auto interfaces = getCheckedPythonInterfaces();
  auto windowNames = getIncludedWindowNames();
  auto filePath = m_ui.projectPath->text();
  auto compress = filePath.endsWith(".gz");

  m_serialiser.save(filePath, wsNames, windowNames, interfaces, compress);
  emit projectSaved();

  close();
  // Set the result code after calling close() because
  // close() sets it to QDialog::Rejected
  setResult(QDialog::Accepted);
}

/**
 * Slot to ask the user to find a new project path.
 *
 * This will open a file dialog and let the user choose a new location for the
 * project.
 */
void ProjectSaveView::findFilePath() {
  QString filter = "MantidPlot project (*.mantid);;";
  filter += "Compressed MantidPlot project (*.mantid.gz)";

  QString selectedFilter;
  QString filename = QFileDialog::getSaveFileName(this, "Save Project As", "",
                                                  filter, &selectedFilter);

  m_ui.projectPath->setText(filename);
}

// Private helper methods
//==============================================================================

/**
 * Get all items with a given check state from the workspace list
 * @param tree :: A reference to the tree containing the items
 * @param state :: any of the values in Qt::CheckState
 * @return a vector of names of workspaces that had the state
 */
std::vector<std::string>
ProjectSaveView::getItemsWithCheckState(const QTreeWidget &tree,
                                        const Qt::CheckState state) const {
  std::vector<std::string> names;
  for (int i = 0; i < tree.topLevelItemCount(); ++i) {
    auto item = tree.topLevelItem(i);
    if (item->checkState(0) == state) {
      auto name = item->data(0, Qt::UserRole).toString().toStdString();
      names.push_back(name);
    }

    // now check the child items and append any that have the check state
    for (int i = 0; i < item->childCount(); ++i) {
      auto child = item->child(i);
      if (child->checkState(0) == state) {
        auto childName = child->text(0).toStdString();
        names.push_back(childName);
      }
    }
  }
  return names;
}

/**
 * Get any included window names
 *
 * These will depend on which workspaces are checked in the view
 *
 * @return vector of included window names
 */
std::vector<std::string> ProjectSaveView::getIncludedWindowNames() const {
  std::vector<std::string> names;
  for (int i = 0; i < m_ui.includedWindows->topLevelItemCount(); ++i) {
    auto item = m_ui.includedWindows->topLevelItem(i);
    auto name = item->text(0).toStdString();
    names.push_back(name);
  }
  return names;
}

/**
 * Remove an item from a QTreeWidget
 * @param widget :: handle to the widget
 * @param name :: name to match widget items with
 */
void ProjectSaveView::removeItem(QTreeWidget *widget, const std::string &name) {
  auto qname = QString::fromStdString(name);
  auto items = widget->findItems(qname, Qt::MatchContains);
  for (auto item : items) {
    delete item;
  }
}

/**
 * Add a new window item to the view
 * @param widget :: the widget to add the item to
 * @param info :: window info object with data to add
 */
void ProjectSaveView::addWindowItem(QTreeWidget *widget,
                                    const WindowInfo &info) {
  QStringList lst;
  WindowIcons icons;
  lst << QString::fromStdString(info.name);
  lst << QString::fromStdString(info.type);

  auto item = new QTreeWidgetItem(lst);
  if (!info.icon_id.empty())
    item->setIcon(0, icons.getIcon(info.type));
  widget->addTopLevelItem(item);
}

/**
 * Add a new workspace item to the view
 * @param info :: workspace info object with data to add
 */
void ProjectSaveView::addWorkspaceItem(const WorkspaceInfo &info) {
  auto item = makeWorkspaceItem(info);

  for (const auto &subInfo : info.subWorkspaces) {
    auto subItem = makeWorkspaceItem(subInfo);
    item->addChild(subItem);
  }

  m_ui.workspaceList->addTopLevelItem(item);
}

/**
 * Build a new QTreeWidgetItem for a WorkspaceInfo
 * @param info :: reference to the WorkspaceInfo to make an item for
 * @return new QTreeWidgetItem for the info object
 */
QTreeWidgetItem *
ProjectSaveView::makeWorkspaceItem(const WorkspaceInfo &info) const {
  QStringList lst;
  lst << QString::fromStdString(info.name);
  lst << QString::fromStdString(info.type);
  lst << QString::fromStdString(info.size);
  lst << QString::number(info.numWindows);

  auto item = new QTreeWidgetItem(lst);

  if (!info.icon_id.empty())
    item->setIcon(0, getQPixmap(info.icon_id));
  item->setCheckState(0, Qt::CheckState::Checked);
  item->setData(0, Qt::UserRole, lst[0]);

  return item;
}

/**
 * Check if project path is a new unsaved project or has been saved before
 * @param projectName :: path to the project
 * @return whether the project already exists
 */
bool ProjectSaveView::checkIfNewProject(const QString &projectName) const {
  return (projectName == "untitled" ||
          projectName.endsWith(".opj", Qt::CaseInsensitive) ||
          projectName.endsWith(".ogm", Qt::CaseInsensitive) ||
          projectName.endsWith(".ogw", Qt::CaseInsensitive) ||
          projectName.endsWith(".ogg", Qt::CaseInsensitive));
}

/**
 * Resize the columns of a widget to match the length of the contents
 * @param widget :: the widget to resize
 */
void ProjectSaveView::resizeWidgetColumns(QTreeWidget *widget) {
  for (int i = 0; i < widget->topLevelItemCount(); ++i)
    widget->resizeColumnToContents(i);
}

/**
 * Connect the internal signals
 */
void ProjectSaveView::connectSignals() {
  // Connect signal to listen for when workspaces are changed
  // (checked/unchecked)
  connect(m_ui.workspaceList, SIGNAL(itemChanged(QTreeWidgetItem *, int)), this,
          SLOT(workspaceItemChanged(QTreeWidgetItem *, int)));

  connect(m_ui.btnBrowseFilePath, SIGNAL(clicked(bool)), this,
          SLOT(findFilePath()));
  connect(m_ui.btnSave, SIGNAL(clicked(bool)), this, SLOT(save(bool)));
  connect(m_ui.btnCancel, SIGNAL(clicked(bool)), this, SLOT(close()));

  connect(&m_serialiser, SIGNAL(setProgressBarRange(int, int)),
          m_ui.saveProgressBar, SLOT(setRange(int, int)));
  connect(&m_serialiser, SIGNAL(setProgressBarValue(int)), m_ui.saveProgressBar,
          SLOT(setValue(int)));
}

/**
 * Update other items that are parents/children of this item.
 *
 * This makes sure that the state of the checkboxes are always logically
 * consistent. E.g. is a parent item is checked, so are all its children
 *
 * @param item :: item that has changed check state
 */
void ProjectSaveView::updateWorkspaceListCheckState(QTreeWidgetItem *item) {
  // block signals so we don't trigger more updates to widget items
  blockSignals(true);

  // update child check state
  // children should match the check state of the parent item
  auto checkState = item->checkState(0);
  for (int i = 0; i < item->childCount(); ++i) {
    item->child(i)->setCheckState(0, checkState);
  }

  // the parent item should be unchecked if any single child is unchecked
  auto parent = item->parent();
  if (parent && checkState == Qt::CheckState::Unchecked)
    parent->setCheckState(0, checkState);

  blockSignals(false);
}
} // namespace MantidWidgets
} // namespace MantidQt
