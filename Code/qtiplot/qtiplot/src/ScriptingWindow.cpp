//-------------------------------------------
// Includes
//-------------------------------------------
#include "ScriptingWindow.h"
#include "ScriptManagerWidget.h"
#include "ScriptingEnv.h"
#include "pixmaps.h"

// Mantid
#include "MantidKernel/ConfigService.h"

//Qt
#include <QTextEdit>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QCloseEvent>
#include <QPrintDialog>
#include <QPrinter>

//***************************************************************************
//
// ScriptOutputDock class
//
//***************************************************************************
/**
 * Constructor 
 * @param title The title
 * @param parent The parent widget
 * @param flags Window flags
 */
ScriptOutputDock::ScriptOutputDock(const QString & title, QWidget * parent, 
				   Qt::WindowFlags flags ) : 
  QDockWidget(title, parent, flags)
{
  setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
  
  // The text display
  m_text_display = new QTextEdit(this);
  m_text_display->setReadOnly(true);
  m_text_display->setLineWrapMode(QTextEdit::FixedColumnWidth);
  m_text_display->setLineWrapColumnOrWidth(105);
  m_text_display->setAutoFormatting(QTextEdit::AutoNone);
  // Change to fix width font so that table formatting isn't screwed up
  QFont f("Andale Mono");
  f.setFixedPitch(true);
  f.setPointSize(8);
  m_text_display->setCurrentFont(f);
  m_text_display->setMinimumWidth(5);
  m_text_display->setMinimumHeight(5);

  m_text_display->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_text_display, SIGNAL(customContextMenuRequested(const QPoint&)), this, 
	  SLOT(showContextMenu(const QPoint&)));

  setWidget(m_text_display);
}

/**
 * Is there anything here
 */
bool ScriptOutputDock::isEmpty() const
{
  return m_text_display->document()->isEmpty();
}

/**
 * Clear the text area
 */
void ScriptOutputDock::clear()
{
  m_text_display->clear();
}

/**
 * Change the title based on the script's execution state
 * @param running The current state of the script environment
 */
void ScriptOutputDock::setScriptIsRunning(bool running)
{
  QString title("Script Output - Status: ");
  if( running )
  {
    title += "Running ...";
  }
  else
  {
    title += "Stopped";
  }
  setWindowTitle(title);
}

//-------------------------------------------
// Private slot member functions
//-------------------------------------------
/**
 * Display an output message in the output dock
 * @param msg The msg
 * @param error Indicate that this is an error
 */
void ScriptOutputDock::displayOutputMessage(const QString &msg, bool error)
{
  if( error )
  {
    m_text_display->setTextColor(Qt::red);
  }
  else
  {
    m_text_display->setTextColor(Qt::black);
  }
  m_text_display->textCursor().insertText(msg);
  m_text_display->moveCursor(QTextCursor::End);
}

/**
 * Display a context menu
 */
void ScriptOutputDock::showContextMenu(const QPoint & pos)
{
  QMenu menu(this);
  
  QAction* clear = new QAction("Clear", this);
  connect(clear, SIGNAL(activated()), this, SLOT(clear()));
  menu.addAction(clear);

  QAction* copy = new QAction(QPixmap(copy_xpm), "Copy", this);
  connect(copy, SIGNAL(activated()), m_text_display, SLOT(copy()));
  menu.addAction(copy);

  if( !m_text_display->document()->isEmpty() )
  {
    QAction* print = new QAction(QPixmap(fileprint_xpm), "&Print", this);
    connect(print, SIGNAL(activated()), this, SLOT(print()));
    menu.addAction(print);
  }
  
  menu.exec(m_text_display->mapToGlobal(pos));
}

void ScriptOutputDock::print()
{
  QPrinter printer;
  QPrintDialog *print_dlg = new QPrintDialog(&printer, this);
  print_dlg->setWindowTitle(tr("Print Output"));
  if (print_dlg->exec() != QDialog::Accepted)
    return;
  QTextDocument document(m_text_display->text());
  document.print(&printer);
}

//***************************************************************************
//
// ScriptingWindow class
//
//***************************************************************************
//-------------------------------------------
// Public member functions
//-------------------------------------------
/**
 * Constructor
 * @param env The scripting environment
 * @param parent The parent widget
 * @param flags Window flags passed to the base class
 */
ScriptingWindow::ScriptingWindow(ScriptingEnv *env, QWidget *parent, Qt::WindowFlags flags) : 
  QMainWindow(parent, flags)
{
  setObjectName("MantidScriptWindow");
  // Sub-widgets
  m_manager = new ScriptManagerWidget(env, this);
  setCentralWidget(m_manager);
  m_output_dock = new ScriptOutputDock(QString(), this);
  m_output_dock->setScriptIsRunning(false);
  //Set the height to 10% of the height of the window
  addDockWidget(Qt::BottomDockWidgetArea, m_output_dock);
  int dock_width = m_output_dock->geometry().width();
  m_output_dock->resize(dock_width, this->geometry().height() * 0.01);
  
  connect(m_manager, SIGNAL(MessageToPrint(const QString&,bool)), m_output_dock, 
	  SLOT(displayOutputMessage(const QString&, bool)));
  connect(m_manager, SIGNAL(ScriptIsActive(bool)), m_output_dock, SLOT(setScriptIsRunning(bool)));

  // Create menus and actions
  initMenus();
  fileAboutToShow();
  editAboutToShow();

  // This connection must occur after the objects have been created and initialized
  connect(m_manager, SIGNAL(currentChanged(int)), this, SLOT(tabSelectionChanged()));

  QSettings settings;
  settings.beginGroup("/ScriptWindow");
  QString lastdir = settings.value("LastDirectoryVisted", "").toString();
  // If nothgin, set the last directory to the Mantid scripts directory (if present)
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
  settings.endGroup();


  setWindowIcon(QIcon(":/MantidPlot_Icon_32offset.png"));
  setWindowTitle("MantidPlot: " + env->scriptingLanguage() + " Window");

  setFocusPolicy(Qt::StrongFocus);
  setFocusProxy(m_manager);
}

/**
 * Destructor
 */
ScriptingWindow::~ScriptingWindow()
{
  delete m_manager;
  delete m_output_dock;
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
  QRect window_size(pos(), size());
  settings.setValue("/x", window_size.x());
  settings.setValue("/y", window_size.y());
  settings.setValue("/width", window_size.width());
  settings.setValue("/height", window_size.height());
  settings.setValue("/ProgressArrow", m_manager->m_toggle_progress->isChecked());
  settings.setValue("/LastDirectoryVisited", m_manager->m_last_dir);
  settings.endGroup();

  m_manager->closeAllTabs();
}

/**
 * Open a script directly. This is here for backwards compatability with the old ScriptWindow
 * class
 * @param filename The file name
 * @param newtab Do we want a new tab
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
 * @param event The custom event
 */ 
void ScriptingWindow::customEvent(QEvent *event)
{
  if( !m_manager->isScriptRunning() && event->type() == SCRIPTING_CHANGE_EVENT )
  {
    ScriptingChangeEvent *sce = static_cast<ScriptingChangeEvent*>(event);
    setWindowTitle("MantidPlot: " + sce->scriptingEnv()->scriptingLanguage() + " Window");
  }
}

/**
 * Construct the file menu
 */
void ScriptingWindow::fileAboutToShow()
{
  m_file_menu->clear();

  // New tab
  m_file_menu->addAction(m_manager->m_new_tab);
  // Open a file in current tab
  m_file_menu->addAction(m_manager->m_open_curtab);
  //Open in new tab
  m_file_menu->addAction(m_manager->m_open_newtab);

  // Save a script
  m_file_menu->insertSeparator();
  m_file_menu->addAction(m_manager->m_save);
  // Save a script under a new file name
  m_file_menu->addAction(m_manager->m_saveas);

  //Print
  if( m_manager->count() > 0 )
  {
    m_file_menu->addAction(m_manager->printAction());
  }
  if( !m_output_dock->isEmpty() )
  {
    m_file_menu->addAction(m_print_output);
  }

  // Close current tab
  m_file_menu->insertSeparator();
  m_file_menu->addAction(m_manager->m_close_tab);

}

/**
 * Construct the edit menu
 */
void ScriptingWindow::editAboutToShow()
{
  m_edit_menu->clear();

  if( m_manager->count() > 0 )
  {
    // Undo
    m_edit_menu->addAction(m_manager->undoAction());
    //Redo 
    m_edit_menu->addAction(m_manager->redoAction());
    //Cut
    m_edit_menu->addAction(m_manager->cutAction());
    //Copy
    m_edit_menu->addAction(m_manager->copyAction());
    //Paste
    m_edit_menu->addAction(m_manager->pasteAction());

    //Find and replace
    m_edit_menu->insertSeparator();
    m_edit_menu->addAction(m_manager->m_find);
    m_edit_menu->insertSeparator();
  
  }
  //Clear output
  m_edit_menu->addAction(m_clear_output);  
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
  // Ensure that the shortcuts are active
  fileAboutToShow();
  editAboutToShow();
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
  connect(m_file_menu, SIGNAL(aboutToShow()), this, SLOT(fileAboutToShow()));

  m_print_output = new QAction(tr("Print &Output"), this);
  connect(m_print_output, SIGNAL(activated()), m_output_dock, SLOT(print()));

  //************* Edit menu *************
  m_edit_menu = menuBar()->addMenu(tr("&Edit"));
  connect(m_edit_menu, SIGNAL(aboutToShow()), this, SLOT(editAboutToShow()));
   // Clear output
  m_clear_output = new QAction(tr("&Clear Output"), this);
  connect(m_clear_output, SIGNAL(activated()), m_output_dock, SLOT(clear()));
  
  //************* Run menu *************
  m_run_menu = menuBar()->addMenu(tr("E&xecute"));
  // Execute script
  m_run_menu->addAction(m_manager->m_exec);
  // Execute everything from a script
  m_run_menu->addAction(m_manager->m_exec_all);
  //Evaluate function for those environments that support one
  //  m_run_menu->addAction(m_manager->m_eval);

  //************* Window menu *************
  m_window_menu = menuBar()->addMenu(tr("&Window"));
  //Always on top
  m_always_on_top = new QAction(tr("Always on &Top"), this);
  m_always_on_top->setCheckable(true);
  connect(m_always_on_top, SIGNAL(toggled(bool)), this, SLOT(updateWindowFlags()));
  m_window_menu->addAction(m_always_on_top);
  //Hide
  m_hide = new QAction(tr("&Hide"), this);
  connect(m_hide, SIGNAL(activated()), this, SLOT(hide()));
  m_window_menu->addAction(m_hide);
  //Toggle output dock
  m_toggle_output = m_output_dock->toggleViewAction();
  m_toggle_output->setText("&Show Output");
  m_toggle_output->setChecked(true);
  m_window_menu->addAction(m_toggle_output);
  //Toggle progress
  m_window_menu->addAction(m_manager->m_toggle_progress);
}

