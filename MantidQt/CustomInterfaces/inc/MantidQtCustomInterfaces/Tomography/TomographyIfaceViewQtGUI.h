#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYIFACEVIEWQTGUI_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYIFACEVIEWQTGUI_H_

#include "MantidAPI/IRemoteJobManager.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/TableRow.h"
#include "MantidQtAPI/BatchAlgorithmRunner.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Tomography/ITomographyIfacePresenter.h"
#include "MantidQtCustomInterfaces/Tomography/ITomographyIfaceView.h"
#include "MantidQtCustomInterfaces/Tomography/ImageROIViewQtWidget.h"
#include "MantidQtCustomInterfaces/Tomography/ImggFormatsConvertViewQtWidget.h"
#include "MantidQtCustomInterfaces/Tomography/TomoSystemSettings.h"

#include "ui_ImageSelectCoRAndRegions.h"
#include "ui_TomographyIfaceQtGUI.h"
#include "ui_TomographyIfaceQtTabEnergy.h"
#include "ui_TomographyIfaceQtTabFiltersSettings.h"
#include "ui_TomographyIfaceQtTabRun.h"
#include "ui_TomographyIfaceQtTabSetup.h"
#include "ui_TomographyIfaceQtTabSystemSettings.h"
#include "ui_TomographyIfaceQtTabVisualize.h"
#include <boost/scoped_ptr.hpp>
#include <json/json.h>

// widgets used in this interface
class QMutex;

namespace MantidQt {
namespace API {
class BatchAlgorithmRunner;
}
}
// Qt classes forward declarations

namespace MantidQt {
namespace CustomInterfaces {

/**
Qt-based view of the Tomography GUI. Provides a concrete view for the
graphical interface for tomography functionality in Mantid. This view
is Qt-based and it is probably the only one that will be implemented
in a foreseeable horizon. The interface of this class is given by
ITomographyIfaceView so that it fits in the MVP (Model-View-Presenter)
design of the tomography GUI.

Copyright &copy; 2014-2016 ISIS Rutherford Appleton Laboratory, NScD
Oak Ridge National Laboratory & European Spallation Source

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
class MANTIDQT_CUSTOMINTERFACES_DLL TomographyIfaceViewQtGUI
    : public MantidQt::API::UserSubWindow,
      public ITomographyIfaceView {
  Q_OBJECT

public:
  /// Default Constructor
  TomographyIfaceViewQtGUI(QWidget *parent = 0);
  /// Destructor
  ~TomographyIfaceViewQtGUI() override;

  /// Interface name
  static std::string name() { return "Tomographic Reconstruction"; }
  /// This interface's categories.
  static QString categoryInfo() { return "Diffraction"; }

  void userWarning(const std::string &warn,
                   const std::string &description) override;

  void userError(const std::string &err,
                 const std::string &description) override;

  std::vector<std::string> logMsgs() const override { return m_logMsgs; }

  void setComputeResources(const std::vector<std::string> &resources,
                           const std::vector<bool> &enabled) override;

  void setReconstructionTools(const std::vector<std::string> &tools,
                              const std::vector<bool> &enabled) override;

  std::string experimentReference() const override {
    return m_setupExperimentRef;
  }

  std::string getUsername() const override;

  std::string getPassword() const override;

  void updateLoginControls(bool loggedIn) override;

  void enableLoggedActions(bool enable) override;

  /// possible for the user to define the configuration of a tool
  void enableConfigTool(bool on) override;

  /// possible for the user to run / submit a job
  void enableRunReconstruct(bool on) override;

  void updateCompResourceStatus(bool online);

  void updateJobsInfoDisplay(
      const std::vector<Mantid::API::IRemoteJobManager::RemoteJobInfo> &status,
      const std::vector<Mantid::API::IRemoteJobManager::RemoteJobInfo> &
          localStatus) override;

  std::vector<std::string> processingJobsIDs() const override {
    return m_processingJobsIDs;
  }

  TomoSystemSettings systemSettings() const override;

  TomoReconFiltersSettings prePostProcSettings() const override;

  std::string currentComputeResource() const override {
    return m_currentComputeRes;
  }
  std::string currentReconTool() const override { return m_currentReconTool; }

  /// get the path to the image that the user has requested to visualize
  std::string visImagePath() const;

  std::string showImagePath() const override { return m_imgPath; }
  void showImage(const Mantid::API::MatrixWorkspace_sptr &wsg) override;
  void showImage(const std::string &path) override;

  int keepAlivePeriod() override { return m_settings.useKeepAlive; }

  TomoPathsConfig currentPathsConfig() const override { return m_pathsConfig; }

  ImageStackPreParams currentROIEtcParams() const override {
    return m_tabROIW->userSelection();
  }

  std::map<std::string, std::string>
  currentAggregateBandsParams() const override {
    return grabCurrentAggParams();
  }

  void runAggregateBands(Mantid::API::IAlgorithm_sptr alg) override;

  bool userConfirmation(const std::string &title,
                        const std::string &body) override;

private slots:
  /// for buttons, run tab, and similar
  void reconstructClicked();
  void toolSetupClicked();
  void runVisualizeClicked();
  void jobCancelClicked();
  void jobTableRefreshClicked();
  void updatedExperimentReference();

  void compResourceIndexChanged(int);
  void runToolIndexChanged(int);
  void SCARFLoginClicked();
  void SCARFLogoutClicked();

  void browseImageClicked();

  void browseLocalInOutDirClicked();
  void browseLocalRemoteDriveOrPath();
  void browseLocalReconScriptsDirClicked();
  void browseLocalExternalInterpreterClicked();

  void flatsPathCheckStatusChanged(int status);
  void darksPathCheckStatusChanged(int status);

  void samplesPathBrowseClicked();
  void flatsPathBrowseClicked();
  void darksPathBrowseClicked();

  void samplesPathEditedByUser();
  void flatsPathEditedByUser();
  void darksPathEditedByUser();

  /// For the filters tab
  void resetPrePostFilters();

  /// open the MantidQT help window for this interface
  void openHelpWin();

  // visualization tools / short-cuts
  void browseFilesToVisualizeClicked();
  void sendToParaviewClicked();
  void sendToOctopusVisClicked();
  void defaultDirLocalVisualizeClicked();
  void defaultDirRemoteVisualizeClicked();
  void browseVisToolParaviewClicked();
  void browseVisToolOctopusClicked();

  // processing of energy bands
  void browseEnergyInputClicked();
  void browseEnergyOutputClicked();

  void systemSettingsEdited();
  void systemSettingsNumericEdited();

  // part of the system / advanced settings
  void resetRemoteSetup();

  // reset all system / advanced settings
  void resetSystemSettings();

  // start aggregation of energy/wavelength bands
  void pushButtonAggClicked();
  void browseAggScriptClicked();
  // aggregation run finished
  void finishedAggBands(bool error);

private:
  /// Setup the interface (tab UI)
  void initLayout() override;

  void doSetupSectionSetup();
  void doSetupSectionRun();
  void doSetupSectionFilters();
  void doSetupSectionVisualize();
  void doSetupSectionEnergy();
  void doSetupSectionSystemSettings();
  void doSetupGeneralWidgets();

  /// Load default interface settings for each tab, normally on startup
  void readSettings();
  /// for the energy bands tab/widget
  void readSettingsEnergy();
  /// save settings (before closing)
  void saveSettings() const override;
  /// for the energy bands tab/widget
  void saveSettingsEnergy() const;

  void updateSystemSettingsTabFields(const TomoSystemSettings &setts);

  void updatePathsConfig(const TomoPathsConfig &cfg) override;

  void showToolConfig(
      MantidQt::CustomInterfaces::TomoToolConfigDialogBase &dialog) override;

  void closeEvent(QCloseEvent *ev) override;

  void processPathBrowseClick(QLineEdit *le, std::string &data);

  void updateFlatsDarksFromSamplePath(const std::string &path);

  TomoSystemSettings grabSystemSettingsFromUser() const;

  TomoReconFiltersSettings grabPrePostProcSettings() const;

  void setPrePostProcSettings(const TomoReconFiltersSettings &opts);

  std::string
  checkUserBrowseDir(QLineEdit *le,
                     const std::string &userMsg = "Open directory/folder",
                     bool remember = true);

  std::string checkUserBrowseFile(QLineEdit *le,
                                  const std::string &userMsg = "Open file",
                                  bool remember = true);

  std::string checkDefaultVisualizeDir(const std::string &basePath,
                                       const std::string &appendComp);

  void sendToVisTool(const std::string &toolName, const std::string &pathString,
                     const std::string &appendBin);

  std::map<std::string, std::string> grabCurrentAggParams() const;

  void sendLog(const std::string &msg);

  static const std::string g_styleSheetOffline;
  static const std::string g_styleSheetOnline;

  /// Interface definition with widgets for the main interface window
  Ui::TomographyIfaceQtGUI m_ui;
  // And its sections/tabs. Note that for compactness they're called simply
  // 'tabs' but they could be separate dialogs, widgets, etc. combined in
  // different ways.
  Ui::TomographyIfaceQtTabRun m_uiTabRun;
  Ui::TomographyIfaceQtTabSetup m_uiTabSetup;
  Ui::TomographyIfaceQtTabFiltersSettings m_uiTabFilters;
  Ui::ImageSelectCoRAndRegions m_uiTabCoR;
  Ui::TomographyIfaceQtTabVisualize m_uiTabVisualize;
  Ui::TomographyIfaceQtTabEnergy m_uiTabEnergy;
  Ui::TomographyIfaceQtTabSystemSettings m_uiTabSystemSettings;

  ImageROIViewQtWidget *m_tabROIW;
  ImggFormatsConvertViewQtWidget *m_tabImggFormats;

  std::vector<std::string> m_processingJobsIDs;

  std::string m_currentComputeRes;
  std::string m_currentReconTool;

  std::string m_imgPath;

  std::vector<Mantid::API::IRemoteJobManager::RemoteJobInfo> m_localJobsStatus;

  // Settings for external tools. where to find the system Python
  static std::vector<std::string> g_defAddPathPython;
  std::vector<std::string> m_defAddPathPython;

  static const std::string g_SCARFName;
  // a general (all tools in principle) default output path
  static const std::string g_defOutPathLocal;
  static const std::string g_defOutPathRemote;

  TomoPathsConfig m_pathsConfig;

  // several paths or path components related to where the files are found
  // (raw files, reconstructions, pre-post processed files, etc.)
  // These are the defaults
  static const std::string g_defPathComponentPhase;
  static const std::string g_defExperimentRef;
  // reconstruction scripts (external, but shipped with Mantid)
  static const std::string g_defPathReconScripts;
  // base dir for the reconstruction outputs
  static const std::string g_defPathReconOut;
  static const std::string g_defParaviewPath;
  static const std::string g_defParaviewAppendPath;
  static const std::string g_defOctopusVisPath;
  static const std::string g_defOctopusAppendPath;
  static const std::string g_defProcessedSubpath;
  // And these are the paths set up
  std::string m_setupPathComponentPhase;
  std::string m_setupExperimentRef;
  std::string m_setupParaviewPath;
  std::string m_setupOctopusVisPath;
  std::string m_setupProcessedSubpath;

  // here the view puts messages before notifying the presenter to show them
  std::vector<std::string> m_logMsgs;

  /// The not-so-small set of paths, path compnents and related parameters for
  /// the local and remote machines
  TomoSystemSettings m_systemSettings;

  // Basic representation of user settings, read/written on startup/close.
  // TODO: this could be done more sophisticated, with a class using
  // QDataStream and in/out stream operators for example. Keeping
  // it simple for now
  struct UserSettings {
    std::string SCARFBasePath;
    int useKeepAlive; // use if >0, number of seconds for a periodic query
    bool onCloseAskForConfirmation;

    UserSettings()
        : SCARFBasePath("/work/imat/runs/test/"), useKeepAlive(60),
          onCloseAskForConfirmation(false) {}
  };
  UserSettings m_settings;

  // path name for persistent settings
  std::string m_settingsGroup;
  std::string m_settingsSubGroupEnergy;

  // To run aggregation of wavelength/energy bands
  std::unique_ptr<MantidQt::API::BatchAlgorithmRunner> m_aggAlgRunner;

  // TODO? move to TomographyIfaceModel or TomographyIfaceSavuModel.h
  // plugins for Savu config files
  Mantid::API::ITableWorkspace_sptr m_availPlugins;
  Mantid::API::ITableWorkspace_sptr m_currPlugins;
  std::string m_currentParamPath;

  // presenter as in the model-view-presenter
  boost::scoped_ptr<ITomographyIfacePresenter> m_presenter;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYIFACEVIEWQTGUI_H_
