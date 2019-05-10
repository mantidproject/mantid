// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "EnggDiffMultiRunFittingWidgetModel.h"

namespace MantidQt {
namespace CustomInterfaces {

void EnggDiffMultiRunFittingWidgetModel::addFittedPeaks(
    const RunLabel &runLabel, const Mantid::API::MatrixWorkspace_sptr ws) {
  m_fittedPeaksMap.add(runLabel, ws);
}

void EnggDiffMultiRunFittingWidgetModel::addFocusedRun(
    const RunLabel &runLabel, const Mantid::API::MatrixWorkspace_sptr ws) {
  m_focusedRunMap.add(runLabel, ws);
}

std::vector<RunLabel>
EnggDiffMultiRunFittingWidgetModel::getAllWorkspaceLabels() const {
  return m_focusedRunMap.getRunLabels();
}

boost::optional<Mantid::API::MatrixWorkspace_sptr>
EnggDiffMultiRunFittingWidgetModel::getFittedPeaks(
    const RunLabel &runLabel) const {
  if (m_fittedPeaksMap.contains(runLabel)) {
    return m_fittedPeaksMap.get(runLabel);
  }
  return boost::none;
}

boost::optional<Mantid::API::MatrixWorkspace_sptr>
EnggDiffMultiRunFittingWidgetModel::getFocusedRun(
    const RunLabel &runLabel) const {
  if (m_focusedRunMap.contains(runLabel)) {
    return m_focusedRunMap.get(runLabel);
  }
  return boost::none;
}

bool EnggDiffMultiRunFittingWidgetModel::hasFittedPeaksForRun(
    const RunLabel &runLabel) const {
  return m_fittedPeaksMap.contains(runLabel);
}

void EnggDiffMultiRunFittingWidgetModel::removeRun(const RunLabel &runLabel) {
  if (!m_focusedRunMap.contains(runLabel)) {
    throw std::runtime_error("Tried to remove non-existent run (run number " +
                             runLabel.runNumber +
                             " and bank ID " + std::to_string(runLabel.bank) +
                             ")");
  }
  m_focusedRunMap.remove(runLabel);
  if (m_fittedPeaksMap.contains(runLabel)) {
    m_fittedPeaksMap.remove(runLabel);
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
