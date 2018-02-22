#include "EnggDiffMultiRunFittingWidgetPresenter.h"

#include "MantidQtWidgets/LegacyQwt/QwtHelper.h"

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffMultiRunFittingWidgetPresenter::EnggDiffMultiRunFittingWidgetPresenter(
    std::unique_ptr<IEnggDiffMultiRunFittingWidgetModel> model,
    std::unique_ptr<IEnggDiffMultiRunFittingWidgetView> view)
    : m_model(std::move(model)), m_view(std::move(view)) {}

void EnggDiffMultiRunFittingWidgetPresenter::addFittedPeaks(
    const RunLabel &runLabel, const Mantid::API::MatrixWorkspace_sptr ws) {
  m_model->addFittedPeaks(runLabel, ws);
}
void EnggDiffMultiRunFittingWidgetPresenter::addFocusedRun(
    const RunLabel &runLabel, const Mantid::API::MatrixWorkspace_sptr ws) {
  m_model->addFocusedRun(runLabel, ws);
  m_view->updateRunList(m_model->getAllWorkspaceLabels());
}

void EnggDiffMultiRunFittingWidgetPresenter::displayFitResults(
    const RunLabel &runLabel) {
  const auto fittedPeaks = m_model->getFittedPeaks(runLabel);
  if (!fittedPeaks) {
    m_view->reportPlotInvalidFittedPeaks(runLabel);
  } else {
    const auto plottablePeaks = API::QwtHelper::curveDataFromWs(*fittedPeaks);
    m_view->plotFittedPeaks(plottablePeaks);
  }
}

boost::optional<Mantid::API::MatrixWorkspace_sptr>
EnggDiffMultiRunFittingWidgetPresenter::getFittedPeaks(
    const RunLabel &runLabel) const {
  return m_model->getFittedPeaks(runLabel);
}

boost::optional<Mantid::API::MatrixWorkspace_sptr>
EnggDiffMultiRunFittingWidgetPresenter::getFocusedRun(
    const RunLabel &runLabel) const {
  return m_model->getFocusedRun(runLabel);
}

void EnggDiffMultiRunFittingWidgetPresenter::notify(
    IEnggDiffMultiRunFittingWidgetPresenter::Notification notif) {
  switch (notif) {
  case Notification::SelectRun:
    processSelectRun();
    break;
  }
}

void EnggDiffMultiRunFittingWidgetPresenter::processSelectRun() {
  const auto selectedRunLabel = m_view->getSelectedRunLabel();
  updatePlot(selectedRunLabel);
}

void EnggDiffMultiRunFittingWidgetPresenter::updatePlot(
    const RunLabel &runLabel) {
  const auto focusedRun = m_model->getFocusedRun(runLabel);

  if (!focusedRun) {
    m_view->reportPlotInvalidFocusedRun(runLabel);
  } else {
    const auto plottableCurve = API::QwtHelper::curveDataFromWs(*focusedRun);

    m_view->resetCanvas();
    m_view->plotFocusedRun(plottableCurve);

    if (m_model->hasFittedPeaksForRun(runLabel) &&
        m_view->showFitResultsSelected()) {
      displayFitResults(runLabel);
    }
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
