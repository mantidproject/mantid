// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMuon/ApplyMuonDetectorGroupPairing.h"
#include "MantidMuon/MuonAlgorithmHelper.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/HistoWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/VisibleWhenProperty.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

const std::vector<std::string> g_analysisTypes = {"Counts", "Asymmetry"};

namespace {

// Take a string of ints and ranges (e.g. "2,5,3-4,5,1") and return
// an ordered set of unique elements "1,2,3,4,5"
std::set<int>
parseGroupStringToSetOfUniqueElements(const std::string &groupString) {
  std::vector<int> groupVec = Mantid::Kernel::Strings::parseRange(groupString);
  std::set<int> groupSet(groupVec.begin(), groupVec.end());
  return groupSet;
}

} // namespace

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::MuonAlgorithmHelper;

namespace Mantid {
namespace Muon {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ApplyMuonDetectorGroupPairing)

void ApplyMuonDetectorGroupPairing::init() {
  std::string emptyString("");

  declareProperty(
      std::make_unique<WorkspaceProperty<WorkspaceGroup>>(
          "InputWorkspaceGroup", emptyString, Direction::InOut,
          PropertyMode::Mandatory),
      "The workspace group to which the output will be added.");

  declareProperty("PairName", emptyString,
                  "The name of the pair. Must "
                  "contain at least one alphanumeric "
                  "character.",
                  Direction::Input);

  declareProperty("Alpha", 1.0,
                  "Alpha parameter used in the asymmetry calculation.",
                  Direction::Input);

  declareProperty("SpecifyGroupsManually", false,
                  "Specify the pair of groups manually using the raw data and "
                  "various optional parameters.");

  // Select groups via workspaces

  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "InputWorkspace1", emptyString, Direction::Input,
          PropertyMode::Optional),
      "Input workspace containing data from grouped detectors.");

  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "InputWorkspace2", emptyString, Direction::Input,
          PropertyMode::Optional),
      "Input workspace containing data from grouped detectors.");

  setPropertySettings("InputWorkspace1",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "0"));
  setPropertySettings("InputWorkspace2",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "0"));

  // Specify groups manually

  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
                      "InputWorkspace", emptyString, Direction::Input,
                      PropertyMode::Optional),
                  "Input workspace containing data from detectors which are to "
                  "be grouped.");
  setPropertySettings("InputWorkspace",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "1"));

  declareProperty("Group1", std::to_string(1),
                  "The grouping of detectors, comma separated list of detector "
                  "IDs or hyphenated ranges of IDs.",
                  Direction::Input);
  declareProperty("Group2", std::to_string(1),
                  "The grouping of detectors, comma separated list of detector "
                  "IDs or hyphenated ranges of IDs.",
                  Direction::Input);
  setPropertySettings("Group1",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "1"));
  setPropertySettings("Group2",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "1"));

  declareProperty("TimeMin", 0.1,
                  "Start time for the data in micro seconds. Only used with "
                  "the asymmetry analysis.",
                  Direction::Input);
  setPropertySettings("TimeMin",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "1"));

  declareProperty("TimeMax", 32.0,
                  "End time for the data in micro seconds. Only used with the "
                  "asymmetry analysis.",
                  Direction::Input);
  setPropertySettings("TimeMax",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "1"));

  declareProperty("RebinArgs", emptyString,
                  "Rebin arguments. No rebinning if left empty.",
                  Direction::Input);
  setPropertySettings("RebinArgs",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "1"));

  declareProperty("TimeOffset", 0.0,
                  "Shift the times of all data by a fixed amount. The value "
                  "given corresponds to the bin that will become time 0.0.",
                  Direction::Input);
  setPropertySettings("TimeOffset",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "1"));

  declareProperty("SummedPeriods", std::to_string(1),
                  "A list of periods to sum in multiperiod data.",
                  Direction::Input);
  setPropertySettings("SummedPeriods",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "1"));

  declareProperty("SubtractedPeriods", emptyString,
                  "A list of periods to subtract in multiperiod data.",
                  Direction::Input);
  setPropertySettings("SubtractedPeriods",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "1"));

  declareProperty(
      "ApplyDeadTimeCorrection", false,
      "Whether dead time correction should be applied to input workspace");
  setPropertySettings("ApplyDeadTimeCorrection",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "1"));

  declareProperty(
      std::make_unique<WorkspaceProperty<TableWorkspace>>(
          "DeadTimeTable", "", Direction::Input, PropertyMode::Optional),
      "Table with dead time information. Must be specified if "
      "ApplyDeadTimeCorrection is set true.");
  setPropertySettings("DeadTimeTable",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "ApplyDeadTimeCorrection", Kernel::IS_EQUAL_TO, "1"));

  // Perform Group Associations.

  std::string workspaceGrp("Specify Group Workspaces");
  setPropertyGroup("InputWorkspace1", workspaceGrp);
  setPropertyGroup("InputWorkspace2", workspaceGrp);

  std::string manualGroupGrp("Specify Detector ID Groups Manually");
  setPropertyGroup("InputWorkspace", manualGroupGrp);
  setPropertyGroup("Group1", manualGroupGrp);
  setPropertyGroup("Group2", manualGroupGrp);
  setPropertyGroup("TimeMin", manualGroupGrp);
  setPropertyGroup("TimeMax", manualGroupGrp);
  setPropertyGroup("RebinArgs", manualGroupGrp);
  setPropertyGroup("TimeOffset", manualGroupGrp);
  setPropertyGroup("SummedPeriods", manualGroupGrp);
  setPropertyGroup("SubtractedPeriods", manualGroupGrp);
  setPropertyGroup("ApplyDeadTimeCorrection", manualGroupGrp);
  setPropertyGroup("DeadTimeTable", manualGroupGrp);
}

/**
 * Performs validation of inputs to the algorithm.
 * - Checks Alpha > 0
 * - Checks TMin > TMax
 * - Checks the workspace and workspaceGroup are different
 * - Check PairName is given and contains at least one alnum character
 * - Check the two groups are different
 * @returns Map of parameter names to errors
 */
std::map<std::string, std::string>
ApplyMuonDetectorGroupPairing::validateInputs() {
  std::map<std::string, std::string> errors;

  double alpha = this->getProperty("Alpha");
  if (alpha <= 0.0) {
    errors["Alpha"] = "Alpha must be greater than 0.";
  }

  const std::string pairName = getPropertyValue("PairName");
  if (pairName.empty()) {
    errors["PairName"] = "The pair must be named.";
  }
  if (!std::all_of(std::begin(pairName), std::end(pairName), isalnum)) {
    errors["PairName"] = "PairName must contain only alphnumeric characters.";
  }

  if (getProperty("SpecifyGroupsManually")) {

    double tmin = this->getProperty("TimeMin");
    double tmax = this->getProperty("TimeMax");
    if (tmin > tmax) {
      errors["TimeMin"] = "TimeMin > TimeMax";
    }

    WorkspaceGroup_sptr groupedWS = getProperty("InputWorkspaceGroup");
    Workspace_sptr inputWS = getProperty("InputWorkspace");

    if (groupedWS->getName() == inputWS->getName()) {
      errors["InputWorkspaceGroup"] = "The InputWorkspaceGroup should not have "
                                      "the same name as InputWorkspace.";
    }

    std::set<int> group1 =
        parseGroupStringToSetOfUniqueElements(this->getProperty("Group1"));
    std::set<int> group2 =
        parseGroupStringToSetOfUniqueElements(this->getProperty("Group2"));
    if (group1.size() > 0 && group1 == group2) {
      errors["Group1"] = "The two groups must contain at least one ID and be "
                         "different.";
    }
  } else {
    MatrixWorkspace_sptr ws1 = getProperty("InputWorkspace1");
    MatrixWorkspace_sptr ws2 = getProperty("InputWorkspace2");
    if (ws1 && ws1->getNumberHistograms() != 1) {
      errors["InputWorkspace1"] =
          "The input workspaces should have exactly one spectra";
    }
    if (ws2 && ws2->getNumberHistograms() != 1) {
      errors["InputWorkspace2"] =
          "The input workspaces should have exactly one spectra";
    }
  }

  // Multi period checks are left for MuonProcess

  return errors;
}

void ApplyMuonDetectorGroupPairing::exec() {
  // Allows exceptions from MuonProcess validator to be also thrown from this
  // algorithm
  this->setRethrows(true);

  const double alpha = static_cast<double>(getProperty("Alpha"));

  WorkspaceGroup_sptr groupedWS = getProperty("InputWorkspaceGroup");
  std::string groupedWSName = groupedWS->getName();
  std::string pairName = getProperty("PairName");

  std::string pairWSName = getPairWorkspaceName(pairName, groupedWSName);
  std::string pairWSNameNoRebin = pairWSName + "_Raw";

  MatrixWorkspace_sptr pairWS, pairWSNoRebin;
  if (getProperty("SpecifyGroupsManually")) {

    Workspace_sptr inputWS = getProperty("InputWorkspace");
    pairWS = createPairWorkspaceManually(inputWS, false);
    pairWSNoRebin = createPairWorkspaceManually(inputWS, true);
    // Rebinning only supported for manually entered groups
    AnalysisDataService::Instance().addOrReplace(pairWSName, pairWS);
    groupedWS->add(pairWSName);

  } else {
    MatrixWorkspace_sptr ws1 = getProperty("InputWorkspace1");
    MatrixWorkspace_sptr ws2 = getProperty("InputWorkspace2");
    if (MuonAlgorithmHelper::checkValidPair(ws1->getName(), ws2->getName())) {
      pairWSNoRebin = createPairWorkspaceFromGroupWorkspaces(ws1, ws2, alpha);
    } else {
      throw std::invalid_argument(
          "Input workspaces are not compatible for pair asymmetry.");
    }
  }

  AnalysisDataService::Instance().addOrReplace(pairWSNameNoRebin,
                                               pairWSNoRebin);
  groupedWS->add(pairWSNameNoRebin);
}

// Get the name of the pair workspace to be saved.
const std::string ApplyMuonDetectorGroupPairing::getPairWorkspaceName(
    const std::string &pairName, const std::string &groupWSName) {
  Muon::DatasetParams params;
  // don't fill in instrument, runs, periods; not required.
  params.label = groupWSName;
  params.itemType = Muon::ItemType::Pair;
  params.itemName = pairName;
  params.plotType = Muon::PlotType::Asymmetry;
  params.version = 1;
  const std::string wsName = MuonAlgorithmHelper::generateWorkspaceName(params);
  return wsName;
}

// Get the names of the two workspaces in the ADS to pair.
const std::string ApplyMuonDetectorGroupPairing::getGroupWorkspaceNamesManually(
    const std::string &groupName, const std::string &groupWSName) {
  Muon::DatasetParams params;
  // don't fill in instrument, runs, periods; not required.
  params.label = groupWSName;
  params.itemType = Muon::ItemType::Group;
  params.itemName = groupName;
  params.plotType = Muon::PlotType::Counts;
  params.version = 1;
  const std::string wsName = generateWorkspaceName(params);
  return wsName;
}

/**
 * calculate asymmetry for a pair of workspaces of grouped detectors, using
 * parameter alpha, returning the resulting workspace.
 */
MatrixWorkspace_sptr
ApplyMuonDetectorGroupPairing::createPairWorkspaceFromGroupWorkspaces(
    MatrixWorkspace_sptr inputWS1, MatrixWorkspace_sptr inputWS2,
    const double &alpha) {

  IAlgorithm_sptr alg = this->createChildAlgorithm("AppendSpectra");
  alg->setProperty("InputWorkspace1", inputWS1);
  alg->setProperty("InputWorkspace2", inputWS2);
  alg->setProperty("ValidateInputs", true);
  alg->execute();

  MatrixWorkspace_sptr ws = alg->getProperty("OutputWorkspace");

  ws->getSpectrum(0).setSpectrumNo(0);
  ws->getSpectrum(1).setSpectrumNo(1);
  std::vector<int> fwd = {0};
  std::vector<int> bwd = {1};

  IAlgorithm_sptr algAsym = this->createChildAlgorithm("AsymmetryCalc");
  algAsym->setProperty("InputWorkspace", ws);
  algAsym->setProperty("ForwardSpectra", fwd);
  algAsym->setProperty("BackwardSpectra", bwd);
  algAsym->setProperty("Alpha", alpha);
  algAsym->setProperty("OutputWorkspace", "__NotUsed__");
  algAsym->execute();
  MatrixWorkspace_sptr outWS = algAsym->getProperty("OutputWorkspace");
  return outWS;
}

/**
 * return a workspace for a pair of detector groups, using the user input
 * options.
 */
MatrixWorkspace_sptr ApplyMuonDetectorGroupPairing::createPairWorkspaceManually(
    Workspace_sptr inputWS, bool noRebin) {

  IAlgorithm_sptr alg = this->createChildAlgorithm("MuonProcess");
  if (!this->isLogging())
    alg->setLogging(false);

  Muon::AnalysisOptions options = getUserInput();
  if (noRebin)
    options.rebinArgs = "";

  checkDetectorIDsInWorkspace(options.grouping, inputWS);

  setMuonProcessPeriodProperties(*alg, inputWS, options);
  setMuonProcessAlgorithmProperties(*alg, options);
  alg->execute();

  Workspace_sptr outWS = alg->getProperty("OutputWorkspace");
  return boost::dynamic_pointer_cast<MatrixWorkspace>(outWS);
}

/*
 * Store the input properties in options
 */
Muon::AnalysisOptions ApplyMuonDetectorGroupPairing::getUserInput() {
  Muon::AnalysisOptions options;

  Grouping grouping;
  grouping.description = "no description";
  grouping.groupNames.emplace_back("group1");
  grouping.groups.emplace_back(this->getPropertyValue("Group1"));
  grouping.groupNames.emplace_back("group2");
  grouping.groups.emplace_back(this->getPropertyValue("Group2"));
  const double alpha = static_cast<double>(getProperty("Alpha"));
  grouping.pairAlphas.emplace_back(alpha);
  grouping.pairNames.emplace_back(this->getPropertyValue("PairName"));
  grouping.pairs.emplace_back(std::make_pair(0, 1));

  options.grouping = grouping;
  options.summedPeriods = this->getPropertyValue("SummedPeriods");
  options.subtractedPeriods = this->getPropertyValue("SubtractedPeriods");
  options.timeZero = 0.0;
  options.loadedTimeZero = this->getProperty("TimeOffset");
  options.timeLimits.first = this->getProperty("TimeMin");
  options.timeLimits.second = this->getProperty("TimeMax");
  options.rebinArgs = this->getPropertyValue("rebinArgs");

  options.plotType = Muon::PlotType::Asymmetry;
  options.groupPairName = this->getPropertyValue("PairName");

  return options;
}

// Checks that the detector IDs in grouping are in the workspace
void ApplyMuonDetectorGroupPairing::checkDetectorIDsInWorkspace(
    API::Grouping &grouping, Workspace_sptr workspace) {
  bool check =
      MuonAlgorithmHelper::checkGroupDetectorsInWorkspace(grouping, workspace);
  if (!check) {
    g_log.error("One or more detector IDs specified in the groups is not "
                "contained in the InputWorkspace");
    throw std::runtime_error(
        "One or more detector IDs specified in the groups is not "
        "contained in the InputWorkspace");
  }
}

/**
 * Set algorithm properties (input workspace and period properties) according
 * to the given options. For use with MuonProcess.
 */
void ApplyMuonDetectorGroupPairing::setMuonProcessPeriodProperties(
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
    alg.setProperty("SubtractedPeriodSet", "");
  } else {
    throw std::runtime_error("Cannot create workspace: workspace must be "
                             "MatrixWorkspace or WorkspaceGroup.");
  }
  alg.setProperty("InputWorkspace", inputGroup);
}

/**
 * Set algorithm properties according to the given options. For use with
 * MuonProcess.
 */
void ApplyMuonDetectorGroupPairing::setMuonProcessAlgorithmProperties(
    IAlgorithm &alg, const Muon::AnalysisOptions &options) const {

  alg.setProperty("Mode", "Combined");
  alg.setProperty("CropWorkspace", false);
  if (!options.rebinArgs.empty()) {
    alg.setProperty("RebinParams", options.rebinArgs);
  }
  setMuonProcessAlgorithmGroupingProperties(alg, options);
  setMuonProcessAlgorithmTimeProperties(alg, options);
  alg.setProperty("OutputType", "PairAsymmetry");
}

// Set grouping properies of MuonProcess
void ApplyMuonDetectorGroupPairing::setMuonProcessAlgorithmGroupingProperties(
    IAlgorithm &alg, const Muon::AnalysisOptions &options) const {

  alg.setProperty("DetectorGroupingTable", options.grouping.toTable());
  alg.setProperty("GroupIndex", 0);
  alg.setProperty("Alpha", options.grouping.pairAlphas[0]);
  int first = static_cast<int>(options.grouping.pairs[0].first);
  int second = static_cast<int>(options.grouping.pairs[0].second);
  alg.setProperty("PairFirstIndex", first);
  alg.setProperty("PairSecondIndex", second);
}

/**
 * Set time properties according to the given options. For use with
 * MuonProcess.
 */
void ApplyMuonDetectorGroupPairing::setMuonProcessAlgorithmTimeProperties(
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

// Allow WorkspaceGroup property to function correctly.
bool ApplyMuonDetectorGroupPairing::checkGroups() { return false; }

} // namespace Muon
} // namespace Mantid
