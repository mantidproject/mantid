// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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

/// Generate new analysis workspace name
DLLExport std::string generateWorkspaceName(const Muon::DatasetParams &params);

/// Find all the detector IDs contained inside a workspace (either matrix or
/// group) and return as an ordered set.
DLLExport std::set<Mantid::detid_t>
getAllDetectorIDsFromWorkspace(Mantid::API::Workspace_sptr ws);

/// Find all the detector IDs contained inside a group workspace
DLLExport std::set<Mantid::detid_t>
getAllDetectorIDsFromGroupWorkspace(Mantid::API::WorkspaceGroup_sptr ws);

/// Find all the detector IDs contained inside a matrix workspace
DLLExport std::set<Mantid::detid_t>
getAllDetectorIDsFromMatrixWorkspace(Mantid::API::MatrixWorkspace_sptr ws);

/// Find all the detector IDs contained inside a grouping object and return as a
/// vector of ints
DLLExport std::vector<int>
getAllDetectorIDsFromGroup(const API::Grouping &grouping);

/// Checks if all the detectors in the groups in a Grouping are in the
/// workspace. Workspace can be matrix or group type.
DLLExport bool checkGroupDetectorsInWorkspace(const API::Grouping &grouping,
                                              API::Workspace_sptr ws);

/// Checks that all of the entries of a vector are contained in a set.
DLLExport bool checkItemsInSet(const std::vector<int> &items,
                               const std::set<int> &set);
/// Parse analysis workspace name
DLLExport Muon::DatasetParams parseWorkspaceName(const std::string &wsName);

/// Parse run label into instrument and runs
DLLExport void parseRunLabel(const std::string &label, std::string &instrument,
                             std::vector<int> &runNumbers);

/// Checks that the workspace names allow a pairing
DLLExport bool checkValidPair(const std::string &name1,
                              const std::string &name2);

/// Check whether a group or pair name is valid
DLLExport bool checkValidGroupPairName(const std::string &name);

DLLExport bool is_alphanumerical_or_underscore(char character);

DLLExport Mantid::API::MatrixWorkspace_sptr
sumPeriods(const Mantid::API::WorkspaceGroup_sptr &inputWS,
           const std::vector<int> &periodsToSum);

DLLExport Mantid::API::MatrixWorkspace_sptr
subtractWorkspaces(const Mantid::API::MatrixWorkspace_sptr &lhs,
                   const Mantid::API::MatrixWorkspace_sptr &rhs);

DLLExport Mantid::API::MatrixWorkspace_sptr
extractSpectrum(const Mantid::API::Workspace_sptr &inputWS, const int index);

DLLExport void addSampleLog(Mantid::API::MatrixWorkspace_sptr workspace,
                            const std::string &logName,
                            const std::string &logValue);

DLLExport bool isAlphanumericOrUnderscore(char character);

//
///// Saves grouping to the XML file specified
// DLLExport std::string groupingToXML(const Mantid::API::Grouping &grouping);

} // namespace MuonAlgorithmHelper
} // namespace Mantid

#endif /* MANTIDQT_CUSTOMINTERFACES_MUONALGORITHMHELPER_H_ */
