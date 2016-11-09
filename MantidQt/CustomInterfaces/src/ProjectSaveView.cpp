#include "MantidQtAPI/IProjectSerialisable.h"
#include "MantidQtCustomInterfaces/ProjectSavePresenter.h"
#include "MantidQtCustomInterfaces/ProjectSaveView.h"

using namespace MantidQt::API;

namespace MantidQt {
namespace CustomInterfaces {

ProjectSaveView::ProjectSaveView(const std::vector<IProjectSerialisable*> &windows, QWidget *parent)
  : QDialog(parent), m_serialisableWindows(windows.cbegin(), windows.cend()) {
  m_presenter.reset(new ProjectSavePresenter(this));
  m_ui.setupUi(this);

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
  for (auto name : workspaces) {
    QStringList lst(QString::fromStdString(name));
    QTreeWidgetItem *item = new QTreeWidgetItem(lst);
    m_ui.workspaceList->addTopLevelItem(item);
  }
}

void ProjectSaveView::updateIncludedWindowsList(const std::vector<std::string> &windows)
{

}

void ProjectSaveView::updateExcludedWindowsList(const std::vector<std::string> &windows)
{

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

}
}
