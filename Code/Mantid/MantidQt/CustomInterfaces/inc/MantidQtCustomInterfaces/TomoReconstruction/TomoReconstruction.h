#ifndef MANTIDQTCUSTOMINTERFACES_TOMORECONSTRUCTION_H_
#define MANTIDQTCUSTOMINTERFACES_TOMORECONSTRUCTION_H_

#include "MantidAPI/IRemoteJobManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/TableRow.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "ui_TomoReconstruction.h"
#include "ui_TomoToolConfigAstra.h"
#include "ui_TomoToolConfigCustom.h"
#include "ui_TomoToolConfigSavu.h"
#include "ui_TomoToolConfigTomoPy.h"

#include <jsoncpp/json/json.h>
#include <QDialog>
#include <QMutex>

// Qt classes forward declarations
class QLineEdit;
class QThread;
class QTimer;
class QTreeWidgetItem;

namespace MantidQt {
namespace CustomInterfaces {
/**
Tomographic reconstruction GUI. Interface for editing parameters,
running and monitoring reconstruction jobs, quick image inspection,
launching visualization windows, etc.

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
class DLLExport TomoReconstruction : public MantidQt::API::UserSubWindow {
  Q_OBJECT

public: // public constructor, destructor and functions
  /// Default Constructor
  TomoReconstruction(QWidget *parent = 0);
  /// Destructor
  virtual ~TomoReconstruction();
  /// Interface name
  static std::string name() { return "Tomographic Reconstruction"; }
  /// This interface's categories.
  static QString categoryInfo() { return "Diffraction"; }
  /// Setup tab UI
  virtual void initLayout();

public slots:
  void periodicStatusUpdateRequested();

  /// for buttons, run tab, and similar
  void reconstructClicked();
  void toolSetupClicked();
  void runVisualizeClicked();
  void jobCancelClicked();
  void jobTableRefreshClicked();
  void getJobStatusInfo();

protected:
  bool doPing();
  void doLogin(const std::string &pw);
  void doLogout();
  void doQueryJobStatus(std::vector<std::string> &ids,
                        std::vector<std::string> &names,
                        std::vector<std::string> &status,
                        std::vector<std::string> &cmds);
  void doSubmitReconstructionJob();
  void doCancelJob(const std::string &id);

  void updateJobsTable();

  void cleanup();

  void makeRunnableWithOptions(std::string &run, std::string &opt);
  std::string getComputeResource();
  std::string getUsername();
  std::string getPassword();

  // current paths set by the user
  std::string currentPathSCARF();
  std::string currentPathFITS();
  std::string currentPathFlat();
  std::string currentPathDark();
  std::string currentPathSavuConfig();

private slots:
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
  void doSetupSectionSetup();
  void doSetupSectionParameters();
  void doSetupSectionRun();
  void doSetupGeneralWidgets();

  void setupComputeResource();
  void setupRunTool();

  void enableLoggedActions(bool enable);
  void updateCompResourceStatus(bool online);

  void processPathBrowseClick(QLineEdit *le, std::string &data);

  /// Show a tool specific configuration dialog
  void showToolConfig(const std::string &name);

  std::string validateCompResource(const std::string &res);

  Mantid::API::WorkspaceGroup_sptr loadFITSImage(const std::string &path);

  void drawImage(const Mantid::API::MatrixWorkspace_sptr &ws);

  void userWarning(std::string err, std::string description);

  void userError(std::string err, std::string description);

  /// Load default interface settings for each tab, normally on startup
  void readSettings();
  /// save settings (before closing)
  void saveSettings();

  /// Starts a periodic query just to keep sessions alive when logged in
  void startKeepAliveMechanism(int period);
  /// Stops/kills the periodic query (for example if the user logs out)
  void killKeepAliveMechanism();

  virtual void closeEvent(QCloseEvent *ev);

  std::string paramValStringFromArray(const Json::Value &jsonVal,
                                      const std::string &name);
  std::string pluginParamValString(const Json::Value &jsonVal,
                                   const std::string &name);

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

  /// Main interface window
  Ui::TomoReconstruction m_ui;
  Ui::TomoToolConfigAstra m_uiAstra;
  Ui::TomoToolConfigCustom m_uiCustom;
  Ui::TomoToolConfigSavu m_uiSavu;
  Ui::TomoToolConfigTomoPy m_uiTomoPy;

  /// login status (from local perspective)
  bool m_loggedIn;

  /// facility for the remote compute resource
  const std::string m_facility;
  /// compute resources suppoted by this GUI (remote ones, clusters, etc.)
  std::vector<std::string> m_computeRes;
  /// display name of the "local" compute resource
  const std::string m_localCompName;

  /// reduction tools
  std::vector<std::string> m_SCARFtools;

  /// file paths, base dir on scarf
  std::string m_pathSCARFbase;
  /// path to fits file (sample data)
  std::string m_pathFITS;
  /// path to flat/open beam/bright image
  std::string m_pathFlat;
  /// path to dark image
  std::string m_pathDark;

  static const std::string m_SCARFName;

  // Names of image reconstruction tools
  static const std::string m_TomoPyTool;
  static const std::string m_AstraTool;
  static const std::string m_CCPiTool;
  static const std::string m_SavuTool;
  static const std::string m_CustomCmdTool;

  // plugins for savu config files
  // std::vector<Mantid::API::ITableWorkspace_sptr> m_availPlugins;
  Mantid::API::ITableWorkspace_sptr m_availPlugins;
  // std::vector<Mantid::API::ITableWorkspace_sptr> m_currPlugins;
  Mantid::API::ITableWorkspace_sptr m_currPlugins;
  std::string m_currentParamPath;
  static size_t m_nameSeqNo;

  // status of remote jobs
  std::vector<Mantid::API::IRemoteJobManager::RemoteJobInfo> m_jobsStatus;
  std::vector<std::string> m_jobsStatusCmds;

  // path name for persistent settings
  std::string m_settingsGroup;

  // for periodic update of the job status table/tree
  QTimer *m_keepAliveTimer;
  QThread *m_keepAliveThread;
  // mutex for the "job status info -> job status table " operations
  QMutex m_statusMutex;

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
  UserSettings settings;
};

class TomoToolConfigTomoPy : public QDialog {
  Q_OBJECT
public:
  TomoToolConfigTomoPy(QWidget *parent = 0);
};

class TomoToolConfigSavu : public QMainWindow {
  Q_OBJECT
public:
  TomoToolConfigSavu(QWidget *parent = 0);
};

class TomoToolConfigAstra : public QDialog {
  Q_OBJECT
public:
  TomoToolConfigAstra(QWidget *parent = 0);
};

class TomoToolConfigCustom : public QDialog {
  Q_OBJECT
public:
  TomoToolConfigCustom(QWidget *parent = 0);
};

class TomoToolSetupDialog : public QDialog {
  Q_OBJECT

public:
  TomoToolSetupDialog(QWidget *parent = 0);

private slots:
  void okClicked();
  void cancelClicked();

private:
  QLabel *labelRun, *labelOpt;
  QLineEdit *editRun, *editOpt;
  QHBoxLayout *hRun, *hOpt;
  QGridLayout *layout;
  QPushButton *okButton, *cancelButton;
};

}
}

#endif // MANTIDQTCUSTOMINTERFACES_TOMORECONSTRUCTION_H_
