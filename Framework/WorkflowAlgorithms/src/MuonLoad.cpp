#include "MantidWorkflowAlgorithms/MuonLoad.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/make_unique.h"
#include "MantidWorkflowAlgorithms/MuonGroupAsymmetryCalculator.h"
#include "MantidWorkflowAlgorithms/MuonGroupCountsCalculator.h"
#include "MantidWorkflowAlgorithms/MuonPairAsymmetryCalculator.h"

// free functions
namespace {
/**
 * Tests if argument is equal to one.
 * @param i :: [input] integer to test
 * @returns True if i == 1, else false.
 */
bool isOne(int i) { return (i == 1); }
}

namespace Mantid {
namespace WorkflowAlgorithms {
using namespace Kernel;
using namespace API;
using namespace DataObjects;
using API::WorkspaceGroup_sptr;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MuonLoad)

//----------------------------------------------------------------------------------------------
/**
 * Constructor
 */
MuonLoad::MuonLoad() {}

//----------------------------------------------------------------------------------------------
/**
 * Destructor
 */
MuonLoad::~MuonLoad() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string MuonLoad::name() const { return "MuonLoad"; }

/// Algorithm's version for identification. @see Algorithm::version
int MuonLoad::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string MuonLoad::category() const { return "Workflow\\Muon"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/*
 * Initialize the algorithm's properties.
 */
void MuonLoad::init() {
  declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace", "",
                                                   Direction::Input,
                                                   PropertyMode::Mandatory),
                  "Input workspace loaded from file (e.g. by LoadMuonNexus)");

  std::vector<std::string> allowedModes;
  allowedModes.push_back("CorrectAndGroup");
  allowedModes.push_back("Analyse");
  allowedModes.push_back("Combined");
  declareProperty("Mode", "Combined",
                  boost::make_shared<StringListValidator>(allowedModes),
                  "Mode to run in. CorrectAndGroup applies dead time "
                  "correction and grouping; Analyse changes bin offset, "
                  "crops, rebins and calculates asymmetry; Combined does all "
                  "of the above.");

  declareProperty(new ArrayProperty<int>("SummedPeriodSet", Direction::Input),
                  "Comma-separated list of periods to be summed");

  declareProperty(
      new ArrayProperty<int>("SubtractedPeriodSet", Direction::Input),
      "Comma-separated list of periods to be subtracted from the "
      "SummedPeriodSet");

  declareProperty(
      "ApplyDeadTimeCorrection", false,
      "Whether dead time correction should be applied to loaded workspace");
  declareProperty(new WorkspaceProperty<TableWorkspace>("DeadTimeTable", "",
                                                        Direction::Input,
                                                        PropertyMode::Optional),
                  "Table with dead time information, e.g. from LoadMuonNexus."
                  "Must be specified if ApplyDeadTimeCorrection is set true.");
  declareProperty(
      new WorkspaceProperty<TableWorkspace>("DetectorGroupingTable", "",
                                            Direction::Input,
                                            PropertyMode::Mandatory),
      "Table with detector grouping information, e.g. from LoadMuonNexus.");

  declareProperty("TimeZero", EMPTY_DBL(),
                  "Value used for Time Zero correction");
  declareProperty("LoadedTimeZero", EMPTY_DBL(),
                  boost::make_shared<MandatoryValidator<double>>(),
                  "Time Zero value loaded from file, e.g. from LoadMuonNexus.");
  declareProperty(
      new ArrayProperty<double>("RebinParams"),
      "Params used for rebinning. If empty - rebinning is not done.");
  declareProperty("Xmin", EMPTY_DBL(), "Minimal X value to include");
  declareProperty("Xmax", EMPTY_DBL(), "Maximal X value to include");

  std::vector<std::string> allowedTypes;
  allowedTypes.push_back("PairAsymmetry");
  allowedTypes.push_back("GroupAsymmetry");
  allowedTypes.push_back("GroupCounts");
  declareProperty("OutputType", "PairAsymmetry",
                  boost::make_shared<StringListValidator>(allowedTypes),
                  "What kind of workspace required for analysis.");

  declareProperty("PairFirstIndex", EMPTY_INT(),
                  "Workspace index of the first pair group");
  declareProperty("PairSecondIndex", EMPTY_INT(),
                  "Workspace index of the second pair group");
  declareProperty("Alpha", 1.0, "Alpha value of the pair");

  declareProperty("GroupIndex", EMPTY_INT(), "Workspace index of the group");

  declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace", "",
                                                   Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/**
 * Execute the algorithm.
 */
void MuonLoad::exec() {
  Progress progress(this, 0, 1, 5);

  // Supplied input workspace
  Workspace_sptr inputWS = getProperty("InputWorkspace");

  std::vector<int> summedPeriods = getProperty("SummedPeriodSet");
  std::vector<int> subtractedPeriods = getProperty("SubtractedPeriodSet");
  auto allPeriodsWS = boost::make_shared<WorkspaceGroup>();
  progress.report();

  // Deal with single-period workspace
  if (auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(inputWS)) {
    if (std::find_if_not(summedPeriods.begin(), summedPeriods.end(), isOne) !=
        summedPeriods.end()) {
      throw std::invalid_argument("Single period data but set of periods to "
                                  "sum contains invalid values.");
    }

    if (!subtractedPeriods.empty())
      throw std::invalid_argument(
          "Single period data but second set of periods specified");

    allPeriodsWS->addWorkspace(ws);
  }
  // Deal with multi-period workspace
  else if (auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(inputWS)) {
    allPeriodsWS = group;
  }
  // Unexpected workspace type
  else {
    throw std::runtime_error("Input workspace is of invalid type");
  }

  progress.report();

  // Check mode
  const std::string mode = getProperty("Mode");

  // Dead time correction and grouping
  if (mode != "Analyse") {
    bool applyDtc = getProperty("ApplyDeadTimeCorrection");
    // Deal with dead time correction (if required)
    if (applyDtc) {
      TableWorkspace_sptr deadTimes = getProperty("DeadTimeTable");
      if (!deadTimes) {
        throw std::runtime_error(
            "Cannot apply dead time correction as no dead times were supplied");
      }
      allPeriodsWS = applyDTC(allPeriodsWS, deadTimes);
    }
    progress.report();
    TableWorkspace_sptr grouping;
    grouping = getProperty("DetectorGroupingTable");
    progress.report();
    // Deal with grouping
    allPeriodsWS = groupWorkspaces(allPeriodsWS, grouping);
  } else {
    progress.report();
    progress.report();
  }

  // If not analysing, the present WS will be the output
  Workspace_sptr outWS = allPeriodsWS;

  if (mode != "CorrectAndGroup") {
    // For these modes, SummedPeriodSet is mandatory
    if (summedPeriods.empty()) {
      throw std::invalid_argument(
          "Cannot analyse: list of periods to sum was empty");
    }

    // Correct bin values
    double loadedTimeZero = getProperty("LoadedTimeZero");
    allPeriodsWS = correctWorkspaces(allPeriodsWS, loadedTimeZero);

    // Perform appropriate calculation
    std::string outputType = getProperty("OutputType");
    int groupIndex = getProperty("GroupIndex");
    std::unique_ptr<IMuonAsymmetryCalculator> asymCalc;
    if (outputType == "GroupCounts") {
      asymCalc = Mantid::Kernel::make_unique<MuonGroupCountsCalculator>(
        allPeriodsWS, summedPeriods, subtractedPeriods, groupIndex);
    }
    else if (outputType == "GroupAsymmetry") {
      asymCalc = Mantid::Kernel::make_unique<MuonGroupAsymmetryCalculator>(
        allPeriodsWS, summedPeriods, subtractedPeriods, groupIndex);
    }
    else if (outputType == "PairAsymmetry") {
      int first = getProperty("PairFirstIndex");
      int second = getProperty("PairSecondIndex");
      double alpha = getProperty("Alpha");
      asymCalc = Mantid::Kernel::make_unique<MuonPairAsymmetryCalculator>(
        allPeriodsWS, summedPeriods, subtractedPeriods, first, second, alpha);
    }
    progress.report();
    outWS = asymCalc->calculate();
  }

  setProperty("OutputWorkspace", outWS);
}

/**
 * Groups specified workspace group according to specified
 * DetectorGroupingTable.
 * @param wsGroup :: WorkspaceGroup to group
 * @param grouping :: Detector grouping table to use
 * @return Grouped workspaces
 */
WorkspaceGroup_sptr MuonLoad::groupWorkspaces(WorkspaceGroup_sptr wsGroup,
                                              TableWorkspace_sptr grouping) {
  WorkspaceGroup_sptr outWS = boost::make_shared<WorkspaceGroup>();
  for (int i = 0; i < wsGroup->getNumberOfEntries(); i++) {
    auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(wsGroup->getItem(i));
    if (ws) {
      MatrixWorkspace_sptr result;
      IAlgorithm_sptr group = createChildAlgorithm("MuonGroupDetectors");
      group->setProperty("InputWorkspace", ws);
      group->setProperty("DetectorGroupingTable", grouping);
      group->execute();
      result = group->getProperty("OutputWorkspace");
      outWS->addWorkspace(result);
    }
  }
  return outWS;
}

/**
 * Applies dead time correction to the workspace group.
 * @param wsGroup :: Workspace group to apply correction to
 * @param dt :: Dead time table to use
 * @return Corrected workspace group
 */
WorkspaceGroup_sptr MuonLoad::applyDTC(WorkspaceGroup_sptr wsGroup,
                                       TableWorkspace_sptr dt) {
  WorkspaceGroup_sptr outWS = boost::make_shared<WorkspaceGroup>();
  for (int i = 0; i < wsGroup->getNumberOfEntries(); i++) {
    auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(wsGroup->getItem(i));
    if (ws) {
      MatrixWorkspace_sptr result;
      IAlgorithm_sptr dtc = createChildAlgorithm("ApplyDeadTimeCorr");
      dtc->setProperty("InputWorkspace", ws);
      dtc->setProperty("DeadTimeTable", dt);
      dtc->execute();
      result = dtc->getProperty("OutputWorkspace");
      outWS->addWorkspace(result);
    }
  }
  return outWS;
}

/**
 * Applies offset, crops and rebin the workspaces in the group according to
 * specified params.
 * @param wsGroup :: Workspaces to correct
 * @param loadedTimeZero :: Time zero of the data, so we can calculate the
 * offset
 * @return Corrected workspaces
 */
WorkspaceGroup_sptr MuonLoad::correctWorkspaces(WorkspaceGroup_sptr wsGroup,
                                                double loadedTimeZero) {
  WorkspaceGroup_sptr outWS = boost::make_shared<WorkspaceGroup>();
  for (int i = 0; i < wsGroup->getNumberOfEntries(); i++) {
    auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(wsGroup->getItem(i));
    if (ws) {
      MatrixWorkspace_sptr result;
      result = correctWorkspace(ws, loadedTimeZero);
      outWS->addWorkspace(result);
    }
  }
  return outWS;
}

/**
 * Applies offset, crops and rebin the workspace according to specified params.
 * @param ws :: Workspace to correct
 * @param loadedTimeZero :: Time zero of the data, so we can calculate the
 * offset
 * @return Corrected workspace
 */
MatrixWorkspace_sptr MuonLoad::correctWorkspace(MatrixWorkspace_sptr ws,
                                                double loadedTimeZero) {
  // Offset workspace, if need to
  double timeZero = getProperty("TimeZero");
  if (timeZero != EMPTY_DBL()) {
    double offset = loadedTimeZero - timeZero;

    IAlgorithm_sptr changeOffset = createChildAlgorithm("ChangeBinOffset");
    changeOffset->setProperty("InputWorkspace", ws);
    changeOffset->setProperty("Offset", offset);
    changeOffset->execute();

    ws = changeOffset->getProperty("OutputWorkspace");
  }

  // Crop workspace, if need to
  double Xmin = getProperty("Xmin");
  double Xmax = getProperty("Xmax");
  if (Xmin != EMPTY_DBL() || Xmax != EMPTY_DBL()) {
    IAlgorithm_sptr crop = createChildAlgorithm("CropWorkspace");
    crop->setProperty("InputWorkspace", ws);

    if (Xmin != EMPTY_DBL())
      crop->setProperty("Xmin", Xmin);

    if (Xmax != EMPTY_DBL())
      crop->setProperty("Xmax", Xmax);

    crop->execute();

    ws = crop->getProperty("OutputWorkspace");
  }

  // Rebin workspace if need to
  std::vector<double> rebinParams = getProperty("RebinParams");
  if (!rebinParams.empty()) {
    IAlgorithm_sptr rebin = createChildAlgorithm("Rebin");
    rebin->setProperty("InputWorkspace", ws);
    rebin->setProperty("Params", rebinParams);
    rebin->setProperty("FullBinsOnly", true);
    rebin->execute();

    ws = rebin->getProperty("OutputWorkspace");
  }

  return ws;
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
