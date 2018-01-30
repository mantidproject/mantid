#include "EnggDiffMultiRunFittingWidgetModel.h"

namespace MantidQt {
namespace CustomInterfaces {

void EnggDiffMultiRunFittingWidgetModel::addFittedPeaks(
    const int runNumber, const size_t bank,
    const Mantid::API::MatrixWorkspace_sptr ws) {
  m_fittedPeaksMap.add(runNumber, bank, ws);
}

void EnggDiffMultiRunFittingWidgetModel::addFocusedRun(
    const int runNumber, const size_t bank,
    const Mantid::API::MatrixWorkspace_sptr ws) {
  m_focusedRunMap.add(runNumber, bank, ws);
}

std::vector<std::pair<int, size_t>>
EnggDiffMultiRunFittingWidgetModel::getAllWorkspaceLabels() const {
  return m_focusedRunMap.getRunNumbersAndBankIDs();
}

boost::optional<Mantid::API::MatrixWorkspace_sptr>
EnggDiffMultiRunFittingWidgetModel::getFittedPeaks(const int runNumber,
                                                   const size_t bank) const {
  if (m_fittedPeaksMap.contains(runNumber, bank)) {
    return m_fittedPeaksMap.get(runNumber, bank);
  }
  return boost::none;
}

boost::optional<Mantid::API::MatrixWorkspace_sptr>
EnggDiffMultiRunFittingWidgetModel::getFocusedRun(const int runNumber,
                                                  const size_t bank) const {
  if (m_focusedRunMap.contains(runNumber, bank)) {
    return m_focusedRunMap.get(runNumber, bank);
  }
  return boost::none;
}

bool EnggDiffMultiRunFittingWidgetModel::hasFittedPeaksForRun(
    const int runNumber, const size_t bank) const {
  return m_fittedPeaksMap.contains(runNumber, bank);
}

} // namespace MantidQt
} // namespace CustomInterfaces
