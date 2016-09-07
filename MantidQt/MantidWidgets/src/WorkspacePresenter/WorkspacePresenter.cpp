#include "MantidQtMantidWidgets/WorkspacePresenter/WorkspacePresenter.h"
#include "MantidQtMantidWidgets/WorkspacePresenter/IWorkspaceDockView.h"
#include "MantidQtMantidWidgets/WorkspacePresenter/ADSAdapter.h"
#include "MantidKernel/make_unique.h"

using namespace Mantid;

namespace MantidQt {
namespace MantidWidgets {

WorkspacePresenter::WorkspacePresenter(DockView_wptr view)
    : m_view(view), m_adapter(Kernel::make_unique<ADSAdapter>()) {
  m_adapter->registerPresenter(m_view.lock()->getPresenterWeakPtr());
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
  case WorkspaceProviderNotifiable::Flag::WorkspacesSaved:
    workspaceSaved();
    break;
  case WorkspaceProviderNotifiable::Flag::WorkspacesGrouped:
    workspacesGrouped();
    break;
  case WorkspaceProviderNotifiable::Flag::WorkspacesSorted:
    workspacesSorted();
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
  case ViewNotifiable::Flag::RenameWorkspace:
    renameWorkspace();
    break;
  case ViewNotifiable::Flag::GroupWorkspaces:
    groupWorkspaces();
    break;
  case ViewNotifiable::Flag::SaveWorkspaces:
    saveWorkspace();
    break;
  case ViewNotifiable::Flag::SortWorkspaces:
    sortWorkspaces();
    break;
  case ViewNotifiable::Flag::DeleteWorkspaces:
    deleteWorkspaces();
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