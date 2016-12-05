#include "MantidQtAPI/IProjectSerialisable.h"
#include "MantidQtMantidWidgets/ProjectSavePresenter.h"
#include "MantidQtAPI/FileDialogHandler.h"
#include "MantidQtAPI/WindowIcons.h"

#include "ProjectSaveView.h"

using namespace MantidQt::API;

namespace MantidQt {
namespace MantidWidgets {

ProjectSaveView::ProjectSaveView(const QString& projectName,
                                 ProjectSerialiser &serialiser,
                                 const std::vector<IProjectSerialisable*> &windows,
                                 QWidget *parent)
  : QDialog(parent), m_serialisableWindows(windows.cbegin(), windows.cend()),
    m_serialiser(serialiser) {

  m_ui.setupUi(this);
  m_presenter.reset(new ProjectSavePresenter(this));

  if(!checkIfNewProject(projectName))
    m_ui.projectPath->setText(projectName);

  m_ui.saveProgressBar->setValue(0);

  connectSignals();
}

// IProjectSaveView interface implementations
//==============================================================================

std::vector<API::IProjectSerialisable *> ProjectSaveView::getWindows()
{
  return m_serialisableWindows;
}

std::vector<std::string> ProjectSaveView::getCheckedWorkspaceNames()
{
  return getItemsWithCheckState(Qt::CheckState::Checked);
}

std::vector<std::string> ProjectSaveView::getUncheckedWorkspaceNames()
{
  return getItemsWithCheckState(Qt::CheckState::Unchecked);
}

QString ProjectSaveView::getProjectPath()
{
  return m_ui.projectPath->text();
}

void ProjectSaveView::setProjectPath(const QString &path)
{
 m_ui.projectPath->setText(path);
}

void ProjectSaveView::updateWorkspacesList(const std::vector<WorkspaceInfo> &workspaces)
{
  m_ui.workspaceList->clear();
  for (auto info : workspaces) {
    addWorkspaceItem(info);
  }

  resizeWidgetColumns(m_ui.workspaceList);
}

void ProjectSaveView::updateIncludedWindowsList(const std::vector<WindowInfo> &windows)
{
  m_ui.includedWindows->clear();
  for (auto info : windows) {
    addWindowItem(m_ui.includedWindows, info);
  }

  resizeWidgetColumns(m_ui.includedWindows);
}

void ProjectSaveView::updateExcludedWindowsList(const std::vector<WindowInfo> &windows)
{
  m_ui.excludedWindows->clear();
  for (auto info: windows) {
    addWindowItem(m_ui.excludedWindows, info);
  }

  resizeWidgetColumns(m_ui.excludedWindows);
}

void ProjectSaveView::removeFromIncludedWindowsList(const std::vector<std::string> &windows)
{
  for (auto name : windows) {
    removeItem(m_ui.includedWindows, name);
  }
}

void ProjectSaveView::removeFromExcludedWindowsList(const std::vector<std::string> &windows)
{
  for (auto name : windows) {
    removeItem(m_ui.excludedWindows, name);
  }
}

// Private slots
//==============================================================================

void ProjectSaveView::workspaceItemChanged(QTreeWidgetItem *item, int column)
{
  if(item->checkState(column) == Qt::CheckState::Checked) {
    m_presenter->notify(ProjectSavePresenter::Notification::CheckWorkspace);
  } else if (item->checkState(column) == Qt::CheckState::Unchecked) {
    m_presenter->notify(ProjectSavePresenter::Notification::UncheckWorkspace);
  }
}

std::vector<std::string> ProjectSaveView::getItemsWithCheckState(const Qt::CheckState state) const
{
  std::vector<std::string> names;
  for( int i = 0; i < m_ui.workspaceList->topLevelItemCount(); ++i )
  {
     auto item = m_ui.workspaceList->topLevelItem( i );
     if(item->checkState(0) == state) {
       auto name = item->text(0).toStdString();
       names.push_back(name);
     }
  }
  return names;
}

void ProjectSaveView::save(bool checked)
{
  UNUSED_ARG(checked);

  if(m_ui.projectPath->text().isEmpty()) {
    QMessageBox::warning(this, "Project Save",
                         "Please choose a valid file path", QMessageBox::Ok);
    return;
  }

  m_presenter->notify(ProjectSavePresenter::Notification::PrepareProjectFolder);
  auto wsNames = getCheckedWorkspaceNames();
  auto windowNames = getIncludedWindowNames();
  auto filePath = m_ui.projectPath->text();
  auto compress = filePath.endsWith(".gz");

  m_serialiser.save(filePath, wsNames, windowNames, compress);
  emit projectSaved();

  close();
}

void ProjectSaveView::findFilePath()
{
  QString fileName = "";
  QString filter = "MantidPlot project (*.mantid);;";
  filter += "Compressed MantidPlot project (*.mantid.gz)";

  QString selectedFilter;
  fileName = MantidQt::API::FileDialogHandler::getSaveFileName(
        this, tr("Save Project As"), "", filter, &selectedFilter);

  m_ui.projectPath->setText(fileName);
}

// Private helper methods
//==============================================================================

std::vector<std::string> ProjectSaveView::getIncludedWindowNames() const
{
  std::vector<std::string> names;
  for( int i = 0; i < m_ui.includedWindows->topLevelItemCount(); ++i )
  {
     auto item = m_ui.includedWindows->topLevelItem(i);
     auto name = item->text(0).toStdString();
     names.push_back(name);
  }
  return names;
}

void ProjectSaveView::removeItem(QTreeWidget *widget, const std::string &name)
{
  auto qname = QString::fromStdString(name);
  auto items = widget->findItems(qname, Qt::MatchContains );
  for (auto item : items) {
    delete item;
  }
}

void ProjectSaveView::addWindowItem(QTreeWidget *widget, const WindowInfo &info)
{
  QStringList lst;
  WindowIcons icons;
  lst << QString::fromStdString(info.name);
  lst << QString::fromStdString(info.type);

  auto item = new QTreeWidgetItem(lst);
  if(!info.icon_id.empty())
    item->setIcon(0, icons.getIcon(info.type));
  widget->addTopLevelItem(item);
}

void ProjectSaveView::addWorkspaceItem(const WorkspaceInfo &info)
{
  QStringList lst;
  lst << QString::fromStdString(info.name);
  lst << QString::fromStdString(info.type);
  lst << QString::fromStdString(info.size);
  lst << QString::number(info.numWindows);

  auto item = new QTreeWidgetItem(lst);
  if(!info.icon_id.empty())
    item->setIcon(0, getQPixmap(info.icon_id));
  item->setCheckState(0, Qt::CheckState::Checked);
  m_ui.workspaceList->addTopLevelItem(item);
}

bool ProjectSaveView::checkIfNewProject(const QString &projectName) const
{
   return (projectName == "untitled" ||
      projectName.endsWith(".opj", Qt::CaseInsensitive) ||
      projectName.endsWith(".ogm", Qt::CaseInsensitive) ||
      projectName.endsWith(".ogw", Qt::CaseInsensitive) ||
           projectName.endsWith(".ogg", Qt::CaseInsensitive));
}

void ProjectSaveView::resizeWidgetColumns(QTreeWidget *widget)
{
  for (int i = 0; i < widget->topLevelItemCount(); ++i)
    widget->resizeColumnToContents(i);
}

void ProjectSaveView::connectSignals()
{
  //Connect signal to listen for when workspaces are changed (checked/unchecked)
  connect(m_ui.workspaceList,
          SIGNAL(itemChanged(QTreeWidgetItem *, int)), this,
          SLOT(workspaceItemChanged(QTreeWidgetItem*, int)));

  connect(m_ui.btnBrowseFilePath, SIGNAL(clicked(bool)), this, SLOT(findFilePath()));
  connect(m_ui.btnSave, SIGNAL(clicked(bool)), this, SLOT(save(bool)));
  connect(m_ui.btnCancel, SIGNAL(clicked(bool)), this, SLOT(close()));

  connect(&m_serialiser, SIGNAL(setProgressBarRange(int, int)),
          m_ui.saveProgressBar, SLOT(setRange(int, int)));
  connect(&m_serialiser, SIGNAL(setProgressBarValue(int)),
          m_ui.saveProgressBar, SLOT(setValue(int)));
}


}
}
