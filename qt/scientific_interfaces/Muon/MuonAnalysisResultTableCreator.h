#ifndef MANTIDQT_CUSTOMINTERFACES_MUONANALYSISRESULTTABLECREATOR_H_
#define MANTIDQT_CUSTOMINTERFACES_MUONANALYSISRESULTTABLECREATOR_H_

#include "DllConfig.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

#include <QStringList>
#include <map>

namespace MantidQt {
namespace CustomInterfaces {
using WSParameterList = QMap<QString, QMap<QString, double>>;
using LogValuesMap = QMap<QString, QMap<QString, QVariant>>;

/** MuonAnalysisResultTableCreator : Creates table of muon fit results
  Used in the "result table" tab of Muon Analysis interface

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
class MANTIDQT_MUONINTERFACE_DLL MuonAnalysisResultTableCreator {
public:
  /// Constructor
  MuonAnalysisResultTableCreator(const QStringList &itemsSelected,
                                 const QStringList &logsSelected,
                                 const LogValuesMap *logValues,
                                 bool multipleLabels = false);

  /// Create results table
  Mantid::API::ITableWorkspace_sptr createTable() const;

protected:
  /// Check if fit tables have same parameters
  bool haveSameParameters(
      const std::vector<Mantid::API::ITableWorkspace_sptr> &tables) const;
  /// Remove error columns for fixed parameters from a results table
  void removeFixedParameterErrors(
      const Mantid::API::ITableWorkspace_sptr table) const;

private:
  /// Get map of label to workspaces
  std::map<QString, std::vector<std::string>> getWorkspacesByLabel() const;
  /// Get parameter table from workspace/label name
  Mantid::API::ITableWorkspace_sptr
  getFitParametersTable(const QString &name) const;
  /// Get parameter table from workspace name
  Mantid::API::ITableWorkspace_sptr
  tableFromWorkspace(const std::string &wsName) const;
  /// Get parameter table from label name
  Mantid::API::ITableWorkspace_sptr
  tableFromLabel(const std::string &labelName) const;
  /// Check workspaces have same parameters
  void checkSameFitModel() const;
  /// Check labels have same number of runs
  void checkSameNumberOfDatasets(
      const std::map<QString, std::vector<std::string>> &workspacesByLabel)
      const;
  /// Get start time of first run
  int64_t getFirstStartTimeNanosec(
      const std::map<QString, std::vector<std::string>> &workspacesByLabel)
      const;
  /// Get parameters by label
  QMap<QString, WSParameterList> getParametersByLabel(
      const std::map<QString, std::vector<std::string>> &workspacesByLabel)
      const;
  /// Add parameter and error columns to table
  QStringList addParameterColumns(
      Mantid::API::ITableWorkspace_sptr &table,
      const QMap<QString, WSParameterList> &paramsByLabel) const;
  /// Finds if a parameter in a fit was global
  bool isGlobal(const QString &param,
                const QMap<QString, WSParameterList> &paramList) const;
  bool isGlobal(const QString &param, const WSParameterList &paramList) const;
  /// Write data to table
  void writeData(Mantid::API::ITableWorkspace_sptr &table,
                 const QMap<QString, WSParameterList> &paramsByLabel,
                 const QStringList &paramsToDisplay) const;
  /// Write data to table for single fit
  void
  writeDataForSingleFit(Mantid::API::ITableWorkspace_sptr &table,
                        const QMap<QString, WSParameterList> &paramsByLabel,
                        const QStringList &paramsToDisplay) const;
  /// Write data to table for multiple fits
  void
  writeDataForMultipleFits(Mantid::API::ITableWorkspace_sptr &table,
                           const QMap<QString, WSParameterList> &paramsByLabel,
                           const QStringList &paramsToDisplay) const;

  void
  addColumnToResultsTable(Mantid::API::ITableWorkspace_sptr &table,
                          const QMap<QString, WSParameterList> &paramsByLabel,
                          const QString &log) const;

  /// Items selected by user (fitted workspaces or fit labels)
  const QStringList m_items;
  /// Log names selected by used
  const QStringList m_logs;
  /// Pointer to data structure containing log data
  const LogValuesMap *m_logValues;
  /// Whether multiple fit labels (true) or just one fit (false)
  const bool m_multiple;
  /// Cached start time of first run in nanoseconds
  mutable int64_t m_firstStart_ns;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_MUONANALYSISRESULTTABLECREATOR_H_ */