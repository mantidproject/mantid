#include "MantidWorkflowAlgorithms/MuonGroupCalculator.h"

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
MuonGroupCalculator::MuonGroupCalculator(
    const Mantid::API::WorkspaceGroup_sptr inputWS,
    const std::vector<int> summedPeriods,
    const std::vector<int> subtractedPeriods, const int groupIndex)
    : IMuonAsymmetryCalculator(inputWS, summedPeriods, subtractedPeriods),
      m_groupIndex(groupIndex) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
MuonGroupCalculator::~MuonGroupCalculator() = default;

} // namespace WorkflowAlgorithms
} // namespace Mantid
