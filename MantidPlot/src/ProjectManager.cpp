#include "ProjectManager.h"
#include "ApplicationWindow.h"
#include "ScriptingWindow.h"

#include "Mantid/MantidUI.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidQtAPI/FileDialogHandler.h"

#include <QMenu>
#include <QTextCodec>
#include <QTextStream>
#include <QTreeWidget>

using namespace Mantid::API;
using namespace MantidQt::API;

// This C function is defined in the third party C lib minigzip.c
extern "C" {
    void file_compress(const char *file, const char *mode);
}

ProjectManager::ProjectManager(ApplicationWindow* window, MantidUI* mantidUI) : window(window), mantidUI(mantidUI)
{
    autoSave = false;
    autoSaveTime = 15;
    savingTimerId = 0;
}

bool ProjectManager::saveProject(bool compress)
{
    if (window->projectname == "untitled" ||
        window->projectname.endsWith(".opj", Qt::CaseInsensitive) ||
        window->projectname.endsWith(".ogm", Qt::CaseInsensitive) ||
        window->projectname.endsWith(".ogw", Qt::CaseInsensitive) ||
        window->projectname.endsWith(".ogg", Qt::CaseInsensitive)) {
      saveProjectAs();
      return true;
      ;
    }

    saveProjectFile(projectFolder(), window->projectname, compress);

    window->setWindowTitle("MantidPlot - " + window->projectname);
    window->savedProject();

    if (autoSave) {
      if (savingTimerId)
        window->killTimer(savingTimerId);
      savingTimerId = window->startTimer(autoSaveTime * 60000);
    } else
      savingTimerId = 0;

    // Back-up file to be removed because file has successfully saved.
    QFile::remove(window->projectname + "~");

    QApplication::restoreOverrideCursor();
    return true;
}

void ProjectManager::saveProjectAs(const QString &fileName, bool compress) {
  QString fn = fileName;
  if (fileName.isEmpty()) {
    QString filter = window->tr("MantidPlot project") + " (*.mantid);;";
    filter += window->tr("Compressed MantidPlot project") + " (*.mantid.gz)";

    QString selectedFilter;
    fn = MantidQt::API::FileDialogHandler::getSaveFileName(
        window, window->tr("Save Project As"), window->workingDir, filter, &selectedFilter);
    if (selectedFilter.contains(".gz"))
      compress = true;
  }

  if (!fn.isEmpty()) {
    // Check if exists. If not, create directory first.
    QFileInfo tempFile(fn);
    if (!tempFile.exists()) {
      // Make the directory
      QString dir(fn);
      if (fn.contains('.'))
        dir = fn.left(fn.indexOf('.'));
      QDir().mkdir(dir);

      // Get the file name
      QString file("temp");
      for (int i = 0; i < dir.size(); ++i) {
        if (dir[i] == '/')
          file = dir.right(dir.size() - i);
        else if (dir[i] == '\\')
          file = dir.right(i);
      }
      fn = dir + file;
    }

    QFileInfo fi(fn);
    window->workingDir = fi.absolutePath();
    QString baseName = fi.fileName();
    if (!baseName.contains("."))
      fn.append(".mantid");

    window->projectname = fn;
    if (saveProject(compress)) {
      window->recentProjects.removeAll(window->projectname);
      window->recentProjects.push_front(window->projectname);
      window->updateRecentProjectsList();

      QFileInfo fi(fn);
      QString baseName = fi.baseName();
      FolderListItem *item =
          dynamic_cast<FolderListItem *>(window->folders->firstChild());
      if (item) {
        item->setText(0, baseName);
        item->folder()->setObjectName(baseName);
      }
    }
  }
}

void ProjectManager::saveProjectFile(Folder *folder, const QString &fn,
                                        bool compress) {
  QFile f(fn);
  if (d_backup_files && f.exists()) { // make byte-copy of current file so that
                                      // there's always a copy of the data on
                                      // disk
    while (!f.open(QIODevice::ReadOnly)) {
      if (f.isOpen())
        f.close();
      int choice = QMessageBox::warning(
          window, window->tr("MantidPlot - File backup error"), // Mantid
          window->tr("Cannot make a backup copy of <b>%1</b> (to %2).<br>If you ignore "
             "this, you run the risk of <b>data loss</b>.")
              .arg(window->projectname)
              .arg(window->projectname + "~"),
          QMessageBox::Retry | QMessageBox::Default,
          QMessageBox::Abort | QMessageBox::Escape, QMessageBox::Ignore);
      if (choice == QMessageBox::Abort)
        return;
      if (choice == QMessageBox::Ignore)
        break;
    }

    if (f.isOpen()) {
      QFile::copy(fn, fn + "~");
      f.close();
    }
  }

  if (!f.open(QIODevice::WriteOnly)) {
    QMessageBox::about(window, window->tr("MantidPlot - File save error"),
                       window->tr("The file: <br><b>%1</b> is opened in read-only mode")
                           .arg(fn)); // Mantid
    return;
  }
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QString text;

  // Save the list of workspaces
  text += mantidUI->saveToString(window->workingDir.toStdString());

  ScriptingWindow* scriptingWindow = window->getScriptWindowHandle();
  if (scriptingWindow)
    text += scriptingWindow->saveToString();

  int windowCount = 0;
  text += saveProjectFolder(folder, windowCount, true);

  text.prepend("<windows>\t" + QString::number(windowCount) + "\n");
  text.prepend("<scripting-lang>\t" + QString(window->scriptingEnv()->objectName()) +
               "\n");

  QString version(Mantid::Kernel::MantidVersion::version());
  text.prepend("MantidPlot " + version + " project file\n");

  QTextStream t(&f);
  t.setCodec(QTextCodec::codecForName("UTF-8"));
  t << text;
  f.close();

  if (compress) {
    file_compress(fn.toLatin1().constData(), "w9");
  }

  QApplication::restoreOverrideCursor();
}

void ProjectManager::saveAsProject() {
  saveFolderAsProject(window->currentFolder());
}

void ProjectManager::saveFolderAsProject(Folder *f) {
  QString filter = window->tr("MantidPlot project") + " (*.qti);;"; // Mantid
  filter += window->tr("Compressed MantidPlot project") + " (*.qti.gz)";

  QString selectedFilter;
  QString fn = MantidQt::API::FileDialogHandler::getSaveFileName(
      window, window->tr("Save project as"), window->workingDir, filter, &selectedFilter);
  if (!fn.isEmpty()) {
    QFileInfo fi(fn);
    window->workingDir = fi.absolutePath();
    QString baseName = fi.fileName();
    if (!baseName.contains("."))
      fn.append(".qti");

    saveProjectFile(f, fn, selectedFilter.contains(".gz"));
  }
}

Folder *ProjectManager::projectFolder() const {
  auto fli = dynamic_cast<FolderListItem *>(window->folders->firstChild());
  if (fli)
    return fli->folder();
  else
    throw std::runtime_error("Couldn't retrieve project folder");
}

QString ProjectManager::saveProjectFolder(Folder *folder, int &windowCount,
                                             bool isTopLevel) {
  QString text;

  // Write the folder opening tag
  if (!isTopLevel) {
    text += "<folder>\t" + QString(folder->objectName()) + "\t" +
            folder->birthDate() + "\t" + folder->modificationDate();

    if (folder == window->currentFolder())
      text += "\tcurrent";
    text += "\n";
    text += "<open>" + QString::number(folder->folderListItem()->isExpanded()) +
            "</open>\n";
  }

  // Write windows
  QList<MdiSubWindow *> windows = folder->windowsList();
  foreach (MdiSubWindow *w, windows) {
    Mantid::IProjectSerialisable *ips =
        dynamic_cast<Mantid::IProjectSerialisable *>(w);
    if (ips)
      text += QString::fromUtf8(ips->saveToProject(window).c_str());

    ++windowCount;
  }

  // Write subfolders
  QList<Folder *> subfolders = folder->folders();
  foreach (Folder *f, subfolders) { text += saveProjectFolder(f, windowCount); }

  // Write log info
  if (!folder->logInfo().isEmpty())
    text += "<log>\n" + folder->logInfo() + "</log>\n";

  // Write the folder closing tag
  if (!isTopLevel) {
    text += "</folder>\n";
  }

  return text;
}

