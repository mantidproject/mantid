#include "EnggDiffMultiRunFittingWidgetPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffMultiRunFittingWidgetPresenter::EnggDiffMultiRunFittingWidgetPresenter(
    std::unique_ptr<IEnggDiffMultiRunFittingWidgetModel> model,
    IEnggDiffMultiRunFittingWidgetView *view)
    : m_model(std::move(model)), m_view(view), m_viewHasClosed(false) {}

EnggDiffMultiRunFittingWidgetPresenter::
    ~EnggDiffMultiRunFittingWidgetPresenter() {}

void EnggDiffMultiRunFittingWidgetPresenter::notify(
    IEnggDiffMultiRunFittingWidgetPresenter::Notification notif) {
  if (m_viewHasClosed) {
    return;
  }
  switch (notif) {

  case IEnggDiffMultiRunFittingWidgetPresenter::AddFocusedRun:
    processAddFocusedRun();
    break;

  case IEnggDiffMultiRunFittingWidgetPresenter::ShutDown:
    processShutDown();
    break;

  case IEnggDiffMultiRunFittingWidgetPresenter::Start:
    processStart();
    break;
  }
}

void EnggDiffMultiRunFittingWidgetPresenter::processAddFocusedRun() {
  const auto ws = m_view->getFocusedWorkspaceToAdd();
  const auto bankID = m_view->getFocusedRunBankIDToAdd();
  const auto runNumber = m_view->getFocusedRunNumberToAdd();
  m_model->addFocusedRun(runNumber, bankID, ws);
}

void EnggDiffMultiRunFittingWidgetPresenter::processShutDown() {
  m_viewHasClosed = true;
}

void EnggDiffMultiRunFittingWidgetPresenter::processStart() {}

} // namespace CustomInterfaces
} // namespace MantidQt
