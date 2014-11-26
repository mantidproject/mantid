//-------------------------------------------
// Includes
//-------------------------------------------
#include "ScriptingWindow.h"
#include "MultiTabScriptInterpreter.h"
#include "ScriptingEnv.h"
#include "ScriptFileInterpreter.h"
#include "pixmaps.h"

// Mantid
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "ApplicationWindow.h"

// MantidQt
#include "MantidQtMantidWidgets/ScriptEditor.h"

//Qt
#include <QTextEdit>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QPrintDialog>
#include <QPrinter>
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QTextStream>
#include <QList>
#include <QUrl>

namespace
{
  /// static logger
  Mantid::Kernel::Logger g_log("ScriptingWindow");
}

//-------------------------------------------
// Public member functions
//-------------------------------------------
/**
 * Constructor
 * @param env :: The scripting environment
 * @param parent :: The parent widget
 * @param flags :: Window flags passed to the base class
 */
ScriptingWindow::ScriptingWindow(ScriptingEnv *env, bool capturePrint, QWidget *parent, Qt::WindowFlags flags) :
  QMainWindow(parent, flags), m_acceptClose(false)
{
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

  setWindowIcon(QIcon(":/MantidPlot_Icon_32offset.png"));
  setWindowTitle("MantidPlot: " + env->languageName() + " Window");

  // Start with a single script
  m_manager->newTab();
}

/**
 * Destructor
 */
ScriptingWindow::~ScriptingWindow()
{
  delete m_manager;
}

/**
 * Is a script executing
 * @returns A flag indicating the current state
 */
bool ScriptingWindow::isExecuting() const
{
  return m_manager->isExecuting();
}

/**
 * Save the settings on the window
 */
void ScriptingWindow::saveSettings()
{
  QSettings settings;
  settings.beginGroup("/ScriptWindow");
  settings.setValue("/AlwaysOnTop", m_alwaysOnTop->isChecked());
  settings.setValue("/ProgressArrow", m_toggleProgress->isChecked());
  settings.setValue("/LastDirectoryVisited", m_manager->m_last_dir);
  settings.setValue("/RecentScripts",m_manager->recentScripts());
  settings.setValue("/ZoomLevel",m_manager->globalZoomLevel());
  settings.setValue("/ShowWhitespace", m_toggleWhitespace->isChecked());
  settings.setValue("/ReplaceTabs", m_manager->m_replaceTabs);
  settings.setValue("/TabWhitespaceCount", m_manager->m_tabWhitespaceCount);
  settings.setValue("/ScriptFontFamily", m_manager->m_fontFamily);
  settings.setValue("/CodeFolding", m_toggleFolding->isChecked());

  settings.endGroup();
}

/**
 * Read the settings on the window
 */
void ScriptingWindow::readSettings()
{
  QSettings settings;
  settings.beginGroup("/ScriptWindow");
  QString lastdir = settings.value("LastDirectoryVisited", "").toString();
  // If nothing, set the last directory to the Mantid scripts directory (if present)
  if( lastdir.isEmpty() )
  {
    lastdir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("pythonscripts.directory"));
  }
  m_manager->m_last_dir = lastdir;
  m_toggleProgress->setChecked(settings.value("ProgressArrow", true).toBool());
  m_manager->setRecentScripts(settings.value("/RecentScripts").toStringList());
  m_manager->m_globalZoomLevel = settings.value("ZoomLevel",0).toInt();
  m_toggleFolding->setChecked(settings.value("CodeFolding", false).toBool());
  m_toggleWhitespace->setChecked(settings.value("ShowWhitespace", false).toBool());
  
  m_manager->m_replaceTabs = settings.value("ReplaceTabs", true ).toBool();
  m_manager->m_tabWhitespaceCount = settings.value("TabWhitespaceCount", 4).toInt();
  m_manager->m_fontFamily = settings.value("ScriptFontFamily","").toString();
  
  settings.endGroup();

}

/**
 * Override the closeEvent
 * @param event :: A pointer to the event object
 */
void ScriptingWindow::closeEvent(QCloseEvent *event)
{
  // We ideally don't want a close button but are force by some window managers.
  // Therefore if someone clicks close and MantidPlot is not quitting then we will just hide
  if( !m_acceptClose ) 
  {
    emit hideMe();
    //this->hide();
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
void ScriptingWindow::showEvent(QShowEvent *event)
{
  if( m_manager->count() == 0 )
  {
    m_manager->newTab();
  }
  event->accept();
}


/**
 * Open a script directly. This is here for backwards compatability with the old ScriptWindow
 * class
 * @param filename :: The file name
 * @param newtab :: Do we want a new tab
 */
void ScriptingWindow::open(const QString & filename, bool newtab)
{
  m_manager->open(newtab, filename);
}

/**
 * Executes whatever is in the current tab. Primarily useful for automatically
 * running a script loaded with open
 * @param mode :: The execution type
 * */
void ScriptingWindow::executeCurrentTab(const Script::ExecutionMode mode)
{
  m_manager->executeAll(mode);
}

//-------------------------------------------
// Private slot member functions
//-------------------------------------------
/// Populate file menu
void ScriptingWindow::populateFileMenu()
{
  m_fileMenu->clear();
  const bool scriptsOpen(m_manager->count() > 0);

  m_fileMenu->addAction(m_newTab);
  m_fileMenu->addAction(m_openInNewTab);

  if(scriptsOpen)
  {
    m_fileMenu->addAction(m_openInCurTab);
    m_fileMenu->insertSeparator();
    m_fileMenu->addAction(m_save);
    m_fileMenu->addAction(m_saveAs);
    m_fileMenu->addAction(m_print);
  }

  m_fileMenu->insertSeparator();
  m_fileMenu->addMenu(m_recentScripts);
  m_recentScripts->setEnabled(m_manager->recentScripts().count() > 0);

  if(scriptsOpen)
  {
    m_fileMenu->insertSeparator();
    m_fileMenu->addAction(m_closeTab);
  }
}

/// Ensure the list is up to date
void ScriptingWindow::populateRecentScriptsMenu()
{
  m_recentScripts->clear();
  QStringList recentScripts = m_manager->recentScripts();
  QStringListIterator iter(recentScripts);
  while(iter.hasNext())
  {
    m_recentScripts->addAction(iter.next());
  }
}


/// Populate edit menu
void ScriptingWindow::populateEditMenu()
{
  m_editMenu->clear();
  m_editMenu->addAction(m_undo);
  m_editMenu->addAction(m_redo);
  m_editMenu->addAction(m_cut);
  m_editMenu->addAction(m_copy);
  m_editMenu->addAction(m_paste);

  m_editMenu->insertSeparator();
  m_editMenu->addAction(m_comment);
  m_editMenu->addAction(m_uncomment);

  m_editMenu->insertSeparator();
  m_editMenu->addAction(m_tabsToSpaces);
  m_editMenu->addAction(m_spacesToTabs);

  m_editMenu->insertSeparator();
  m_editMenu->addAction(m_find);
}
/// Populate execute menu
void ScriptingWindow::populateExecMenu()
{
  m_runMenu->clear();
  m_runMenu->addAction(m_execSelect);
  m_runMenu->addAction(m_execAll);

  m_runMenu->addSeparator();

  m_runMenu->addAction(m_clearScriptVars);

  m_runMenu->addSeparator();

  m_execModeMenu->clear();
  m_execModeMenu->addAction(m_execParallel);
  m_execModeMenu->addAction(m_execSerial);
  m_runMenu->addMenu(m_execModeMenu);
}

/// Populate window menu
void ScriptingWindow::populateWindowMenu()
{
  m_windowMenu->clear();
  const bool scriptsOpen(m_manager->count() > 0);

  m_windowMenu->addAction(m_alwaysOnTop);
  m_windowMenu->addAction(m_hide);

  if(scriptsOpen)
  {
    m_windowMenu->insertSeparator();
    m_windowMenu->addAction(m_zoomIn);
    m_windowMenu->addAction(m_zoomOut);
    m_windowMenu->addAction(m_resetZoom);

    m_windowMenu->insertSeparator();
    m_windowMenu->addAction(m_toggleProgress);
    m_windowMenu->addAction(m_toggleFolding);

    m_windowMenu->addAction(m_toggleWhitespace);
      m_windowMenu->insertSeparator();
    m_windowMenu->addAction(m_openConfigTabs);
    m_windowMenu->addAction(m_selectFont);
  }
}

/**
 *
 */
void ScriptingWindow::updateWindowFlags()
{
  Qt::WindowFlags flags = Qt::Window;
  if( m_alwaysOnTop->isChecked() )
  {
    flags |= Qt::WindowStaysOnTopHint;
  }
  setWindowFlags(flags);
  //This is necessary due to the setWindowFlags function reparenting the window and causing is
  //to hide itself
  show();
}

/**
 *  Update menus based on current tab states. Called when
 *  the number of tabs changes
 *  @param ntabs :: The number of tabs now open
 */
void ScriptingWindow::setMenuStates(int ntabs)
{
  const bool tabsOpen(ntabs > 0);
  m_editMenu->setEnabled(tabsOpen);
  m_runMenu->setEnabled(tabsOpen);
}

/**
 * Set the state of the execution actions/menu depending on the flag
 * @param state :: If the true the items are enabled, otherwise the are disabled
 */
void ScriptingWindow::setEditActionsDisabled(bool state)
{
  m_editMenu->setDisabled(state);
}

/**
 * Set the state of the execution actions/menu depending on the flag
 * @param state :: If the true the items are enabled, otherwise the are disabled
 */
void ScriptingWindow::setExecutionActionsDisabled(bool state)
{
  m_execSelect->setDisabled(state);
  m_execAll->setDisabled(state);
  m_execModeMenu->setDisabled(state);
  m_runMenu->setDisabled(state);
}

/**
 * Maps the QAction to an index in the recent scripts list
 * @param item A pointer to the action that triggered the slot
 */
void ScriptingWindow::openRecentScript(QAction* item)
{
  const QList<QAction*> actions = m_recentScripts->actions();
  const int index = actions.indexOf(item);
  assert(index >= 0);
  m_manager->openRecentScript(index);
}

/**
 * Ask the manager to execute all code based on the currently selected mode
 */
void ScriptingWindow::executeAll()
{
  m_manager->executeAll(this->getExecutionMode());
}

/**
 * Ask the manager to execute the current selection based on the currently selected mode
 */
void ScriptingWindow::executeSelection()
{
  m_manager->executeSelection(this->getExecutionMode());
}

/**
 */
void ScriptingWindow::clearScriptVariables()
{
  m_manager->clearScriptVariables();
}

/**
 * calls MultiTabScriptInterpreter saveToString and  
 *  saves the currently opened script file names to a string
 */
QString ScriptingWindow::saveToString()
{
  return m_manager->saveToString();
}

/**
 * Saves scripts file names to a string 
 * @param value If true a future close event will be accepted otherwise it will be ignored
 */
void ScriptingWindow::acceptCloseEvent(const bool value)
{
  m_acceptClose = value;
}

//-------------------------------------------
// Private non-slot member functions
//-------------------------------------------

/**
 * Initialise the menus
 */
void ScriptingWindow::initMenus()
{
  initActions();

  m_fileMenu = menuBar()->addMenu(tr("&File"));
#ifdef SCRIPTING_DIALOG
  m_scripting_lang = new QAction(tr("Scripting &language"), this);
  connect(m_scripting_lang, SIGNAL(triggered()), this, SIGNAL(chooseScriptingLanguage()));
#endif
  connect(m_fileMenu, SIGNAL(aboutToShow()), this, SLOT(populateFileMenu()));

  m_editMenu = menuBar()->addMenu(tr("&Edit"));
  connect(m_editMenu, SIGNAL(aboutToShow()), this, SLOT(populateEditMenu()));
  connect(m_manager, SIGNAL(executionStateChanged(bool)), this, SLOT(setEditActionsDisabled(bool)));

  m_runMenu = menuBar()->addMenu(tr("E&xecute"));
  connect(m_runMenu, SIGNAL(aboutToShow()), this, SLOT(populateExecMenu()));
  connect(m_manager, SIGNAL(executionStateChanged(bool)), this, SLOT(setExecutionActionsDisabled(bool)));
  m_execModeMenu = new QMenu("Mode", this);

  m_windowMenu = menuBar()->addMenu(tr("&Window"));
  connect(m_windowMenu, SIGNAL(aboutToShow()), this, SLOT(populateWindowMenu()));

  connect(m_manager, SIGNAL(tabCountChanged(int)), this, SLOT(setMenuStates(int)));

  // The menu items must be populated for the shortcuts to work
  populateFileMenu();
  populateEditMenu();
  populateExecMenu();
  populateWindowMenu();
  connect(m_manager, SIGNAL(tabCountChanged(int)), this, SLOT(populateFileMenu()));
  connect(m_manager, SIGNAL(tabCountChanged(int)), this, SLOT(populateEditMenu()));
  connect(m_manager, SIGNAL(tabCountChanged(int)), this, SLOT(populateExecMenu()));
  connect(m_manager, SIGNAL(tabCountChanged(int)), this, SLOT(populateWindowMenu()));

}


/**
 *  Create all actions
 */
void ScriptingWindow::initActions()
{
  initFileMenuActions();
  initEditMenuActions();
  initExecMenuActions();
  initWindowMenuActions();
}

/**
 * Create the file actions
 */
void ScriptingWindow::initFileMenuActions()
{
  m_newTab = new QAction(tr("&New Tab"), this);
  connect(m_newTab, SIGNAL(triggered()), m_manager, SLOT(newTab()));
  m_newTab->setShortcut(tr("Ctrl+N"));

  m_openInCurTab = new QAction(tr("&Open"), this);
  connect(m_openInCurTab, SIGNAL(triggered()), m_manager, SLOT(openInCurrentTab()));
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

  m_recentScripts = new QMenu(tr("&Recent Scripts"),this);
  connect(m_recentScripts, SIGNAL(aboutToShow()), this, SLOT(populateRecentScriptsMenu()));
  connect(m_recentScripts, SIGNAL(triggered(QAction*)), this, SLOT(openRecentScript(QAction*)));
}

/**
 * Create the edit menu action*/
void ScriptingWindow::initEditMenuActions()
{
  m_undo = new QAction(tr("&Undo"), this);
  connect(m_undo, SIGNAL(triggered()), m_manager, SLOT(undo()));
  connect(m_manager, SIGNAL(undoAvailable(bool)), m_undo, SLOT(setEnabled(bool)));
  m_undo->setShortcut(QKeySequence::Undo);

  m_redo = new QAction(tr("&Redo"), this);
  connect(m_redo, SIGNAL(triggered()), m_manager, SLOT(redo()));
  connect(m_manager, SIGNAL(redoAvailable(bool)), m_redo, SLOT(setEnabled(bool)));
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
void ScriptingWindow::initExecMenuActions()
{
  m_execSelect = new QAction(tr("E&xecute Selection"), this);
  connect(m_execSelect, SIGNAL(triggered()), this, SLOT(executeSelection()));

  QList<QKeySequence> shortcuts;
  shortcuts << Qt::CTRL + Qt::Key_Return << Qt::CTRL + Qt::Key_Enter;
  m_execSelect->setShortcuts(shortcuts);

  m_execAll = new QAction(tr("Execute &All"), this);
  connect(m_execAll, SIGNAL(triggered()), this, SLOT(executeAll()));
  shortcuts.clear();
  shortcuts << Qt::CTRL + Qt::SHIFT + Qt::Key_Return << Qt::CTRL + Qt::SHIFT + Qt::Key_Enter;
  m_execAll->setShortcuts(shortcuts);

  m_clearScriptVars = new QAction(tr("&Clear Variables"), this);
  connect(m_clearScriptVars, SIGNAL(triggered()), this, SLOT(clearScriptVariables()));
  m_clearScriptVars->setToolTip("Clear all variable definitions in this script");

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
void ScriptingWindow::initWindowMenuActions()
{
  m_alwaysOnTop = new QAction(tr("Always on &Top"), this);
  m_alwaysOnTop->setCheckable(true);
  connect(m_alwaysOnTop, SIGNAL(toggled(bool)), this, SLOT(updateWindowFlags()));

  m_hide = new QAction(tr("&Hide"), this);
#ifdef __APPLE__
  m_hide->setShortcut(tr("Ctrl+3")); // F3 is used by the window manager on Mac
#else
  m_hide->setShortcut(tr("F3"));
#endif
  // Note that we channel the hide through the parent so that we can save the geometry state
  connect(m_hide, SIGNAL(triggered()), this, SIGNAL(hideMe()));

  m_zoomIn = new QAction(("&Increase font size"), this);
  // Setting two shortcuts makes it work for both the plus on the keypad and one above an =
  // Despite the Qt docs advertising the use of QKeySequence::ZoomIn as the solution to this,
  // it doesn't seem to work for me
  m_zoomIn->setShortcut(Qt::SHIFT+Qt::CTRL+Qt::Key_Equal);
  m_zoomIn->setShortcut(Qt::CTRL+Qt::Key_Plus);
  connect(m_zoomIn, SIGNAL(triggered()), m_manager, SLOT(zoomIn()));
  connect(m_zoomIn, SIGNAL(triggered()), m_manager, SLOT(trackZoomIn()));

  m_zoomOut = new QAction(("&Decrease font size"), this);
  m_zoomOut->setShortcut(QKeySequence::ZoomOut);
  connect(m_zoomOut, SIGNAL(triggered()), m_manager, SLOT(zoomOut()));
  connect(m_zoomOut, SIGNAL(triggered()), m_manager, SLOT(trackZoomOut()));

  m_resetZoom = new QAction(("&Reset font size"), this);
  connect(m_resetZoom, SIGNAL(triggered()), m_manager, SLOT(resetZoom()));

  // Toggle the progress arrow
  m_toggleProgress = new QAction(tr("&Progress Reporting"), this);
  m_toggleProgress->setCheckable(true);
  connect(m_toggleProgress, SIGNAL(toggled(bool)), m_manager, SLOT(toggleProgressReporting(bool)));

  // Toggle code folding
  m_toggleFolding = new QAction(tr("Code &Folding"), this);
  m_toggleFolding->setCheckable(true);
  connect(m_toggleFolding, SIGNAL(toggled(bool)), m_manager, SLOT(toggleCodeFolding(bool)));

  // Toggle the whitespace arrow
  m_toggleWhitespace = new QAction(tr("&Show Whitespace"), this);
  m_toggleWhitespace->setCheckable(true);  
  connect(m_toggleWhitespace, SIGNAL(toggled(bool)), m_manager, SLOT(toggleWhitespace(bool)));

  // Open Config Tabs dialog
  m_openConfigTabs = new QAction(tr("Configure Tabs"), this);
  connect(m_openConfigTabs, SIGNAL(triggered()), m_manager, SLOT(openConfigTabs()));

  // Show font selection dialog
  m_selectFont = new QAction(tr("Select Font"), this);
  connect(m_selectFont, SIGNAL(triggered()), m_manager, SLOT(showSelectFont()));
}

/**
 * Returns the current execution mode set in the menu
 */
Script::ExecutionMode ScriptingWindow::getExecutionMode() const
{
  if(m_execParallel->isChecked()) return Script::Asynchronous;
  else return Script::Serialised;
}

/**
 * Accept a custom event and in this case test if it is a ScriptingChangeEvent
 * @param event :: The custom event
 */
void ScriptingWindow::customEvent(QEvent *event)
{
  if( !m_manager->isExecuting() && event->type() == SCRIPTING_CHANGE_EVENT )
  {
    ScriptingChangeEvent *sce = static_cast<ScriptingChangeEvent*>(event);
    setWindowTitle("MantidPlot: " + sce->scriptingEnv()->languageName() + " Window");
  }
}


/**
 * Accept a drag move event and selects whether to accept the action
 * @param de :: The drag move event
 */
void ScriptingWindow::dragMoveEvent(QDragMoveEvent *de)
{
  const QMimeData *mimeData = de->mimeData();  
  if (mimeData->hasUrls())
  {
    if (extractPyFiles(mimeData->urls()).size() > 0)
    {
      de->accept();
    }
  }
}
 
/**
 * Accept a drag enter event and selects whether to accept the action
 * @param de :: The drag enter event
 */
void ScriptingWindow::dragEnterEvent(QDragEnterEvent *de)
{
  const QMimeData *mimeData = de->mimeData();  
  if (mimeData->hasUrls())
  {
    if (extractPyFiles(mimeData->urls()).size() > 0)
    {
      de->acceptProposedAction();
    }
  }
}
 
/**
 * Accept a drag drop event and process the data appropriately
 * @param de :: The drag drop event
 */
void ScriptingWindow::dropEvent(QDropEvent *de)
{
  const QMimeData *mimeData = de->mimeData();  
  if (mimeData->hasUrls()) 
  {
    QStringList filenames = extractPyFiles(mimeData->urls());
    de->acceptProposedAction();

    for (int i = 0; i < filenames.size(); ++i) 
    {
      m_manager->openInNewTab(filenames[i]);
    }
  }
}

QStringList ScriptingWindow::extractPyFiles(const QList<QUrl>& urlList) const
{
  QStringList filenames;
  for (int i = 0; i < urlList.size(); ++i) 
  {
    QString fName = urlList[i].toLocalFile();
    if (fName.size()>0)
    {
      QFileInfo fi(fName);
      
      if (fi.suffix().upper()=="PY")
      {
        filenames.append(fName);
      }
    }
  }
  return filenames;
}
