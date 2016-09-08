#include "MantidQtMantidWidgets/WorkspacePresenter/WorkspaceDockView.h"
#include "MantidQtMantidWidgets/WorkspacePresenter/ADSAdapter.h"
#include "MantidQtMantidWidgets/WorkspacePresenter/WorkspacePresenter.h"
#include <MantidKernel/make_unique.h>

using namespace Mantid::Kernel;

namespace MantidQt {
namespace MantidWidgets {
WorkspaceDockView::WorkspaceDockView() {}

void WorkspaceDockView::init() {
  presenter = boost::make_shared<WorkspacePresenter>(shared_from_this());
  presenter->init();
}

WorkspacePresenter_wptr WorkspaceDockView::getPresenterWeakPtr() {
  return presenter;
}

WorkspacePresenter_sptr WorkspaceDockView::getPresenterSharedPtr() {
  return presenter;
}

void WorkspaceDockView::showLoadDialog() {}

void WorkspaceDockView::updateTree(
    const std::map<std::string, Mantid::API::Workspace_sptr> &items) {}

} // namespace MantidQt
} // namespace MantidWidgets