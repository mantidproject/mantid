#include "MantidQtAPI/IProjectSerialisable.h"
#include "MantidQtMantidWidgets/ProjectSavePresenter.h"
#include "MantidQtMantidWidgets/ProjectSaveView.h"

using namespace MantidQt::API;

namespace MantidQt {
namespace MantidWidgets {

ProjectSaveView::ProjectSaveView(const std::vector<IProjectSerialisable*> &windows, QWidget *parent)
  : QDialog(parent), m_serialisableWindows(windows.cbegin(), windows.cend()) {
  m_ui.setupUi(this);
  m_presenter.reset(new ProjectSavePresenter(this));

  //Connect signal to listen for when workspaces are changed (checked/unchecked)
  connect(m_ui.workspaceList,
          SIGNAL(itemChanged(QTreeWidgetItem *, int)), this,
          SLOT(workspaceItemChanged(QTreeWidgetItem*, int)));
}

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

void ProjectSaveView::updateWorkspacesList(const std::vector<std::string> &workspaces)
{
  m_ui.workspaceList->clear();
  for (auto name : workspaces) {
    addWorkspaceItem(name);
  }
}

void ProjectSaveView::updateIncludedWindowsList(const std::vector<std::string> &windows)
{
  m_ui.includedWindows->clear();
  for (auto name : windows) {
    addWindowItem(m_ui.includedWindows, name);
  }
}

void ProjectSaveView::updateExcludedWindowsList(const std::vector<std::string> &windows)
{
  m_ui.excludedWindows->clear();
  for (auto name : windows) {
    addWindowItem(m_ui.excludedWindows, name);
  }
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

void ProjectSaveView::removeItem(QTreeWidget *widget, const std::string &name)
{
  auto qname = QString::fromStdString(name);
  auto items = widget->findItems(qname, Qt::MatchContains );
  for (auto item : items) {
    delete item;
  }
}

void ProjectSaveView::addWindowItem(QTreeWidget *widget, const std::string &name)
{
  auto it = new QTreeWidgetItem(widget);
  it->setText(0, QString::fromStdString(name));
  widget->addTopLevelItem(it);
}

void ProjectSaveView::addWorkspaceItem(const std::string &name)
{
  QStringList lst(QString::fromStdString(name));
  lst.append("0");
  QTreeWidgetItem *item = new QTreeWidgetItem(lst);
  item->setCheckState(0, Qt::CheckState::Checked);
  m_ui.workspaceList->addTopLevelItem(item);
}

}
}
