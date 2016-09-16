#include "MantidQtMantidWidgets/WorkspacePresenter/WorkspacePresenter.h"
#include "MantidKernel/make_unique.h"
#include "MantidQtMantidWidgets/WorkspacePresenter/ADSAdapter.h"
#include "MantidQtMantidWidgets/WorkspacePresenter/IWorkspaceDockView.h"

using namespace Mantid;

namespace MantidQt {
namespace MantidWidgets {

WorkspacePresenter::WorkspacePresenter(DockView_wptr view)
    : m_view(view), m_adapter(Kernel::make_unique<ADSAdapter>()) {}

WorkspacePresenter::~WorkspacePresenter() {}

void WorkspacePresenter::init() {
  m_adapter->registerPresenter(std::move(m_view.lock()->getPresenterWeakPtr()));
}

void WorkspacePresenter::notifyFromWorkspaceProvider(
    WorkspaceProviderNotifiable::Flag flag) {
  switch (flag) {
  case WorkspaceProviderNotifiable::Flag::WorkspaceLoaded:
    workspaceLoaded();
    break;
  case WorkspaceProviderNotifiable::Flag::WorkspaceRenamed:
    workspaceRenamed();
    break;
  case WorkspaceProviderNotifiable::Flag::WorkspacesGrouped:
    workspacesGrouped();
    break;
  case WorkspaceProviderNotifiable::Flag::WorkspacesUngrouped:
    workspacesUngrouped();
    break;
  case WorkspaceProviderNotifiable::Flag::WorkspaceGroupUpdated:
    workspaceGroupUpdated();
    break;
  case WorkspaceProviderNotifiable::Flag::WorkspaceDeleted:
    workspacesDeleted();
    break;
  }
}

void WorkspacePresenter::notifyFromView(ViewNotifiable::Flag flag) {
  switch (flag) {
  case ViewNotifiable::Flag::LoadWorkspace:
    loadWorkspace();
    break;
  case ViewNotifiable::Flag::LoadLiveDataWorkspace:
    loadLiveData();
    break;
  case ViewNotifiable::Flag::RenameWorkspace:
    renameWorkspace();
    break;
  case ViewNotifiable::Flag::GroupWorkspaces:
    groupWorkspaces();
    break;
  case ViewNotifiable::Flag::UngroupWorkspaces:
    ungroupWorkspaces();
    break;
  case ViewNotifiable::Flag::SortWorkspaces:
    sortWorkspaces();
    break;
  case ViewNotifiable::Flag::DeleteWorkspaces:
    deleteWorkspaces();
    break;
  case ViewNotifiable::Flag::SaveSingleWorkspace:
    saveSingleWorkspace();
    break;
  case ViewNotifiable::Flag::SaveWorkspaceCollection:
    saveWorkspaceCollection();
    break;
  case ViewNotifiable::Flag::FilterWorkspaces:
    filterWorkspaces();
    break;
  }
}

void WorkspacePresenter::loadWorkspace() {
  auto view = lockView();
  view->showLoadDialog();
}

void WorkspacePresenter::loadLiveData() {
  auto view = lockView();
  view->showLiveDataDialog();
}

void WorkspacePresenter::renameWorkspace() {
  auto view = lockView();
  view->showRenameDialog(view->getSelectedWorkspaceNames());
}

void WorkspacePresenter::groupWorkspaces() {
  auto view = lockView();
  view->groupWorkspaces(view->getSelectedWorkspaceNames());
}

void WorkspacePresenter::ungroupWorkspaces() {
  auto view = lockView();
  view->ungroupWorkspaces(view->getSelectedWorkspaceNames());
}

void WorkspacePresenter::sortWorkspaces() {
  auto view = lockView();

  view->sortWorkspaces(view->getSortCriteria(), view->getSortDirection());
}

void WorkspacePresenter::deleteWorkspaces() {
  auto view = lockView();

  if (view->deleteConfirmation())
    view->deleteWorkspaces(view->getSelectedWorkspaceNames());
}

void WorkspacePresenter::saveSingleWorkspace() {
  auto view = lockView();
  auto selected = view->getSelectedWorkspaceNames();
  view->saveWorkspace(selected[0], view->getSaveFileType());
}

void WorkspacePresenter::saveWorkspaceCollection() {
  auto view = lockView();
  view->saveWorkspaces(view->getSelectedWorkspaceNames());
}

void WorkspacePresenter::filterWorkspaces() {
  auto view = lockView();
  view->filterWorkspaces(view->getFilterText());
}

void WorkspacePresenter::workspaceLoaded() { updateView(); }

void WorkspacePresenter::workspaceRenamed() { updateView(); }

void WorkspacePresenter::workspacesGrouped() { updateView(); }
void WorkspacePresenter::workspacesUngrouped() { updateView(); }
void WorkspacePresenter::workspaceGroupUpdated() { updateView(); }

void WorkspacePresenter::workspacesDeleted() { updateView(); }

DockView_sptr WorkspacePresenter::lockView() {
  auto view_sptr = m_view.lock();

  if (view_sptr == nullptr)
    throw std::runtime_error("Could not obtain pointer to DockView.");

  return std::move(view_sptr);
}

void WorkspacePresenter::updateView() {
  auto view = lockView();
  view->updateTree(m_adapter->topLevelItems());
}

} // namespace MantidQt
} // namespace MantidWidgets