#include "MantidWorkflowAlgorithms/MuonGroupAsymmetryCalculator.h"

using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::Workspace_sptr;
using Mantid::API::IAlgorithm_sptr;
using Mantid::API::AlgorithmManager;

namespace Mantid {
namespace WorkflowAlgorithms {

//----------------------------------------------------------------------------------------------
/** Constructor
* @param inputWS :: [input] Input workspace group
* @param summedPeriods :: [input] Vector of period indexes to be summed
* @param subtractedPeriods :: [input] Vector of period indexes to be subtracted
* from summed periods
* @param groupIndex :: [input] Workspace index of the group to analyse
 */
MuonGroupAsymmetryCalculator::MuonGroupAsymmetryCalculator(
    const Mantid::API::WorkspaceGroup_sptr inputWS,
    const std::vector<int> summedPeriods,
    const std::vector<int> subtractedPeriods, const int groupIndex)
    : MuonGroupCalculator(inputWS, summedPeriods, subtractedPeriods,
                          groupIndex) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
MuonGroupAsymmetryCalculator::~MuonGroupAsymmetryCalculator() = default;

/**
* Calculates asymmetry between given group (specified via group index) and Muon
* exponential decay
* @returns Workspace containing result of calculation
*/
MatrixWorkspace_sptr MuonGroupAsymmetryCalculator::calculate() const {
  // The output workspace
  MatrixWorkspace_sptr tempWS;

  int numPeriods = m_inputWS->getNumberOfEntries();
  if (numPeriods > 1) {
    // Several period workspaces were supplied

    auto summedWS = sumPeriods(m_summedPeriods);
    auto subtractedWS = sumPeriods(m_subtractedPeriods);

    // Remove decay (summed periods ws)
    MatrixWorkspace_sptr asymSummedPeriods =
        removeExpDecay(summedWS, m_groupIndex);

    if (!m_subtractedPeriods.empty()) {
      // Remove decay (subtracted periods ws)
      MatrixWorkspace_sptr asymSubtractedPeriods =
          removeExpDecay(subtractedWS, m_groupIndex);
      // Now subtract
      tempWS = subtractWorkspaces(asymSummedPeriods, asymSubtractedPeriods);
    } else {
      tempWS = asymSummedPeriods;
    }
  } else {
    // Only one period was supplied
    tempWS = removeExpDecay(m_inputWS->getItem(0), -1);
  }

  // Extract the requested spectrum
  MatrixWorkspace_sptr outWS = extractSpectrum(tempWS, m_groupIndex);

  return outWS;
}

/**
* Removes exponential decay from the given workspace.
* @param inputWS :: [input] Workspace to remove decay from
* @param index :: [input] GroupIndex (fit only the requested spectrum): use -1
* for "unset"
* @returns Result of the removal
*/
MatrixWorkspace_sptr
MuonGroupAsymmetryCalculator::removeExpDecay(const Workspace_sptr &inputWS,
                                             const int index) const {
  MatrixWorkspace_sptr outWS;
  // Remove decay
  if (inputWS) {
    IAlgorithm_sptr asym =
        AlgorithmManager::Instance().create("RemoveExpDecay");
    asym->setChild(true);
    asym->setProperty("InputWorkspace", inputWS);
    if (index > 0) {
      // GroupIndex as vector
      // Necessary if we want RemoveExpDecay to fit only the requested
      // spectrum
      std::vector<int> spec(1, index);
      asym->setProperty("Spectra", spec);
    }
    asym->setProperty("OutputWorkspace", "__NotUsed__");
    asym->execute();
    outWS = asym->getProperty("OutputWorkspace");
  }
  return outWS;
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
