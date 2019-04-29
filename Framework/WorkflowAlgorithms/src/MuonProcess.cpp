// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidWorkflowAlgorithms/MuonProcess.h"

#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
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
} // namespace

namespace Mantid {
namespace WorkflowAlgorithms {
using namespace Kernel;
using namespace API;
using namespace DataObjects;
using API::WorkspaceGroup_sptr;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MuonProcess)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string MuonProcess::name() const { return "MuonProcess"; }

/// Algorithm's version for identification. @see Algorithm::version
int MuonProcess::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string MuonProcess::category() const { return "Workflow\\Muon"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/*
 * Initialize the algorithm's properties.
 */
void MuonProcess::init() {
  declareProperty(
      make_unique<WorkspaceProperty<Workspace>>(
          "InputWorkspace", "", Direction::Input, PropertyMode::Mandatory),
      "Input workspace loaded from file (e.g. by LoadMuonNexus)");

  std::vector<std::string> allowedModes{"CorrectAndGroup", "Analyse",
                                        "Combined"};
  auto modeVal = boost::make_shared<CompositeValidator>();
  modeVal->add(boost::make_shared<StringListValidator>(allowedModes));
  modeVal->add(boost::make_shared<MandatoryValidator<std::string>>());
  declareProperty("Mode", "Combined", modeVal,
                  "Mode to run in. CorrectAndGroup applies dead time "
                  "correction and grouping; Analyse changes bin offset, "
                  "crops, rebins and calculates asymmetry; Combined does all "
                  "of the above.");

  declareProperty(
      make_unique<ArrayProperty<int>>("SummedPeriodSet", Direction::Input),
      "Comma-separated list of periods to be summed");

  declareProperty(
      make_unique<ArrayProperty<int>>("SubtractedPeriodSet", Direction::Input),
      "Comma-separated list of periods to be subtracted from the "
      "SummedPeriodSet");

  declareProperty(
      "ApplyDeadTimeCorrection", false,
      "Whether dead time correction should be applied to loaded workspace");
  declareProperty(
      make_unique<WorkspaceProperty<TableWorkspace>>(
          "DeadTimeTable", "", Direction::Input, PropertyMode::Optional),
      "Table with dead time information, e.g. from LoadMuonNexus."
      "Must be specified if ApplyDeadTimeCorrection is set true.");
  declareProperty(
      make_unique<WorkspaceProperty<TableWorkspace>>("DetectorGroupingTable",
                                                     "", Direction::Input,
                                                     PropertyMode::Optional),
      "Table with detector grouping information, e.g. from LoadMuonNexus.");

  declareProperty("TimeZero", EMPTY_DBL(),
                  "Value used for Time Zero correction");
  declareProperty("LoadedTimeZero", EMPTY_DBL(),
                  boost::make_shared<MandatoryValidator<double>>(),
                  "Time Zero value loaded from file, e.g. from LoadMuonNexus.");
  declareProperty(
      make_unique<ArrayProperty<double>>("RebinParams"),
      "Params used for rebinning. If empty - rebinning is not done.");
  declareProperty("Xmin", EMPTY_DBL(), "Minimal X value to include");
  declareProperty("Xmax", EMPTY_DBL(), "Maximal X value to include");

  std::vector<std::string> allowedTypes{"PairAsymmetry", "GroupAsymmetry",
                                        "GroupCounts"};
  declareProperty("OutputType", "PairAsymmetry",
                  boost::make_shared<StringListValidator>(allowedTypes),
                  "What kind of workspace required for analysis.");

  declareProperty("PairFirstIndex", EMPTY_INT(),
                  "Workspace index of the first pair group");
  declareProperty("PairSecondIndex", EMPTY_INT(),
                  "Workspace index of the second pair group");
  declareProperty("Alpha", 1.0, "Alpha value of the pair");

  declareProperty("GroupIndex", EMPTY_INT(), "Workspace index of the group");

  declareProperty(make_unique<WorkspaceProperty<Workspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");

  declareProperty("CropWorkspace", true,
                  "Determines if the input workspace "
                  "should be cropped at Xmax, Xmin is "
                  "still aplied.");

  declareProperty("WorkspaceName", "", "The name of the input workspace");
}

//----------------------------------------------------------------------------------------------
/**
 * Execute the algorithm.
 */
void MuonProcess::exec() {
  Progress progress(this, 0.0, 1.0, 5);

  // Supplied input workspace
  Workspace_sptr inputWS = getProperty("InputWorkspace");

  std::vector<int> summedPeriods = getProperty("SummedPeriodSet");
  std::vector<int> subtractedPeriods = getProperty("SubtractedPeriodSet");
  auto allPeriodsWS = boost::make_shared<WorkspaceGroup>();
  progress.report();

  // Deal with single-period workspace
  if (auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(inputWS)) {
    allPeriodsWS->addWorkspace(ws);
  }
  // Deal with multi-period workspace
  else if (auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(inputWS)) {
    allPeriodsWS = group;
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
    } else if (outputType == "GroupAsymmetry") {
      asymCalc = Mantid::Kernel::make_unique<MuonGroupAsymmetryCalculator>(
          allPeriodsWS, summedPeriods, subtractedPeriods, groupIndex,
          getProperty("Xmin"), getProperty("Xmax"),
          getProperty("WorkspaceName"));
    } else if (outputType == "PairAsymmetry") {
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
WorkspaceGroup_sptr MuonProcess::groupWorkspaces(WorkspaceGroup_sptr wsGroup,
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
WorkspaceGroup_sptr MuonProcess::applyDTC(WorkspaceGroup_sptr wsGroup,
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
      if (result) {
        outWS->addWorkspace(result);
      } else {
        throw std::runtime_error("ApplyDeadTimeCorr failed to apply dead time "
                                 "correction in MuonProcess");
      }
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
WorkspaceGroup_sptr MuonProcess::correctWorkspaces(WorkspaceGroup_sptr wsGroup,
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
MatrixWorkspace_sptr MuonProcess::correctWorkspace(MatrixWorkspace_sptr ws,
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
    bool toCrop = getProperty("CropWorkspace");
    if (toCrop) {
      if (Xmax != EMPTY_DBL())
        crop->setProperty("Xmax", Xmax);
    }
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

/**
 * Performs validation of inputs to the algorithm.
 * - Input workspace must be single-period or a workspace group
 * - Single-period input must only use period 1
 * - Supplied period numbers must all be valid (between 1 and total number of
 * periods)
 * - If analysis will take place, SummedPeriodSet is mandatory
 * - If ApplyDeadTimeCorrection is true, DeadTimeTable is mandatory
 * @returns Map of parameter names to errors
 */
std::map<std::string, std::string> MuonProcess::validateInputs() {
  std::map<std::string, std::string> errors;

  // Supplied input workspace and sets of periods
  const std::string propInputWS("InputWorkspace"),
      propSummedPeriodSet("SummedPeriodSet"),
      propSubtractedPeriodSet("SubtractedPeriodSet");
  Workspace_sptr inputWS = getProperty(propInputWS);
  std::vector<int> summedPeriods = getProperty(propSummedPeriodSet);
  std::vector<int> subtractedPeriods = getProperty(propSubtractedPeriodSet);

  // If single-period data, test the sets of periods specified
  if (auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(inputWS)) {
    if (std::find_if_not(summedPeriods.begin(), summedPeriods.end(), isOne) !=
        summedPeriods.end()) {
      errors[propSummedPeriodSet] = "Single period data but set of periods to "
                                    "sum contains invalid values.";
    }
    if (!subtractedPeriods.empty()) {
      errors[propSubtractedPeriodSet] =
          "Single period data but second set of periods specified";
    }
  } else {
    // If not a MatrixWorkspace, must be a multi-period WorkspaceGroup
    auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(inputWS);
    if (group == nullptr) {
      errors[propInputWS] = "Input workspace is of invalid type";
    } else {
      auto numPeriods = group->getNumberOfEntries();
      if (numPeriods < 1) {
        errors[propInputWS] = "Input workspace contains no periods";
      }
      // check summed period numbers
      std::vector<int> invalidPeriods;
      std::copy_if(summedPeriods.cbegin(), summedPeriods.cend(),
                   std::back_inserter(invalidPeriods),
                   [numPeriods](auto period) {
                     return period < 1 || period > numPeriods;
                   });
      if (!invalidPeriods.empty()) {
        errors[propSummedPeriodSet] = buildErrorString(invalidPeriods);
        invalidPeriods.clear();
      }
      // check subtracted period numbers
      std::copy_if(subtractedPeriods.cbegin(), subtractedPeriods.cend(),
                   std::back_inserter(invalidPeriods),
                   [numPeriods](auto period) {
                     return period < 1 || period > numPeriods;
                   });
      if (!invalidPeriods.empty()) {
        errors[propSubtractedPeriodSet] = buildErrorString(invalidPeriods);
      }
    }
  }

  // Some parameters can be mandatory or not depending on the mode
  const std::string propMode("Mode"), propApplyDTC("ApplyDeadTimeCorrection"),
      propDeadTime("DeadTimeTable"), propDetGroup("DetectorGroupingTable");
  const std::string mode = getProperty(propMode);
  // If analysis will take place, SummedPeriodSet is mandatory
  if (mode != "CorrectAndGroup") {
    if (summedPeriods.empty()) {
      errors[propSummedPeriodSet] =
          "Cannot analyse: list of periods to sum was empty";
    }
  }
  // If correcting/grouping will take place, DetectorGroupingTable is mandatory
  if (mode != "Analyse") {
    TableWorkspace_sptr grouping = getProperty(propDetGroup);
    if (grouping == nullptr) {
      errors[propDetGroup] = "No detector grouping table supplied";
    }
  }
  // If dead time correction is to be applied, must supply dead times
  bool applyDtc = getProperty(propApplyDTC);
  if (applyDtc) {
    TableWorkspace_sptr deadTimes = getProperty(propDeadTime);
    if (!deadTimes) {
      errors[propDeadTime] =
          "Cannot apply dead time correction as no dead times were supplied";
    }
  }

  return errors;
}

/**
 * Builds an error message from the supplied parameters.
 * @param invalidPeriods :: [input] Vector containing invalid periods
 * @returns An error message
 */
std::string
MuonProcess::buildErrorString(const std::vector<int> &invalidPeriods) const {
  std::stringstream message;
  message << "Invalid periods specified: ";
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
