#ifndef PROJECT_SERIALISER_H
#define PROJECT_SERIALISER_H

#include <string>
#include <vector>
#include <unordered_map>

#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QStringList>
#include <QDir>
#include <QApplication>

#include "MantidQtWidgets/Common/TSVSerialiser.h"

#include "qstring.h"
#include "Graph3D.h"
#include "Mantid/MantidMatrix.h"

// Forward declare Mantid classes.
class ApplicationWindow;
class Folder;

/** Manages saving and loading Mantid project files.

  @author Samuel Jackson, ISIS, RAL
  @date 21/06/2016

  Copyright &copy; 2007-2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
*/

namespace MantidQt {
namespace API {

using groupNameToWsNamesT =
    std::unordered_map<std::string, std::vector<std::string>>;

class ProjectSerialiser : public QObject {
  Q_OBJECT
public:
  /// Create a new serialiser with the current application window
  explicit ProjectSerialiser(ApplicationWindow *window);
  explicit ProjectSerialiser(ApplicationWindow *window, Folder *folder);

  explicit ProjectSerialiser(ApplicationWindow *window, bool isRecovery);
  explicit ProjectSerialiser(ApplicationWindow *window, Folder *folder,
                             bool isRecovery);

  /// Save the current state of the project to disk
  bool save(const QString &projectName, const std::vector<std::string> &wsNames,
            const std::vector<std::string> &windowNames, bool compress = false);
  bool save(const QString &projectName, bool compress = false,
            bool saveAll = true);
  /// Load a project file from disk
  bool load(std::string filepath, const int fileVersion,
            const bool isTopLevel = true);
  /// Open the script window and load scripts from string
  void openScriptWindow(const QStringList &files);

signals:
  /// Set the curret progress of serialisation
  void setProgressBarRange(int min, int max);
  /// Set the forcasted range of things to do when saving
  void setProgressBarValue(int value);
  /// Set what is currently happening to listening progress bars
  void setProgressBarText(QString text);

private:
  // Instance Variables

  /// Store a reference to the caller application window instance
  ApplicationWindow *window;
  /// Store a reference to the current folder
  Folder *m_currentFolder;
  /// Vector of names of windows to save to file
  std::vector<std::string> m_windowNames;
  /// Vector of names of workspaces to save to file
  std::vector<std::string> m_workspaceNames;
  /// Store a count of the number of windows during saving
  int m_windowCount;
  /// Flag to check if we should save all workspaces
  bool m_saveAll;
  /// Flag to check if we are operating for project recovery
  const bool m_projectRecovery;

  // Saving Functions

  /// Attempt to backup files before save
  bool canBackupProjectFiles(QFile *fileHandle, const QString &projectName);
  /// Check that the project is writable
  bool canWriteToProject(QFile *fileHandle, const QString &projectName);
  /// Convert the current state of the application to a project file
  QString serialiseProjectState(Folder *folder);
  /// Save the project file to disk
  void saveProjectFile(QFile *fileHandle, const QString &projectName,
                       QString &text, bool compress);
  /// Save the state of a folder
  QString saveFolderState(Folder *folder, const bool isTopLevel = false);
  /// Save the header information about a folder
  QString saveFolderHeader(Folder *folder, bool isCurrentFolder);
  /// Save sub-windows for a folder
  QString saveFolderSubWindows(Folder *folder);
  /// Save the footer contents of a folder
  QString saveFolderFooter();
  /// Save any currently loaded workspaces
  QString saveWorkspaces();
  /// Save additional windows
  QString saveAdditionalWindows();

  // Loading Functions

  /// Load sections of the folder
  void loadProjectSections(const std::string &lines, const int fileVersion,
                           const bool isTopLevel);
  /// Load workspaces from the project file
  void loadWorkspaces(const TSVSerialiser &tsv);
  /// Load project windows from the project file
  void loadWindows(const TSVSerialiser &tsv, const int fileVersion);
  /// Load all subfolders of the current folder
  void loadSubFolders(const TSVSerialiser &tsv, const int fileVersion);
  /// Load scripts into the script window
  void loadScriptWindow(const TSVSerialiser &tsv, const int fileVersion);
  /// Load saved log data into the log
  void loadLogData(const TSVSerialiser &tsv);
  /// Load information about the current folder
  void loadCurrentFolder(const TSVSerialiser &tsv);
  /// Open the script window and load scripts from string
  void openScriptWindow(const std::string &files, const int fileVersion);
  /// Load Nexus files and add workspaces to the ADS
  void loadWorkspacesIntoMantid(const groupNameToWsNamesT &workspaces);
  /// Load a single workspaces to the ADS
  void loadWsToMantidTree(const std::string &wsName);
  /// Load additional windows (e.g. slice viewer)
  void loadAdditionalWindows(const std::string &lines, const int fileVersion);

  // Misc functions

  /// Create a handle to a new QMdiSubWindow instance
  QMdiSubWindow *setupQMdiSubWindow() const;
  /// Check if a vector of strings contains a string
  bool contains(const std::vector<std::string> &vec, const std::string &value);

  groupNameToWsNamesT parseWsNames(const std::string &wsNames);
};
}
}

#endif // PROJECT_SERIALISER_H
