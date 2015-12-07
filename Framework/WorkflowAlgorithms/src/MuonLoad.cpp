#include "MantidWorkflowAlgorithms/MuonLoad.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/MandatoryValidator.h"

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
  declareProperty(new FileProperty("Filename", "", FileProperty::Load, ".nxs"),
                  "The name of the Nexus file to load");

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

  declareProperty(
      "ApplyDeadTimeCorrection", false,
      "Whether dead time correction should be applied to loaded workspace");
  declareProperty(
      new WorkspaceProperty<TableWorkspace>(
          "CustomDeadTimeTable", "", Direction::Input, PropertyMode::Optional),
      "Table with dead time information. See LoadMuonNexus for format expected."
      "If not specified -- algorithm tries to use dead times stored in the "
      "data file.");
  declareProperty(new WorkspaceProperty<TableWorkspace>("DetectorGroupingTable",
                                                        "", Direction::Input,
                                                        PropertyMode::Optional),
                  "Table with detector grouping information. See LoadMuonNexus "
                  "for format expected. "
                  "If not specified -- algorithm tries to get grouping "
                  "information from the data file.");

  declareProperty("TimeZero", EMPTY_DBL(),
                  "Value used for Time Zero correction.");
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

  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/**
 * Execute the algorithm.
 */
void MuonLoad::exec() {
  Progress progress(this, 0, 1, 6);

  const std::string filename = getProperty("Filename");

  // Whether Dead Time Correction should be applied
  bool applyDtc = getProperty("ApplyDeadTimeCorrection");

  // If DetectorGropingTable not specified - use auto-grouping
  bool autoGroup =
      !static_cast<TableWorkspace_sptr>(getProperty("DetectorGroupingTable"));

  // Load the file
  IAlgorithm_sptr load = createChildAlgorithm("LoadMuonNexus");
  load->setProperty("Filename", filename);

  if (applyDtc) // Load dead times as well, if needed
    load->setProperty("DeadTimeTable", "__NotUsed");

  if (autoGroup) // Load grouping as well, if needed
    load->setProperty("DetectorGroupingTable", "__NotUsed");

  progress.report();
  load->execute();

  Workspace_sptr loadedWS = load->getProperty("OutputWorkspace");

  std::vector<int> summedPeriods = getProperty("SummedPeriodSet");
  std::vector<int> subtractedPeriods = getProperty("SubtractedPeriodSet");
  WorkspaceGroup_sptr allPeriodsWS = boost::make_shared<WorkspaceGroup>();
  progress.report();

  // Deal with single-period workspace
  if (auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(loadedWS)) {
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
  else if (auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(loadedWS)) {
    allPeriodsWS = group;
  }
  // Unexpected workspace type
  else {
    throw std::runtime_error("Loaded workspace is of invalid type");
  }

  progress.report();
  // Deal with dead time correction (if required)
  if (applyDtc) {
    TableWorkspace_sptr deadTimes = getProperty("CustomDeadTimeTable");

    if (!deadTimes) {
      // If no dead times specified - try to use ones from the file
      Workspace_sptr loadedDeadTimes = load->getProperty("DeadTimeTable");

      if (auto table =
              boost::dynamic_pointer_cast<TableWorkspace>(loadedDeadTimes)) {
        deadTimes = table;
      } else if (auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(
                     loadedDeadTimes)) {
        // XXX: using first table only for now. Can use the one for appropriate
        // period if necessary.
        deadTimes =
            boost::dynamic_pointer_cast<TableWorkspace>(group->getItem(0));
      }

      if (!deadTimes)
        throw std::runtime_error("File doesn't contain any dead times");
    }

    allPeriodsWS = applyDTC(allPeriodsWS, deadTimes);
  }

  TableWorkspace_sptr grouping;

  progress.report();
  if (autoGroup) {
    Workspace_sptr loadedGrouping = load->getProperty("DetectorGroupingTable");

    if (auto table =
            boost::dynamic_pointer_cast<TableWorkspace>(loadedGrouping)) {
      grouping = table;
    } else if (auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(
                   loadedGrouping)) {
      // XXX: using first table only for now. Can use the one for appropriate
      // period if necessary.
      grouping = boost::dynamic_pointer_cast<TableWorkspace>(group->getItem(0));
    }

    if (!grouping)
      throw std::runtime_error("File doesn't contain grouping information");
  } else {
    grouping = getProperty("DetectorGroupingTable");
  }

  progress.report();

  // Deal with grouping
  allPeriodsWS = groupWorkspaces(allPeriodsWS, grouping);

  // Correct bin values
  double loadedTimeZero = load->getProperty("TimeZero");
  allPeriodsWS = correctWorkspaces(allPeriodsWS, loadedTimeZero);

  IAlgorithm_sptr calcAssym = createChildAlgorithm("MuonCalculateAsymmetry");

  calcAssym->setProperty("InputWorkspace", allPeriodsWS);

  // Copy similar properties over
  calcAssym->setProperty("SummedPeriodSet", summedPeriods);
  calcAssym->setProperty("SubtractedPeriodSet", subtractedPeriods);
  calcAssym->setProperty("OutputType",
                         static_cast<std::string>(getProperty("OutputType")));
  calcAssym->setProperty("PairFirstIndex",
                         static_cast<int>(getProperty("PairFirstIndex")));
  calcAssym->setProperty("PairSecondIndex",
                         static_cast<int>(getProperty("PairSecondIndex")));
  calcAssym->setProperty("Alpha", static_cast<double>(getProperty("Alpha")));
  calcAssym->setProperty("GroupIndex",
                         static_cast<int>(getProperty("GroupIndex")));

  progress.report();

  calcAssym->execute();

  MatrixWorkspace_sptr outWS = calcAssym->getProperty("OutputWorkspace");
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
