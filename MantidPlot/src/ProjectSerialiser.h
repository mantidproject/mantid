#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <string>

#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QStringList>
#include <QDir>
#include <QApplication>

#include "qstring.h"
#include "Folder.h"

// Forward declare Mantid classes.
class ApplicationWindow;

/** Manages saving and loading Mantid project files.

  @author Samuel Jackson, ISIS, RAL
  @date 21/06/2016

  Copyright &copy; 2007-2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class ProjectSerialiser {
public:
  /// Create a new serialiser with the current application window
  explicit ProjectSerialiser(ApplicationWindow *window);
  /// Save the current state of the project to disk
  void save(Folder *folder, const QString &projectName, bool compress = false);
  /// Load a project file from disk
  void load(std::string lines, const int fileVersion,
            const bool isTopLevel = true);
  /// Open the script window and load scripts from string
  void openScriptWindow(const QStringList &files);

private:
  // Instance Variables

  /// Store a reference to the caller application window instance
  ApplicationWindow *window;
  /// Store a count of the number of windows during saving
  int m_windowCount;

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

  // Loading Functions

  /// Load sections of the folder
  void loadProjectSections(const std::string &lines, const int fileVersion,
                           const bool isTopLevel);
  /// Open the script window and load scripts from string
  void openScriptWindow(const std::string &files, const int fileVersion);
  /// Open a Mantid matrix from a workspaces
  void openMantidMatrix(const std::string &lines);
  /// Open a (non-Mantid) matrix
  void openMatrix(const std::string &lines, const int fileVersion);
  /// Open a multi-layer plot window
  void openMultiLayer(const std::string &lines, const int fileVersion);
  /// Open a surface plot window
  void openSurfacePlot(const std::string &lines, const int fileVersion);
  /// Open a table window
  void openTable(const std::string &lines, const int fileVersion);
  /// Open a table statistics window
  void openTableStatistics(const std::string &lines, const int fileVersion);
  /// Load Nexus files and add workspaces to the ADS
  void populateMantidTreeWidget(const QString &lines);
  /// Load a single workspaces to the ADS
  void loadWsToMantidTree(const std::string &wsName);
};
}
}

#endif // PROJECTMANAGER_H
