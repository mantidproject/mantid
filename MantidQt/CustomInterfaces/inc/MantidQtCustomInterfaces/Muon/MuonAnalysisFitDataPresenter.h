#ifndef MANTID_CUSTOMINTERFACES_MUONANALYSISFITDATAPRESENTER_H_
#define MANTID_CUSTOMINTERFACES_MUONANALYSISFITDATAPRESENTER_H_

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Muon/MuonAnalysisDataLoader.h"
#include "MantidQtCustomInterfaces/Muon/MuonAnalysisHelper.h"
#include "MantidQtCustomInterfaces/Muon/MuonAnalysisOptionTab.h"
#include "MantidQtMantidWidgets/IMuonFitDataSelector.h"
#include "MantidQtMantidWidgets/IWorkspaceFitControl.h"
#include <QObject>

/// Save some typing
typedef std::pair<
    MantidQt::CustomInterfaces::Muon::MuonAnalysisOptionTab::RebinType,
    std::string> RebinOptions;

namespace Mantid {
namespace API {
class Grouping;
}
}

namespace MantidQt {
namespace CustomInterfaces {

/** MuonAnalysisFitDataPresenter : Updates fit browser from data widget

  When data widget (View) reports changes, MuonAnalysis uses this presenter
  class to update the fit browser (Model).

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
class MANTIDQT_CUSTOMINTERFACES_DLL MuonAnalysisFitDataPresenter
    : public QObject {
  Q_OBJECT
public:
  /// Constructor overload with default arguments
  MuonAnalysisFitDataPresenter(
      MantidQt::MantidWidgets::IWorkspaceFitControl *fitBrowser,
      MantidQt::MantidWidgets::IMuonFitDataSelector *dataSelector,
      MuonAnalysisDataLoader &dataLoader, const Mantid::API::Grouping &grouping,
      const Muon::PlotType &plotType);
  /// Constructor overload with default argument
  MuonAnalysisFitDataPresenter(
      MantidQt::MantidWidgets::IWorkspaceFitControl *fitBrowser,
      MantidQt::MantidWidgets::IMuonFitDataSelector *dataSelector,
      MuonAnalysisDataLoader &dataLoader, const Mantid::API::Grouping &grouping,
      const Muon::PlotType &plotType, double timeZero);
  /// Constructor
  MuonAnalysisFitDataPresenter(
      MantidQt::MantidWidgets::IWorkspaceFitControl *fitBrowser,
      MantidQt::MantidWidgets::IMuonFitDataSelector *dataSelector,
      MuonAnalysisDataLoader &dataLoader, const Mantid::API::Grouping &grouping,
      const Muon::PlotType &plotType, double timeZero,
      const RebinOptions &rebinArgs);
  /// Handles "selected data changed"
  void handleSelectedDataChanged(bool overwrite);
  /// Handles peak picker being reassigned to a new graph
  void setAssignedFirstRun(const QString &wsName);
  /// Get the workspace the peak picker is currently assigned to
  QString getAssignedFirstRun() const { return m_PPAssignedFirstRun; };
  /// Change the stored time zero
  void setTimeZero(double timeZero) { m_timeZero = timeZero; }
  /// Change the stored rebin args
  void setRebinArgs(const RebinOptions &rebinArgs) { m_rebinArgs = rebinArgs; }
  /// Generate names of workspaces to be created
  std::vector<std::string> generateWorkspaceNames(const std::string &instrument,
                                                  const std::string &runString,
                                                  bool overwrite) const;
  /// Update the stored grouping
  void setGrouping(const Mantid::API::Grouping &grouping) {
    m_grouping = grouping;
  }
  /// Update the stored plot type
  void setPlotType(const Muon::PlotType &plotType) { m_plotType = plotType; }
  /// Create workspaces to fit
  void createWorkspacesToFit(const std::vector<std::string> &names) const;
  /// Rename fit workspaces, add logs and generate params table
  void handleFittedWorkspaces(const std::string &baseName,
                              const std::string &groupName = "") const;
  /// Extract workspaces from group and move up a level
  void extractFittedWorkspaces(const std::string &baseName,
                               const std::string &groupName = "") const;
  /// Set selected workspace
  void setSelectedWorkspace(const QString &wsName);

public slots:
  /// Transforms fit results when a simultaneous fit finishes
  void handleFitFinished(const QString &status = QString("success")) const;
  /// Handles "data properties changed"
  void handleDataPropertiesChanged();
  /// Handles user changing X range by dragging lines
  void handleXRangeChangedGraphically(double start, double end);
  /// Handles label for simultaneous fit results being changed
  void handleSimultaneousFitLabelChanged() const;
  /// Handles user changing selected dataset to display
  void handleDatasetIndexChanged(int index);
  /// Open sequential fit dialog
  void openSequentialFitDialog();
  /// Updates label to avoid overwriting existing results
  void checkAndUpdateFitLabel(bool sequentialFit);

private:
  /// Generate names of workspaces to be created
  std::vector<std::string> generateWorkspaceNames(bool overwrite) const;
  /// Create analysis workspace
  Mantid::API::Workspace_sptr createWorkspace(const std::string &name,
                                              std::string &groupLabel) const;
  /// Update model and view with names of workspaces to fit
  void updateWorkspaceNames(const std::vector<std::string> &names) const;
  /// Get rebin options for analysis
  std::string getRebinParams(const Mantid::API::Workspace_sptr ws) const;
  /// Add special logs to fitted workspaces
  void addSpecialLogs(
      const std::string &wsName,
      const MantidQt::CustomInterfaces::Muon::DatasetParams &wsParams) const;
  /// Split parameters table into one for this dataset
  Mantid::API::ITableWorkspace_sptr generateParametersTable(
      const std::string &wsName,
      const Mantid::API::ITableWorkspace_sptr inputTable) const;
  /// Set up connections
  void doConnect();
  /// Checks if current fit is simultaneous
  bool isSimultaneousFit() const;
  /// Set up UI based on workspace
  void setUpDataSelector(const QString &wsName);
  /// Check if multiple runs are selected
  bool isMultipleRuns() const;
  /// Fit browser to update (non-owning pointer)
  MantidQt::MantidWidgets::IWorkspaceFitControl *m_fitBrowser;
  /// Data selector to get input from (non-owning pointer)
  MantidQt::MantidWidgets::IMuonFitDataSelector *m_dataSelector;
  /// Workspace assigned to peak picker
  QString m_PPAssignedFirstRun;
  /// Loader to load and analyse data (reference)
  MuonAnalysisDataLoader &m_dataLoader;
  /// Stored time zero
  double m_timeZero;
  /// Stored rebin args
  RebinOptions m_rebinArgs;
  /// Stored grouping
  Mantid::API::Grouping m_grouping;
  /// Stored plot type
  Muon::PlotType m_plotType;
};
} // namespace CustomInterfaces
} // namespace Mantid

#endif /* MANTID_CUSTOMINTERFACES_MUONANALYSISFITDATAPRESENTER_H_ */