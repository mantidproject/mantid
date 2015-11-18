#include "MantidWorkflowAlgorithms/MuonCalculateAsymmetry.h"
#include "MantidKernel/ListValidator.h"

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

  declareProperty("PeriodOperation", "+",
                  "If several periods specified, what operation to apply to "
                  "workspaces to get a final one.");

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
  if (inputWSGroup->getNumberOfEntries() < 1) {
    throw std::invalid_argument(
        "Must supply at least one workspace with period data!");
  }

  // The type of calculation
  const std::string type = getPropertyValue("OutputType");

  // The group index
  int groupIndex = getProperty("GroupIndex");

  // The type of period operation (+ or -)
  std::string op = getProperty("PeriodOperation");

  if (type == "GroupCounts") {

    auto outWS =
        calculateGroupCounts(inputWSGroup, groupIndex, op);

    setProperty("OutputWorkspace", outWS);

  } else if (type == "GroupAsymmetry") {

    auto outWS =
        calculateGroupAsymmetry(inputWSGroup, groupIndex, op);

    setProperty("OutputWorkspace", outWS);

  } else if (type == "PairAsymmetry") {

    int pairFirstIndex = getProperty("PairFirstIndex");
    int pairSecondIndex = getProperty("PairSecondIndex");
    double alpha = getProperty("Alpha");

    auto outWS =
        calculatePairAsymmetry(inputWSGroup, pairFirstIndex,
                               pairSecondIndex, alpha, op);

    setProperty("OutputWorkspace", outWS);

  } else {

    throw std::invalid_argument("Specified OutputType is not supported");
  }
}

/**
* Calculates raw counts according to period operation
* @param inputWSGroup :: [input] WorkspaceGroup containing period workspaces
* @param groupIndex :: [input] Index of the workspace to extract counts from
* @param op :: [input] Period operation (+ or -)
*/
MatrixWorkspace_sptr MuonCalculateAsymmetry::calculateGroupCounts(
    const WorkspaceGroup_const_sptr &inputWSGroup, int groupIndex,
    std::string op) {

  int numPeriods = inputWSGroup->getNumberOfEntries();
  if (numPeriods > 1) {
    // Two periods supplied

    MatrixWorkspace_sptr tempWS;

    if (op == "+") {

      IAlgorithm_sptr alg = createChildAlgorithm("Plus");
      alg->initialize();
      alg->setProperty("LHSWorkspace", inputWSGroup->getItem(0));
      alg->setProperty("RHSWorkspace", inputWSGroup->getItem(1));
      alg->execute();
      tempWS = alg->getProperty("OutputWorkspace");

    } else if (op == "-") {

      IAlgorithm_sptr alg = createChildAlgorithm("Minus");
      alg->initialize();
      alg->setProperty("LHSWorkspace", inputWSGroup->getItem(0));
      alg->setProperty("RHSWorkspace", inputWSGroup->getItem(1));
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
* @param op :: [input] Period operation (+ or -)
*/
MatrixWorkspace_sptr MuonCalculateAsymmetry::calculateGroupAsymmetry(
    const WorkspaceGroup_const_sptr &inputWSGroup, int groupIndex,
    std::string op) {

  // The output workspace
  MatrixWorkspace_sptr tempWS;

  int numPeriods = inputWSGroup->getNumberOfEntries();
  if (numPeriods > 1) {
    // Two period workspaces where supplied

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
* @param op :: [input] Period operation (+ or -)
*/
MatrixWorkspace_sptr MuonCalculateAsymmetry::calculatePairAsymmetry(
    const WorkspaceGroup_const_sptr &inputWSGroup, int firstPairIndex,
    int secondPairIndex, double alpha, std::string op) {

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
} // namespace WorkflowAlgorithms
} // namespace Mantid
