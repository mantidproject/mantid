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

  QSettings settings;
  settings.beginGroup("/ScriptWindow");
  QString lastdir = settings.value("LastDirectoryVisited", "").toString();
  // If nothing, set the last directory to the Mantid scripts directory (if present)
  if( lastdir.isEmpty() )
  {  
    lastdir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("pythonscripts.directory"));
  }
  m_manager->m_last_dir = lastdir;
  if( env->supportsProgressReporting() )
  {
    m_manager->m_toggle_progress->setChecked(settings.value("ProgressArrow", true).toBool());
  }
  else
  {
    m_manager->m_toggle_progress->setChecked(false);
  }
  //get the recent scripts values from registry
  m_manager->setRecentScripts(settings.value("/RecentScripts").toStringList());
  settings.endGroup();

  // Create menus and actions
  initMenus();

  // This connection must occur after the objects have been created and initialized
  connect(m_manager, SIGNAL(currentChanged(int)), this, SLOT(tabSelectionChanged()));

  setWindowIcon(QIcon(":/MantidPlot_Icon_32offset.png"));
  setWindowTitle("MantidPlot: " + env->languageName() + " Window");

  setFocusProxy(m_manager);
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
  settings.setValue("/AlwaysOnTop", m_always_on_top->isChecked());
  settings.setValue("/ProgressArrow", m_manager->m_toggle_progress->isChecked());
  settings.setValue("/LastDirectoryVisited", m_manager->m_last_dir);
  settings.setValue("/RecentScripts",m_manager->recentScripts());
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
 * Construct the file menu
 */
void ScriptingWindow::fileMenuAboutToShow()
{
  m_file_menu->clear();
  m_manager->populateFileMenu(*m_file_menu);
}

/**
 * Construct the edit menu
 */
void ScriptingWindow::editMenuAboutToShow()
{
  m_edit_menu->clear();
  m_manager->populateEditMenu(*m_edit_menu);
}

/**
 * Construct the exec menu for the current context
 */
void ScriptingWindow::execMenuAboutToShow()
{
  m_run_menu->clear();
  m_manager->populateExecMenu(*m_run_menu);
}

void ScriptingWindow::windowMenuAboutToShow()
{
  m_window_menu->clear();

//  m_window_menu->addAction(m_always_on_top);
//  m_window_menu->addAction(m_hide);
//
//  if( m_manager->count() > 0 )
//  {
//    m_window_menu->insertSeparator();
//    m_window_menu->addAction(m_manager->zoomInAction());
//    m_window_menu->addAction(m_manager->zoomOutAction());
//    m_window_menu->insertSeparator();
//
//    //Toggle progress
//    m_window_menu->addAction(m_manager->m_toggle_progress);
//
//    m_window_menu->insertSeparator();
//    //Toggle folding
//    m_window_menu->addAction(m_manager->m_toggle_folding);
//    //Toggle code completion
//    m_window_menu->addAction(m_manager->m_toggle_completion);
//    //Toggle call tips
//    m_window_menu->addAction(m_manager->m_toggle_calltips);
//  }
}

/**
 *
 */
void ScriptingWindow::updateWindowFlags()
{
  Qt::WindowFlags flags = Qt::Window;
  if( m_always_on_top->isChecked() )
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
 * Initialize the menus and actions
 */
void ScriptingWindow::initMenus()
{
  //************* File menu *************
  m_file_menu = menuBar()->addMenu(tr("&File"));
#ifdef SCRIPTING_DIALOG
  m_scripting_lang = new QAction(tr("Scripting &language"), this);
  connect(m_scripting_lang, SIGNAL(activated()), this, SIGNAL(chooseScriptingLanguage()));
#endif
  connect(m_file_menu, SIGNAL(aboutToShow()), this, SLOT(fileMenuAboutToShow()));

  //************* Edit menu *************
  m_edit_menu = menuBar()->addMenu(tr("&Edit"));
  connect(m_edit_menu, SIGNAL(aboutToShow()), this, SLOT(editMenuAboutToShow()));
  
  //************* Run menu *************
  m_run_menu = menuBar()->addMenu(tr("E&xecute"));
  connect(m_run_menu, SIGNAL(aboutToShow()), this, SLOT(execMenuAboutToShow()));

  //************* Window menu *************
  m_window_menu = menuBar()->addMenu(tr("&Window"));
  initWindowActions();
}

/**
 * Create window actions
 */
void ScriptingWindow::initWindowActions()
{
  m_always_on_top = new QAction(tr("Always on &Top"), this);
  m_always_on_top->setCheckable(true);
  connect(m_always_on_top, SIGNAL(toggled(bool)), this, SLOT(updateWindowFlags()));

  m_hide = new QAction(tr("&Hide"), this);
#ifdef __APPLE__
  m_hide->setShortcut(tr("Ctrl+3")); // F3 is used by the window manager on Mac
#else
  m_hide->setShortcut(tr("F3"));
#endif
  // Note that we channel the hide through the parent so that we can save the geometry state
  connect(m_hide, SIGNAL(activated()), this, SIGNAL(hideMe()));

}

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
