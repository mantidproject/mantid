#ifndef MANTIDQT_CUSTOMINTERFACES_MUONANALYSISDATALOADER_H_
#define MANTIDQT_CUSTOMINTERFACES_MUONANALYSISDATALOADER_H_

#include "DllConfig.h"
#include "MuonAnalysisHelper.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/GroupingLoader.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include <QMap>
#include <QRegExp>
#include <QStringList>

namespace MantidQt {
namespace CustomInterfaces {
namespace Muon {
/// Ways to deal with dead time correction
enum class DeadTimesType { None, FromFile, FromDisk };

/// Data loaded from file
struct LoadResult {
  Mantid::API::Workspace_sptr loadedWorkspace;
  Mantid::API::Workspace_sptr loadedGrouping;
  Mantid::API::Workspace_sptr loadedDeadTimes;
  std::string mainFieldDirection;
  double timeZero = 0;
  double firstGoodData = 0;
  std::string label;
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
  std::string wsName = "";               /// Name of the ws
  explicit AnalysisOptions(const Mantid::API::Grouping &g) : grouping(g) {}
};
} // namespace Muon

/** MuonAnalysisDataLoader : Loads and processes muon data for MuonAnalysis

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTIDQT_MUONINTERFACE_DLL MuonAnalysisDataLoader {
public:
  /// constructor
  MuonAnalysisDataLoader(const Muon::DeadTimesType &deadTimesType,
                         const QStringList &instruments,
                         const std::string &deadTimesFile = "");
  /// change dead times type
  void setDeadTimesType(const Muon::DeadTimesType &deadTimesType,
                        const std::string &deadTimesFile = "");
  /// change list of supported instruments
  void setSupportedInstruments(const QStringList &instruments);

  /// load files
  Muon::LoadResult loadFiles(const QStringList &files) const;
  /// correct and group loaded data
  Mantid::API::Workspace_sptr
  correctAndGroup(const Muon::LoadResult &loadedData,
                  const Mantid::API::Grouping &grouping) const;
  /// create analysis workspace
  Mantid::API::Workspace_sptr
  createAnalysisWorkspace(const Mantid::API::Workspace_sptr inputWS,
                          const Muon::AnalysisOptions &options) const;
  /// Get dead time table
  Mantid::API::ITableWorkspace_sptr
  getDeadTimesTable(const Muon::LoadResult &loadedData) const;
  /// Load dead times from file
  Mantid::API::Workspace_sptr
  loadDeadTimesFromFile(const std::string &filename) const;
  // empty the cache
  void clearCache();
  // Find if name is in group/pair collection
  static bool isContainedIn(const std::string &name,
                            const std::vector<std::string> &collection) {
    return std::find(collection.begin(), collection.end(), name) !=
           collection.end();
  };

protected:
  /// Set properties of algorithm from options
  void
  setProcessAlgorithmProperties(Mantid::API::IAlgorithm_sptr alg,
                                const Muon::AnalysisOptions &options) const;
  /// Remove from cache any workspaces that have been deleted in the meantime
  void updateCache() const;

private:
  /// Get instrument name from workspace
  std::string
  getInstrumentName(const Mantid::API::Workspace_sptr workspace) const;
  /// Check if we should cache result of a load of the given files
  bool shouldBeCached(const QStringList &filenames) const;

  /// Dead times type
  Muon::DeadTimesType m_deadTimesType;
  /// Dead times file
  std::string m_deadTimesFile;
  /// Muon instruments supported
  QStringList m_instruments;
  /// Cache of previously loaded data
  mutable std::map<std::string, Muon::LoadResult> m_loadedDataCache;
  /// Regex blacklisting certain files from being cached
  QRegExp m_cacheBlacklist;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQt_CUSTOMINTERFACES_MUONANALYSISDATALOADER_H_ */
