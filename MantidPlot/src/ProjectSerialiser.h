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
        class ProjectSerialiser
        {
        public:
            /// Create a new serialiser with the current application window
            ProjectSerialiser(ApplicationWindow* window);
            /// Save the current state of the project to disk
            void save(Folder* folder, const QString& projectName, bool compress = false);
            /// Load a project file from disk
            void load(std::string lines, const int fileVersion, const bool isTopLevel = false);
            /// Open the script window and load scripts from string
            void openScriptWindow(const QStringList &files);

        private:
            /// Store a reference to the caller application window instance
            ApplicationWindow* window;

            /// Attempt to backup files before save
            bool canBackupProjectFiles(QFile* fileHandle, const QString& projectName);
            /// Check that the project is writable
            bool canWriteToProject(QFile* fileHandle, const QString& projectName);
            /// Convert the current state of the application to a project file
            QString serialiseProjectState(Folder* folder);
            /// Save the project file to disk
            void saveProjectFile(QFile* fileHandle, const QString &projectName, QString& text, bool compress);
            /// Load sections of the folder
            void loadProjectSections(const std::string& lines, const int fileVersion, const bool isTopLevel);
            /// Open the script window and load scripts from string
            void openScriptWindow(const std::string &files, const int fileVersion);
            void openMantidMatrix(const std::string &lines);
            void openMatrix(const std::string &lines, const int fileVersion);
            void openMultiLayer(const std::string &lines, const int fileVersion);
            void openSurfacePlot(const std::string &lines, const int fileVersion);
            void openTable(const std::string &lines, const int fileVersion);
            void openTableStatistics(const std::string &lines, const int fileVersion);
            void populateMantidTreeWidget(const QString &s);
            void loadWsToMantidTree(const std::string &wsName);

            std::string saveFolderState(Folder *folder);
            QString saveFolderHeader(Folder* folder, bool isCurrentFolder);
            QString saveFolderSubWindows(Folder *folder, int &windowCount);
            QString saveFolderFooter();
            std::string saveWorkspaces();
        };
    }
}

#endif // PROJECTMANAGER_H
