// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMuon/MuonAlgorithmHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/Strings.h"

#include <fstream>
#include <string>
#include <utility>

#include <vector>

namespace Mantid::MuonAlgorithmHelper {

using namespace Mantid::Kernel;
using namespace Mantid::API;

/**
 * Return a first period MatrixWorkspace in a run workspace. If the run
 * workspace has one period only - it is returned.
 * @param ws :: Run workspace
 */
MatrixWorkspace_sptr firstPeriod(const Workspace_sptr &ws) {

  if (auto group = std::dynamic_pointer_cast<WorkspaceGroup>(ws)) {
    return std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));
  } else {
    return std::dynamic_pointer_cast<MatrixWorkspace>(ws);
  }
}

/**
 * Get a run label for a single workspace.
 * @param ws :: [input] workspace pointer
 * @return :: run label
 */
std::string getRunLabel(Mantid::API::Workspace_sptr ws) {
  const std::vector<Mantid::API::Workspace_sptr> wsList{std::move(ws)};
  return getRunLabel(wsList);
}

/**
 * Get a run label for a list of workspaces.
 * E.g. for MUSR data of runs 15189, 15190, 15191 it will look like
 * MUSR00015189-91.
 * (Assumes all runs have the same instrument)
 * @param wsList :: [input] Vector of workspace pointers
 * @return :: run label
 * @throws std::invalid_argument if empty list given
 */
std::string getRunLabel(const std::vector<Workspace_sptr> &wsList) {
  if (wsList.empty())
    throw std::invalid_argument("Unable to run on an empty list");

  const std::string instrument = firstPeriod(wsList.front())->getInstrument()->getName();

  // Extract the run numbers
  std::vector<int> runNumbers;
  runNumbers.reserve(wsList.size());
  for (auto &&workspace : wsList) {
    int runNumber = firstPeriod(workspace)->getRunNumber();
    runNumbers.emplace_back(runNumber);
  }

  return getRunLabel(instrument, runNumbers);
}

/**
 * Get a run label for a given instrument and list of runs.
 * E.g. for MUSR data of runs 15189, 15190, 15191 it will look like
 * MUSR00015189-91.
 * (Assumes all runs have the same instrument)
 * @param instrument :: [input] instrument name
 * @param runNumbers :: [input] List of run numbers
 * @return :: run label
 * @throws std::invalid_argument if empty run list given
 */
std::string getRunLabel(const std::string &instrument, const std::vector<int> &runNumbers) {
  if (runNumbers.empty()) {
    throw std::invalid_argument("Cannot run on an empty list");
  }

  // Find ranges of consecutive runs
  auto ranges = findConsecutiveRuns(runNumbers);

  // Zero-padding for the first run
  int zeroPadding;
  try {
    zeroPadding = ConfigService::Instance().getInstrument(instrument).zeroPadding(ranges.begin()->first);
  } catch (const Mantid::Kernel::Exception::NotFoundError &) {
    // Old muon instrument without an IDF - default to 3 zeros
    zeroPadding = 3;
  }

  std::ostringstream label;
  label << instrument;
  for (const auto &range : ranges) {
    label << createStringFromRange(range, zeroPadding);
    // Only pad the first set
    zeroPadding = 0;
    if (range != ranges.back()) {
      label << ", ";
    }
  }

  return label.str();
}

/**
 * Create a string from a range
 * @param range :: [input] a pair of integers representing the ends of the range
 * (may be the same)
 * @param zeroPadding :: [input] pad the lower element of range with zeros up to
 * length zeroPadding.
 * @return :: range in the form "1234-45" removing common digits from the upper
 * end of the range.
 */
std::string createStringFromRange(const std::pair<int, int> &range, const int &zeroPadding) {
  std::string firstRun;
  std::string lastRun;
  if (range.second > range.first) {
    firstRun = std::to_string(range.first);
    lastRun = std::to_string(range.second);
  } else {
    firstRun = std::to_string(range.second);
    lastRun = std::to_string(range.first);
  }

  // Begin string output with full label of first run
  std::ostringstream paddedLabel;
  paddedLabel << std::setw(zeroPadding) << std::setfill('0') << std::right;
  paddedLabel << firstRun;

  if (range.second != range.first) {
    // Remove the common part of the first and last run, so we get e.g.
    // "12345-56" instead of "12345-12356"
    for (size_t i = 0; i < firstRun.size() && i < lastRun.size(); ++i) {
      if (firstRun[i] != lastRun[i]) {
        lastRun.erase(0, i);
        break;
      }
    }
    paddedLabel << "-" << lastRun;
  }

  return paddedLabel.str();
}

/**
 * Given a vector of run numbers, returns the consecutive ranges of runs.
 * e.g. 1,2,3,5,6,8 -> (1,3), (5,6), (8,8)
 * @param runs :: [input] Vector of run numbers - need not be sorted
 * @returns Vector of pairs of (start, end) of consecutive runs
 */
std::vector<std::pair<int, int>> findConsecutiveRuns(const std::vector<int> &runs) {
  // Groups to output
  std::vector<std::pair<int, int>> ranges;
  // Sort the vector to begin with
  std::vector<int> runNumbers(runs); // local copy
  std::sort(runNumbers.begin(), runNumbers.end());

  // Iterate through vector looking for consecutive groups
  auto a = runNumbers.begin();
  auto start = a;
  auto b = a + 1;
  while (b != runNumbers.end()) {
    if (*b - 1 == *a) { // Still consecutive
      a++;
      b++;
    } else { // Reached end of consecutive group
      ranges.emplace_back(*start, *a);
      start = b++;
      a = start;
    }
  }
  // Reached end of last consecutive group
  ranges.emplace_back(*start, runNumbers.back());
  return ranges;
}

/**
 * Makes sure the specified workspaces are in specified group. If group exists
 * already - missing workspaces are added to it, otherwise new group is created.
 * If ws exists in ADS under groupName, and it is not a group - it's
 * overwritten.
 * @param groupName :: Name of the group workspaces should be in
 * @param inputWorkspaces :: Names of the workspaces to group
 */
void groupWorkspaces(const std::string &groupName, const std::vector<std::string> &inputWorkspaces) {
  auto &ads = AnalysisDataService::Instance();

  WorkspaceGroup_sptr group;
  if (ads.doesExist(groupName)) {
    group = ads.retrieveWS<WorkspaceGroup>(groupName);
  }

  if (group) {
    // Exists and is a group -> add missing workspaces to it
    for (const auto &inputWorkspace : inputWorkspaces) {
      if (!group->contains(inputWorkspace)) {
        group->add(inputWorkspace);
      }
    }
  } else {
    // Doesn't exist or isn't a group -> create/overwrite
    auto groupingAlg = AlgorithmManager::Instance().createUnmanaged("GroupWorkspaces");
    groupingAlg->initialize();
    groupingAlg->setProperty("InputWorkspaces", inputWorkspaces);
    groupingAlg->setPropertyValue("OutputWorkspace", groupName);
    groupingAlg->execute();
  }
}

/**
 * Generate a workspace name from the given parameters
 * Format: "INST00012345; Pair; long; Asym;[ 1;] #1"
 * @param params :: [input] Struct containing dataset parameters
 * @returns :: Name for analysis workspace
 */
std::string generateWorkspaceName(const Muon::DatasetParams &params) {
  std::ostringstream workspaceName;
  const static std::string sep("; ");

  // Instrument and run number
  if (params.label.empty()) {
    workspaceName << getRunLabel(params.instrument, params.runs) << sep;
  } else {
    workspaceName << params.label << sep;
  }

  // Pair/group and name of pair/group
  if (params.itemType == Muon::ItemType::Pair) {
    workspaceName << "Pair" << sep;
  } else if (params.itemType == Muon::ItemType::Group) {
    workspaceName << "Group" << sep;
  }
  workspaceName << params.itemName << sep;

  // Type of plot
  switch (params.plotType) {
  case Muon::PlotType::Asymmetry:
    workspaceName << "Asym";
    break;
  case Muon::PlotType::Counts:
    workspaceName << "Counts";
    break;
  case Muon::PlotType::Logarithm:
    workspaceName << "Logs";
    break;
  }

  // Period(s)
  const auto periods = params.periods;
  if (!periods.empty()) {
    workspaceName << sep << periods;
  }

  // Version - always "#1" if overwrite is on, otherwise increment
  workspaceName << sep << "#" << params.version;

  return workspaceName.str();
}

/**
 * Find all the detector IDs contained inside a workspace (either matrix or
 * group) and return as an ordered set.
 */
std::set<Mantid::detid_t> getAllDetectorIDsFromWorkspace(const Mantid::API::Workspace_sptr &ws) {

  std::set<Mantid::detid_t> detectorIDs;
  if (auto workspace = std::dynamic_pointer_cast<MatrixWorkspace>(ws)) {
    detectorIDs = getAllDetectorIDsFromMatrixWorkspace(workspace);
  } else if (auto workspace = std::dynamic_pointer_cast<WorkspaceGroup>(ws)) {
    detectorIDs = getAllDetectorIDsFromGroupWorkspace(workspace);
  }
  return detectorIDs;
}

/**
 * Find all the detector IDs contained inside a matrix workspace
 */
std::set<Mantid::detid_t> getAllDetectorIDsFromMatrixWorkspace(const Mantid::API::MatrixWorkspace_sptr &ws) {

  std::set<Mantid::detid_t> detectorIDs;
  auto numSpectra = ws->getNumberHistograms();
  for (size_t i = 0; i < numSpectra; i++) {
    std::set<Mantid::detid_t> spectrumIDs = ws->getSpectrum(i).getDetectorIDs();
    detectorIDs.insert(spectrumIDs.begin(), spectrumIDs.end());
  }
  return detectorIDs;
}

/**
 * Find all the detector IDs contained inside a group workspace
 */
std::set<Mantid::detid_t> getAllDetectorIDsFromGroupWorkspace(const Mantid::API::WorkspaceGroup_sptr &ws) {

  std::set<Mantid::detid_t> detectorIDs;

  std::vector<Workspace_sptr> workspaces = ws->getAllItems();
  for (const auto &workspace : workspaces) {
    MatrixWorkspace_sptr matrixWS = std::dynamic_pointer_cast<MatrixWorkspace>(workspace);
    std::set<Mantid::detid_t> detectorIDsSingleWorkspace = getAllDetectorIDsFromMatrixWorkspace(matrixWS);
    detectorIDs.insert(detectorIDsSingleWorkspace.begin(), detectorIDsSingleWorkspace.end());
  }
  return detectorIDs;
}

/**
 * Find all the detector IDs contained inside a grouping object and return as a
 * vector of ints
 */
std::vector<int> getAllDetectorIDsFromGroup(const Grouping &grouping) {
  std::vector<int> groupDetectors;
  for (const auto &group : grouping.groups) {
    std::vector<int> groupDetectorIDs = Mantid::Kernel::Strings::parseRange(group);
    groupDetectors.insert(groupDetectors.end(), groupDetectorIDs.begin(), groupDetectorIDs.end());
  }
  return groupDetectors;
}

// Checks if all the detectors in the groups in a Grouping are in the workspace.
// Workspace can be matrix or group type.
bool checkGroupDetectorsInWorkspace(const Grouping &grouping, const Workspace_sptr &ws) {
  std::set<int> detectorIDs = getAllDetectorIDsFromWorkspace(ws);
  std::vector<int> groupDetectorIDs = getAllDetectorIDsFromGroup(grouping);
  return checkItemsInSet(groupDetectorIDs, detectorIDs);
}

// Checks that all of the entries of a vector are contained in a set, returns
// true/false
bool checkItemsInSet(const std::vector<int> &items, const std::set<int> &set) {
  return std::all_of(items.cbegin(), items.cend(), [&set](const auto item) { return set.find(item) != set.cend(); });
}

/**
 * Parse a workspace name into dataset parameters
 * Format: "INST00012345; Pair; long; Asym;[ 1;] #1"
 * count:     1             2    3      4    (5)  5/6
 * @param wsName :: [input] Name of workspace
 * @returns :: Struct containing dataset parameters
 */
Muon::DatasetParams parseWorkspaceName(const std::string &wsName) {
  Muon::DatasetParams params;

  Mantid::Kernel::StringTokenizer tokenizer(wsName, ";", Mantid::Kernel::StringTokenizer::TOK_TRIM);
  const size_t numTokens = tokenizer.count();
  // Name contains min of 5 ";" separated values and max 6.
  if (numTokens < 5 || numTokens > 6) {
    throw std::invalid_argument("Could not parse workspace name: " + wsName);
  }

  params.label = tokenizer[0];
  parseRunLabel(params.label, params.instrument, params.runs);
  const std::string itemType = tokenizer[1];
  params.itemType = (itemType == "Group") ? Muon::ItemType::Group : Muon::ItemType::Pair;
  params.itemName = tokenizer[2];
  const std::string plotType = tokenizer[3];
  if (plotType == "Asym") {
    params.plotType = Muon::PlotType::Asymmetry;
  } else if (plotType == "Counts") {
    params.plotType = Muon::PlotType::Counts;
  } else {
    params.plotType = Muon::PlotType::Logarithm;
  }
  std::string versionString;
  if (numTokens > 5) { // periods included
    params.periods = tokenizer[4];
    versionString = tokenizer[5];
  } else {
    versionString = tokenizer[4];
  }
  // Remove the # from the version string
  versionString.erase(std::remove(versionString.begin(), versionString.end(), '#'), versionString.end());

  try {
    params.version = boost::lexical_cast<size_t>(versionString);
  } catch (const boost::bad_lexical_cast &) {
    params.version = 1; // Set to 1 and ignore the error
  }

  return params;
}

/**
 * Parse a run label e.g. "MUSR00015189-91, 15193" into instrument
 * ("MUSR") and set of runs (15189, 15190, 15191, 15193).
 * Assumes instrument name doesn't contain a digit (true for muon instruments).
 * @param label :: [input] Label to parse
 * @param instrument :: [output] Name of instrument
 * @param runNumbers :: [output] Vector to fill with run numbers
 * @throws std::invalid_argument if input cannot be parsed
 */
void parseRunLabel(const std::string &label, std::string &instrument, std::vector<int> &runNumbers) {
  const size_t instPos = label.find_first_of("0123456789");
  instrument = label.substr(0, instPos);
  const size_t numPos = label.find_first_not_of('0', instPos);
  runNumbers.clear();
  if (numPos != std::string::npos) {
    std::string runString = label.substr(numPos, label.size());
    // sets of continuous ranges
    Mantid::Kernel::StringTokenizer rangeTokenizer(runString, ",", Mantid::Kernel::StringTokenizer::TOK_TRIM);
    for (const auto &range : rangeTokenizer.asVector()) {
      Mantid::Kernel::StringTokenizer pairTokenizer(range, "-", Mantid::Kernel::StringTokenizer::TOK_TRIM);
      try {
        if (pairTokenizer.count() == 2) {
          // Range of run numbers
          // Deal with common part of string: "151" in "15189-91"
          const size_t diff = pairTokenizer[0].length() - pairTokenizer[1].length();
          const std::string endRun = pairTokenizer[0].substr(0, diff) + pairTokenizer[1];
          const auto start = boost::lexical_cast<int>(pairTokenizer[0]);
          const auto end = boost::lexical_cast<int>(endRun);
          for (int run = start; run < end + 1; run++) {
            runNumbers.emplace_back(run);
          }
        } else if (pairTokenizer.count() == 1) {
          // Single run
          runNumbers.emplace_back(boost::lexical_cast<int>(pairTokenizer[0]));
        } else {
          throw std::invalid_argument("Failed to parse run label: " + label + " too many tokens ");
        }
      } catch (const boost::bad_lexical_cast &) {
        throw std::invalid_argument("Failed to parse run label: " + label + " not a good run number");
      } catch (...) {
        throw std::invalid_argument("Failed to parse run label: " + label);
      }
    }
  } else {
    // The string was "INST000" or similar...
    runNumbers.emplace_back(0);
  }
}

// Validate the input group workspaces using their names.
bool checkValidPair(const std::string &WSname1, const std::string &WSname2) {
  Muon::DatasetParams group1, group2;
  try {
    group1 = parseWorkspaceName(WSname1);
    group2 = parseWorkspaceName(WSname2);
  } catch (const std::invalid_argument &) {
    throw std::invalid_argument("Ensure workspaces have the correctly formatted name (see "
                                "documentation).");
  }

  if (group1.instrument != group2.instrument) {
    throw std::invalid_argument("Group workspaces named with different instruments.");
  }

  if (group1.itemName == group2.itemName) {
    throw std::invalid_argument("Groups used for pairing must have differnt names.");
  }

  if (group1.itemType != Muon::ItemType::Group || group2.itemType != Muon::ItemType::Group) {
    throw std::invalid_argument("Workspaces must be of group type (not pair)");
  }

  if (group1.plotType != Muon::PlotType::Counts || group2.plotType != Muon::PlotType::Counts) {
    throw std::invalid_argument("Workspaces must be of counts type (not asymmetry)");
  }

  return true;
}

/// Check whether a group or pair name is valid
bool checkValidGroupPairName(const std::string &name) {
  if (name.empty()) {
    return false;
  }
  if (!std::all_of(std::begin(name), std::end(name), isalnum)) {
    return false;
  }
  if (name == "Group" || name == "Pair") {
    return false;
  }
  return true;
}

bool is_alphanumerical_or_underscore(char character) {
  return (isalpha(character) || isdigit(character) || (character == '_'));
}

/**
 * Sums the specified periods of the input workspace group
 * @param periodsToSum :: [input] List of period indexes (1-based) to be summed
 * @returns Workspace containing the sum
 */
MatrixWorkspace_sptr sumPeriods(const WorkspaceGroup_sptr &inputWS, const std::vector<int> &periodsToSum) {
  MatrixWorkspace_sptr outWS;
  if (!periodsToSum.empty()) {
    auto LHSWorkspace = inputWS->getItem(periodsToSum[0] - 1);
    outWS = std::dynamic_pointer_cast<MatrixWorkspace>(LHSWorkspace);
    if (outWS != nullptr && periodsToSum.size() > 1) {
      auto numPeriods = static_cast<int>(periodsToSum.size());
      for (int i = 1; i < numPeriods; i++) {
        auto RHSWorkspace = inputWS->getItem(periodsToSum[i] - 1);
        auto alg = AlgorithmManager::Instance().createUnmanaged("Plus");
        alg->initialize();
        alg->setChild(true);
        alg->setRethrows(true);
        alg->setProperty("LHSWorkspace", outWS);
        alg->setProperty("RHSWorkspace", RHSWorkspace);
        alg->setProperty("OutputWorkspace", "__NotUsed__");
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
MatrixWorkspace_sptr subtractWorkspaces(const MatrixWorkspace_sptr &lhs, const MatrixWorkspace_sptr &rhs) {
  MatrixWorkspace_sptr outWS;
  if (lhs && rhs) {
    auto alg = AlgorithmManager::Instance().createUnmanaged("Minus");
    alg->initialize();
    alg->setChild(true);
    alg->setRethrows(true);
    alg->setProperty("LHSWorkspace", lhs);
    alg->setProperty("RHSWorkspace", rhs);
    alg->setProperty("OutputWorkspace", "__NotUsed__");
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
MatrixWorkspace_sptr extractSpectrum(const Workspace_sptr &inputWS, const int index) {
  MatrixWorkspace_sptr outWS;
  if (inputWS) {
    auto alg = AlgorithmManager::Instance().createUnmanaged("ExtractSingleSpectrum");
    alg->initialize();
    alg->setChild(true);
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", inputWS);
    alg->setProperty("WorkspaceIndex", index);
    alg->setProperty("OutputWorkspace", "__NotUsed__");
    alg->execute();
    outWS = alg->getProperty("OutputWorkspace");
  }
  return outWS;
}

void addSampleLog(const MatrixWorkspace_sptr &workspace, const std::string &logName, const std::string &logValue) {
  auto alg = AlgorithmManager::Instance().createUnmanaged("AddSampleLog");
  alg->initialize();
  alg->setChild(true);
  alg->setRethrows(true);
  alg->setProperty("Workspace", workspace);
  alg->setProperty("LogName", logName);
  alg->setProperty("LogText", logValue);
  alg->execute();
}

bool isAlphanumericOrUnderscore(char character) {
  return (isalpha(character) || isdigit(character) || (character == '_'));
}

} // namespace Mantid::MuonAlgorithmHelper
