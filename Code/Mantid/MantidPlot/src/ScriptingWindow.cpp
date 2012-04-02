//-------------------------------------------
// Includes
//-------------------------------------------
#include "ScriptingWindow.h"
#include "ScriptManagerWidget.h"
#include "ScriptingEnv.h"
#include "pixmaps.h"

// Mantid
#include "MantidKernel/ConfigService.h"
#include "ApplicationWindow.h"
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
  // Sub-widgets
  m_manager = new ScriptManagerWidget(env, this);
  setCentralWidget(m_manager);
  setFocusProxy(m_manager);

  // Create menus and actions
  initMenus();
  readSettings();

  // This connection must occur after the objects have been created and initialized
  connect(m_manager, SIGNAL(currentChanged(int)), this, SLOT(tabSelectionChanged()));

  setWindowIcon(QIcon(":/MantidPlot_Icon_32offset.png"));
  setWindowTitle("MantidPlot: " + env->languageName() + " Window");

}

/**
 * Destructor
 */
ScriptingWindow::~ScriptingWindow()
{
  delete m_manager;
}

/**
 * Is a script executing?
 * @returns A flag indicating the current state
 */
bool ScriptingWindow::isScriptRunning() const
{
  return m_manager->isScriptRunning();
}

/**
 * Save the settings on the window
 */
void ScriptingWindow::saveSettings()
{
  QSettings settings;
  settings.beginGroup("/ScriptWindow");
  settings.setValue("/AlwaysOnTop", m_alwaysOnTop->isChecked());
  settings.setValue("/CodeFolding", m_toggleFolding->isChecked());
  settings.setValue("/CallTips", m_toggleCallTips->isChecked());
  settings.setValue("/ProgressArrow", m_toggleProgress->isChecked());
  settings.setValue("/LastDirectoryVisited", m_manager->m_last_dir);
  settings.setValue("/RecentScripts",m_manager->recentScripts());
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
  m_toggleFolding->setChecked(settings.value("CodeFolding", true).toBool());
  m_toggleCallTips->setChecked(settings.value("CallTips", true).toBool());
  m_toggleProgress->setChecked(settings.value("ProgressArrow", true).toBool());

  m_manager->setRecentScripts(settings.value("/RecentScripts").toStringList());
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

void ScriptingWindow::executeAll()
{
  m_manager->executeAll();
}

//-------------------------------------------
// Private non-slot member functions
//-------------------------------------------
/**
 * Accept a custom event and in this case test if it is a ScriptingChangeEvent
 * @param event :: The custom event
 */ 
void ScriptingWindow::customEvent(QEvent *event)
{
  if( !m_manager->isScriptRunning() && event->type() == SCRIPTING_CHANGE_EVENT )
  {
    ScriptingChangeEvent *sce = static_cast<ScriptingChangeEvent*>(event);
    setWindowTitle("MantidPlot: " + sce->scriptingEnv()->languageName() + " Window");
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

void ScriptingWindow::tabSelectionChanged()
{

}

//-------------------------------------------
// Private non-slot member functions
//-------------------------------------------
/**
 * calls ScriptManagerWidget saveToString and  
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

/**
 * Initialise the menus
 */
void ScriptingWindow::initMenus()
{
  initActions();

  m_fileMenu = menuBar()->addMenu(tr("&File"));
#ifdef SCRIPTING_DIALOG
  m_scripting_lang = new QAction(tr("Scripting &language"), this);
  connect(m_scripting_lang, SIGNAL(activated()), this, SIGNAL(chooseScriptingLanguage()));
#endif
  connect(m_fileMenu, SIGNAL(aboutToShow()), this, SLOT(populateFileMenu()));

  m_editMenu = menuBar()->addMenu(tr("&Edit"));
  connect(m_editMenu, SIGNAL(aboutToShow()), this, SLOT(populateFileMenu()));

  m_runMenu = menuBar()->addMenu(tr("E&xecute"));
  connect(m_runMenu, SIGNAL(aboutToShow()), this, SLOT(populateExecMenu()));

  m_windowMenu = menuBar()->addMenu(tr("&Window"));
  connect(m_windowMenu, SIGNAL(aboutToShow()), this, SLOT(populateExecMenu()));
}

/// Populate file menu
void ScriptingWindow::populateFileMenu()
{
  m_fileMenu->clear();
  const bool scriptsOpen(m_manager->count() > 0);
  m_fileMenu->addAction(m_newTab);
  m_fileMenu->addAction(m_openInNewTab);

  if( scriptsOpen )
  {
    m_fileMenu->addAction(m_openInCurTab);

    m_fileMenu->insertSeparator();
    m_fileMenu->addAction(m_save);
    m_fileMenu->addAction(m_saveAs);
    m_fileMenu->addAction(m_print);
  }

  m_fileMenu->insertSeparator();
  m_fileMenu->addMenu(m_recentScripts);

  if( scriptsOpen )
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
  m_editMenu->addAction(m_find);

}
/// Populate execute menu
void ScriptingWindow::populateExecMenu()
{
  m_runMenu->clear();
  m_runMenu->addAction(m_execSelect);
  m_runMenu->addAction(m_execAll);

}

/// Populate window menu
void ScriptingWindow::populateWindowMenu()
{
  m_windowMenu->clear();
  m_windowMenu->addAction(m_alwaysOnTop);
  m_windowMenu->addAction(m_hide);

  if(m_manager->count() > 0)
  {
    m_windowMenu->insertSeparator();
    m_windowMenu->addAction(m_zoomIn);
    m_windowMenu->addAction(m_zoomOut);

    m_windowMenu->insertSeparator();

  }
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
  connect(m_newTab, SIGNAL(activated()), m_manager, SLOT(newTab()));
  m_newTab->setShortcut(tr("Ctrl+N"));

  m_openInCurTab = new QAction(tr("&Open"), this);
  connect(m_openInCurTab, SIGNAL(activated()), m_manager, SLOT(openInCurrentTab()));
  m_openInCurTab->setShortcut(tr("Ctrl+O"));

  m_openInNewTab = new QAction(tr("&Open in New Tab"), this);
  connect(m_openInNewTab, SIGNAL(activated()), m_manager, SLOT(openInNewTab()));
  m_openInNewTab->setShortcut(tr("Ctrl+Shift+O"));

  m_save = new QAction(tr("&Save"), this);
  connect(m_save, SIGNAL(activated()), m_manager, SLOT(saveToCurrentFile()));
  m_save->setShortcut(QKeySequence::Save);

  m_saveAs = new QAction(tr("&Save As"), this);
  connect(m_saveAs, SIGNAL(activated()), m_manager, SLOT(saveAs()));
  m_saveAs->setShortcut(tr("Ctrl+Shift+S"));  

  m_print = new QAction(tr("&Print script"), this);
  connect(m_print, SIGNAL(activated()), m_manager, SLOT(print()));
  m_print->setShortcut(QKeySequence::Print);

  m_closeTab = new QAction(tr("&Close Tab"), this);
  connect(m_closeTab, SIGNAL(activated()), m_manager, SLOT(closeCurrentTab()));
  m_closeTab->setShortcut(tr("Ctrl+W"));

  m_recentScripts = new QMenu(tr("&Recent Scripts"),this);
  connect(m_recentScripts, SIGNAL(aboutToShow()), this, SLOT(populateRecentScriptsMenu()));
  connect(m_recentScripts, SIGNAL(activated(int)), m_manager, SLOT(openRecentScript(int)));
}

/**
 * Create the edit menu action*/
void ScriptingWindow::initEditMenuActions()
{
  m_undo = new QAction(tr("&Undo"), this);
  connect(m_undo, SIGNAL(activated()), m_manager, SLOT(undo()));
  m_undo->setShortcut(QKeySequence::Undo);

  m_redo = new QAction(tr("&Redo"), this);
  connect(m_redo, SIGNAL(activated()), m_manager, SLOT(redo()));
  m_redo->setShortcut(QKeySequence::Redo);

  m_cut = new QAction(tr("C&ut"), this);
  connect(m_cut, SIGNAL(activated()), m_manager, SLOT(cut()));
  m_cut->setShortcut(QKeySequence::Cut);

  m_copy = new QAction(tr("&Copy"), this);
  connect(m_copy, SIGNAL(activated()), m_manager, SLOT(copy()));
  m_copy->setShortcut(QKeySequence::Copy);

  m_paste = new QAction(tr("&Paste"), this);
  connect(m_paste, SIGNAL(activated()), m_manager, SLOT(paste()));
  m_paste->setShortcut(QKeySequence::Paste);

  m_find = new QAction(tr("&Find/Replace"), this);
  connect(m_find, SIGNAL(activated()), m_manager, SLOT(findInScript()));
  m_find->setShortcut(QKeySequence::Find);
}


/**
 * Create the execute menu actions
 */
void ScriptingWindow::initExecMenuActions()
{
  m_execSelect = new QAction(tr("E&xecute Selection"), this);
  connect(m_execSelect, SIGNAL(activated()), m_manager, SLOT(executeSelection()));
  QList<QKeySequence> shortcuts;
  shortcuts << Qt::CTRL + Qt::Key_Return << Qt::CTRL + Qt::Key_Enter;
  m_execSelect->setShortcuts(shortcuts);

  m_execAll = new QAction(tr("Execute &All"), this);
  connect(m_execAll, SIGNAL(activated()), m_manager, SLOT(executeAll()));
  shortcuts.clear();
  shortcuts << Qt::CTRL + Qt::SHIFT + Qt::Key_Return << Qt::CTRL + Qt::SHIFT + Qt::Key_Enter;
  m_execAll->setShortcuts(shortcuts);
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
  connect(m_hide, SIGNAL(activated()), this, SIGNAL(hideMe()));

  m_zoomIn = new QAction(("Increase font size"), this);
  // Setting two shortcuts makes it work for both the plus on the keypad and one above an =
  // Despite the Qt docs advertising the use of QKeySequence::ZoomIn as the solution to this,
  // it doesn't seem to work for me
  m_zoomIn->setShortcut(Qt::SHIFT+Qt::CTRL+Qt::Key_Equal);
  m_zoomIn->setShortcut(Qt::CTRL+Qt::Key_Plus);
  connect(m_zoomIn, SIGNAL(activated()), m_manager, SLOT(zoomIn()));

  m_zoomOut = new QAction(("Decrease font size"), this);
  m_zoomOut->setShortcut(QKeySequence::ZoomOut);
  connect(m_zoomOut, SIGNAL(activated()), m_manager, SLOT(zoomOut()));

  // Toggle the progress arrow
  m_toggleProgress = new QAction(tr("Show &Progress Marker"), this);
  m_toggleProgress->setCheckable(true);
  connect(m_toggleProgress, SIGNAL(toggled(bool)), this, SLOT(toggleProgressArrow(bool)));

  // Toggle code folding
  m_toggleFolding = new QAction(tr("Code &Folding"), this);
  m_toggleFolding->setCheckable(true);
  connect(m_toggleFolding, SIGNAL(toggled(bool)), this, SLOT(toggleCodeFolding(bool)));

  // Toggle call tips
  m_toggleCallTips = new QAction(tr("Call &Tips"), this);
  m_toggleCallTips->setCheckable(true);
  connect(m_toggleCallTips, SIGNAL(toggled(bool)), this, SLOT(toggleCallTips(bool)));
}
