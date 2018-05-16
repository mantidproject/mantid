#ifndef MANTID_MUON_MUONALGORITHMHELPER_H_
#define MANTID_MUON_MUONALGORITHMHELPER_H_

#include "MantidKernel/System.h"
#include "MantidAPI/GroupingLoader.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/DateAndTime.h"

#include <string>
#include <vector>

namespace Muon {
/// Types of entities we are dealing with
enum ItemType { Pair, Group };

/// Possible plot types users might request
enum PlotType { Asymmetry, Counts, Logarithm };

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
	std::string rebinArgs;     /// Arguments for rebin (empty to not rebin)
	std::string groupPairName; /// Name of group or pair to use
	const Mantid::API::Grouping grouping; /// Grouping to use
	PlotType plotType = {};               /// Type of analysis to perform
	explicit AnalysisOptions(const Mantid::API::Grouping &g) : grouping(g) {}
};


/// Whether multiple fitting is enabled or disabled
enum class MultiFitState { Enabled, Disabled };
}

namespace MuonAlgorithmHelper {
	//
	///// Sets double validator for specified field
	//void setDoubleValidator(QLineEdit *field,bool allowEmpty = false);
	//
	/// Returns a first period MatrixWorkspace in a run workspace
	Mantid::API::MatrixWorkspace_sptr
	firstPeriod(Mantid::API::Workspace_sptr ws);
	//
	///// Validates the field and returns the value
	//double
	//getValidatedDouble(QLineEdit *field, const QString &defaultValue,
	//                   const QString &valueDescr, Mantid::Kernel::Logger &log);
	//
	///// Returns a number of periods in a run workspace
	//size_t numPeriods(Mantid::API::Workspace_sptr ws);
	//
	///// Print various information about the run
	//void
	//printRunInfo(Mantid::API::MatrixWorkspace_sptr runWs, std::ostringstream &out);
	//

	/// Get a run label for the workspace
	std::string getRunLabel(const Mantid::API::Workspace_sptr &ws);
	
	/// Get a run label for a list of workspaces
	std::string getRunLabel(const std::vector<Mantid::API::Workspace_sptr> &wsList);
	
	/// Get a run label given instrument and run numbers
	std::string getRunLabel(const std::string &instrument, const std::vector<int> &runNumbers);
	
	///// Sums a list of workspaces together
	//Mantid::API::Workspace_sptr
	//sumWorkspaces(const std::vector<Mantid::API::Workspace_sptr> &workspaces);
	
	/// Makes sure the specified workspaces are in specified group
	void groupWorkspaces(const std::string &groupName,
	                const std::vector<std::string> &inputWorkspaces);
	//
	/// Finds runs of consecutive numbers
	std::vector<std::pair<int, int>> findConsecutiveRuns(const std::vector<int> &runs);
	
	///// Replaces sample log value
	//void replaceLogValue(const std::string &wsName,
	//                                                const std::string &logName,
	//                                                const std::string &logValue);
	//
	///// Finds all of the values for a log
	//std::vector<std::string>
	//findLogValues(const Mantid::API::Workspace_sptr ws, const std::string &logName);
	//
	///// Finds the range of values for a log
	//std::pair<std::string, std::string> findLogRange(
	//    const Mantid::API::Workspace_sptr ws, const std::string &logName,
	//    bool (*isLessThan)(const std::string &first, const std::string &second));
	//
	///// Finds the range of values for a log for a vector of workspaces
	//std::pair<std::string, std::string> findLogRange(
	//    const std::vector<Mantid::API::Workspace_sptr> &workspaces,
	//    const std::string &logName,
	//    bool (*isLessThan)(const std::string &first, const std::string &second));
	//
	///// Concatenates time-series log of one workspace with the second
	//void
	//appendTimeSeriesLogs(boost::shared_ptr<Mantid::API::Workspace> toAppend,
	//                     boost::shared_ptr<Mantid::API::Workspace> resultant,
	//                     const std::string &logName);
	//
	///// Parse analysis workspace name
	//MantidQt::CustomInterfaces::Muon::DatasetParams
	//parseWorkspaceName(const std::string &wsName);


	/// Generate new analysis workspace name
	std::string generateWorkspaceName(const Muon::DatasetParams &params);


	//
	///// Get "run: period" string from workspace name
	//QString runNumberString(const std::string &workspaceName, const std::string &firstRun);
	//
	///// Parse run label into instrument and runs
	//void parseRunLabel(const std::string &label,
	//                                              std::string &instrument,
	//                                              std::vector<int> &runNumbers);

}
#endif /* MANTIDQT_CUSTOMINTERFACES_MUONALGORITHMHELPER_H_ */
