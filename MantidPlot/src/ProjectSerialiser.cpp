#include "globals.h"
#include "ProjectSerialiser.h"
#include "ApplicationWindow.h"
#include "ScriptingWindow.h"
#include "TSVSerialiser.h"
#include "Note.h"
#include "Matrix.h"
#include "TableStatistics.h"
#include "Graph3D.h"

#include "Mantid/MantidMatrix.h"
#include "Mantid/MantidUI.h"
#include "MantidKernel/MantidVersion.h"
#include "Mantid/InstrumentWidget/InstrumentWindow.h"

#include <QTextCodec>
#include <QTextStream>

using namespace Mantid::API;
using namespace MantidQt::API;

// This C function is defined in the third party C lib minigzip.c
extern "C" {
void file_compress(const char *file, const char *mode);
}

ProjectSerialiser::ProjectSerialiser(ApplicationWindow *window)
    : window(window), m_windowCount(0) {}

/**
 * Save the current state of the application as a Mantid project file
 *
 * @param folder :: the folder instance to save
 * @param projectName :: the name of the project to write to
 * @param compress :: whether to compress the project (default false)
 */
void ProjectSerialiser::save(Folder *folder, const QString &projectName,
                             bool compress) {
  m_windowCount = 0;
  QFile fileHandle(projectName);

  // attempt to backup project files and check we can write
  if (!canBackupProjectFiles(&fileHandle, projectName) ||
      !canWriteToProject(&fileHandle, projectName)) {
    return;
  }

  QString text = serialiseProjectState(folder);
  saveProjectFile(&fileHandle, projectName, text, compress);
}

/**
 * Load the state of Mantid from a collection of lines read from a project file
 *
 * @param lines :: string of characters from a project file
 * @param fileVersion :: project file version used
 * @param isTopLevel :: whether this function is being called on a top level
 * 		folder. (Default True)
 */
void ProjectSerialiser::load(std::string lines, const int fileVersion,
                             const bool isTopLevel) {
  // If we're not the top level folder, read the folder settings and create the
  // folder
  // This is a legacy edgecase because folders are written
  // <folder>\tsettings\tgo\there
  if (!isTopLevel && lines.size() > 0) {
    std::vector<std::string> lineVec;
    boost::split(lineVec, lines, boost::is_any_of("\n"));

    std::string firstLine = lineVec.front();

    std::vector<std::string> values;
    boost::split(values, firstLine, boost::is_any_of("\t"));

    auto newFolder =
        new Folder(window->currentFolder(), QString::fromStdString(values[1]));
    newFolder->setBirthDate(QString::fromStdString(values[2]));
    newFolder->setModificationDate(QString::fromStdString(values[3]));

    if (values.size() > 4 && values[4] == "current")
      window->d_loaded_current = newFolder;

    auto fli = new FolderListItem(window->currentFolder()->folderListItem(),
                                  newFolder);
    newFolder->setFolderListItem(fli);

    window->d_current_folder = newFolder;

    // Remove the first line (i.e. the folder's settings line)
    lineVec.erase(lineVec.begin());
    lines = boost::algorithm::join(lineVec, "\n");
  }

  loadProjectSections(lines, fileVersion, isTopLevel);

  // We're returning to our parent folder, so set d_current_folder to our parent
  auto parent = dynamic_cast<Folder *>(window->currentFolder()->parent());
  if (!parent)
    window->d_current_folder = window->projectFolder();
  else
    window->d_current_folder = parent;
}

/**
 * Load sections of the project file back into Mantid
 *
 * This function looks at individual sections of the TSV project file
 * and loads the relevant windows etc.
 *
 * @param lines :: string of characters from a Mantid project file.
 * @param fileVersion :: version of the project file loaded
 * @param isTopLevel :: whether this is being called on a top level folder.
 */
void ProjectSerialiser::loadProjectSections(const std::string &lines,
                                            const int fileVersion,
                                            const bool isTopLevel) {
  // This now ought to be the regular contents of a folder. Parse as normal.
  TSVSerialiser tsv(lines);

  // If this is the top level folder of the project, we'll need to load the
  // workspaces before anything else.
  if (isTopLevel && tsv.hasSection("mantidworkspaces")) {
    // There should only be one of these, so we only read the first.
    std::string workspaces = tsv.sections("mantidworkspaces").front();
    populateMantidTreeWidget(QString::fromStdString(workspaces));
  }

  if (tsv.hasSection("open")) {
    std::string openStr = tsv.sections("open").front();
    int openValue = 0;
    std::stringstream(openStr) >> openValue;
    window->currentFolder()->folderListItem()->setExpanded(openValue);
  }

  if (tsv.hasSection("mantidmatrix")) {
    auto matrices = tsv.sections("mantidmatrix");
    for (auto &it : matrices) {
      openMantidMatrix(it);
    }
  }

  if (tsv.hasSection("table")) {
    auto tableSections = tsv.sections("table");
    for (auto &it : tableSections) {
      openTable(it, fileVersion);
    }
  }

  if (tsv.hasSection("TableStatistics")) {
    auto tableStatsSections = tsv.sections("TableStatistics");
    for (auto &it : tableStatsSections) {
      openTableStatistics(it, fileVersion);
    }
  }

  if (tsv.hasSection("matrix")) {
    auto matrixSections = tsv.sections("matrix");
    for (auto &it : matrixSections) {
      openMatrix(it, fileVersion);
    }
  }

  if (tsv.hasSection("multiLayer")) {
    auto multiLayer = tsv.sections("multiLayer");
    for (auto &it : multiLayer) {
      openMultiLayer(it, fileVersion);
    }
  }

  if (tsv.hasSection("SurfacePlot")) {
    auto plotSections = tsv.sections("SurfacePlot");
    for (auto &it : plotSections) {
      openSurfacePlot(it, fileVersion);
    }
  }

  if (tsv.hasSection("log")) {
    auto logSections = tsv.sections("log");
    for (auto &it : logSections) {
      window->currentFolder()->appendLogInfo(QString::fromStdString(it));
    }
  }

  if (tsv.hasSection("note")) {
    auto noteSections = tsv.sections("note");
    for (auto &it : noteSections) {
      auto n = window->newNote("");
      n->loadFromProject(it, window, fileVersion);
    }
  }

  if (tsv.hasSection("scriptwindow")) {
    auto scriptSections = tsv.sections("scriptwindow");
    for (auto &it : scriptSections) {
      openScriptWindow(it, fileVersion);
    }
  }

  if (tsv.hasSection("instrumentwindow")) {
    auto instrumentSections = tsv.sections("instrumentwindow");
    for (auto &it : instrumentSections) {
      TSVSerialiser iws(it);

      if (iws.selectLine("WorkspaceName")) {
        std::string wsName = iws.asString(1);
        QString name = QString::fromStdString(wsName);

        auto obj = window->mantidUI->getInstrumentView(name);
        auto iw = dynamic_cast<InstrumentWindow *>(obj);

        if (iw) {
          iw->loadFromProject(it, window, fileVersion);
        }
      }
    }
  }

  // Deal with subfolders last.
  if (tsv.hasSection("folder")) {
    auto folders = tsv.sections("folder");
    for (auto &it : folders) {
      load(it, fileVersion, false);
    }
  }
}

/**
 * Check if the file can we written to
 * @param f :: the file handle
 * @param projectName :: the name of the project
 * @return true if the file handle is writable
 */
bool ProjectSerialiser::canWriteToProject(QFile *fileHandle,
                                          const QString &projectName) {
  // check if we can write
  if (!fileHandle->open(QIODevice::WriteOnly)) {
    QMessageBox::about(window, window->tr("MantidPlot - File save error"),
                       window->tr("The file: <br><b>%1</b> is opened in "
                                  "read-only mode").arg(projectName)); // Mantid
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
QString ProjectSerialiser::serialiseProjectState(Folder *folder) {
  QString text;

  // Save the list of workspaces
  if (window->mantidUI) {
    text += saveWorkspaces();
  }

  // Save the scripting window
  ScriptingWindow *scriptingWindow = window->getScriptWindowHandle();
  if (scriptingWindow) {
    std::string scriptString = scriptingWindow->saveToProject(window);
    text += QString::fromStdString(scriptString);
  }

  // Finally, recursively save folders
  if (folder) {
    text += saveFolderState(folder, true);
  }

  return text;
}

/**
 * Save the folder structure to a Mantid project file.
 *
 * @param app :: the current application window instance
 * @return string represnetation of the folder's data
 */
QString ProjectSerialiser::saveFolderState(Folder *folder,
                                           const bool isTopLevel) {
  QString text;
  bool isCurrentFolder = window->currentFolder() == folder;

  if (!isTopLevel) {
    text += saveFolderHeader(folder, isCurrentFolder);
  }

  text += saveFolderSubWindows(folder);

  if (!isTopLevel) {
    text += saveFolderFooter();
  }

  return text;
}

/**
 * Generate the opening tags and meta information about
 * a folder record for the Mantid project file.
 *
 * @param isCurrentFolder :: whether this folder is the current one.
 * @return string representation of the folder's header data
 */
QString ProjectSerialiser::saveFolderHeader(Folder *folder,
                                            bool isCurrentFolder) {
  QString text;

  // Write the folder opening tag
  text += "<folder>\t" + QString(folder->objectName()) + "\t" +
          folder->birthDate() + "\t" + folder->modificationDate();

  // label it as current if necessary
  if (isCurrentFolder) {
    text += "\tcurrent";
  }

  text += "\n";
  text += "<open>" + QString::number(folder->folderListItem()->isExpanded()) +
          "</open>\n";
  return text;
}

/**
 * Generate the subfolder and subwindow records for the current folder.
 * This method will recursively convert subfolders to their text representation
 *
 * @param app :: the current application window instance
 * @param folder :: the folder to generate the text for.
 * @param windowCount :: count of the number of windows
 * @return string representation of the folder's subfolders
 */
QString ProjectSerialiser::saveFolderSubWindows(Folder *folder) {
  QString text;

  // Write windows
  QList<MdiSubWindow *> windows = folder->windowsList();
  for (auto &w : windows) {
    Mantid::IProjectSerialisable *ips =
        dynamic_cast<Mantid::IProjectSerialisable *>(w);

    if (ips) {
      text += QString::fromUtf8(ips->saveToProject(window).c_str());
    }
  }

  m_windowCount += windows.size();

  // Write subfolders
  QList<Folder *> subfolders = folder->folders();
  foreach (Folder *f, subfolders) { text += saveFolderState(f); }

  // Write log info
  if (!folder->logInfo().isEmpty()) {
    text += "<log>\n" + folder->logInfo() + "</log>\n";
  }

  return text;
}

/**
 * Generate the closing folder data and end tag.
 * @return footer string for this folder
 */
QString ProjectSerialiser::saveFolderFooter() { return "</folder>\n"; }

/** This method saves the currently loaded workspaces in
 * the project.
 *
 * Saves the names of all the workspaces loaded into mantid workspace tree.
 * Creates a string and calls save nexus on each workspace to save the data
 * to a nexus file.
 *
 * @return workspace names formatted as a Mantid project file string
 */
QString ProjectSerialiser::saveWorkspaces() {
  using namespace Mantid::API;
  std::string workingDir = window->workingDir.toStdString();
  QString wsNames;
  wsNames = "<mantidworkspaces>\n";
  wsNames += "WorkspaceNames";

  auto workspaceItems = AnalysisDataService::Instance().getObjectNames();
  for (auto &itemIter : workspaceItems) {
    QString wsName = QString::fromStdString(itemIter);

    auto ws = AnalysisDataService::Instance().retrieveWS<Workspace>(
        wsName.toStdString());
    auto group = boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(ws);

    // We don't split up multiperiod workspaces for performance reasons.
    // There's significant optimisations we can perform on load if they're a
    // single file.
    if (ws->id() == "WorkspaceGroup" && group && !group->isMultiperiod()) {
      wsNames += "\t";
      wsNames += wsName;
      std::vector<std::string> secondLevelItems = group->getNames();
      for (size_t j = 0; j < secondLevelItems.size(); j++) {
        wsNames += ",";
        wsNames += QString::fromStdString(secondLevelItems[j]);
        std::string fileName(workingDir + "//" + secondLevelItems[j] + ".nxs");
        window->mantidUI->savedatainNexusFormat(fileName, secondLevelItems[j]);
      }
    } else {
      wsNames += "\t";
      wsNames += wsName;

      std::string fileName(workingDir + "//" + wsName.toStdString() + ".nxs");
      window->mantidUI->savedatainNexusFormat(fileName, wsName.toStdString());
    }
  }
  wsNames += "\n</mantidworkspaces>\n";
  return wsNames;
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
bool ProjectSerialiser::canBackupProjectFiles(QFile *fileHandle,
                                              const QString &projectName) {

  if (window->d_backup_files &&
      fileHandle->exists()) { // make byte-copy of current file so that
                              // there's always a copy of the data on
                              // disk
    while (!fileHandle->open(QIODevice::ReadOnly)) {
      if (fileHandle->isOpen())
        fileHandle->close();
      int choice = QMessageBox::warning(
          window, window->tr("MantidPlot - File backup error"), // Mantid
          window->tr("Cannot make a backup copy of <b>%1</b> (to %2).<br>If "
                     "you ignore "
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
void ProjectSerialiser::saveProjectFile(QFile *fileHandle,
                                        const QString &projectName,
                                        QString &text, bool compress) {
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  // add number of MdiSubWindows saved to file
  text.prepend("<windows>\t" + QString::number(m_windowCount) + "\n");

  // add some header content to the file
  QString lang = QString(window->scriptingEnv()->objectName());
  text.prepend("<scripting-lang>\t" + lang + "\n");

  // construct MantidPlot version number
  QString version;
  version += QString::number(maj_version) + ".";
  version += QString::number(min_version) + ".";
  version += QString::number(patch_version);

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

/**
 * Open a new matrix window
 *
 * @param lines :: string of characters from a Mantid project file
 * @param fileVersion :: the version of the project file
 */
void ProjectSerialiser::openMatrix(const std::string &lines,
                                   const int fileVersion) {
  // The first line specifies the name, dimensions and date.
  std::vector<std::string> lineVec;
  boost::split(lineVec, lines, boost::is_any_of("\n"));
  std::string firstLine = lineVec.front();
  lineVec.erase(lineVec.begin());
  std::string newLines = boost::algorithm::join(lineVec, "\n");

  // Parse the first line
  std::vector<std::string> values;
  boost::split(values, firstLine, boost::is_any_of("\t"));

  if (values.size() < 4) {
    return;
  }

  const std::string caption = values[0];
  const std::string date = values[3];

  int rows = 0;
  int cols = 0;
  Mantid::Kernel::Strings::convert<int>(values[1], rows);
  Mantid::Kernel::Strings::convert<int>(values[2], cols);

  auto m = window->newMatrix(QString::fromStdString(caption), rows, cols);
  window->setListViewDate(QString::fromStdString(caption),
                          QString::fromStdString(date));
  m->setBirthDate(QString::fromStdString(date));

  TSVSerialiser tsv(newLines);

  if (tsv.hasLine("geometry")) {
    std::string gStr = tsv.lineAsString("geometry");
    window->restoreWindowGeometry(window, m, QString::fromStdString(gStr));
  }

  m->loadFromProject(newLines, window, fileVersion);
}

/**
 * Open a new Mantid Matrix window.
 *
 * @param lines :: string of characters from a Mantid project file
 */
void ProjectSerialiser::openMantidMatrix(const std::string &lines) {
  TSVSerialiser tsv(lines);

  MantidMatrix *m = nullptr;

  if (tsv.selectLine("WorkspaceName")) {
    m = window->mantidUI->openMatrixWorkspace(tsv.asString(1), -1, -1);
  }

  if (!m)
    return;

  if (tsv.selectLine("geometry")) {
    const std::string geometry = tsv.lineAsString("geometry");
    window->restoreWindowGeometry(window, m, QString::fromStdString(geometry));
  }

  if (tsv.selectLine("tgeometry")) {
    const std::string geometry = tsv.lineAsString("tgeometry");
    window->restoreWindowGeometry(window, m, QString::fromStdString(geometry));
  }

  // Append to the list of mantid matrix windows
  window->addMantidMatrixWindow(m);
}

/**
 * Open a new Multi-Layer plot window
 * @param lines :: string of characters from a Mantid project file.
 * @param fileVersion :: the version of the project file
 */
void ProjectSerialiser::openMultiLayer(const std::string &lines,
                                       const int fileVersion) {
  MultiLayer *plot = nullptr;
  std::string multiLayerLines = lines;

  // The very first line of a multilayer section has some important settings,
  // and lacks a name. Take it out and parse it manually.

  if (multiLayerLines.length() == 0)
    return;

  std::vector<std::string> lineVec;
  boost::split(lineVec, multiLayerLines, boost::is_any_of("\n"));

  std::string firstLine = lineVec.front();
  // Remove the first line
  lineVec.erase(lineVec.begin());
  multiLayerLines = boost::algorithm::join(lineVec, "\n");

  // Split the line up into its values
  std::vector<std::string> values;
  boost::split(values, firstLine, boost::is_any_of("\t"));

  std::string caption = values[0];
  int rows = 1;
  int cols = 1;
  Mantid::Kernel::Strings::convert<int>(values[1], rows);
  Mantid::Kernel::Strings::convert<int>(values[2], cols);
  std::string birthDate = values[3];

  plot =
      window->multilayerPlot(QString::fromUtf8(caption.c_str()), 0, rows, cols);
  plot->setBirthDate(QString::fromStdString(birthDate));
  window->setListViewDate(QString::fromStdString(caption),
                          QString::fromStdString(birthDate));

  plot->loadFromProject(multiLayerLines, window, fileVersion);
}

/**
 * Open a new table window
 * @param lines :: chracters from a Mantid project file
 * @param fileVersion :: the version of the project file
 */
void ProjectSerialiser::openTable(const std::string &lines,
                                  const int fileVersion) {
  std::vector<std::string> lineVec, valVec;
  boost::split(lineVec, lines, boost::is_any_of("\n"));

  const std::string firstLine = lineVec.front();
  boost::split(valVec, firstLine, boost::is_any_of("\t"));

  if (valVec.size() < 4)
    return;

  std::string caption = valVec[0];
  std::string date = valVec[3];
  int rows = 1;
  int cols = 1;
  Mantid::Kernel::Strings::convert<int>(valVec[1], rows);
  Mantid::Kernel::Strings::convert<int>(valVec[2], cols);

  auto t = window->newTable(QString::fromStdString(caption), rows, cols);
  window->setListViewDate(QString::fromStdString(caption),
                          QString::fromStdString(date));
  t->setBirthDate(QString::fromStdString(date));
  t->loadFromProject(lines, window, fileVersion);
}

/**
 * Open a new table statistics window
 * @param lines :: chracters from a Mantid project file
 * @param fileVersion :: the version of the project file
 */
void ProjectSerialiser::openTableStatistics(const std::string &lines,
                                            const int fileVersion) {
  std::vector<std::string> lineVec;
  boost::split(lineVec, lines, boost::is_any_of("\n"));

  const std::string firstLine = lineVec.front();

  std::vector<std::string> firstLineVec;
  boost::split(firstLineVec, firstLine, boost::is_any_of("\t"));

  if (firstLineVec.size() < 4)
    return;

  const std::string name = firstLineVec[0];
  const std::string tableName = firstLineVec[1];
  const std::string type = firstLineVec[2];
  const std::string birthDate = firstLineVec[3];

  TSVSerialiser tsv(lines);

  if (!tsv.hasLine("Targets"))
    return;

  const std::string targetsLine = tsv.lineAsString("Targets");

  std::vector<std::string> targetsVec;
  boost::split(targetsVec, targetsLine, boost::is_any_of("\t"));

  // Erase the first item ("Targets")
  targetsVec.erase(targetsVec.begin());

  QList<int> targets;
  for (auto &it : targetsVec) {
    int target = 0;
    Mantid::Kernel::Strings::convert<int>(it, target);
    targets << target;
  }

  auto t = window->newTableStatistics(
      window->table(QString::fromStdString(tableName)),
      type == "row" ? TableStatistics::row : TableStatistics::column, targets,
      QString::fromStdString(name));

  if (!t)
    return;

  window->setListViewDate(QString::fromStdString(name),
                          QString::fromStdString(birthDate));
  t->setBirthDate(QString::fromStdString(birthDate));

  t->loadFromProject(lines, window, fileVersion);
}

/**
 * Open a new surface plot window
 * @param lines :: the string of characters from a Mantid project file
 * @param fileVersion :: the version of the project file
 */
void ProjectSerialiser::openSurfacePlot(const std::string &lines,
                                        const int fileVersion) {
  std::vector<std::string> lineVec, valVec;
  boost::split(lineVec, lines, boost::is_any_of("\n"));

  // First line is name\tdate
  const std::string firstLine = lineVec[0];
  boost::split(valVec, firstLine, boost::is_any_of("\t"));

  if (valVec.size() < 2)
    return;

  const std::string caption = valVec[0];
  const std::string dateStr = valVec[1];
  valVec.clear();

  const std::string tsvLines = boost::algorithm::join(lineVec, "\n");

  TSVSerialiser tsv(tsvLines);

  Graph3D *plot = nullptr;

  if (tsv.selectLine("SurfaceFunction")) {
    std::string funcStr;
    double val2, val3, val4, val5, val6, val7;
    tsv >> funcStr >> val2 >> val3 >> val4 >> val5 >> val6 >> val7;

    const QString funcQStr = QString::fromStdString(funcStr);

    if (funcQStr.endsWith("(Y)", Qt::CaseSensitive)) {
      plot = window->dataPlot3D(QString::fromStdString(caption),
                                QString::fromStdString(funcStr), val2, val3,
                                val4, val5, val6, val7);
    } else if (funcQStr.contains("(Z)", Qt::CaseSensitive) > 0) {
      plot = window->openPlotXYZ(QString::fromStdString(caption),
                                 QString::fromStdString(funcStr), val2, val3,
                                 val4, val5, val6, val7);
    } else if (funcQStr.startsWith("matrix<", Qt::CaseSensitive) &&
               funcQStr.endsWith(">", Qt::CaseInsensitive)) {
      plot = window->openMatrixPlot3D(QString::fromStdString(caption),
                                      QString::fromStdString(funcStr), val2,
                                      val3, val4, val5, val6, val7);
    } else if (funcQStr.contains("mantidMatrix3D")) {
      MantidMatrix *m = nullptr;
      if (tsv.selectLine("title")) {
        std::string wsName = tsv.asString(1);

        // wsName is actually "Workspace workspacename", so we chop off
        // the first 10 characters.
        if (wsName.length() < 11)
          return;

        wsName = wsName.substr(10, std::string::npos);
        m = window->findMantidMatrixWindow(wsName);
      } // select line "title"

      int style = Qwt3D::WIREFRAME;
      if (tsv.selectLine("Style"))
        tsv >> style;

      if (m)
        plot = m->plotGraph3D(style);
    } else if (funcQStr.contains(",")) {
      QStringList l = funcQStr.split(",", QString::SkipEmptyParts);
      plot = window->plotParametricSurface(
          l[0], l[1], l[2], l[3].toDouble(), l[4].toDouble(), l[5].toDouble(),
          l[6].toDouble(), l[7].toInt(), l[8].toInt(), l[9].toInt(),
          l[10].toInt());
    } else {
      QStringList l = funcQStr.split(";", QString::SkipEmptyParts);
      if (l.count() == 1) {
        plot =
            window->plotSurface(funcQStr, val2, val3, val4, val5, val6, val7);
      } else if (l.count() == 3) {
        plot = window->plotSurface(l[0], val2, val3, val4, val5, val6, val7,
                                   l[1].toInt(), l[2].toInt());
      }
      window->setWindowName(plot, QString::fromStdString(caption));
    }
  }

  if (!plot)
    return;

  window->setListViewDate(QString::fromStdString(caption),
                          QString::fromStdString(dateStr));
  plot->setBirthDate(QString::fromStdString(dateStr));
  plot->setIgnoreFonts(true);
  window->restoreWindowGeometry(
      window, plot, QString::fromStdString(tsv.lineAsString("geometry")));
  plot->loadFromProject(tsvLines, window, fileVersion);
}

/**
 * Open a new script window
 * @param lines :: the string of characters from a Mantid project file
 * @param fileVersion :: the version of the project file
 */
void ProjectSerialiser::openScriptWindow(const std::string &lines,
                                         const int fileVersion) {
  window->showScriptWindow();
  auto scriptingWindow = window->getScriptWindowHandle();

  if (!scriptingWindow)
    return;

  scriptingWindow->setWindowTitle(
      "MantidPlot: " + window->scriptingEnv()->languageName() + " Window");

  scriptingWindow->loadFromProject(lines, window, fileVersion);
}

/**
 * Open a new script window
 * @param files :: list of strings representing file names for python scripts
 */
void ProjectSerialiser::openScriptWindow(const QStringList &files) {
  window->showScriptWindow();
  auto scriptingWindow = window->getScriptWindowHandle();

  if (!scriptingWindow)
    return;

  scriptingWindow->setWindowTitle(
      "MantidPlot: " + window->scriptingEnv()->languageName() + " Window");

  scriptingWindow->loadFromFileList(files);
}

/**
 * Populate Mantid and the ADS with workspaces loaded from the project file
 *
 * @param s :: the string of characters loaded from a Mantid project file
 */
void ProjectSerialiser::populateMantidTreeWidget(const QString &lines) {
  QStringList list = lines.split("\t");
  QStringList::const_iterator line = list.begin();
  for (++line; line != list.end(); ++line) {
    if ((*line)
            .contains(',')) // ...it is a group and more work needs to be done
    {
      // Format of string is "GroupName, Workspace, Workspace, Workspace, ....
      // and so on "
      QStringList groupWorkspaces = (*line).split(',');
      std::string groupName = groupWorkspaces[0].toStdString();
      std::vector<std::string> inputWsVec;
      // Work through workspaces, load into Mantid and then push into
      // vectorgroup (ignore group name, start at 1)
      for (int i = 1; i < groupWorkspaces.size(); i++) {
        std::string wsName = groupWorkspaces[i].toStdString();
        loadWsToMantidTree(wsName);
        inputWsVec.push_back(wsName);
      }

      try {
        bool smallGroup(inputWsVec.size() < 2);
        if (smallGroup) // if the group contains less than two items...
        {
          // ...create a new workspace and then delete it later on (group
          // workspace requires two workspaces in order to run the alg)
          Mantid::API::IAlgorithm_sptr alg =
              Mantid::API::AlgorithmManager::Instance().create(
                  "CreateWorkspace", 1);
          alg->setProperty("OutputWorkspace", "boevsMoreBoevs");
          alg->setProperty<std::vector<double>>("DataX",
                                                std::vector<double>(2, 0.0));
          alg->setProperty<std::vector<double>>("DataY",
                                                std::vector<double>(2, 0.0));
          // execute the algorithm
          alg->execute();
          // name picked because random and won't ever be used.
          inputWsVec.emplace_back("boevsMoreBoevs");
        }

        // Group the workspaces as they were when the project was saved
        std::string algName("GroupWorkspaces");
        Mantid::API::IAlgorithm_sptr groupingAlg =
            Mantid::API::AlgorithmManager::Instance().create(algName, 1);
        groupingAlg->initialize();
        groupingAlg->setProperty("InputWorkspaces", inputWsVec);
        groupingAlg->setPropertyValue("OutputWorkspace", groupName);
        // execute the algorithm
        groupingAlg->execute();

        if (smallGroup) {
          // Delete the temporary workspace used to create a group of 1 or less
          // (currently can't have group of 0)
          Mantid::API::AnalysisDataService::Instance().remove("boevsMoreBoevs");
        }
      }
      // Error catching for algorithms
      catch (std::invalid_argument &) {
        QMessageBox::critical(window, "MantidPlot - Algorithm error",
                              " Error in Grouping Workspaces");
      } catch (Mantid::Kernel::Exception::NotFoundError &) {
        QMessageBox::critical(window, "MantidPlot - Algorithm error",
                              " Error in Grouping Workspaces");
      } catch (std::runtime_error &) {
        QMessageBox::critical(window, "MantidPlot - Algorithm error",
                              " Error in Grouping Workspaces");
      } catch (std::exception &) {
        QMessageBox::critical(window, "MantidPlot - Algorithm error",
                              " Error in Grouping Workspaces");
      }
    } else // ...not a group so just load the workspace
    {
      loadWsToMantidTree((*line).toStdString());
    }
  }
}

/**
   * Load a workspace into Mantid from a project directory
   *
   * @param wsName :: the name of the workspace to load
   */
void ProjectSerialiser::loadWsToMantidTree(const std::string &wsName) {
  if (wsName.empty()) {
    throw std::runtime_error("Workspace Name not found in project file ");
  }
  std::string fileName(window->workingDir.toStdString() + "/" + wsName);
  fileName.append(".nxs");
  window->mantidUI->loadWSFromFile(wsName, fileName);
}
