#ifndef MANTIDQTCUSTOMINTERFACES_TOMORECONSTRUCTION_H_
#define MANTIDQTCUSTOMINTERFACES_TOMORECONSTRUCTION_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/TableRow.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "ui_TomoReconstruction.h"
#include "ui_TomoToolConfigAstra.h"
#include "ui_TomoToolConfigCustom.h"
#include "ui_TomoToolConfigSavu.h"
#include "ui_TomoToolConfigTomoPy.h"

#include <QDialog>
#include <jsoncpp/json/json.h>

class QTreeWidgetItem;
class QLineEdit;

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

protected slots:
  /// for buttons, run tab
  void on_reconstructClicked();
  void on_toolSetupClicked();
  void on_runVisualizeClicked();
  void on_jobCancelClicked();
  void on_jobTableRefreshClicked();

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
  void on_compResourceIndexChanged(int);
  void on_runToolIndexChanged(int);
  void on_SCARFLoginClicked();
  void on_SCARFLogoutClicked();

  void on_browseImageClicked();

  void on_fitsPathBrowseClicked();
  void on_flatPathBrowseClicked();
  void on_darkPathBrowseClicked();

  /// open the MantidQT help window for this interface
  void on_openHelpWin();
  void on_closeInterface();

  void on_menuSaveClicked();
  void on_menuSaveAsClicked();
  void on_availablePluginSelected();
  void on_currentPluginSelected();
  void on_transferClicked();
  void on_moveUpClicked();
  void on_moveDownClicked();
  void on_removeClicked();
  void on_menuOpenClicked();
  void on_paramValModified(QTreeWidgetItem *, int);
  void on_expandedItem(QTreeWidgetItem *);

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

  // path name for persistent settings
  std::string m_settingsGroup;
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
