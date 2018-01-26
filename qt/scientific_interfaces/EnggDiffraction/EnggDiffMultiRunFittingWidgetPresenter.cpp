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

  case IEnggDiffMultiRunFittingWidgetPresenter::AddFittedPeaks:
    processAddFittedPeaks();
    break;

  case IEnggDiffMultiRunFittingWidgetPresenter::AddFocusedRun:
    processAddFocusedRun();
    break;

  case IEnggDiffMultiRunFittingWidgetPresenter::GetFittedPeaks:
    processGetFittedPeaks();
    break;

  case IEnggDiffMultiRunFittingWidgetPresenter::ShutDown:
    processShutDown();
    break;

  case IEnggDiffMultiRunFittingWidgetPresenter::Start:
    processStart();
    break;
  }
}

void EnggDiffMultiRunFittingWidgetPresenter::processAddFittedPeaks() {
  const auto ws = m_view->getFittedPeaksWorkspaceToAdd();
  const auto bankID = m_view->getFittedPeaksBankIDToAdd();
  const auto runNumber = m_view->getFittedPeaksRunNumberToAdd();
  m_model->addFittedPeaks(runNumber, bankID, ws);
}

void EnggDiffMultiRunFittingWidgetPresenter::processAddFocusedRun() {
  const auto ws = m_view->getFocusedWorkspaceToAdd();
  const auto bankID = m_view->getFocusedRunBankIDToAdd();
  const auto runNumber = m_view->getFocusedRunNumberToAdd();
  m_model->addFocusedRun(runNumber, bankID, ws);
}

void EnggDiffMultiRunFittingWidgetPresenter::processGetFittedPeaks() {
  const auto bankID = m_view->getFittedPeaksBankIDToReturn();
  const auto runNumber = m_view->getFittedPeaksRunNumberToReturn();
  const auto ws = m_model->getFittedPeaks(runNumber, bankID);

  if (!ws) {
    m_view->userError(
        "Invalid fitted peaks run identifier",
        "Could not find a fitted peaks workspace with run number " +
            std::to_string(runNumber) + " and bank ID " +
            std::to_string(bankID) +
            ". Please contact the development team with this message");
    return;
  }
  m_view->setFittedPeaksWorkspaceToReturn(*ws);
}

void EnggDiffMultiRunFittingWidgetPresenter::processShutDown() {
  m_viewHasClosed = true;
}

void EnggDiffMultiRunFittingWidgetPresenter::processStart() {}

} // namespace CustomInterfaces
} // namespace MantidQt
