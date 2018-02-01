#include "EnggDiffMultiRunFittingWidgetPresenter.h"

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

} // namespace CustomInterfaces
} // namespace MantidQt
