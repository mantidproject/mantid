// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidWorkflowAlgorithms/IMuonAsymmetryCalculator.h"

using Mantid::API::AlgorithmManager;
using Mantid::API::IAlgorithm_sptr;
using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::WorkspaceGroup_sptr;
using Mantid::API::Workspace_sptr;

namespace Mantid {
namespace WorkflowAlgorithms {

//----------------------------------------------------------------------------------------------
/** Constructor
 *
 * @param inputWS :: [input] Input workspace group
 * @param summedPeriods :: [input] Vector of period indexes to be summed
 * @param subtractedPeriods :: [input] Vector of period indexes to be subtracted
 * from summed periods
 */
IMuonAsymmetryCalculator::IMuonAsymmetryCalculator(
    const WorkspaceGroup_sptr inputWS, const std::vector<int> summedPeriods,
    const std::vector<int> subtractedPeriods)
    : m_inputWS(inputWS), m_summedPeriods(summedPeriods),
      m_subtractedPeriods(subtractedPeriods) {}

/**
 * Sums the specified periods of the input workspace group
 * @param periodsToSum :: [input] List of period indexes (1-based) to be summed
 * @returns Workspace containing the sum
 */
MatrixWorkspace_sptr IMuonAsymmetryCalculator::sumPeriods(
    const std::vector<int> &periodsToSum) const {
  MatrixWorkspace_sptr outWS;
  if (!periodsToSum.empty()) {
    auto LHSWorkspace = m_inputWS->getItem(periodsToSum[0] - 1);
    outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(LHSWorkspace);
    if (outWS != nullptr && periodsToSum.size() > 1) {
      auto numPeriods = static_cast<int>(periodsToSum.size());
      for (int i = 1; i < numPeriods; i++) {
        auto RHSWorkspace = m_inputWS->getItem(periodsToSum[i] - 1);
        IAlgorithm_sptr alg = AlgorithmManager::Instance().create("Plus");
        alg->setChild(true);
        alg->setProperty("LHSWorkspace", outWS);
        alg->setProperty("RHSWorkspace", RHSWorkspace);
        alg->setProperty("OutputWorkspace", "__NotUsed__");
        alg->execute();
        outWS = alg->getProperty("OutputWorkspace");
      }
    }
  }
  return outWS;
}

/**
 * Subtracts one workspace from another: lhs - rhs.
 * @param lhs :: [input] Workspace on LHS of subtraction
 * @param rhs :: [input] Workspace on RHS of subtraction
 * @returns Result of the subtraction
 */
MatrixWorkspace_sptr IMuonAsymmetryCalculator::subtractWorkspaces(
    const MatrixWorkspace_sptr &lhs, const MatrixWorkspace_sptr &rhs) const {
  MatrixWorkspace_sptr outWS;
  if (lhs && rhs) {
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("Minus");
    alg->setChild(true);
    alg->setProperty("LHSWorkspace", lhs);
    alg->setProperty("RHSWorkspace", rhs);
    alg->setProperty("OutputWorkspace", "__NotUsed__");
    alg->execute();
    outWS = alg->getProperty("OutputWorkspace");
  }
  return outWS;
}

/**
 * Extracts a single spectrum from the given workspace.
 * @param inputWS :: [input] Workspace to extract spectrum from
 * @param index :: [input] Index of spectrum to extract
 * @returns Result of the extraction
 */
MatrixWorkspace_sptr
IMuonAsymmetryCalculator::extractSpectrum(const Workspace_sptr &inputWS,
                                          const int index) const {
  MatrixWorkspace_sptr outWS;
  if (inputWS) {
    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().create("ExtractSingleSpectrum");
    alg->setChild(true);
    alg->setProperty("InputWorkspace", inputWS);
    alg->setProperty("WorkspaceIndex", index);
    alg->setProperty("OutputWorkspace", "__NotUsed__");
    alg->execute();
    outWS = alg->getProperty("OutputWorkspace");
  }
  return outWS;
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
