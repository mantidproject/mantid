#include "EnggDiffGSASFittingPresenter.h"
#include "MantidQtWidgets/LegacyQwt/QwtHelper.h"

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffGSASFittingPresenter::EnggDiffGSASFittingPresenter(
    std::unique_ptr<IEnggDiffGSASFittingModel> model,
    std::unique_ptr<IEnggDiffGSASFittingView> view)
    : m_model(std::move(model)), m_view(std::move(view)),
      m_viewHasClosed(false) {}

EnggDiffGSASFittingPresenter::~EnggDiffGSASFittingPresenter() {}

void EnggDiffGSASFittingPresenter::notify(
    IEnggDiffGSASFittingPresenter::Notification notif) {

  if (m_viewHasClosed) {
    return;
  }

  switch (notif) {

  case IEnggDiffGSASFittingPresenter::LoadRun:
    processLoadRun();
    break;

  case IEnggDiffGSASFittingPresenter::SelectRun:
    processSelectRun();
    break;

  case IEnggDiffGSASFittingPresenter::Start:
    processStart();
    break;

  case IEnggDiffGSASFittingPresenter::ShutDown:
    processShutDown();
    break;
  }
}

void EnggDiffGSASFittingPresenter::processLoadRun() {
  const auto focusedFileName = m_view->getFocusedFileName();
  const auto loadSuccess = m_model->loadFocusedRun(focusedFileName);

  if (loadSuccess.empty()) {
    const auto runLabels = m_model->getRunLabels();
    m_view->updateRunList(runLabels);
  } else {
    m_view->userWarning(loadSuccess);
  }
}

void EnggDiffGSASFittingPresenter::processSelectRun() {
  const auto runLabel = m_view->getSelectedRunLabel();
  const auto runNumber = runLabel.first;
  const auto bank = runLabel.second;

  Mantid::API::MatrixWorkspace_sptr focusedWorkspace;
  try {
    focusedWorkspace = m_model->getFocusedWorkspace(runNumber, bank);
  } catch (const std::runtime_error e) {
    m_view->userWarning("Tried to access invalid run, runNumber " +
                        std::to_string(runNumber) + " and bank ID " +
                        std::to_string(bank));
    return;
  }

  const auto plottableCurve = API::QwtHelper::curveDataFromWs(focusedWorkspace);

  m_view->resetCanvas();
  m_view->plotCurve(plottableCurve);

  if (m_model->hasFittedPeaksForRun(runNumber, bank)) {
    const auto fittedPeaks = m_model->getFittedPeaks(runNumber, bank);
    const auto plottablePeaks = API::QwtHelper::curveDataFromWs(fittedPeaks);
    m_view->plotCurve(plottablePeaks);
  }
}

void EnggDiffGSASFittingPresenter::processStart() {}

void EnggDiffGSASFittingPresenter::processShutDown() { m_viewHasClosed = true; }

} // MantidQt
} // CustomInterfaces
