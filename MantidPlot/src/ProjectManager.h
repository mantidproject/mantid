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
class MantidUI;
class ScriptingWindow;

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
        class ProjectManager
        {
        public:
            ProjectManager(ApplicationWindow* window, MantidUI* mantidUI);
            bool saveProject(bool compress = false);

        private:
            ApplicationWindow* window;
            MantidUI* mantidUI;

            bool autoSave;
            int autoSaveTime;
            int savingTimerId;
            bool d_backup_files;

            friend class MantidUI;
            friend class ApplicationWindow;

            void saveProjectAs(const QString& filename = QString(), bool compress = false);
            Folder *projectFolder() const;
            void saveFolderAsProject(Folder *f);
            void saveAsProject();
            void saveProjectFile(Folder *folder, const QString &fn, bool compress);
            QString saveProjectFolder(Folder *folder, int &windowCount, bool isTopLevel = false);
            QString saveToString(const std::string &workingDir);
            void savedatainNexusFormat(const std::string &fileName, const std::string &wsName);
        };
    }
}

#endif // PROJECTMANAGER_H
