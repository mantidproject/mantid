#include "MantidWorkflowAlgorithms/MuonCalculateAsymmetry.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"

namespace Mantid {
namespace WorkflowAlgorithms {
using namespace Kernel;
using namespace API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MuonCalculateAsymmetry)

//----------------------------------------------------------------------------------------------

/**
 * Constructor
 */
MuonCalculateAsymmetry::MuonCalculateAsymmetry() {}

//----------------------------------------------------------------------------------------------

/**
 * Destructor
 */
MuonCalculateAsymmetry::~MuonCalculateAsymmetry() {}

//----------------------------------------------------------------------------------------------

/// Algorithm's name for identification. @see Algorithm::name
const std::string MuonCalculateAsymmetry::name() const {
  return "MuonCalculateAsymmetry";
}

/// Algorithm's version for identification. @see Algorithm::version
int MuonCalculateAsymmetry::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string MuonCalculateAsymmetry::category() const {
  return "Workflow\\Muon";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------

/**
 * Initialize the algorithm's properties.
 */
void MuonCalculateAsymmetry::init() {
  declareProperty(new WorkspaceProperty<WorkspaceGroup>("InputWorkspace", "",
                                                        Direction::Input),
                  "Workspace group containing period data. If it only contains "
                  "one period, then only one is used.");

  declareProperty(
      new ArrayProperty<int>(
          "SummedPeriodSet", "1",
          boost::make_shared<MandatoryValidator<std::vector<int>>>(),
          Direction::Input),
      "Comma-separated list of periods to be summed");

  declareProperty(
      new ArrayProperty<int>("SubtractedPeriodSet", Direction::Input),
      "Comma-separated list of periods to be subtracted from the "
      "SummedPeriodSet");

  std::vector<std::string> allowedTypes;
  allowedTypes.push_back("PairAsymmetry");
  allowedTypes.push_back("GroupAsymmetry");
  allowedTypes.push_back("GroupCounts");
  declareProperty("OutputType", "PairAsymmetry",
                  boost::make_shared<StringListValidator>(allowedTypes),
                  "What kind of calculation required for analysis.");

  declareProperty(
      "PairFirstIndex", EMPTY_INT(),
      "Workspace index of the first group of the pair. Only used when "
      "OutputType is PairAsymmetry.");

  declareProperty(
      "PairSecondIndex", EMPTY_INT(),
      "Workspace index of the second group of the pair. Only used when "
      "OutputType is PairAsymmetry.");

  declareProperty(
      "Alpha", 1.0,
      "Alpha value of the pair. Only used when OutputType is PairAsymmetry.");

  declareProperty("GroupIndex", EMPTY_INT(),
                  "Workspace index of the group to analyse. "
                  "Only used then OutputType is "
                  "GroupAsymmetry or GroupCounts.");

  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "",
                                             Direction::Output),
      "Output workspace. Type of the data depends on the OutputType.");
}

//----------------------------------------------------------------------------------------------

/**
 * Execute the algorithm.
 */
void MuonCalculateAsymmetry::exec() {

  WorkspaceGroup_const_sptr inputWSGroup = getProperty("InputWorkspace");

  // The type of calculation
  const std::string type = getPropertyValue("OutputType");

  // The group index
  int groupIndex = getProperty("GroupIndex");

  // The periods to sum and subtract
  // e.g. if summedPeriods is (1,2) and subtractedPeriods is (3,4),
  // the operation will be (1 + 2) - (3 + 4)
  std::vector<int> summedPeriods = getProperty("SummedPeriodSet");
  std::vector<int> subtractedPeriods = getProperty("SubtractedPeriodSet");

  if (type == "GroupCounts") {

    auto outWS = calculateGroupCounts(inputWSGroup, groupIndex, summedPeriods,
                                      subtractedPeriods);

    setProperty("OutputWorkspace", outWS);

  } else if (type == "GroupAsymmetry") {

    auto outWS = calculateGroupAsymmetry(inputWSGroup, groupIndex,
                                         summedPeriods, subtractedPeriods);

    setProperty("OutputWorkspace", outWS);

  } else if (type == "PairAsymmetry") {

    int pairFirstIndex = getProperty("PairFirstIndex");
    int pairSecondIndex = getProperty("PairSecondIndex");
    double alpha = getProperty("Alpha");

    auto outWS =
        calculatePairAsymmetry(inputWSGroup, pairFirstIndex, pairSecondIndex,
                               alpha, summedPeriods, subtractedPeriods);

    setProperty("OutputWorkspace", outWS);

  } else {

    throw std::invalid_argument("Specified OutputType is not supported");
  }
}

/**
* Calculates raw counts according to period operation
* @param inputWSGroup :: [input] WorkspaceGroup containing period workspaces
* @param groupIndex :: [input] Index of the workspace to extract counts from
* @param summedPeriods :: [input] Periods to be summed
* @param subtractedPeriods :: [input] Periods to be summed together and their
* sum subtracted from summedPeriods
*/
MatrixWorkspace_sptr MuonCalculateAsymmetry::calculateGroupCounts(
    const WorkspaceGroup_const_sptr &inputWSGroup, int groupIndex,
    const std::vector<int> &summedPeriods,
    const std::vector<int> &subtractedPeriods) {

  MatrixWorkspace_sptr outWS;
  int numPeriods = inputWSGroup->getNumberOfEntries();
  if (numPeriods > 1) {
    // Several periods supplied
    MatrixWorkspace_sptr tempWS = sumPeriods(inputWSGroup, summedPeriods);
    if (!subtractedPeriods.empty()) {
      MatrixWorkspace_sptr toSubtractWS =
          sumPeriods(inputWSGroup, subtractedPeriods);
      tempWS = subtractWorkspaces(tempWS, toSubtractWS);
    }
    outWS = extractSpectrum(tempWS, groupIndex);
  } else {
    // Only one period supplied
    outWS = extractSpectrum(inputWSGroup->getItem(0), groupIndex);
  }
  return outWS;
}

/**
* Calculates single-spectrum asymmetry according to period operation
* @param inputWSGroup :: [input] WorkspaceGroup containing period workspaces
* @param groupIndex :: [input] Workspace index for which to calculate asymmetry
* @param summedPeriods :: [input] Periods to be summed
* @param subtractedPeriods :: [input] Periods to be summed together and their
* sum subtracted from summedPeriods
*/
MatrixWorkspace_sptr MuonCalculateAsymmetry::calculateGroupAsymmetry(
    const WorkspaceGroup_const_sptr &inputWSGroup, int groupIndex,
    const std::vector<int> &summedPeriods,
    const std::vector<int> &subtractedPeriods) {

  // The output workspace
  MatrixWorkspace_sptr tempWS;

  int numPeriods = inputWSGroup->getNumberOfEntries();
  if (numPeriods > 1) {
    // Several period workspaces were supplied

    auto summedWS = sumPeriods(inputWSGroup, summedPeriods);
    auto subtractedWS = sumPeriods(inputWSGroup, subtractedPeriods);

    // Remove decay (summed periods ws)
    MatrixWorkspace_sptr asymSummedPeriods =
        removeExpDecay(summedWS, groupIndex);

    if (!subtractedPeriods.empty()) {
      // Remove decay (subtracted periods ws)
      MatrixWorkspace_sptr asymSubtractedPeriods =
          removeExpDecay(subtractedWS, groupIndex);
      // Now subtract
      tempWS = subtractWorkspaces(asymSummedPeriods, asymSubtractedPeriods);
    } else {
      tempWS = asymSummedPeriods;
    }
  } else {
    // Only one period was supplied
    tempWS = removeExpDecay(inputWSGroup->getItem(0), -1);
  }

  // Extract the requested spectrum
  MatrixWorkspace_sptr outWS = extractSpectrum(tempWS, groupIndex);
  return outWS;
}

/**
* Calculates pair asymmetry according to period operation
* @param inputWSGroup :: [input] WorkspaceGroup containing period workspaces
* @param firstPairIndex :: [input] Workspace index for the forward group
* @param secondPairIndex :: [input] Workspace index for the backward group
* @param alpha :: [input] The balance parameter
* @param summedPeriods :: [input] Periods to be summed
* @param subtractedPeriods :: [input] Periods to be summed together and their
* sum subtracted from summedPeriods
*/
MatrixWorkspace_sptr MuonCalculateAsymmetry::calculatePairAsymmetry(
    const WorkspaceGroup_const_sptr &inputWSGroup, int firstPairIndex,
    int secondPairIndex, double alpha, const std::vector<int> &summedPeriods,
    const std::vector<int> &subtractedPeriods) {

  MatrixWorkspace_sptr outWS;
  int numPeriods = inputWSGroup->getNumberOfEntries();
  if (numPeriods > 1) {

    auto summedWS = sumPeriods(inputWSGroup, summedPeriods);
    auto subtractedWS = sumPeriods(inputWSGroup, subtractedPeriods);

    // Summed periods asymmetry
    MatrixWorkspace_sptr asymSummedPeriods =
        asymmetryCalc(summedWS, firstPairIndex, secondPairIndex, alpha);

    if (!subtractedPeriods.empty()) {
      // Subtracted periods asymmetry
      MatrixWorkspace_sptr asymSubtractedPeriods =
          asymmetryCalc(subtractedWS, firstPairIndex, secondPairIndex, alpha);
      // Now subtract
      outWS = subtractWorkspaces(asymSummedPeriods, asymSubtractedPeriods);
    } else {
      outWS = asymSummedPeriods;
    }

  } else {
    outWS = asymmetryCalc(inputWSGroup->getItem(0), firstPairIndex,
                          secondPairIndex, alpha);
  }
  return outWS;
}

/**
 * Performs validation of the input parameters:
 * - input WorkspaceGroup must have at least one workspace in it
 * - input period sets must contain valid period numbers (0 < n <= numPeriods)
 * @returns a map of errors to show the user
 * @see Algorithm::validateInputs
 */
std::map<std::string, std::string> MuonCalculateAsymmetry::validateInputs() {
  std::map<std::string, std::string> errors;

  // input group must not be empty
  WorkspaceGroup_const_sptr inputWSGroup = getProperty("InputWorkspace");
  int numPeriods = inputWSGroup->getNumberOfEntries();
  if (numPeriods < 1) {
    errors["InputWorkspace"] =
        "Must supply at least one workspace with period data!";
  }

  // Check SummedPeriodSet for invalid period numbers
  std::vector<int> summedPeriods = getProperty("SummedPeriodSet");
  std::vector<int> invalidPeriods = findInvalidPeriods(summedPeriods);
  if (!invalidPeriods.empty()) {
    errors["SummedPeriodSet"] = buildErrorString(invalidPeriods);
  }

  // If SubtractedPeriodSet is not empty, check that too
  std::vector<int> subtractedPeriods = getProperty("SubtractedPeriodSet");
  if (!subtractedPeriods.empty()) {
    invalidPeriods = findInvalidPeriods(subtractedPeriods);
    if (!invalidPeriods.empty()) {
      errors["SubtractedPeriodSet"] = buildErrorString(invalidPeriods);
    }
  }

  return errors;
}

/**
 * Checks the supplied list of periods for any invalid values.
 * Invalid means 0, negative or greater than total number of periods.
 * @param periodSet :: [input] vector of period numbers to check
 * @returns a vector of invalid period numbers
 */
std::vector<int> MuonCalculateAsymmetry::findInvalidPeriods(
    const std::vector<int> &periodSet) const {
  WorkspaceGroup_const_sptr inputWSGroup = getProperty("InputWorkspace");
  int numPeriods = inputWSGroup->getNumberOfEntries();
  std::vector<int> invalidPeriods;
  for (auto it = periodSet.begin(); it != periodSet.end(); it++) {
    if ((*it < 1) || (*it > numPeriods)) {
      invalidPeriods.push_back(*it);
    }
  }
  return invalidPeriods;
}

/**
 * Uses the supplied list of invalid period numbers to build an error string
 * @param invalidPeriods :: [input] vector of invalid period numbers
 * @returns an error message
 */
std::string MuonCalculateAsymmetry::buildErrorString(
    const std::vector<int> &invalidPeriods) const {
  std::stringstream message;
  message << "Invalid periods specified: ";
  for (auto it = invalidPeriods.begin(); it != invalidPeriods.end(); it++) {
    message << *it << ", ";
  }
  return message.str();
}

/**
 * Sums the specified periods of the supplied workspace group
 * @param inputWSGroup :: [input] WorkspaceGroup containing period workspaces
 * @param periodsToSum :: [input] List of period indexes (1-based) to be summed
 * @returns Workspace containing the sum
 */
MatrixWorkspace_sptr MuonCalculateAsymmetry::sumPeriods(
    const WorkspaceGroup_const_sptr &inputWSGroup,
    const std::vector<int> &periodsToSum) {
  MatrixWorkspace_sptr outWS;
  if (!periodsToSum.empty()) {
    auto LHSWorkspace = inputWSGroup->getItem(periodsToSum[0] - 1);
    outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(LHSWorkspace);
    if (outWS != nullptr && periodsToSum.size() > 1) {
      int numPeriods = static_cast<int>(periodsToSum.size());
      for (int i = 1; i < numPeriods; i++) {
        auto RHSWorkspace = inputWSGroup->getItem(periodsToSum[i] - 1);
        IAlgorithm_sptr alg = createChildAlgorithm("Plus");
        alg->initialize();
        alg->setProperty("LHSWorkspace", outWS);
        alg->setProperty("RHSWorkspace", RHSWorkspace);
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
MatrixWorkspace_sptr
MuonCalculateAsymmetry::subtractWorkspaces(const MatrixWorkspace_sptr &lhs,
                                           const MatrixWorkspace_sptr &rhs) {
  MatrixWorkspace_sptr outWS;
  if (lhs && rhs) {
    IAlgorithm_sptr alg = createChildAlgorithm("Minus");
    alg->initialize();
    alg->setProperty("LHSWorkspace", lhs);
    alg->setProperty("RHSWorkspace", rhs);
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
MuonCalculateAsymmetry::extractSpectrum(const Workspace_sptr &inputWS,
                                        const int index) {
  MatrixWorkspace_sptr outWS;
  if (inputWS) {
    IAlgorithm_sptr alg = createChildAlgorithm("ExtractSingleSpectrum");
    alg->initialize();
    alg->setProperty("InputWorkspace", inputWS);
    alg->setProperty("WorkspaceIndex", index);
    alg->execute();
    outWS = alg->getProperty("OutputWorkspace");
  }
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
MuonCalculateAsymmetry::removeExpDecay(const Workspace_sptr &inputWS,
                                       const int index) {
  MatrixWorkspace_sptr outWS;
  // Remove decay
  if (inputWS) {
    IAlgorithm_sptr asym = createChildAlgorithm("RemoveExpDecay");
    asym->initialize();
    asym->setProperty("InputWorkspace", inputWS);
    if (index > 0) {
      // GroupIndex as vector
      // Necessary if we want RemoveExpDecay to fit only the requested
      // spectrum
      std::vector<int> spec(1, index);
      asym->setProperty("Spectra", spec);
    }
    asym->execute();
    outWS = asym->getProperty("OutputWorkspace");
  }
  return outWS;
}

/**
 * Performs asymmetry calculation on the given workspace.
 * @param inputWS :: [input] Workspace to calculate asymmetry from
 * @param firstPairIndex :: [input] Workspace index for the forward group
 * @param secondPairIndex :: [input] Workspace index for the backward group
 * @param alpha :: [input] The balance parameter
 * @returns Result of the calculation
 */
MatrixWorkspace_sptr MuonCalculateAsymmetry::asymmetryCalc(
    const Workspace_sptr &inputWS, const int firstPairIndex,
    const int secondPairIndex, const double alpha) {
  MatrixWorkspace_sptr outWS;

  if (inputWS) {
    // Pair indices as vectors
    std::vector<int> fwd(1, firstPairIndex + 1);
    std::vector<int> bwd(1, secondPairIndex + 1);

    IAlgorithm_sptr alg = createChildAlgorithm("AsymmetryCalc");
    alg->setProperty("InputWorkspace", inputWS);
    alg->setProperty("ForwardSpectra", fwd);
    alg->setProperty("BackwardSpectra", bwd);
    alg->setProperty("Alpha", alpha);
    alg->execute();
    outWS = alg->getProperty("OutputWorkspace");
  }

  return outWS;
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
