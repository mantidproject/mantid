#include "MantidWorkflowAlgorithms/IMuonAsymmetryCalculator.h"

using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::WorkspaceGroup_sptr;
using Mantid::API::Workspace_sptr;
using Mantid::API::IAlgorithm_sptr;
using Mantid::API::AlgorithmManager;

namespace Mantid {
namespace WorkflowAlgorithms {

//----------------------------------------------------------------------------------------------
/** Constructor
 *
 * @param inputWS :: [input] Input workspace group
 * @param summedPeriods :: [input] Vector of period indexes to be summed
 * @param subtractedPeriods :: [input] Vector of period indexes to be subtracted
 * from summed periods
 *
 * Constructor will throw if any of the arguments are invalid.
 * @throws std::invalid_argument if arguments are invalid
 * @see IMuonAsymmetryCalculator::validateInputs()
 */
IMuonAsymmetryCalculator::IMuonAsymmetryCalculator(
    const WorkspaceGroup_sptr inputWS, const std::vector<int> summedPeriods,
    const std::vector<int> subtractedPeriods)
    : m_inputWS(inputWS), m_summedPeriods(summedPeriods),
      m_subtractedPeriods(subtractedPeriods) {
  validateInputs();
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
IMuonAsymmetryCalculator::~IMuonAsymmetryCalculator() {}

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
      int numPeriods = static_cast<int>(periodsToSum.size());
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

/**
 * Checks the arguments given to the constructor are ok and throws an exception
 * if not.
 * - input WorkspaceGroup must have 1 or more periods
 * - input period sets must contain valid period numbers (0 < n <= numPeriods)
 * @throws std::invalid_argument if arguments are invalid
 */
void IMuonAsymmetryCalculator::validateInputs() const {
  // input group must not be empty
  int numPeriods = m_inputWS->getNumberOfEntries();
  if (numPeriods < 1) {
    throw std::invalid_argument(
        "Must supply at least one workspace with period data!");
  }
  // check summed period numbers
  std::vector<int> invalidPeriods;
  for (auto period : m_summedPeriods) {
    if ((period < 1) || (period > numPeriods)) {
      invalidPeriods.push_back(period);
    }
  }
  if (!invalidPeriods.empty()) {
    throw std::invalid_argument(
        buildErrorString("SummedPeriodSet", invalidPeriods));
  }
  // check subtracted period numbers, if present
  if (!m_subtractedPeriods.empty()) {
    for (auto period : m_subtractedPeriods) {
      if ((period < 1) || (period > numPeriods)) {
        invalidPeriods.push_back(period);
      }
    }
    if (!invalidPeriods.empty()) {
      throw std::invalid_argument(
          buildErrorString("SubtractedPeriodSet", invalidPeriods));
    }
  }
}

/**
 * Builds an error message from the supplied parameters.
 * @param paramName :: [input] Name of parameter with invalid periods
 * @param invalidPeriods :: [input] Vector containing invalid periods
 * @returns An error message
 */
std::string IMuonAsymmetryCalculator::buildErrorString(
    const std::string &paramName,
    const std::vector<int> &invalidPeriods) const {
  std::stringstream message;
  message << "Invalid periods specified in " << paramName << ": ";
  for (auto it = invalidPeriods.begin(); it != invalidPeriods.end(); it++) {
    message << *it;
    if (it != invalidPeriods.end() - 1) {
      message << ", ";
    }
  }
  return message.str();
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
