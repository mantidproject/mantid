#include "MantidMuon/MuonAlgorithmHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/InstrumentInfo.h"

#include <string>
#include <vector>

namespace Mantid {
namespace MuonAlgorithmHelper {

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Types::Core::DateAndTime;

/**
 * Return a first period MatrixWorkspace in a run workspace. If the run
 * workspace has one period
 * only - it is returned.
 * @param ws :: Run workspace
 */
MatrixWorkspace_sptr firstPeriod(Workspace_sptr ws) {
  if (auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(ws)) {
    return boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));
  } else {
    return boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
  }
}

std::string getRunLabel(const Mantid::API::Workspace_sptr &ws) {
  const std::vector<Mantid::API::Workspace_sptr> wsList{ws};
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

  const std::string instrument =
      firstPeriod(wsList.front())->getInstrument()->getName();

  // Extract the run numbers
  std::vector<int> runNumbers;
  runNumbers.reserve(wsList.size());
  for (auto &&workspace : wsList) {
    int runNumber = firstPeriod(workspace)->getRunNumber();
    runNumbers.push_back(runNumber);
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
std::string getRunLabel(const std::string &instrument,
                        const std::vector<int> &runNumbers) {
  if (runNumbers.empty()) {
    throw std::invalid_argument("Cannot run on an empty list");
  }

  // Find ranges of consecutive runs
  auto ranges = findConsecutiveRuns(runNumbers);

  // Zero-padding for the first run
  int zeroPadding;
  try {
    zeroPadding = ConfigService::Instance()
                      .getInstrument(instrument)
                      .zeroPadding(ranges.begin()->first);
  } catch (const Mantid::Kernel::Exception::NotFoundError &) {
    // Old muon instrument without an IDF - default to 3 zeros
    zeroPadding = 3;
  }

  std::ostringstream label;
  label << instrument;
  for (auto range : ranges) {
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
std::string createStringFromRange(const std::pair<int, int> &range,
                                  const int &zeroPadding) {
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
    size_t sharedDigits = 0;
	for (size_t i = 0; i < firstRun.size() && i < lastRun.size(); ++i) {
		if (firstRun[i] != lastRun[i]) {
			lastRun.erase(0, i);
			break;
		}
	}
	if (sharedDigits > 0) {
		lastRun.erase(0, sharedDigits);
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
std::vector<std::pair<int, int>>
findConsecutiveRuns(const std::vector<int> &runs) {
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
void groupWorkspaces(const std::string &groupName,
                     const std::vector<std::string> &inputWorkspaces) {
  auto &ads = AnalysisDataService::Instance();

  WorkspaceGroup_sptr group;
  if (ads.doesExist(groupName)) {
    group = ads.retrieveWS<WorkspaceGroup>(groupName);
  }

  if (group) {
    // Exists and is a group -> add missing workspaces to it
    for (auto it = inputWorkspaces.begin(); it != inputWorkspaces.end(); ++it) {
      if (!group->contains(*it)) {
        group->add(*it);
      }
    }
  } else {
    // Doesn't exist or isn't a group -> create/overwrite
    IAlgorithm_sptr groupingAlg =
        AlgorithmManager::Instance().create("GroupWorkspaces");
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

} // namespace MuonAlgorithmHelper
} // namespace Mantid
