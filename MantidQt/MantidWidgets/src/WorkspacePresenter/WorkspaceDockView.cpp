#include "MantidQtMantidWidgets/WorkspacePresenter/WorkspaceDockView.h"
#include "MantidQtMantidWidgets/WorkspacePresenter/ADSAdapter.h"
#include "MantidQtMantidWidgets/WorkspacePresenter/WorkspacePresenter.h"
#include <MantidKernel/make_unique.h>

using namespace Mantid::Kernel;

namespace MantidQt {
namespace MantidWidgets {
WorkspaceDockView::WorkspaceDockView() {
  presenter = boost::make_shared<WorkspacePresenter>(
      shared_from_this(), Mantid::Kernel::make_unique<ADSAdapter>());
}

WorkspacePresenter_wptr WorkspaceDockView::getPresenterWeakPtr() {
  return presenter;
}
} // namespace MantidQt
} // namespace MantidWidgets