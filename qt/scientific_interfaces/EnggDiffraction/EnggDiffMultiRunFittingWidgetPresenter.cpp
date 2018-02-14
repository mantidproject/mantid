#include "EnggDiffMultiRunFittingWidgetPresenter.h"

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

} // namespace CustomInterfaces
} // namespace MantidQt
