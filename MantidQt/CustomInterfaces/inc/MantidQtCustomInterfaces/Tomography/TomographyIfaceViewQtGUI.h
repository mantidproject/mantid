#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYIFACEVIEWQTGUI_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYIFACEVIEWQTGUI_H_

#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/TableRow.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Tomography/ITomographyIfacePresenter.h"
#include "MantidQtCustomInterfaces/Tomography/ITomographyIfaceView.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialog.h"
#include "ui_TomographyIfaceQtGUI.h"
#include "ui_TomographyIfaceQtTabSetup.h"
#include "ui_TomographyIfaceQtTabRun.h"

#include <boost/scoped_ptr.hpp>
#include <json/json.h>

// Qt classes forward declarations
class QMutex;

namespace MantidQt {
namespace CustomInterfaces {

/**
Qt-based view of the Tomography GUI. Provides a concrete view for the
graphical interface for tomography functionality in Mantid. This view
is Qt-based and it is probably the only one that will be implemented
in a foreseeable horizon. The interface of this class is given by
ITomographyIfaceView so that it fits in the MVP (Model-View-Presenter)
design of the tomography GUI.

Copyright &copy; 2014,205 ISIS Rutherford Appleton Laboratory, NScD
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
  virtual ~TomographyIfaceViewQtGUI();

  /// Interface name
  static std::string name() { return "Tomographic Reconstruction"; }
  /// This interface's categories.
  static QString categoryInfo() { return "Diffraction"; }

  void userWarning(const std::string &warn, const std::string &description);

  void userError(const std::string &err, const std::string &description);

  std::vector<std::string> logMsgs() const { return m_logMsgs; }

  void setComputeResources(const std::vector<std::string> &resources,
                           const std::vector<bool> &enabled);

  void setReconstructionTools(const std::vector<std::string> &tools,
                              const std::vector<bool> &enabled);

  std::string getUsername() const;

  std::string getPassword() const;

  void updateLoginControls(bool loggedIn);

  void enableLoggedActions(bool enable);

  /// possible for the user to define the configuration of a tool
  void enableConfigTool(bool on);

  /// possible for the user to run / submit a job
  void enableRunReconstruct(bool on);

  void updateCompResourceStatus(bool online);

  void updateJobsInfoDisplay(
      const std::vector<Mantid::API::IRemoteJobManager::RemoteJobInfo> &status);

  std::vector<std::string> processingJobsIDs() const {
    return m_processingJobsIDs;
  }

  /// Get the current reconstruction tooll settings set by the user
  TomoReconToolsUserSettings reconToolsSettings() const {
    return m_toolsSettings;
  }

  std::string currentComputeResource() const { return m_currentComputeRes; }
  std::string currentReconTool() const { return m_currentReconTool; }

  /// get the path to the image that the user has requested to visualize
  std::string visImagePath() const;

  std::string showImagePath() const { return m_imgPath; }
  void showImage(const Mantid::API::MatrixWorkspace_sptr &wsg);
  void showImage(const std::string &path);

  int keepAlivePeriod() { return m_settings.useKeepAlive; }

  TomoPathsConfig currentPathsConfig() const { return m_pathsConfig; }

private slots:
  /// for buttons, run tab, and similar
  void reconstructClicked();
  void toolSetupClicked();
  void runVisualizeClicked();
  void jobCancelClicked();
  void jobTableRefreshClicked();

  void compResourceIndexChanged(int);
  void runToolIndexChanged(int);
  void SCARFLoginClicked();
  void SCARFLogoutClicked();

  void browseImageClicked();

  void fitsPathBrowseClicked();
  void flatPathBrowseClicked();
  void darkPathBrowseClicked();

  /// open the MantidQT help window for this interface
  void openHelpWin();

  void menuSaveClicked();
  void menuSaveAsClicked();
  void availablePluginSelected();
  void currentPluginSelected();
  void transferClicked();
  void moveUpClicked();
  void moveDownClicked();
  void removeClicked();
  void menuOpenClicked();
  void paramValModified(QTreeWidgetItem *, int);
  void expandedItem(QTreeWidgetItem *);

private:
  /// Setup the interface (tab UI)
  virtual void initLayout();

  void doSetupSectionSetup();
  void doSetupSectionRun();
  void doSetupGeneralWidgets();
  void doSetupSavu();

  /// Load default interface settings for each tab, normally on startup
  void readSettings();
  /// save settings (before closing)
  void saveSettings() const;

  void showToolConfig(const std::string &name);

  virtual void closeEvent(QCloseEvent *ev);

  void processPathBrowseClick(QLineEdit *le, std::string &data);

  // Begin of Savu related functionality. This will grow and will need
  // separation

  /// to load plugins (savu classification / API)
  void loadAvailablePlugins();

  /// refresh the list/tree of savu plugins
  void refreshAvailablePluginListUI();

  void refreshCurrentPluginListUI();

  /// make a tree entry from a row of a table of savu plugins
  void createPluginTreeEntry(Mantid::API::TableRow &row);
  void createPluginTreeEntries(Mantid::API::ITableWorkspace_sptr table);

  std::string createUniqueNameHidden();

  QString tableWSRowToString(Mantid::API::ITableWorkspace_sptr table, size_t i);

  void loadSavuTomoConfig(std::string &filePath,
                          Mantid::API::ITableWorkspace_sptr &currentPlugins);

  std::string paramValStringFromArray(const Json::Value &jsonVal,
                                      const std::string &name);
  std::string pluginParamValString(const Json::Value &jsonVal,
                                   const std::string &name);

  static size_t g_nameSeqNo;

  // end of Savu related methods

  /// Interface definition with widgets for the main interface window
  Ui::TomographyIfaceQtGUI m_ui;
  // And its sections/tabs. Note that for compactness they're called simply
  // 'tabs'
  // but they could be separate dialogs, widgets, etc.
  Ui::TomographyIfaceQtTabSetup m_uiTabSetup;
  Ui::TomographyIfaceQtTabRun m_uiTabRun;

  /// Tool specific setup dialogs
  Ui::TomoToolConfigAstra m_uiAstra;
  Ui::TomoToolConfigCustom m_uiCustom;
  Ui::TomoToolConfigSavu m_uiSavu;
  Ui::TomoToolConfigTomoPy m_uiTomoPy;

  std::vector<std::string> m_processingJobsIDs;

  std::string m_currentComputeRes;
  std::string m_currentReconTool;

  std::string m_imgPath;

  static const std::string g_SCARFName;
  // a general (all tools in principle) default output path
  static const std::string g_defOutPath;

  static const std::string g_TomoPyTool;
  static const std::string g_AstraTool;
  static const std::string g_CCPiTool;
  static const std::string g_SavuTool;
  static const std::string g_customCmdTool;

  TomoPathsConfig m_pathsConfig;

  // here the view puts messages before notifying the presenter to show them
  std::vector<std::string> m_logMsgs;

  /// Settings for the third party (tomographic reconstruction) tools
  TomoReconToolsUserSettings m_toolsSettings;

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
