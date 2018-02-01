#include "EnggDiffMultiRunFittingWidgetPresenter.h"

#include "MantidQtWidgets/LegacyQwt/QwtHelper.h"

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffMultiRunFittingWidgetPresenter::EnggDiffMultiRunFittingWidgetPresenter(
    std::unique_ptr<IEnggDiffMultiRunFittingWidgetModel> model,
    std::unique_ptr<IEnggDiffMultiRunFittingWidgetView> view)
    : m_model(std::move(model)), m_view(std::move(view)) {}

void EnggDiffMultiRunFittingWidgetPresenter::addFittedPeaks(
    const int runNumber, const size_t bank,
    const Mantid::API::MatrixWorkspace_sptr ws) {
  m_model->addFittedPeaks(runNumber, bank, ws);
}
void EnggDiffMultiRunFittingWidgetPresenter::addFocusedRun(
    const int runNumber, const size_t bank,
    const Mantid::API::MatrixWorkspace_sptr ws) {
  m_model->addFocusedRun(runNumber, bank, ws);
  m_view->updateRunList(m_model->getAllWorkspaceLabels());
}

void EnggDiffMultiRunFittingWidgetPresenter::displayFitResults(
    const int runNumber, const size_t bank) {
  const auto fittedPeaks = m_model->getFittedPeaks(runNumber, bank);
  if (!fittedPeaks) {
    m_view->userError("Invalid fitted peaks run identifer",
                      "Unexpectedly tried to plot fit results for invalid "
                      "run, run number = " +
                          std::to_string(runNumber) + ", bank ID = " +
                          std::to_string(bank) +
                          ". Please contact the development team");
    return;
  }
  const auto plottablePeaks = API::QwtHelper::curveDataFromWs(*fittedPeaks);
  m_view->plotFittedPeaks(plottablePeaks);
}

boost::optional<Mantid::API::MatrixWorkspace_sptr>
EnggDiffMultiRunFittingWidgetPresenter::getFittedPeaks(
    const int runNumber, const size_t bank) const {
  return m_model->getFittedPeaks(runNumber, bank);
}

boost::optional<Mantid::API::MatrixWorkspace_sptr>
EnggDiffMultiRunFittingWidgetPresenter::getFocusedRun(const int runNumber,
                                                      const size_t bank) const {
  return m_model->getFocusedRun(runNumber, bank);
}

void EnggDiffMultiRunFittingWidgetPresenter::notify(
    IEnggDiffMultiRunFittingWidgetPresenter::Notification notif) {
  switch (notif) {
  case IEnggDiffMultiRunFittingWidgetPresenter::SelectRun:
    processSelectRun();
    break;
  }
}

void EnggDiffMultiRunFittingWidgetPresenter::processSelectRun() {
  const auto selectedRunLabel = m_view->getSelectedRunLabel();
  const auto runNumber = selectedRunLabel.first;
  const auto bank = selectedRunLabel.second;
  updatePlot(runNumber, bank);
}

void EnggDiffMultiRunFittingWidgetPresenter::updatePlot(const int runNumber,
                                                        const size_t bank) {
  const auto focusedRun = m_model->getFocusedRun(runNumber, bank);

  if (!focusedRun) {
    m_view->userError(
        "Invalid focused run identifier",
        "Tried to access invalid run, run number " + std::to_string(runNumber) +
            " and bank ID " + std::to_string(bank) +
            ". Please contact the development team with this message");
    return;
  }

  const auto plottableCurve = API::QwtHelper::curveDataFromWs(*focusedRun);

  m_view->resetCanvas();
  m_view->plotFocusedRun(plottableCurve);

  if (m_model->hasFittedPeaksForRun(runNumber, bank) &&
      m_view->showFitResultsSelected()) {
    displayFitResults(runNumber, bank);
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
