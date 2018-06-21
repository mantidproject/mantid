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
enum class ItemType { Pair, Group };

/// Possible plot types users might request
enum class PlotType { Asymmetry, Counts, Logarithm };

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
  std::string rebinArgs;          /// Arguments for rebin (empty to not rebin)
  std::string groupPairName;      /// Name of group or pair to use
  Mantid::API::Grouping grouping; /// Grouping to use
  PlotType plotType = {};         /// Type of analysis to perform
  explicit AnalysisOptions() {}
};

} // namespace Muon

namespace MuonAlgorithmHelper {

/// Returns a first period MatrixWorkspace in a run workspace
DLLExport Mantid::API::MatrixWorkspace_sptr firstPeriod(API::Workspace_sptr ws);

/// Get a run label for a workspace
DLLExport std::string getRunLabel(API::Workspace_sptr ws);

/// Get a run label for a list of workspaces
DLLExport std::string
getRunLabel(const std::vector<API::Workspace_sptr> &wsList);

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

/// Generate string of the period algebra
DLLExport std::string
generatePeriodAlgebraString(std::string summedPeriods,
                            std::string subtractedPeriods);
DLLExport std::string
generatePeriodAlgebraString(std::vector<int> summedPeriods,
                            std::vector<int> subtractedPeriods);

/// Generate new analysis workspace name
DLLExport std::string generateWorkspaceName(const Muon::DatasetParams &params);

/// Parse analysis workspace name
DLLExport Muon::DatasetParams parseWorkspaceName(const std::string &wsName);

/// Parse run label into instrument and runs
DLLExport void parseRunLabel(const std::string &label, std::string &instrument,
                             std::vector<int> &runNumbers);

/// Checks that the names allow a pairing
DLLExport bool checkValidPair(const std::string &name1,
                              const std::string &name2);

} // namespace MuonAlgorithmHelper
} // namespace Mantid

#endif /* MANTIDQT_CUSTOMINTERFACES_MUONALGORITHMHELPER_H_ */
