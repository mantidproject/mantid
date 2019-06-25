// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//-------------------------------------------
// Includes
//-------------------------------------------
#include "ScriptingWindow.h"
#include "ApplicationWindow.h"
#include "MantidQtWidgets/Common/DropEventHelper.h"
#include "MantidQtWidgets/Common/TSVSerialiser.h"
#include "MantidQtWidgets/Common/pixmaps.h"
#include "MultiTabScriptInterpreter.h"
#include "ScriptFileInterpreter.h"
#include "ScriptingEnv.h"

// Mantid
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/Common/IProjectSerialisable.h"

// MantidQt
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/ScriptEditor.h"

// Qt
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QFileInfo>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPrintDialog>
#include <QPrinter>
#include <QSettings>
#include <QTextEdit>
#include <QTextStream>
#include <QUrl>

using namespace Mantid;
using namespace MantidQt::API;
namespace DropEventHelper = MantidQt::MantidWidgets::DropEventHelper;

namespace {
/// static logger
Mantid::Kernel::Logger g_log("ScriptingWindow");
} // namespace

//-------------------------------------------
// Public member functions
//-------------------------------------------
/**
 * Constructor
 * @param env :: The scripting environment
 * @param parent :: The parent widget
 * @param flags :: Window flags passed to the base class
 */
ScriptingWindow::ScriptingWindow(ScriptingEnv *env, bool capturePrint,
                                 QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags), m_acceptClose(false) {
  Q_UNUSED(capturePrint);
  setObjectName("MantidScriptWindow");
  setAcceptDrops(true);

  // Sub-widgets
  m_manager = new MultiTabScriptInterpreter(env, this);
  setCentralWidget(m_manager);
  setFocusProxy(m_manager);

  // Create menus and actions
  initMenus();
  readSettings();

  setWindowIcon(QIcon(":/mantidplot.png"));
  setWindowTitle("MantidPlot: " + env->languageName() + " Window");

#ifdef Q_OS_MAC
  // Work around to ensure that floating windows remain on top of the main
  // application window, but below other applications on Mac.
  // Note: Qt::Tool cannot have both a max and min button on OSX
  flags |= Qt::Tool;
  flags |= Qt::Dialog;
  flags |= Qt::CustomizeWindowHint;
  flags |= Qt::WindowMinimizeButtonHint;
  flags |= Qt::WindowCloseButtonHint;
  setWindowFlags(flags);
#endif
}

/**
 * Destructor
 */
ScriptingWindow::~ScriptingWindow() { delete m_manager; }

/**
 * Is a script executing
 * @returns A flag indicating the current state
 */
bool ScriptingWindow::isExecuting() const { return m_manager->isExecuting(); }

/**
 * Save the settings on the window
 */
void ScriptingWindow::saveSettings() {
  QSettings settings;
  settings.beginGroup("/ScriptWindow");
  settings.setValue("/AlwaysOnTop", m_alwaysOnTop->isChecked());
  settings.setValue("/ProgressArrow", m_toggleProgress->isChecked());
  settings.setValue("/LastDirectoryVisited", m_manager->m_last_dir);
  settings.setValue("/RecentScripts", m_manager->recentScripts());
  settings.setValue("/ZoomLevel", m_manager->globalZoomLevel());
  settings.setValue("/ShowWhitespace", m_toggleWhitespace->isChecked());
  settings.setValue("/ReplaceTabs", m_manager->m_replaceTabs);
  settings.setValue("/TabWhitespaceCount", m_manager->m_tabWhitespaceCount);
  settings.setValue("/ScriptFontFamily", m_manager->m_fontFamily);
  settings.setValue("/CodeFolding", m_toggleFolding->isChecked());
  settings.setValue("/LineWrapping", m_toggleWrapping->isChecked());
  settings.setValue("/PreviousFiles", m_manager->fileNamesToQStringList());
  settings.endGroup();
}

/**
 * Read the settings on the window
 */
void ScriptingWindow::readSettings() {
  QSettings settings;
  settings.beginGroup("/ScriptWindow");
  QString lastdir = settings.value("LastDirectoryVisited", "").toString();
  // If nothing, set the last directory to the Mantid scripts directory (if
  // present)
  if (lastdir.isEmpty()) {
    lastdir = QString::fromStdString(
        Mantid::Kernel::ConfigService::Instance().getString(
            "pythonscripts.directory"));
  }
  m_manager->m_last_dir = lastdir;
  m_toggleProgress->setChecked(settings.value("ProgressArrow", true).toBool());
  m_manager->setRecentScripts(settings.value("/RecentScripts").toStringList());
  m_manager->m_globalZoomLevel = settings.value("ZoomLevel", 0).toInt();
  m_toggleFolding->setChecked(settings.value("CodeFolding", false).toBool());
  m_toggleWrapping->setChecked(settings.value("LineWrapping", false).toBool());
  m_toggleWhitespace->setChecked(
      settings.value("ShowWhitespace", false).toBool());

  m_manager->m_showWhitespace = m_toggleWhitespace->isChecked();
  m_manager->m_replaceTabs = settings.value("ReplaceTabs", true).toBool();
  m_manager->m_tabWhitespaceCount =
      settings.value("TabWhitespaceCount", 4).toInt();
  m_manager->m_fontFamily = settings.value("ScriptFontFamily", "").toString();
  openPreviousTabs(settings.value("/PreviousFiles", "").toStringList());

  settings.endGroup();
}

/**
 * Override the closeEvent
 * @param event :: A pointer to the event object
 */
void ScriptingWindow::closeEvent(QCloseEvent *event) {
  // We ideally don't want a close button but are force by some window managers.
  // Therefore if someone clicks close and MantidPlot is not quitting then we
  // will just hide
  if (!m_acceptClose) {
    emit hideMe();
    // this->hide();
    return;
  }

  emit closeMe();
  // This will ensure each is saved correctly
  m_manager->closeAllTabs();
  event->accept();
}

/**
 * Override the showEvent function
 * @param event :: A pointer to the event object
 */
void ScriptingWindow::showEvent(QShowEvent *event) {
  if (m_manager->count() == 0) {
    m_manager->newTab();
  }
  event->accept();
}

/**
 * Open a script directly. This is here for backwards compatibility with the old
 * ScriptWindow
 * class
 * @param filename :: The file name
 * @param newtab :: Do we want a new tab
 */
void ScriptingWindow::open(const QString &filename, bool newtab) {
  m_manager->open(newtab, filename);
}

/**
 * Executes whatever is in the current tab. Primarily useful for automatically
 * running a script loaded with open
 * @param mode :: The execution type
 * */
void ScriptingWindow::executeCurrentTab(const Script::ExecutionMode mode) {
  // Async will always return true before executing
  m_failureFlag = !m_manager->executeAll(mode);
}

//-------------------------------------------
// Private slot member functions
//-------------------------------------------
/// Populate file menu
void ScriptingWindow::populateFileMenu() {
  m_fileMenu->clear();
  const bool scriptsOpen(m_manager->count() > 0);

  m_fileMenu->addAction(m_newTab);
  m_fileMenu->addAction(m_openInNewTab);

  if (scriptsOpen) {
    m_fileMenu->addAction(m_openInCurTab);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_save);
    m_fileMenu->addAction(m_saveAs);
    m_fileMenu->addAction(m_print);
  }

  m_fileMenu->addSeparator();
  m_fileMenu->addMenu(m_recentScripts);
  m_recentScripts->setEnabled(m_manager->recentScripts().count() > 0);

  if (scriptsOpen) {
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_closeTab);
  }
}

/// Ensure the list is up to date
void ScriptingWindow::populateRecentScriptsMenu() {
  m_recentScripts->clear();
  QStringList recentScripts = m_manager->recentScripts();
  QStringListIterator iter(recentScripts);
  while (iter.hasNext()) {
    m_recentScripts->addAction(iter.next());
  }
}

/// Populate edit menu
void ScriptingWindow::populateEditMenu() {
  m_editMenu->clear();
  m_editMenu->addAction(m_undo);
  m_editMenu->addAction(m_redo);
  m_editMenu->addAction(m_cut);
  m_editMenu->addAction(m_copy);
  m_editMenu->addAction(m_paste);

  m_editMenu->addSeparator();
  m_editMenu->addAction(m_comment);
  m_editMenu->addAction(m_uncomment);

  m_editMenu->addSeparator();
  m_editMenu->addAction(m_tabsToSpaces);
  m_editMenu->addAction(m_spacesToTabs);

  m_editMenu->addSeparator();
  m_editMenu->addAction(m_find);
}

/// Populate execute menu
void ScriptingWindow::populateExecMenu() {
  m_runMenu->clear();
  m_runMenu->addAction(m_execSelect);
  m_runMenu->addAction(m_execAll);
  m_runMenu->addSeparator();
  m_runMenu->addAction(m_abortCurrent);
  m_runMenu->addSeparator();
  m_runMenu->addAction(m_clearScriptVars);
  m_runMenu->addSeparator();

  m_execModeMenu->clear();
  m_execModeMenu->addAction(m_execParallel);
  m_execModeMenu->addAction(m_execSerial);
  m_runMenu->addMenu(m_execModeMenu);
}

/// Populate window menu
void ScriptingWindow::populateWindowMenu() {
  m_windowMenu->clear();
  const bool scriptsOpen(m_manager->count() > 0);

  m_windowMenu->addAction(m_alwaysOnTop);
  m_windowMenu->addAction(m_hide);

  if (scriptsOpen) {
    m_windowMenu->addSeparator();
    m_windowMenu->addAction(m_zoomIn);
    m_windowMenu->addAction(m_zoomOut);
    m_windowMenu->addAction(m_resetZoom);
    m_windowMenu->addAction(m_selectFont);

    m_windowMenu->addSeparator();
    m_windowMenu->addAction(m_toggleProgress);
    m_windowMenu->addAction(m_toggleFolding);
    m_windowMenu->addAction(m_toggleWrapping);
    m_windowMenu->addAction(m_toggleWhitespace);

    m_windowMenu->addSeparator();
    m_windowMenu->addAction(m_openConfigTabs);
  }
}

/// Populate help menu
void ScriptingWindow::populateHelpMenu() {
  m_helpMenu->clear();
  m_helpMenu->addAction(m_showHelp);
  m_helpMenu->addAction(m_showPythonHelp);
}

/**
 */
void ScriptingWindow::updateWindowFlags() {
  Qt::WindowFlags flags = Qt::Window;
  if (m_alwaysOnTop->isChecked()) {
    flags |= Qt::WindowStaysOnTopHint;
  }
#ifdef Q_OS_MAC
  // Work around to ensure that floating windows remain on top of the main
  // application window, but below other applications on Mac.
  // Note: Qt::Tool cannot have both a max and min button on OSX
  flags |= Qt::Tool;
  flags |= Qt::CustomizeWindowHint;
  flags |= Qt::WindowMinimizeButtonHint;
  flags |= Qt::WindowCloseButtonHint;
  setWindowFlags(flags);
#endif
  setWindowFlags(flags);
  // This is necessary due to the setWindowFlags function reparenting the window
  // and causing is
  // to hide itself
  show();
}

/**
 *  Update menus based on current tab states. Called when
 *  the number of tabs changes
 *  @param ntabs :: The number of tabs now open
 */
void ScriptingWindow::setMenuStates(int ntabs) {
  const bool tabsOpen(ntabs > 0);
  m_editMenu->setEnabled(tabsOpen);
  m_runMenu->setEnabled(tabsOpen);
}

/**
 * Set the state of the execution actions depending on the flag
 * @param off :: If the true the items are disabled, otherwise the are enabled
 */
void ScriptingWindow::setEditActionsDisabled(bool off) {
  auto actions = m_editMenu->actions();
  foreach (QAction *action, actions) {
    if (strcmp("Find", action->objectName().toAscii().constData()) != 0) {
      action->setDisabled(off);
    }
  }
}

/**
 * Set the state of the execution actions/menu depending on the flag
 * @param off :: If the true the items are disabled, otherwise the are enabled
 */
void ScriptingWindow::setExecutionActionsDisabled(bool off) {
  m_execSelect->setDisabled(off);
  m_execAll->setDisabled(off);
  m_execModeMenu->setDisabled(off);
  m_clearScriptVars->setDisabled(off);
  // Abort should be opposite
  setAbortActionsDisabled(!off);
}

/**
 * Set the state of the execution actions/menu depending on the flag
 * @param off :: If the true the items are disabled else they are enabled
 */
void ScriptingWindow::setAbortActionsDisabled(bool off) {
  if (!shouldEnableAbort())
    off = true;
  m_abortCurrent->setDisabled(off);
}

/**
 * Maps the QAction to an index in the recent scripts list
 * @param item A pointer to the action that triggered the slot
 */
void ScriptingWindow::openRecentScript(QAction *item) {
  const QList<QAction *> actions = m_recentScripts->actions();
  const int index = actions.indexOf(item);
  assert(index >= 0);
  m_manager->openRecentScript(index);
}

/**
 * Ask the manager to execute all code based on the currently selected mode
 */
void ScriptingWindow::executeAll() {
  m_manager->executeAll(this->getExecutionMode());
}

/**
 * Ask the manager to execute the current selection based on the currently
 * selected mode
 */
void ScriptingWindow::executeSelection() {
  m_manager->executeSelection(this->getExecutionMode());
}

/**
 * Ask the manager to abort the script execution for the current script.
 */
void ScriptingWindow::abortCurrent() { m_manager->abortCurrentScript(); }

/**
 */
void ScriptingWindow::clearScriptVariables() {
  m_manager->clearScriptVariables();
}

/**
 * Opens the Qt help windows for the scripting window.
 */
void ScriptingWindow::showHelp() {
  MantidQt::API::HelpWindow::showCustomInterface(nullptr,
                                                 QString("Scripting Window"));
}

/**
 * Opens the Qt help windows for the Python API.
 */
void ScriptingWindow::showPythonHelp() {
  MantidQt::API::HelpWindow::showPage(
      nullptr, QString("qthelp://org.mantidproject/doc/api/python/index.html"));
}

/**
 * Calls MultiTabScriptInterpreter to save the currently opened
 * script file names to a string.
 *
 * @param app :: the current application window instance
 * @return script file names in the matid project format
 */
std::string ScriptingWindow::saveToProject(ApplicationWindow *app) {
  (void)app; // suppress unused variable warnings
  return m_manager->saveToString().toStdString();
}

/**
 * Load script files from the project file
 *
 * @param lines :: raw lines from the project file
 * @param app :: the current application window instance
 * @param fileVersion :: the file version used when saved
 */
void ScriptingWindow::loadFromProject(const std::string &lines,
                                      ApplicationWindow *app,
                                      const int fileVersion) {
  Q_UNUSED(fileVersion);

  MantidQt::API::TSVSerialiser sTSV(lines);
  QStringList files;

  setWindowTitle("MantidPlot: " + app->scriptingEnv()->languageName() +
                 " Window");

  auto scriptNames = sTSV.values("ScriptNames");

  // Iterate, ignoring scriptNames[0] which is just "ScriptNames"
  for (size_t i = 1; i < scriptNames.size(); ++i)
    files.append(QString::fromStdString(scriptNames[i]));

  loadFromFileList(files);
}

/**
 * Load script files from a list of file names
 * @param files :: List of file names to oepn
 */
void ScriptingWindow::loadFromFileList(const QStringList &files) {
  for (auto file = files.begin(); file != files.end(); ++file) {
    if (file->isEmpty())
      continue;
    openUnique(*file);
  }
}

/**
 * Saves scripts file names to a string
 * @param value If true a future close event will be accepted otherwise it will
 * be ignored
 */
void ScriptingWindow::acceptCloseEvent(const bool value) {
  m_acceptClose = value;
}

//-------------------------------------------
// Protected non-slot member functions
//-------------------------------------------
/**
 * Accept a custom event and in this case test if it is a ScriptingChangeEvent
 * @param event :: The custom event
 */
void ScriptingWindow::customEvent(QEvent *event) {
  if (!m_manager->isExecuting() && event->type() == SCRIPTING_CHANGE_EVENT) {
    ScriptingChangeEvent *sce = static_cast<ScriptingChangeEvent *>(event);
    setWindowTitle("MantidPlot: " + sce->scriptingEnv()->languageName() +
                   " Window");
  }
}

/**
 * Accept a drag enter event and selects whether to accept the action
 * @param de :: The drag enter event
 */
void ScriptingWindow::dragEnterEvent(QDragEnterEvent *de) {
  const QMimeData *mimeData = de->mimeData();
  if (mimeData->hasUrls()) {
    const auto pythonFilenames = DropEventHelper::extractPythonFiles(de);
    if (!pythonFilenames.empty()) {
      de->acceptProposedAction();
    }
  }
}

/**
 * Accept a drag move event and selects whether to accept the action
 * @param de :: The drag move event
 */
void ScriptingWindow::dragMoveEvent(QDragMoveEvent *de) {
  const QMimeData *mimeData = de->mimeData();
  if (mimeData->hasUrls()) {
    const auto pythonFilenames = DropEventHelper::extractPythonFiles(de);
    if (!pythonFilenames.empty()) {
      de->accept();
    }
  }
}

/**
 * Accept a drag drop event and process the data appropriately
 * @param de :: The drag drop event
 */
void ScriptingWindow::dropEvent(QDropEvent *de) {
  const QMimeData *mimeData = de->mimeData();
  if (mimeData->hasUrls()) {
    const auto filenames = DropEventHelper::extractPythonFiles(de);
    de->acceptProposedAction();

    for (const auto &name : filenames) {
      m_manager->openInNewTab(name);
    }
  }
}

//-------------------------------------------
// Private non-slot member functions
//-------------------------------------------

/**
 * Initialise the menus
 */
void ScriptingWindow::initMenus() {
  initActions();

  m_fileMenu = menuBar()->addMenu(tr("&File"));
#ifdef SCRIPTING_DIALOG
  m_scripting_lang = new QAction(tr("Scripting &language"), this);
  connect(m_scripting_lang, SIGNAL(triggered()), this,
          SIGNAL(chooseScriptingLanguage()));
#endif
  connect(m_fileMenu, SIGNAL(aboutToShow()), this, SLOT(populateFileMenu()));

  m_editMenu = menuBar()->addMenu(tr("&Edit"));
  connect(m_editMenu, SIGNAL(aboutToShow()), this, SLOT(populateEditMenu()));
  connect(m_manager, SIGNAL(executionStateChanged(bool)), this,
          SLOT(setEditActionsDisabled(bool)));

  m_runMenu = menuBar()->addMenu(tr("E&xecute"));
  connect(m_runMenu, SIGNAL(aboutToShow()), this, SLOT(populateExecMenu()));
  connect(m_manager, SIGNAL(executionStateChanged(bool)), this,
          SLOT(setExecutionActionsDisabled(bool)));
  m_execModeMenu = new QMenu("Mode", this);

  m_windowMenu = menuBar()->addMenu(tr("&Window"));
  connect(m_windowMenu, SIGNAL(aboutToShow()), this,
          SLOT(populateWindowMenu()));

  m_helpMenu = menuBar()->addMenu(tr("&Help"));
  connect(m_windowMenu, SIGNAL(aboutToShow()), this, SLOT(populateHelpMenu()));

  connect(m_manager, SIGNAL(tabCountChanged(int)), this,
          SLOT(setMenuStates(int)));

  // The menu items must be populated for the shortcuts to work
  populateFileMenu();
  populateEditMenu();
  populateExecMenu();
  populateWindowMenu();
  populateHelpMenu();
  connect(m_manager, SIGNAL(tabCountChanged(int)), this,
          SLOT(populateFileMenu()));
  connect(m_manager, SIGNAL(tabCountChanged(int)), this,
          SLOT(populateEditMenu()));
  connect(m_manager, SIGNAL(tabCountChanged(int)), this,
          SLOT(populateExecMenu()));
  connect(m_manager, SIGNAL(tabCountChanged(int)), this,
          SLOT(populateWindowMenu()));
  connect(m_manager, SIGNAL(tabCountChanged(int)), this,
          SLOT(populateHelpMenu()));
}

/**
 *  Create all actions
 */
void ScriptingWindow::initActions() {
  initFileMenuActions();
  initEditMenuActions();
  initExecMenuActions();
  initWindowMenuActions();
  initHelpMenuActions();
}

/**
 * Create the file actions
 */
void ScriptingWindow::initFileMenuActions() {
  m_newTab = new QAction(tr("&New Tab"), this);
  connect(m_newTab, SIGNAL(triggered()), m_manager, SLOT(newTab()));
  m_newTab->setShortcut(tr("Ctrl+N"));

  m_openInCurTab = new QAction(tr("&Open"), this);
  connect(m_openInCurTab, SIGNAL(triggered()), m_manager,
          SLOT(openInCurrentTab()));
  m_openInCurTab->setShortcut(tr("Ctrl+O"));

  m_openInNewTab = new QAction(tr("&Open in New Tab"), this);
  connect(m_openInNewTab, SIGNAL(triggered()), m_manager, SLOT(openInNewTab()));
  m_openInNewTab->setShortcut(tr("Ctrl+Shift+O"));

  m_save = new QAction(tr("&Save"), this);
  connect(m_save, SIGNAL(triggered()), m_manager, SLOT(saveToCurrentFile()));
  m_save->setShortcut(QKeySequence::Save);

  m_saveAs = new QAction(tr("&Save As"), this);
  connect(m_saveAs, SIGNAL(triggered()), m_manager, SLOT(saveAs()));
  m_saveAs->setShortcut(tr("Ctrl+Shift+S"));

  m_print = new QAction(tr("&Print script"), this);
  connect(m_print, SIGNAL(triggered()), m_manager, SLOT(print()));
  m_print->setShortcut(QKeySequence::Print);

  m_closeTab = new QAction(tr("&Close Tab"), this);
  connect(m_closeTab, SIGNAL(triggered()), m_manager, SLOT(closeCurrentTab()));
  m_closeTab->setShortcut(tr("Ctrl+W"));

  m_recentScripts = new QMenu(tr("&Recent Scripts"), this);
  connect(m_recentScripts, SIGNAL(aboutToShow()), this,
          SLOT(populateRecentScriptsMenu()));
  connect(m_recentScripts, SIGNAL(triggered(QAction *)), this,
          SLOT(openRecentScript(QAction *)));
}

/**
 * Create the edit menu action*/
void ScriptingWindow::initEditMenuActions() {
  m_undo = new QAction(tr("&Undo"), this);
  connect(m_undo, SIGNAL(triggered()), m_manager, SLOT(undo()));
  connect(m_manager, SIGNAL(undoAvailable(bool)), m_undo,
          SLOT(setEnabled(bool)));
  m_undo->setShortcut(QKeySequence::Undo);

  m_redo = new QAction(tr("&Redo"), this);
  connect(m_redo, SIGNAL(triggered()), m_manager, SLOT(redo()));
  connect(m_manager, SIGNAL(redoAvailable(bool)), m_redo,
          SLOT(setEnabled(bool)));
  m_redo->setShortcut(QKeySequence::Redo);

  m_cut = new QAction(tr("C&ut"), this);
  connect(m_cut, SIGNAL(triggered()), m_manager, SLOT(cut()));
  m_cut->setShortcut(QKeySequence::Cut);

  m_copy = new QAction(tr("&Copy"), this);
  connect(m_copy, SIGNAL(triggered()), m_manager, SLOT(copy()));
  m_copy->setShortcut(QKeySequence::Copy);

  m_paste = new QAction(tr("&Paste"), this);
  connect(m_paste, SIGNAL(triggered()), m_manager, SLOT(paste()));
  m_paste->setShortcut(QKeySequence::Paste);

  m_comment = new QAction(tr("Co&mment"), this);
  connect(m_comment, SIGNAL(triggered()), m_manager, SLOT(comment()));
  m_comment->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_M));

  m_uncomment = new QAction(tr("Uncomment"), this);
  connect(m_uncomment, SIGNAL(triggered()), m_manager, SLOT(uncomment()));
  m_uncomment->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_M));

  m_tabsToSpaces = new QAction(tr("Tabs to Spaces"), this);
  connect(m_tabsToSpaces, SIGNAL(triggered()), m_manager, SLOT(tabsToSpaces()));

  m_spacesToTabs = new QAction(tr("Spaces to Tabs"), this);
  connect(m_spacesToTabs, SIGNAL(triggered()), m_manager, SLOT(spacesToTabs()));

  m_find = new QAction(tr("&Find/Replace"), this);
  connect(m_find, SIGNAL(triggered()), m_manager,
          SLOT(showFindReplaceDialog()));
  m_find->setShortcut(QKeySequence::Find);
}

/**
 * Create the execute menu actions
 */
void ScriptingWindow::initExecMenuActions() {
  m_execSelect = new QAction(tr("E&xecute Selection"), this);
  connect(m_execSelect, SIGNAL(triggered()), this, SLOT(executeSelection()));

  QList<QKeySequence> shortcuts;
  shortcuts << Qt::CTRL + Qt::Key_Return << Qt::CTRL + Qt::Key_Enter;
  m_execSelect->setShortcuts(shortcuts);

  m_execAll = new QAction(tr("Execute &All"), this);
  connect(m_execAll, SIGNAL(triggered()), this, SLOT(executeAll()));
  shortcuts.clear();
  shortcuts << Qt::CTRL + Qt::SHIFT + Qt::Key_Return
            << Qt::CTRL + Qt::SHIFT + Qt::Key_Enter;
  m_execAll->setShortcuts(shortcuts);

  m_abortCurrent = new QAction(tr("A&bort"), this);
  connect(m_abortCurrent, SIGNAL(triggered()), this, SLOT(abortCurrent()));
  shortcuts.clear();
  shortcuts << Qt::CTRL + Qt::Key_D;
  m_abortCurrent->setShortcuts(shortcuts);
  setAbortActionsDisabled(false);

  m_clearScriptVars = new QAction(tr("&Clear Variables"), this);
  connect(m_clearScriptVars, SIGNAL(triggered()), this,
          SLOT(clearScriptVariables()));
  m_clearScriptVars->setToolTip(
      "Clear all variable definitions in this script");

  m_execParallel = new QAction("Asynchronous", this);
  m_execParallel->setCheckable(true);
  m_execSerial = new QAction("Serialised", this);
  m_execSerial->setCheckable(true);

  m_execModeGroup = new QActionGroup(this);
  m_execModeGroup->addAction(m_execParallel);
  m_execModeGroup->addAction(m_execSerial);
  m_execParallel->setChecked(true);
}

/**
 * Create the window menu actions
 */
void ScriptingWindow::initWindowMenuActions() {
  m_alwaysOnTop = new QAction(tr("Always on &Top"), this);
  m_alwaysOnTop->setCheckable(true);
  connect(m_alwaysOnTop, SIGNAL(toggled(bool)), this,
          SLOT(updateWindowFlags()));

  m_hide = new QAction(tr("&Hide"), this);
#ifdef __APPLE__
  m_hide->setShortcut(tr("Ctrl+3")); // F3 is used by the window manager on Mac
#else
  m_hide->setShortcut(tr("F3"));
#endif
  // Note that we channel the hide through the parent so that we can save the
  // geometry state
  connect(m_hide, SIGNAL(triggered()), this, SIGNAL(hideMe()));

  m_zoomIn = new QAction(("&Increase font size"), this);
  // Setting two shortcuts makes it work for both the plus on the keypad and one
  // above an =
  // Despite the Qt docs advertising the use of QKeySequence::ZoomIn as the
  // solution to this,
  // it doesn't seem to work for me
  m_zoomIn->setShortcut(Qt::SHIFT + Qt::CTRL + Qt::Key_Equal);
  m_zoomIn->setShortcut(Qt::CTRL + Qt::Key_Plus);
  connect(m_zoomIn, SIGNAL(triggered()), m_manager, SLOT(zoomIn()));
  connect(m_zoomIn, SIGNAL(triggered()), m_manager, SLOT(trackZoomIn()));

  m_zoomOut = new QAction(("&Decrease font size"), this);
  m_zoomOut->setShortcut(QKeySequence::ZoomOut);
  connect(m_zoomOut, SIGNAL(triggered()), m_manager, SLOT(zoomOut()));
  connect(m_zoomOut, SIGNAL(triggered()), m_manager, SLOT(trackZoomOut()));

  m_resetZoom = new QAction(("&Reset font size"), this);
  connect(m_resetZoom, SIGNAL(triggered()), m_manager, SLOT(resetZoom()));

  // Show font selection dialog
  m_selectFont = new QAction(tr("Select Font"), this);
  connect(m_selectFont, SIGNAL(triggered()), m_manager, SLOT(showSelectFont()));

  // Toggle the progress arrow
  m_toggleProgress = new QAction(tr("&Progress Reporting"), this);
  m_toggleProgress->setCheckable(true);
  connect(m_toggleProgress, SIGNAL(toggled(bool)), m_manager,
          SLOT(toggleProgressReporting(bool)));

  // Toggle code folding
  m_toggleFolding = new QAction(tr("Code &Folding"), this);
  m_toggleFolding->setCheckable(true);
  connect(m_toggleFolding, SIGNAL(toggled(bool)), m_manager,
          SLOT(toggleCodeFolding(bool)));

  m_toggleWrapping = new QAction(tr("Line &Wrapping"), this);
  m_toggleWrapping->setCheckable(true);
  connect(m_toggleWrapping, SIGNAL(toggled(bool)), m_manager,
          SLOT(toggleLineWrapping(bool)));

  // Toggle the whitespace arrow
  m_toggleWhitespace = new QAction(tr("&Show Whitespace"), this);
  m_toggleWhitespace->setCheckable(true);
  connect(m_toggleWhitespace, SIGNAL(toggled(bool)), m_manager,
          SLOT(toggleWhitespace(bool)));

  // Open Config Tabs dialog
  m_openConfigTabs = new QAction(tr("Configure Tabs"), this);
  connect(m_openConfigTabs, SIGNAL(triggered()), m_manager,
          SLOT(openConfigTabs()));
}

/**
 * Create the help menu actions
 */
void ScriptingWindow::initHelpMenuActions() {
  // Show Qt help window
  m_showHelp = new QAction(tr("Scripting Window Help"), this);
  connect(m_showHelp, SIGNAL(triggered()), this, SLOT(showHelp()));

  // Show Qt help window for Python API
  m_showPythonHelp = new QAction(tr("Python API Help"), this);
  connect(m_showPythonHelp, SIGNAL(triggered()), this, SLOT(showPythonHelp()));
}

/// Should we enable abort functionality
bool ScriptingWindow::shouldEnableAbort() const {
  return m_manager->scriptingEnv()->supportsAbortRequests();
}

/**
 * Opens a script providing a copy is not already open. On exit the
 * active tab will be the one containing the given script.
 * @param filename The name of the newTab to open
 */
void ScriptingWindow::openUnique(QString filename) {
  auto openFiles = m_manager->fileNamesToQStringList();
  // The list of open files contains absolute paths so make sure we have one
  // here
  filename = QFileInfo(filename).absoluteFilePath();
  auto position = openFiles.indexOf(filename);
  if (position < 0) {
    m_manager->newTab(openFiles.size(), filename);
  } else {
    // make it the current tab
    m_manager->setCurrentIndex(position);
  }
}

/**
 * Opens a set of files in new tabs
 * @param tabsToOpen A list of filenames to open in new tabs
 */
void ScriptingWindow::openPreviousTabs(const QStringList &tabsToOpen) {
  const int totalFiles = tabsToOpen.size();
  QStringList files = QStringList();
  // Check files can be opened
  for (int i = 0; i < totalFiles; i++) {
    if (FILE *file = fopen(tabsToOpen[i].toStdString().c_str(), "r")) {
      fclose(file);
      files.append(tabsToOpen[i]);
    }
  }

  // Remove duplicates
  files.removeDuplicates();

  // opens files providing there are more than 0
  const int validTotal = files.size();
  if (validTotal == 0) {
    m_manager->newTab();
  } else {
    for (int i = 0; i < validTotal; i++) {
      m_manager->newTab(i, files[i]);
    }
  }
}

/**
 * Returns the current execution mode set in the menu
 */
Script::ExecutionMode ScriptingWindow::getExecutionMode() const {
  if (m_execParallel->isChecked())
    return Script::Asynchronous;
  else
    return Script::Serialised;
}

const Script &ScriptingWindow::getCurrentScriptRunner() {
  return m_manager->currentInterpreter()->getRunner();
}