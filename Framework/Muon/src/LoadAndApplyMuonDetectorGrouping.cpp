#include "MantidMuon/LoadAndApplyMuonDetectorGrouping.h"
#include "MantidMuon/ApplyMuonDetectorGrouping.h"
#include "MantidMuon/MuonAlgorithmHelper.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Strings.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace {

// Convert the enum PlotType to a string to pass to ApplyMuonDetectorGrouping
std::string plotTypeToString(const Mantid::Muon::PlotType &plotType) {
  switch (plotType) {
  case Mantid::Muon::PlotType::Asymmetry:
    return "Asymmetry";
  case Mantid::Muon::PlotType::Counts:
    return "Counts";
  case Mantid::Muon::PlotType::Logarithm:
    return "Counts";
  }
  // Default to counts
  return "Counts";
}

} // namespace

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::MuonAlgorithmHelper;

namespace Mantid {
namespace Muon {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadAndApplyMuonDetectorGrouping)

void LoadAndApplyMuonDetectorGrouping::init() {

  declareProperty(
      Mantid::Kernel::make_unique<FileProperty>(
          "Filename", "", API::FileProperty::Load, ".xml"),
      "The XML file containing the grouping and pairing information");

  declareProperty(
      Mantid::Kernel::make_unique<WorkspaceProperty<Workspace>>(
          "InputWorkspace", "", Direction::Input, PropertyMode::Mandatory),
      "Input workspace containing data from detectors that the "
      "grouping/pairing will be applied to.");

  declareProperty(
      Mantid::Kernel::make_unique<WorkspaceProperty<WorkspaceGroup>>(
          "WorkspaceGroup", "", Direction::Input, PropertyMode::Optional),
      "The workspaces created by the algorithm will be placed inside this "
      "group. If not specified will save to \"MuonAnalysisGroup\" ");

  declareProperty("ApplyAsymmetryToGroups", true,
                  "Whether to calculate asymmetry and store the workspaces.");
}

/**
 * Performs validation of inputs to the algorithm.
 * -
 */
std::map<std::string, std::string>
LoadAndApplyMuonDetectorGrouping::validateInputs() {
  std::map<std::string, std::string> errors;

  return errors;
}

void LoadAndApplyMuonDetectorGrouping::exec() {
  this->setRethrows(true);

  AnalysisOptions options = setDefaultOptions();
  options.grouping = loadGroupsAndPairs();

  Workspace_sptr inputWS = getProperty("InputWorkspace");

  checkDetectorIDsInWorkspace(options.grouping, inputWS);

  WorkspaceGroup_sptr groupedWS = getProperty("WorkspaceGroup");
  if (!groupedWS) {
    groupedWS = addGroupedWSWithDefaultName(inputWS);
  }

  // Perform the analysis specified in options
  addGroupingToADS(options, inputWS, groupedWS);
  addPairingToADS(options, inputWS, groupedWS);

  if (getProperty("ApplyAsymmetryToGroups")) {
    options.plotType = PlotType::Asymmetry;
    addGroupingToADS(options, inputWS, groupedWS);
  }
  addGroupingInformationToADS(options.grouping);
}

// Checks that the detector IDs are consistsent with the supplied workspace
void LoadAndApplyMuonDetectorGrouping::checkDetectorIDsInWorkspace(
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
 * Adds an empty workspaceGroup to the ADS with a name
 * that it would have if created by the MuonAnalysis GUI.
 */
WorkspaceGroup_sptr
LoadAndApplyMuonDetectorGrouping::addGroupedWSWithDefaultName(
    Workspace_sptr workspace) {
  std::string groupedWSName = MuonAlgorithmHelper::getRunLabel(workspace);
  WorkspaceGroup_sptr groupedWS = boost::make_shared<WorkspaceGroup>();
  AnalysisDataService::Instance().addOrReplace(groupedWSName, groupedWS);
  return groupedWS;
}

AnalysisOptions LoadAndApplyMuonDetectorGrouping::setDefaultOptions() {
  AnalysisOptions options;
  options.summedPeriods = "1";
  options.subtractedPeriods = "";
  options.timeZero = 0;
  options.loadedTimeZero = 0;
  options.rebinArgs = "";
  options.plotType = Muon::PlotType::Counts;
  return options;
};

/**
 * Adds the group names and corresponding detector IDs to
 * a TableWorkspace in the ADS named "MuonGroupings"
 */
void LoadAndApplyMuonDetectorGrouping::addGroupingInformationToADS(
    const Mantid::API::Grouping &grouping) {

  auto groupingTable = boost::dynamic_pointer_cast<ITableWorkspace>(
      WorkspaceFactory::Instance().createTable("TableWorkspace"));
  groupingTable->addColumn("str", "GroupName");
  groupingTable->addColumn("vector_int", "Detectors");

  size_t numGroups = grouping.groups.size();
  for (auto i = 0; i < numGroups; i++) {
    TableRow newRow = groupingTable->appendRow();
    newRow << grouping.groupNames[i];
    newRow << Kernel::Strings::parseRange(grouping.groups[i]);
  }

  AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();
  ads.addOrReplace("MuonGroupings", groupingTable);
}

/**
 * Load the grouping and pairing information from the input file
 * into the Grouping struct.
 */
API::Grouping LoadAndApplyMuonDetectorGrouping::loadGroupsAndPairs() {
  Grouping grouping;
  std::string filename = getProperty("Filename");
  GroupingLoader::loadGroupingFromXML(filename, grouping);

  CheckValidGroupsAndPairs(grouping);

  return grouping;
};

void LoadAndApplyMuonDetectorGrouping::CheckValidGroupsAndPairs(
    const Grouping &grouping) {
  for (auto &&groupName : grouping.groupNames) {
    if (!MuonAlgorithmHelper::checkValidGroupPairName(groupName)) {
      throw std::invalid_argument("Some group names are invalid");
    }
  }

  for (size_t p = 0; p < grouping.pairs.size(); p++) {
    std::string pairName = grouping.pairNames[p];
    if (!MuonAlgorithmHelper::checkValidGroupPairName(pairName)) {
      throw std::invalid_argument("Some pair names are invalid");
    }
    std::string pairGroup1 = grouping.groupNames[grouping.pairs[p].first];
    std::string pairGroup2 = grouping.groupNames[grouping.pairs[p].second];

    bool group1inPair =
        (std::find(grouping.groupNames.begin(), grouping.groupNames.end(),
                   pairGroup1) != grouping.groupNames.end());
    bool group2inPair =
        (std::find(grouping.groupNames.begin(), grouping.groupNames.end(),
                   pairGroup1) != grouping.groupNames.end());

    if (!group1inPair) {
      throw std::invalid_argument("Pairing " + pairName + " : required group " +
                                  pairGroup1 + " is missing");
    }
    if (!group2inPair) {
      throw std::invalid_argument("Pairing " + pairName + " : required group " +
                                  pairGroup2 + " is missing");
    }
  }
}

/**
 * Add all the supplied groups to the ADS, inside wsGrouped, by
 * executing the ApplyMuonDetectorGrouping algorithm
 */
void LoadAndApplyMuonDetectorGrouping::addGroupingToADS(
    const Mantid::Muon::AnalysisOptions &options,
    Mantid::API::Workspace_sptr ws,
    Mantid::API::WorkspaceGroup_sptr wsGrouped) {

  size_t numGroups = options.grouping.groups.size();
  for (auto i = 0; i < numGroups; i++) {
    IAlgorithm_sptr alg =
        this->createChildAlgorithm("ApplyMuonDetectorGrouping");
    alg->setProperty("InputWorkspace", ws->getName());
    alg->setProperty("InputWorkspaceGroup", wsGrouped->getName());
    alg->setProperty("GroupName", options.grouping.groupNames[i]);
    alg->setProperty("Grouping", options.grouping.groups[i]);
    alg->setProperty("AnalysisType", plotTypeToString(options.plotType));
    alg->execute();
  }
};

// The pairing algorithm is not yet implemented
void LoadAndApplyMuonDetectorGrouping::addPairingToADS(
    const Mantid::Muon::AnalysisOptions &options,
    Mantid::API::Workspace_sptr ws,
    Mantid::API::WorkspaceGroup_sptr wsGrouped) {

  size_t numPairs = options.grouping.pairs.size();
  for (auto i = 0; i < numPairs; i++) {
    IAlgorithm_sptr alg =
        this->createChildAlgorithm("ApplyMuonDetectorGroupPairing");
    alg->setProperty("SpecifyGroupsManually", true);
    alg->setProperty("InputWorkspace", ws->getName());
    alg->setProperty("InputWorkspaceGroup", wsGrouped->getName());

    alg->setProperty("PairName", options.grouping.pairNames[i]);
    alg->setProperty("Alpha", options.grouping.pairAlphas[i]);

    std::string group1 =
        options.grouping.groups[options.grouping.pairs[i].first];
    std::string group2 =
        options.grouping.groups[options.grouping.pairs[i].second];
    alg->setProperty("Group1", group1);
    alg->setProperty("Group2", group2);

    alg->execute();
  };
};

// Allow WorkspaceGroup property to function correctly.
bool LoadAndApplyMuonDetectorGrouping::checkGroups() { return false; }

} // namespace Muon
} // namespace Mantid
