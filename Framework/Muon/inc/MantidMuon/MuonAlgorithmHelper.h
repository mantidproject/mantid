#ifndef MANTID_MUON_MUONALGORITHMHELPER_H_
#define MANTID_MUON_MUONALGORITHMHELPER_H_

#include "MantidAPI/GroupingLoader.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/System.h"

#include <string>
#include <vector>

namespace Mantid {
namespace Muon {

/// Types of entities we are dealing with
const enum class ItemType { Pair, Group };

/// Possible plot types users might request
const enum class PlotType { Asymmetry, Counts, Logarithm };

/// Parameters from parsed workspace name
struct DatasetParams {
  std::string label;
  std::string instrument;
  std::vector<int> runs;
  ItemType itemType;
  std::string itemName;
  PlotType plotType;
  std::string periods;
  size_t version;
};

/// Parameters for creating analysis workspace
struct AnalysisOptions {
  std::string summedPeriods;            /// Set of periods to sum
  std::string subtractedPeriods;        /// Set of periods to subtract
  double timeZero = 0;                  /// Value to use for t0 correction
  double loadedTimeZero = 0;            /// Time zero from data file
  std::pair<double, double> timeLimits; /// Min, max X values
  std::string rebinArgs;                /// Arguments for rebin (empty to not rebin)
  std::string groupPairName;            /// Name of group or pair to use
  Mantid::API::Grouping grouping;       /// Grouping to use
  PlotType plotType = {};               /// Type of analysis to perform
  explicit AnalysisOptions() {}
};

} // namespace Muon

namespace MuonAlgorithmHelper {

/// Returns a first period MatrixWorkspace in a run workspace
DLLExport Mantid::API::MatrixWorkspace_sptr firstPeriod(API::Workspace_sptr ws);

/// Get a run label for the workspace
DLLExport std::string getRunLabel(const API::Workspace_sptr &ws);

/// Get a run label for a list of workspaces
DLLExport std::string getRunLabel(const std::vector<API::Workspace_sptr> &wsList);

/// Get a run label given instrument and run numbers
DLLExport std::string getRunLabel(const std::string &instrument,
                        const std::vector<int> &runNumbers);

/// Create a string from a range "first-last", removing common digits from last.
/// Also pads with zeros up to zeroPadding digits.
DLLExport std::string createStringFromRange(const std::pair<int, int> &range,
                                            const int &zeroPadding);

/// Makes sure the specified workspaces are in specified group
DLLExport void groupWorkspaces(const std::string &groupName,
                     const std::vector<std::string> &inputWorkspaces);

/// Finds runs of consecutive numbers
DLLExport std::vector<std::pair<int, int>>
findConsecutiveRuns(const std::vector<int> &runs);

/// Generate new analysis workspace name
DLLExport std::string generateWorkspaceName(const Muon::DatasetParams &params);
} // namespace MuonAlgorithmHelper
} // namespace Mantid

#endif /* MANTIDQT_CUSTOMINTERFACES_MUONALGORITHMHELPER_H_ */
