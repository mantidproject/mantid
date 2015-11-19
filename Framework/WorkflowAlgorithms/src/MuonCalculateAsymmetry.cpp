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
                                         subtractedPeriods, subtractedPeriods);

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

  int numPeriods = inputWSGroup->getNumberOfEntries();
  if (numPeriods > 1) {
    // Several periods supplied

    MatrixWorkspace_sptr tempWS = sumPeriods(inputWSGroup, summedPeriods);
    if (!subtractedPeriods.empty()) {
      MatrixWorkspace_sptr toSubtractWS =
          sumPeriods(inputWSGroup, subtractedPeriods);
      IAlgorithm_sptr alg = createChildAlgorithm("Minus");
      alg->initialize();
      alg->setProperty("LHSWorkspace", tempWS);
      alg->setProperty("RHSWorkspace", toSubtractWS);
      alg->execute();
      tempWS = alg->getProperty("OutputWorkspace");
    }

    IAlgorithm_sptr alg = createChildAlgorithm("ExtractSingleSpectrum");
    alg->initialize();
    alg->setProperty("InputWorkspace", tempWS);
    alg->setProperty("WorkspaceIndex", groupIndex);
    alg->execute();
    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");

    return outWS;

  } else {
    // Only one period supplied

    IAlgorithm_sptr alg = createChildAlgorithm("ExtractSingleSpectrum");
    alg->initialize();
    alg->setProperty("InputWorkspace", inputWSGroup->getItem(0));
    alg->setProperty("WorkspaceIndex", groupIndex);
    alg->execute();
    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");

    return outWS;
  }
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

    if (op == "+") {

      // Sum
      IAlgorithm_sptr alg = createChildAlgorithm("Plus");
      alg->initialize();
      alg->setProperty("LHSWorkspace", inputWSGroup->getItem(0));
      alg->setProperty("RHSWorkspace", inputWSGroup->getItem(1));
      alg->execute();
      MatrixWorkspace_sptr sumWS = alg->getProperty("OutputWorkspace");

      // GroupIndex as vector
      // Necessary if we want RemoveExpDecay to fit only the requested
      // spectrum
      std::vector<int> spec(1, groupIndex);

      // Remove decay
      IAlgorithm_sptr asym = createChildAlgorithm("RemoveExpDecay");
      asym->setProperty("InputWorkspace", sumWS);
      asym->setProperty("Spectra", spec);
      asym->execute();
      tempWS = asym->getProperty("OutputWorkspace");

    } else if (op == "-") {

      // GroupIndex as vector
      std::vector<int> spec(1, groupIndex);

      // Remove decay (first period ws)
      IAlgorithm_sptr asym = createChildAlgorithm("RemoveExpDecay");
      asym->initialize();
      asym->setProperty("InputWorkspace", inputWSGroup->getItem(0));
      asym->setProperty("Spectra", spec);
      asym->execute();
      MatrixWorkspace_sptr asymFirstPeriod =
          asym->getProperty("OutputWorkspace");

      // Remove decay (second period ws)
      asym = createChildAlgorithm("RemoveExpDecay");
      asym->initialize();
      asym->setProperty("InputWorkspace", inputWSGroup->getItem(1));
      asym->setProperty("Spectra", spec);
      asym->execute();
      MatrixWorkspace_sptr asymSecondPeriod =
          asym->getProperty("OutputWorkspace");

      // Now subtract
      IAlgorithm_sptr alg = createChildAlgorithm("Minus");
      alg->initialize();
      alg->setProperty("LHSWorkspace", asymFirstPeriod);
      alg->setProperty("RHSWorkspace", asymSecondPeriod);
      alg->execute();
      tempWS = alg->getProperty("OutputWorkspace");
    }
  } else {
    // Only one period was supplied

    IAlgorithm_sptr alg = createChildAlgorithm("RemoveExpDecay");
    alg->initialize();
    alg->setProperty("InputWorkspace", inputWSGroup->getItem(0));
    alg->execute();
    tempWS = alg->getProperty("OutputWorkspace");
  }

  // Extract the requested spectrum
  IAlgorithm_sptr alg = createChildAlgorithm("ExtractSingleSpectrum");
  alg->initialize();
  alg->setProperty("InputWorkspace", tempWS);
  alg->setProperty("WorkspaceIndex", groupIndex);
  alg->execute();
  MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
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

  // Pair indices as vectors
  std::vector<int> fwd(1, firstPairIndex + 1);
  std::vector<int> bwd(1, secondPairIndex + 1);

  int numPeriods = inputWSGroup->getNumberOfEntries();
  if (numPeriods > 1) {

    MatrixWorkspace_sptr outWS;

    if (op == "+") {

      // Sum
      IAlgorithm_sptr alg = createChildAlgorithm("Plus");
      alg->initialize();
      alg->setProperty("LHSWorkspace", inputWSGroup->getItem(0));
      alg->setProperty("RHSWorkspace", inputWSGroup->getItem(1));
      alg->execute();
      MatrixWorkspace_sptr sumWS = alg->getProperty("OutputWorkspace");

      // Asymmetry calculation
      alg = createChildAlgorithm("AsymmetryCalc");
      alg->setProperty("InputWorkspace", sumWS);
      alg->setProperty("ForwardSpectra", fwd);
      alg->setProperty("BackwardSpectra", bwd);
      alg->setProperty("Alpha", alpha);
      alg->execute();
      outWS = alg->getProperty("OutputWorkspace");

    } else if (op == "-") {

      std::vector<int> fwd(1, firstPairIndex + 1);
      std::vector<int> bwd(1, secondPairIndex + 1);

      // First period asymmetry
      IAlgorithm_sptr alg = createChildAlgorithm("AsymmetryCalc");
      alg->setProperty("InputWorkspace", inputWSGroup->getItem(0));
      alg->setProperty("ForwardSpectra", fwd);
      alg->setProperty("BackwardSpectra", bwd);
      alg->setProperty("Alpha", alpha);
      alg->execute();
      MatrixWorkspace_sptr asymFirstPeriod =
          alg->getProperty("OutputWorkspace");

      // Second period asymmetry
      alg = createChildAlgorithm("AsymmetryCalc");
      alg->setProperty("InputWorkspace", inputWSGroup->getItem(1));
      alg->setProperty("ForwardSpectra", fwd);
      alg->setProperty("BackwardSpectra", bwd);
      alg->setProperty("Alpha", alpha);
      alg->execute();
      MatrixWorkspace_sptr asymSecondPeriod =
          alg->getProperty("OutputWorkspace");

      // Now subtract
      alg = createChildAlgorithm("Minus");
      alg->initialize();
      alg->setProperty("LHSWorkspace", asymFirstPeriod);
      alg->setProperty("RHSWorkspace", asymSecondPeriod);
      alg->execute();
      outWS = alg->getProperty("OutputWorkspace");
    }

    return outWS;

  } else {

    IAlgorithm_sptr alg = createChildAlgorithm("AsymmetryCalc");
    alg->setProperty("InputWorkspace", inputWSGroup->getItem(0));
    alg->setProperty("ForwardSpectra", fwd);
    alg->setProperty("BackwardSpectra", bwd);
    alg->setProperty("Alpha", alpha);
    alg->execute();
    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");

    return outWS;
  }
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
    if (periodsToSum.size() > 1) {
      for (int i = 1; i < periodsToSum.size(); i++) {
        auto RHSWorkspace = inputWSGroup->getItem(periodsToSum[i] - 1);
        IAlgorithm_sptr alg = createChildAlgorithm("Plus");
        alg->initialize();
        alg->setProperty("LHSWorkspace", LHSWorkspace);
        alg->setProperty("RHSWorkspace", RHSWorkspace);
        alg->execute();
        LHSWorkspace = alg->getProperty("OutputWorkspace");
      }
    }
    outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(LHSWorkspace);
  }
  return outWS;
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
