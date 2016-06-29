#ifndef MANTID_CUSTOMINTERFACES_MUONANALYSISFITDATAPRESENTER_H_
#define MANTID_CUSTOMINTERFACES_MUONANALYSISFITDATAPRESENTER_H_

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Muon/MuonAnalysisDataLoader.h"
#include "MantidQtCustomInterfaces/Muon/MuonAnalysisHelper.h"
#include "MantidQtCustomInterfaces/Muon/MuonAnalysisOptionTab.h"
#include "MantidQtMantidWidgets/IMuonFitDataSelector.h"
#include "MantidQtMantidWidgets/IWorkspaceFitControl.h"

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
class MANTIDQT_CUSTOMINTERFACES_DLL MuonAnalysisFitDataPresenter {
public:
  /// Constructor
  MuonAnalysisFitDataPresenter(
      MantidQt::MantidWidgets::IWorkspaceFitControl *fitBrowser,
      MantidQt::MantidWidgets::IMuonFitDataSelector *dataSelector,
      MuonAnalysisDataLoader &dataLoader, double timeZero = 0,
      RebinOptions &rebinArgs =
          RebinOptions(MantidQt::CustomInterfaces::Muon::MuonAnalysisOptionTab::
                           RebinType::NoRebin,
                       ""));
  /// Handles "data properties changed"
  void handleDataPropertiesChanged();
  /// Handles "selected data changed"
  void handleSelectedDataChanged(
      const Mantid::API::Grouping &grouping,
      const MantidQt::CustomInterfaces::Muon::PlotType &plotType,
      bool overwrite);
  /// Handles user changing X range by dragging lines
  void handleXRangeChangedGraphically(double start, double end);
  /// Handles peak picker being reassigned to a new graph
  void setAssignedFirstRun(const QString &wsName);
  /// Get the workspace the peak picker is currently assigned to
  QString getAssignedFirstRun() const { return m_PPAssignedFirstRun; };
  /// Change the stored time zero
  void setTimeZero(double timeZero) { m_timeZero = timeZero; }
  /// Change the stored rebin args
  void setRebinArgs(const RebinOptions &rebinArgs) { m_rebinArgs = rebinArgs; }
  /// Handles label for simultaneous fit results being changed
  void handleSimultaneousFitLabelChanged() const;
  /// Transforms fit results when a simultaneous fit finishes
  void handleFitFinished() const;

private:
  /// Create workspaces to fit and update fit browser (model)
  void createWorkspacesToFit(const std::vector<std::string> &names,
                             const Mantid::API::Grouping &grouping) const;
  /// Generate names of workspaces to be created
  std::vector<std::string> generateWorkspaceNames(
      const Mantid::API::Grouping &grouping,
      const MantidQt::CustomInterfaces::Muon::PlotType &plotType,
      bool overwrite) const;
  /// Create analysis workspace
  Mantid::API::Workspace_sptr
  createWorkspace(const std::string &name,
                  const Mantid::API::Grouping &grouping) const;
  /// Get rebin options for analysis
  std::string getRebinParams(const Mantid::API::Workspace_sptr ws) const;
  /// Rename fit workspaces
  void renameFittedWorkspaces(const std::string &groupName) const;
  /// Extract workspaces from group and move up a level
  void extractFittedWorkspaces(const std::string &groupName) const;
  /// Add special logs to fitted workspaces
  void addSpecialLogs(
      const std::string &wsName,
      const MantidQt::CustomInterfaces::Muon::DatasetParams &wsParams) const;
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
};
} // namespace CustomInterfaces
} // namespace Mantid

#endif /* MANTID_CUSTOMINTERFACES_MUONANALYSISFITDATAPRESENTER_H_ */