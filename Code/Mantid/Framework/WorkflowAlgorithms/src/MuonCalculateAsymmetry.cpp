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
};

/// Algorithm's version for identification. @see Algorithm::version
int MuonCalculateAsymmetry::version() const { return 1; };

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
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("FirstPeriodWorkspace",
                                                         "", Direction::Input),
                  "First period data. If second period is not specified - the "
                  "only one used.");

  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>("SecondPeriodWorkspace", "",
                                             Direction::Input,
                                             PropertyMode::Optional),
      "Second period data. If not spefied - first period used only.");

  std::vector<std::string> allowedOperations;
  allowedOperations.push_back("+");
  allowedOperations.push_back("-");
  declareProperty("PeriodOperation", "+",
                  boost::make_shared<StringListValidator>(allowedOperations),
                  "If two periods specified, what operation to apply to "
                  "workspaces to get a final one.");

  std::vector<std::string> allowedTypes;
  allowedTypes.push_back("PairAsymmetry");
  allowedTypes.push_back("GroupAsymmetry");
  allowedTypes.push_back("GroupCounts");
  declareProperty("OutputType", "PairAsymmetry",
                  boost::make_shared<StringListValidator>(allowedTypes),
                  "What kind of workspace required for analysis.");

  declareProperty("PairFirstIndex", EMPTY_INT(),
                  "Workspace index of the first group of the pair. Used when "
                  "OutputType is PairAsymmetry.");

  declareProperty("PairSecondIndex", EMPTY_INT(),
                  "Workspace index of the second group of the pair. Used when "
                  "OutputType is PairAsymmetry.");

  declareProperty(
      "Alpha", 1.0,
      "Alpha value of the pair. Used when OutputType is PairAsymmetry.");

  declareProperty("GroupIndex", EMPTY_INT(), "Workspace index of the group. "
                                             "Used then OutputType is "
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
  MatrixWorkspace_sptr firstPeriodWS = getProperty("FirstPeriodWorkspace");
  MatrixWorkspace_sptr secondPeriodWS = getProperty("SecondPeriodWorkspace");

  MatrixWorkspace_sptr firstConverted = convertWorkspace(firstPeriodWS);

  if (secondPeriodWS) {
    // Two periods
    MatrixWorkspace_sptr secondConverted = convertWorkspace(secondPeriodWS);

    setProperty("OutputWorkspace",
                mergePeriods(firstConverted, secondConverted));
  } else {
    // Single period only

    setProperty("OutputWorkspace", firstConverted);
  }
}

/**
 * Converts given workspace according to the OutputType.
 * @param ws :: Workspace to convert
 * @return Converted workspace
 */
MatrixWorkspace_sptr
MuonCalculateAsymmetry::convertWorkspace(MatrixWorkspace_sptr ws) {
  const std::string type = getPropertyValue("OutputType");

  if (type == "GroupCounts" || type == "GroupAsymmetry") {
    int groupIndex = getProperty("GroupIndex");

    if (groupIndex == EMPTY_INT())
      throw std::runtime_error("GroupIndex is not specified");

    // Yank out the counts of requested group
    IAlgorithm_sptr alg = createChildAlgorithm("ExtractSingleSpectrum");
    alg->initialize();
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("WorkspaceIndex", groupIndex);
    alg->execute();

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");

    if (type == "GroupAsymmetry") {
      // GroupAsymmetry - counts with ExpDecay removed and normalized

      IAlgorithm_sptr alg = createChildAlgorithm("RemoveExpDecay");
      alg->initialize();
      alg->setProperty("InputWorkspace", outWS);
      alg->execute();

      outWS = alg->getProperty("OutputWorkspace");
    }

    return outWS;
  } else if (type == "PairAsymmetry") {
    // PairAsymmetry - result of AsymmetryCalc algorithm

    int pairFirstIndex = getProperty("PairFirstIndex");
    int pairSecondIndex = getProperty("PairSecondIndex");

    if (pairFirstIndex == EMPTY_INT() || pairSecondIndex == EMPTY_INT())
      throw std::invalid_argument("Both pair indices should be specified");

    double alpha = getProperty("Alpha");

    // We get pair groups as their workspace indices, but AsymmetryCalc wants
    // spectra numbers,
    // so need to convert
    specid_t spectraNo1 = ws->getSpectrum(pairFirstIndex)->getSpectrumNo();
    specid_t spectraNo2 = ws->getSpectrum(pairSecondIndex)->getSpectrumNo();

    if (spectraNo1 == -1 || spectraNo2 == -1 || spectraNo1 == spectraNo2)
      throw std::invalid_argument(
          "Spectra numbers of the input workspace are not set properly");

    IAlgorithm_sptr alg = createChildAlgorithm("AsymmetryCalc");
    alg->setProperty("InputWorkspace", ws);
    // As strings, cause otherwise would need to create arrays with single
    // elements
    alg->setPropertyValue("ForwardSpectra",
                          boost::lexical_cast<std::string>(spectraNo1));
    alg->setPropertyValue("BackwardSpectra",
                          boost::lexical_cast<std::string>(spectraNo2));
    alg->setProperty("Alpha", alpha);
    alg->execute();

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");

    return outWS;
  }

  throw std::invalid_argument("Specified OutputType is not supported");
}

/**
 * Merges two period workspaces according to PeriodOperation specified.
 * @param ws1 :: First period workspace
 * @param ws2 :: Second period workspace
 * @return Merged workspace
 */
MatrixWorkspace_sptr
MuonCalculateAsymmetry::mergePeriods(MatrixWorkspace_sptr ws1,
                                     MatrixWorkspace_sptr ws2) {
  std::string op = getProperty("PeriodOperation");

  std::string algorithmName;

  if (op == "+") {
    algorithmName = "Plus";
  } else if (op == "-") {
    algorithmName = "Minus";
  }

  IAlgorithm_sptr alg = createChildAlgorithm(algorithmName);
  alg->initialize();
  alg->setProperty("LHSWorkspace", ws1);
  alg->setProperty("RHSWorkspace", ws2);
  alg->execute();

  MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");

  return outWS;
}
} // namespace WorkflowAlgorithms
} // namespace Mantid
