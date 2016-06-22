#include "ProjectSerialiser.h"
#include "ApplicationWindow.h"
#include "ScriptingWindow.h"


#include "Mantid/MantidUI.h"
#include "MantidKernel/MantidVersion.h"

#include <QTextCodec>
#include <QTextStream>

using namespace Mantid::API;
using namespace MantidQt::API;

// This C function is defined in the third party C lib minigzip.c
extern "C" {
    void file_compress(const char *file, const char *mode);
}

ProjectSerialiser::ProjectSerialiser(ApplicationWindow* window)
    : window(window)
{
}

/**
 * Save the current state of the application as a Mantid project file
 *
 * @param folder :: the folder instance to save
 * @param projectName :: the name of the project to write to
 * @param compress :: whether to compress the project (default false)
 */
void ProjectSerialiser::save(Folder *folder, const QString &projectName, bool compress)
{
     QFile fileHandle(projectName);

    // attempt to backup project files and check we can write
    if (!canBackupProjectFiles(&fileHandle, projectName) || !canWriteToProject(&fileHandle, projectName)) {
        return;
    }

    QString text = serialiseProjectState(folder);
    saveProjectFile(&fileHandle, projectName, text, compress);
}

/**
 * Check if the file can we written to
 * @param f :: the file handle
 * @param projectName :: the name of the project
 * @return true if the file handle is writable
 */
bool ProjectSerialiser::canWriteToProject(QFile* fileHandle, const QString& projectName)
{
    // check if we can write
    if (!fileHandle->open(QIODevice::WriteOnly)) {
        QMessageBox::about(window, window->tr("MantidPlot - File save error"),
                              window->tr("The file: <br><b>%1</b> is opened in read-only mode")
                                  .arg(projectName)); // Mantid
        return false;
    }
    return true;
}

/**
 * Serialise the state of Mantid
 *
 * This will go through all the parts that need to be serialised
 * and create a string for them which will be written to the .mantid file.
 *
 * This will also save workspaces etc. to the project location.
 *
 * @param folder :: the folder to write out
 * @return a string representation of the current project state
 */
QString ProjectSerialiser::serialiseProjectState(Folder* folder)
{
    QString text;

    // Save the list of workspaces
    if(window->mantidUI) {
        std::string workspaceString = window->mantidUI->saveToProject(window);
        text += QString::fromStdString(workspaceString);
    }

    // Save the scripting window
    ScriptingWindow* scriptingWindow = window->getScriptWindowHandle();
    if (scriptingWindow) {
         std::string scriptString = scriptingWindow->saveToProject(window);
         text += QString::fromStdString(scriptString);
    }

    // Recursively save folders
    if(folder) {
        std::string folderString = folder->saveToProject(window);
        text += QString::fromStdString(folderString);
    }

    return text;
}

/**
 * Check if the project can be backed up.
 *
 * If files cannot be backed up then the user will be queried
 * for permission to skip. If they do not want to skip for any
 * file the function will return false.
 *
 * @param fileHandle :: the file handle
 * @param projectName :: the name of the project
 * @return true if the project can be backed up or the user does not care
 */
bool ProjectSerialiser::canBackupProjectFiles(QFile* fileHandle, const QString& projectName)
{

    if (window->d_backup_files && fileHandle->exists()) { // make byte-copy of current file so that
                                        // there's always a copy of the data on
                                        // disk
      while (!fileHandle->open(QIODevice::ReadOnly)) {
        if (fileHandle->isOpen())
          fileHandle->close();
        int choice = QMessageBox::warning(
            window, window->tr("MantidPlot - File backup error"), // Mantid
            window->tr("Cannot make a backup copy of <b>%1</b> (to %2).<br>If you ignore "
               "this, you run the risk of <b>data loss</b>.")
                .arg(projectName)
                .arg(projectName + "~"),
            QMessageBox::Retry | QMessageBox::Default,
            QMessageBox::Abort | QMessageBox::Escape, QMessageBox::Ignore);
        if (choice == QMessageBox::Abort)
          return false;
        if (choice == QMessageBox::Ignore)
          break;
      }

      if (fileHandle->isOpen()) {
        QFile::copy(projectName, projectName + "~");
        fileHandle->close();
      }
    }

    return true;
}

/**
 * Save the project file to disk
 *
 * @param fileHandle :: the file handle
 * @param projectName :: the name of the project
 * @param text :: the string representation of the current state
 * @param compress :: whether to compress the project
 */
void ProjectSerialiser::saveProjectFile(QFile* fileHandle, const QString &projectName, QString& text, bool compress) {
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  // add some header content to the file
  text.prepend("<scripting-lang>\t" + QString(window->scriptingEnv()->objectName()) +
               "\n");

  QString version(Mantid::Kernel::MantidVersion::version());
  text.prepend("MantidPlot " + version + " project file\n");

  // write out the saved project state
  QTextStream t(fileHandle);
  t.setCodec(QTextCodec::codecForName("UTF-8"));
  t << text;
  fileHandle->close();

  // compress the project if needed
  if (compress) {
    file_compress(projectName.toLatin1().constData(), "w9");
  }

  QApplication::restoreOverrideCursor();
}

