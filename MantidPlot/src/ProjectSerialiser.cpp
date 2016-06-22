#include "ProjectSerialiser.h"
#include "ApplicationWindow.h"
#include "ScriptingWindow.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
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

ProjectSerialiser::ProjectSerialiser(ApplicationWindow* window, MantidUI* mantidUI)
    : window(window), mantidUI(mantidUI)
{
}

void ProjectSerialiser::save(Folder *folder, const QString &fn, bool compress)
{
    // save workspaces
    // save scripting window
    // save sub windows
    // save project file
    saveProjectFile(folder, fn, compress);
}

void ProjectSerialiser::saveProjectFile(Folder *folder, const QString &fn,
                                        bool compress) {
  QFile f(fn);
  if (window->d_backup_files && f.exists()) { // make byte-copy of current file so that
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


QString ProjectSerialiser::saveProjectFolder(Folder *folder, int &windowCount,
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
