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
  m_focusedWorkspaceMap.add(runNumber, bank, ws);
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
  if (m_focusedWorkspaceMap.contains(runNumber, bank)) {
    return m_focusedWorkspaceMap.get(runNumber, bank);
  }
  return boost::none;
}

} // namespace MantidQt
} // namespace CustomInterfaces
