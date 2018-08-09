#include "MantidWorkflowAlgorithms/MuonPairAsymmetryCalculator.h"

using Mantid::API::AlgorithmManager;
using Mantid::API::IAlgorithm_sptr;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::Workspace_sptr;

namespace Mantid {
namespace WorkflowAlgorithms {

//----------------------------------------------------------------------------------------------
/** Constructor
 * @param inputWS :: [input] Input workspace group
 * @param summedPeriods :: [input] Vector of period indexes to be summed
 * @param subtractedPeriods :: [input] Vector of period indexes to be subtracted
 * from summed periods
 * @param firstPairIndex :: [input] Workspace index of the first (forward) group
 * of the pair
 * @param secondPairIndex :: [input] Workspace index of the second (backward)
 * group of the pair
 * @param alpha :: [input] Alpha (balance) value of the pair
 */
MuonPairAsymmetryCalculator::MuonPairAsymmetryCalculator(
    const API::WorkspaceGroup_sptr inputWS,
    const std::vector<int> summedPeriods,
    const std::vector<int> subtractedPeriods, const int firstPairIndex,
    const int secondPairIndex, const double alpha)
    : IMuonAsymmetryCalculator(inputWS, summedPeriods, subtractedPeriods),
      m_alpha(alpha), m_firstPairIndex(firstPairIndex),
      m_secondPairIndex(secondPairIndex) {}

/**
 * Calculates asymmetry for the given pair of groups, using the alpha value
 * provided.
 * @returns Workspace containing result of calculation
 */
MatrixWorkspace_sptr MuonPairAsymmetryCalculator::calculate() const {
  MatrixWorkspace_sptr outWS;

  int numPeriods = m_inputWS->getNumberOfEntries();
  if (numPeriods > 1) {

    auto summedWS = sumPeriods(m_summedPeriods);
    auto subtractedWS = sumPeriods(m_subtractedPeriods);

    // Summed periods asymmetry
    MatrixWorkspace_sptr asymSummedPeriods = asymmetryCalc(summedWS);

    if (!m_subtractedPeriods.empty()) {
      // Subtracted periods asymmetry
      MatrixWorkspace_sptr asymSubtractedPeriods = asymmetryCalc(subtractedWS);
      // Now subtract
      outWS = subtractWorkspaces(asymSummedPeriods, asymSubtractedPeriods);
    } else {
      outWS = asymSummedPeriods;
    }

  } else {
    outWS = asymmetryCalc(m_inputWS->getItem(0));
  }

  return outWS;
}

/**
 * Performs asymmetry calculation on the given workspace.
 * @param inputWS :: [input] Workspace to calculate asymmetry from
 * @returns Result of the calculation
 */
MatrixWorkspace_sptr MuonPairAsymmetryCalculator::asymmetryCalc(
    const Workspace_sptr &inputWS) const {
  MatrixWorkspace_sptr outWS;

  if (inputWS) {
    // Pair indices as vectors
    std::vector<int> fwd(1, m_firstPairIndex + 1);
    std::vector<int> bwd(1, m_secondPairIndex + 1);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("AsymmetryCalc");
    alg->setChild(true);
    alg->setProperty("InputWorkspace", inputWS);
    alg->setProperty("ForwardSpectra", fwd);
    alg->setProperty("BackwardSpectra", bwd);
    alg->setProperty("Alpha", m_alpha);
    alg->setProperty("OutputWorkspace", "__NotUsed__");
    alg->execute();
    outWS = alg->getProperty("OutputWorkspace");
  }

  return outWS;
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
