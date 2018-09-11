#include "MantidWorkflowAlgorithms/MuonGroupCountsCalculator.h"

using Mantid::API::MatrixWorkspace_sptr;

namespace Mantid {
namespace WorkflowAlgorithms {

//----------------------------------------------------------------------------------------------
/** Constructor
 *
 * @param inputWS :: [input] Input workspace group
 * @param summedPeriods :: [input] Vector of period indexes to be summed
 * @param subtractedPeriods :: [input] Vector of period indexes to be subtracted
 * from summed periods
 * @param groupIndex :: [input] Workspace index of the group to analyse
 */
MuonGroupCountsCalculator::MuonGroupCountsCalculator(
    const Mantid::API::WorkspaceGroup_sptr inputWS,
    const std::vector<int> summedPeriods,
    const std::vector<int> subtractedPeriods, const int groupIndex)
    : MuonGroupCalculator(inputWS, summedPeriods, subtractedPeriods,
                          groupIndex) {}

/**
 * Calculates raw counts according to period arithmetic
 * @returns Workspace containing result of calculation
 */
MatrixWorkspace_sptr MuonGroupCountsCalculator::calculate() const {
  MatrixWorkspace_sptr outWS;
  int numPeriods = m_inputWS->getNumberOfEntries();
  if (numPeriods > 1) {
    // Several periods supplied
    MatrixWorkspace_sptr tempWS = sumPeriods(m_summedPeriods);
    if (!m_subtractedPeriods.empty()) {
      MatrixWorkspace_sptr toSubtractWS = sumPeriods(m_subtractedPeriods);
      tempWS = subtractWorkspaces(tempWS, toSubtractWS);
    }
    outWS = extractSpectrum(tempWS, m_groupIndex);
  } else {
    // Only one period supplied
    outWS = extractSpectrum(m_inputWS->getItem(0), m_groupIndex);
  }
  return outWS;
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
