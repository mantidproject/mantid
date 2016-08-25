#include "MantidQtMantidWidgets/WorkspacePresenter/WorkspacePresenter.h"
#include "MantidQtMantidWidgets/WorkspacePresenter/IWorkspaceDockView.h"
#include "MantidQtMantidWidgets/WorkspacePresenter/WorkspaceProvider.h"

namespace MantidQt {
namespace MantidWidgets {

WorkspacePresenter::WorkspacePresenter(DockView_wptr view,
                                       ADSAdapter_uptr adapter)
    : m_view(view), m_adapter(std::move(adapter)) {
  m_adapter->registerPresenter(m_view.lock()->getPresenterWeakPtr());
}

void WorkspacePresenter::notifyFromWorkspaceProvider(
    WorkspaceProviderNotifiable::Flag flag) {
  switch (flag) {
  case WorkspaceProviderNotifiable::Flag::WorkspaceLoaded:
    break;
  case WorkspaceProviderNotifiable::Flag::WorkspaceRenamed:
    break;
  case WorkspaceProviderNotifiable::Flag::WorkspacesSaved:
    break;
  case WorkspaceProviderNotifiable::Flag::WorkspacesGrouped:
    break;
  case WorkspaceProviderNotifiable::Flag::WorkspacesSorted:
    break;
  case WorkspaceProviderNotifiable::Flag::WorkspaceDeleted:
    break;
  }
}

void WorkspacePresenter::notifyFromView(ViewNotifiable::Flag flag) {
  switch (flag) {
  case ViewNotifiable::Flag::LoadWorkspace:
    break;
  case ViewNotifiable::Flag::RenameWorkspace:
    break;
  case ViewNotifiable::Flag::GroupWorkspaces:
    break;
  case ViewNotifiable::Flag::SaveWorkspaces:
    break;
  case ViewNotifiable::Flag::SortWorkspaces:
    break;
  case ViewNotifiable::Flag::DeleteWorkspaces:
    break;
  }
}

void WorkspacePresenter::loadWorkspace() {}
void WorkspacePresenter::saveWorkspace() {}
void WorkspacePresenter::renameWorkspace() {}
void WorkspacePresenter::groupWorkspaces() {}
void WorkspacePresenter::sortWorkspaces() {}
void WorkspacePresenter::deleteWorkspaces() {}

void WorkspacePresenter::workspaceLoaded() {}
void WorkspacePresenter::workspaceSaved() {}
void WorkspacePresenter::workspaceRenamed() {}
void WorkspacePresenter::workspacesGrouped() {}
void WorkspacePresenter::workspacesSorted() {}
void WorkspacePresenter::workspacesDeleted() {}

} // namespace MantidQt
} // namespace MantidWidgets