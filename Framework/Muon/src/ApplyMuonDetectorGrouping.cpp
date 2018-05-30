#include "MantidMuon/ApplyMuonDetectorGrouping.h"
#include "MantidMuon/MuonAlgorithmHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"

#include <algorithm>
#include <ctype.h>
#include <string>
#include <vector>

const std::vector<std::string> g_analysisTypes = {"Counts", "Asymmetry"};

namespace {

// Convert input string plot type to PlotType.
Mantid::Muon::PlotType getPlotType(const std::string &plotType) {
  if (plotType == "Counts") {
    return Mantid::Muon::PlotType::Counts;
  } else if (plotType == "Asymmetry") {
    return Mantid::Muon::PlotType::Asymmetry;
  } else {
    // default to Counts.
    return Mantid::Muon::PlotType::Counts;
  }
}

/**
 * Convert the input workspace into a workspace group if e.g. it has only a
 * single period otherwise leave it alone.
 */
Mantid::API::WorkspaceGroup_sptr
convertInputWStoWSGroup(Mantid::API::Workspace_sptr inputWS) {

  // Cast input WS to a WorkspaceGroup
  auto muonWS = boost::make_shared<Mantid::API::WorkspaceGroup>();
  if (auto test =
          boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(inputWS)) {
    muonWS->addWorkspace(test);
  } else {
    muonWS = boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(inputWS);
  }
  return muonWS;
}

} // namespace

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::MuonAlgorithmHelper;

namespace Mantid {
namespace Muon {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ApplyMuonDetectorGrouping)

void ApplyMuonDetectorGrouping::init() {

  std::string emptyString("");

  declareProperty(Mantid::Kernel::make_unique<WorkspaceProperty<Workspace>>(
                      "InputWorkspace", emptyString, Direction::Input,
                      PropertyMode::Mandatory),
                  "Input workspace containing data from detectors which are to "
                  "be grouped.");

  declareProperty(
      Mantid::Kernel::make_unique<WorkspaceProperty<WorkspaceGroup>>(
          "InputWorkspaceGroup", emptyString, Direction::InOut,
          PropertyMode::Mandatory),
      "The workspace group to which the output will be added.");

  declareProperty("GroupName", emptyString, "The name of the group. Must "
                                            "contain at least one alphanumeric "
                                            "character.",
                  Direction::Input);
  declareProperty("Grouping", std::to_string(1),
                  "The grouping of detectors, comma separated list of detector "
                  "IDs or hyphenated ranges of IDs.",
                  Direction::Input);

  declareProperty(
      "AnalysisType", "Counts",
      boost::make_shared<Kernel::ListValidator<std::string>>(g_analysisTypes),
      "The type of analysis to perform on the spectra.", Direction::Input);

  declareProperty(
      "TimeMin", 0.1,
      "Start time for the data in ms. Only used with the asymmetry analysis.",
      Direction::Input);
  setPropertySettings("TimeMin",
                      make_unique<Kernel::EnabledWhenProperty>(
                          "AnalysisType", Kernel::IS_EQUAL_TO, "Asymmetry"));

  declareProperty(
      "TimeMax", 32.0,
      "End time for the data in ms. Only used with the asymmetry analysis.",
      Direction::Input);
  setPropertySettings("TimeMax",
                      make_unique<Kernel::EnabledWhenProperty>(
                          "AnalysisType", Kernel::IS_EQUAL_TO, "Asymmetry"));

  declareProperty("RebinArgs", emptyString,
                  "Rebin arguments. No rebinning if left empty.",
                  Direction::Input);

  declareProperty("TimeOffset", 0.0,
                  "Shift the times of all data by a fixed amount. The value "
                  "given corresponds to the bin that will become 0.0 seconds.",
                  Direction::Input);

  declareProperty("SummedPeriods", std::to_string(1),
                  "A list of periods to sum in multiperiod data.",
                  Direction::Input);
  declareProperty("SubtractedPeriods", emptyString,
                  "A list of periods to subtract in multiperiod data.",
                  Direction::Input);

  declareProperty(
      "ApplyDeadTimeCorrection", false,
      "Whether dead time correction should be applied to input workspace");
  declareProperty(
      make_unique<WorkspaceProperty<TableWorkspace>>(
          "DeadTimeTable", "", Direction::Input, PropertyMode::Optional),
      "Table with dead time information. Must be specified if "
      "ApplyDeadTimeCorrection is set true.");
  setPropertySettings("DeadTimeTable", make_unique<Kernel::EnabledWhenProperty>(
                                           "ApplyDeadTimeCorrection",
                                           Kernel::IS_NOT_DEFAULT, ""));

  // Perform Group Associations.

  std::string workspaceGrp("Workspaces");
  setPropertyGroup("InputWorkspace", workspaceGrp);
  setPropertyGroup("InputWorkspaceGroup", workspaceGrp);

  std::string groupingGrp("Grouping Information");
  setPropertyGroup("GroupName", groupingGrp);
  setPropertyGroup("Grouping", groupingGrp);

  std::string analysisGrp("Analysis");
  setPropertyGroup("AnalysisType", analysisGrp);
  setPropertyGroup("TimeMin", analysisGrp);
  setPropertyGroup("TimeMax", analysisGrp);

  std::string dtcGrp("Dead Time Correction");
  setPropertyGroup("ApplyDeadTimeCorrection", dtcGrp);
  setPropertyGroup("DeadTimeTable", dtcGrp);
}

void ApplyMuonDetectorGrouping::exec() {

  WorkspaceGroup_sptr groupWS = getProperty("InputWorkspaceGroup");
  Workspace_sptr inputWS = getProperty("InputWorkspace");

  auto options = getUserInput();
  std::string groupedWSName = groupWS->getName();

  WorkspaceGroup_sptr muonWS = convertInputWStoWSGroup(inputWS);
  clipXRangeToWorkspace(*muonWS, options);

  addCountsToMuonAnalysisGrouped(inputWS, options);

  auto ws = createAnalysisWorkspace(inputWS, false, options);
  auto wsRaw = createAnalysisWorkspace(inputWS, true, options);

  const std::string wsName = getNewWorkspaceName(options, groupedWSName);
  const std::string wsRawName = wsName + "_Raw";

  AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();
  ads.addOrReplace(wsName, ws);
  ads.addOrReplace(wsRawName, wsRaw);

  std::vector<std::string> wsNames = {wsName, wsRawName};
  MuonAlgorithmHelper::groupWorkspaces(groupedWSName, wsNames);
}

/*
 * Add counts data to the workspace names "MuonAnalysisGrouped" which
 * is a WorkspaceGroup
 */
void ApplyMuonDetectorGrouping::addCountsToMuonAnalysisGrouped(
    Workspace_sptr inputWS, Muon::AnalysisOptions options) {

  options.plotType = Mantid::Muon::PlotType::Counts;
  options.rebinArgs = "";

  IAlgorithm_sptr alg = Algorithm::createChildAlgorithm("MuonProcess");
  setMuonProcessPeriodProperties(*alg, inputWS, options);
  setMuonProcessAlgorithmProperties(*alg, options);
  alg->execute();
  Workspace_sptr wsTemp = alg->getProperty("OutputWorkspace");
  MatrixWorkspace_sptr wsOut =
      boost::dynamic_pointer_cast<MatrixWorkspace>(wsTemp);
  AnalysisDataService::Instance().addOrReplace("wsTemp", wsOut);

  std::vector<std::string> names = getMuonAnalysisGroupedWorkspaceNames();

  for (auto &name : names) {
    IAlgorithm_sptr algAppend =
        Algorithm::createChildAlgorithm("AppendSpectra");
    // auto ws = AnalysisDataService::Instance().retrieve(name);
    // auto wsAppended = boost::make_shared<MatrixWorkspace>();
    algAppend->setPropertyValue("InputWorkspace1", name);
    algAppend->setPropertyValue("InputWorkspace2", wsOut->getName());
    // alg->setPropertyValue("OutputWorkspace", wsAppended->getName());

    algAppend->execute();
    MatrixWorkspace_sptr wsAppended = algAppend->getProperty("OutputWorkspace");

    if (AnalysisDataService::Instance().doesExist(name)) {
      IAlgorithm_sptr deleteAlg =
          AlgorithmManager::Instance().create("DeleteWorkspace");
      deleteAlg->setLogging(false);
      deleteAlg->setPropertyValue("Workspace", name);
      deleteAlg->execute();
    }
    AnalysisDataService::Instance().add(name, wsAppended);
  }

  deleteWorkspaceIfExists(wsOut->getName());

  auto wsGroup = boost::make_shared<WorkspaceGroup>();
  AnalysisDataService::Instance().addOrReplace("MuonAnalysisGrouped", wsGroup);
  MuonAlgorithmHelper::groupWorkspaces("MuonAnalysisGrouped", names);
}

void ApplyMuonDetectorGrouping::deleteWorkspaceIfExists(
    const std::string &name) {
  if (AnalysisDataService::Instance().doesExist(name)) {
    IAlgorithm_sptr deleteAlg =
        AlgorithmManager::Instance().create("DeleteWorkspace");
    deleteAlg->setLogging(false);
    deleteAlg->setPropertyValue("Workspace", name);
    deleteAlg->execute();
  }
}

std::vector<std::string>
ApplyMuonDetectorGrouping::getMuonAnalysisGroupedWorkspaceNames() {

  std::vector<std::string> names;
  names.clear();

  auto &ads = AnalysisDataService::Instance();

  if (!ads.doesExist("MuonAnalysisGrouped")) {
    g_log.notice("MuonAnalysisGrouped workspace not found in ADS.");
    return names;
  }

  auto muonGroupedWS = ads.retrieve("MuonAnalysisGrouped");

  if (auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(muonGroupedWS)) {
    names.emplace_back(ws->getName());
    return names;
  }
  // if WorkspaceGroup
  else {
    auto wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(muonGroupedWS);
    return wsGroup->getNames();
  }
}

/*
 * Generate the name of the new workspace
 */
const std::string ApplyMuonDetectorGrouping::getNewWorkspaceName(
    const Muon::AnalysisOptions &options, const std::string &groupWSName) {

  Muon::DatasetParams params;
  // don't fill in instrument, runs, periods; not required.
  params.label = groupWSName;
  params.itemType = Muon::ItemType::Group;
  params.itemName = options.groupPairName;
  params.plotType = options.plotType;
  params.version = 1;
  const std::string wsName = generateWorkspaceName(params);
  return wsName;
}

/*
 * Store the input properties in options
 */
Muon::AnalysisOptions ApplyMuonDetectorGrouping::getUserInput() {
  Muon::AnalysisOptions options;

  Grouping grouping;
  grouping.description = "no description";
  grouping.groupNames.emplace_back(this->getPropertyValue("GroupName"));
  grouping.groups.emplace_back(this->getPropertyValue("Grouping"));

  options.grouping = grouping;
  options.summedPeriods = this->getPropertyValue("SummedPeriods");
  options.subtractedPeriods = this->getPropertyValue("SubtractedPeriods");
  options.timeZero = 0.0;
  options.loadedTimeZero = this->getProperty("TimeOffset");
  options.timeLimits.first = this->getProperty("TimeMin");
  options.timeLimits.second = this->getProperty("TimeMax");
  options.rebinArgs = this->getPropertyValue("rebinArgs");
  options.plotType = getPlotType(this->getPropertyValue("AnalysisType"));
  options.groupPairName = this->getPropertyValue("GroupName");

  return options;
}

/**
 * Clip Xmin/Xmax to the range in the first histogram of the input WS group.
 */
void ApplyMuonDetectorGrouping::clipXRangeToWorkspace(
    const WorkspaceGroup &ws, Muon::AnalysisOptions &options) {

  MatrixWorkspace_sptr clipWS;
  clipWS = boost::dynamic_pointer_cast<MatrixWorkspace>(ws.getItem(0));
  double dataXMin;
  double dataXMax;
  clipWS->getXMinMax(dataXMin, dataXMax);

  if (options.timeLimits.first < dataXMin) {
    const std::string message("Requested TimeMin outside of data range.");
    g_log.notice(message);
    options.timeLimits.first = dataXMin;
  }
  if (options.timeLimits.second > dataXMax) {
    const std::string message("Requested TimeMax outside of data range.");
    g_log.notice(message);
    options.timeLimits.second = dataXMax;
  }
}

/**
 * Creates workspace, processing the data using the MuonProcess algorithm.
 */
Workspace_sptr ApplyMuonDetectorGrouping::createAnalysisWorkspace(
    Workspace_sptr inputWS, bool noRebin, Muon::AnalysisOptions &options) {

  IAlgorithm_sptr alg =
      AlgorithmManager::Instance().createUnmanaged("MuonProcess");

  alg->initialize();
  setMuonProcessPeriodProperties(*alg, inputWS, options);
  setMuonProcessAlgorithmProperties(*alg, options);
  alg->setChild(true);
  alg->setPropertyValue("OutputWorkspace", "__NotUsed__");

  if (noRebin) {
    options.rebinArgs = "";
  }

  alg->execute();
  return alg->getProperty("OutputWorkspace");
}

/**
 * Set algorithm properties (input workspace, and period properties) according
 * to the given options. For use with MuonProcess.
 */
void ApplyMuonDetectorGrouping::setMuonProcessPeriodProperties(
    IAlgorithm &alg, Workspace_sptr inputWS,
    const Muon::AnalysisOptions &options) const {

  auto inputGroup = boost::make_shared<WorkspaceGroup>();
  // If is a group, will need to handle periods
  if (auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(inputWS)) {
    for (int i = 0; i < group->getNumberOfEntries(); i++) {
      auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(i));
      inputGroup->addWorkspace(ws);
    }
    alg.setProperty("SummedPeriodSet", options.summedPeriods);
    alg.setProperty("SubtractedPeriodSet", options.subtractedPeriods);
  } else if (auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(inputWS)) {
    // Put this single WS into a group and set it as the input property
    inputGroup->addWorkspace(ws);
    alg.setProperty("SummedPeriodSet", "1");
  } else {
    throw std::runtime_error("Cannot create workspace: workspace must be "
                             "MatrixWorkspace or WorkspaceGroup.");
  }
  alg.setProperty("InputWorkspace", inputGroup);
}

/**
 * Set time properties according to the given options. For use with
 * MuonProcess.
 */
void ApplyMuonDetectorGrouping::setMuonProcessAlgorithmTimeProperties(
    IAlgorithm &alg, const Muon::AnalysisOptions &options) const {
  alg.setProperty("TimeZero", options.timeZero);
  alg.setProperty("LoadedTimeZero", options.loadedTimeZero);
  alg.setProperty("Xmin", options.timeLimits.first);
  double Xmax = options.timeLimits.second;
  if (Xmax != Mantid::EMPTY_DBL()) {
    alg.setProperty("Xmax", Xmax);
  }

  bool applyDTC = getProperty("ApplyDeadTimeCorrection");
  if (applyDTC) {
    TableWorkspace_sptr DTC = getProperty("DeadTimeTable");
    alg.setProperty("ApplyDeadTimeCorrection", true);
    alg.setProperty("DeadTimeTable", DTC);
  }
}

// Set OutputType property of MuonProcess
void ApplyMuonDetectorGrouping::setMuonProcessAlgorithmOutputTypeProperty(
    IAlgorithm &alg, const Muon::AnalysisOptions &options) const {

  std::string outputType;
  switch (options.plotType) {
  case Muon::PlotType::Counts:
  case Muon::PlotType::Logarithm:
    outputType = "GroupCounts";
    break;
  case Muon::PlotType::Asymmetry:
    outputType = "GroupAsymmetry";
    break;
  default:
    throw std::invalid_argument(
        "Cannot create analysis workspace: Unsupported plot type");
  }
  alg.setProperty("OutputType", outputType);
}

// Set grouping properies of MuonProcess
void ApplyMuonDetectorGrouping::setMuonProcessAlgorithmGroupingProperties(
    IAlgorithm &alg, const Muon::AnalysisOptions &options) const {

  alg.setProperty("DetectorGroupingTable", options.grouping.toTable());
  alg.setProperty("GroupIndex", 0);
}

/**
 * Set algorithm properties according to the given options. For use with
 * MuonProcess.
 */
void ApplyMuonDetectorGrouping::setMuonProcessAlgorithmProperties(
    IAlgorithm &alg, const Muon::AnalysisOptions &options) const {

  alg.setProperty("Mode", "Combined");
  alg.setProperty("CropWorkspace", false);
  if (!options.rebinArgs.empty()) {
    alg.setProperty("RebinParams", options.rebinArgs);
  }
  setMuonProcessAlgorithmGroupingProperties(alg, options);
  setMuonProcessAlgorithmOutputTypeProperty(alg, options);
  setMuonProcessAlgorithmTimeProperties(alg, options);
}

/**
 * Performs validation of inputs to the algorithm.
 * - Checks the bounds on X axis are sensible
 * - Checks that the workspaceGroup is named differently to the workspace with
 * the data.
 * - Checks that a group name is entered.
 * @returns Map of parameter names to errors
 */
std::map<std::string, std::string> ApplyMuonDetectorGrouping::validateInputs() {
  std::map<std::string, std::string> errors;

  double tmin = this->getProperty("TimeMin");
  double tmax = this->getProperty("TimeMax");
  if (tmin > tmax) {
    errors["TimeMin"] = "TimeMin > TimeMax";
  }
  if (tmin < 0) {
    errors["TimeMin"] = "Time values are negative.";
  }
  if (tmax < 0) {
    errors["TimeMax"] = "Time values are negative.";
  }

  WorkspaceGroup_sptr groupedWS = getProperty("InputWorkspaceGroup");
  Workspace_sptr inputWS = getProperty("InputWorkspace");
  if (groupedWS->getName() == inputWS->getName()) {
    errors["InputWorkspaceGroup"] = "The InputWorkspaceGroup should not have "
                                    "the same name as InputWorkspace.";
  }

  const std::string groupName = getPropertyValue("GroupName");
  if (groupName.empty()) {
    errors["GroupName"] = "The group must be named.";
  }

  if (!std::any_of(std::begin(groupName), std::end(groupName), isalnum)) {
    errors["GroupName"] =
        "The group name must contain at least one alphnumeric character.";
  }

  return errors;
}

// Allow WorkspaceGroup property to function correctly.
bool ApplyMuonDetectorGrouping::checkGroups() { return false; }

} // namespace Muon
} // namespace Mantid
