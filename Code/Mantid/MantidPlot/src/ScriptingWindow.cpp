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

//***************************************************************************
//
// ScriptOutputDock class
//
//***************************************************************************
/**
 * Constructor 
 * @param title :: The title
 * @param parent :: The parent widget
 * @param flags :: Window flags
 */
ScriptOutputDock::ScriptOutputDock(const QString & title, ScriptManagerWidget* manager, 
				   QWidget * parent, Qt::WindowFlags flags ) : 
  QDockWidget(title, parent, flags), m_manager(manager)
{
  setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
  
  // The text display
  m_text_display = new QTextEdit(this);
  m_text_display->setReadOnly(true);
  m_text_display->setLineWrapMode(QTextEdit::FixedColumnWidth);
  m_text_display->setLineWrapColumnOrWidth(105);
  m_text_display->setAutoFormatting(QTextEdit::AutoNone);
  // Change to fix width font so that table formatting isn't screwed up
  resetFont();
  
  m_text_display->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_text_display, SIGNAL(customContextMenuRequested(const QPoint&)), this, 
	  SLOT(showContextMenu(const QPoint&)));

  initActions();

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
 * @param running :: The current state of the script environment
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
 * @param msg :: The msg
 * @param error :: Indicate that this is an error
 * @param timestamp :: Indicates if the message has a timestamp attached to it. In this case the 
 * message will appear on a new line regardless of the last cursor position
 */
void ScriptOutputDock::displayOutputMessage(const QString &msg, bool error, bool timestamp)
{
  // Ensure the cursor is in the correct position. This affects the font unfortunately
  m_text_display->moveCursor(QTextCursor::End);
  resetFont();

  if( error )
  {
    m_text_display->setTextColor(Qt::red);
  }
  else
  {
    m_text_display->setTextColor(Qt::black);
  }
  QString msg_to_print = msg;

  if( error || timestamp )
  {
    if( timestamp )
    {
      QString separator(75, '-'); 
      msg_to_print  = separator + "\n" + QDateTime::currentDateTime().toString() 
	+ ": " + msg.trimmed() + "\n" + separator + '\n';
    }

    // Check for last character being a new line character unless we are at the start of the 
    // scroll area
    if( !m_text_display->text().endsWith('\n') && m_text_display->textCursor().position() != 0 )
    {
      m_text_display->textCursor().insertText("\n");    
    }
  }

  m_text_display->textCursor().insertText(msg_to_print);    
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

  //Copy action
  menu.addAction(m_copy);

  //Save to file
  QAction* save_to_file = new QAction("Save to file", this);
  connect(save_to_file, SIGNAL(activated()), this, SLOT(saveToFile()));
  menu.addAction(save_to_file);

  if( !m_text_display->document()->isEmpty() )
  {
    QAction* print = new QAction(getQPixmap("fileprint_xpm"), "&Print", this);
    connect(print, SIGNAL(activated()), this, SLOT(print()));
    menu.addAction(print);
  }
  
  menu.exec(m_text_display->mapToGlobal(pos));
}

/**
 * Print the window output
 */
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

/**
 * Save script output to a file
 */
void ScriptOutputDock::saveToFile()
{
  QString filter = tr("Text") + " (*.txt *.TXT);;";
  filter += tr("All Files")+" (*)";
  QString selected_filter;
  QString filename = QFileDialog::getSaveFileName(this, tr("MantidPlot - Save script"), 
						  m_manager->m_last_dir, filter, &selected_filter);
  if( filename.isEmpty() ) return;

  if( QFileInfo(filename).suffix().isEmpty() )
  {
    QString ext = selected_filter.section('(',1).section(' ', 0, 0);
    ext.remove(0,1);
    if( ext != ")" ) filename += ext;
  }

  QFile file(filename);
  if( !file.open(QIODevice::WriteOnly) )
  {
    QMessageBox::critical(this, tr("MantidPlot - File error"), 
			  tr("Could not open file \"%1\" for writing.").arg(filename));
    return;
  }

  QTextStream writer(&file);
  QApplication::setOverrideCursor(Qt::WaitCursor);
  writer << m_text_display->toPlainText();
  QApplication::restoreOverrideCursor();
  file.close();
}

//-------------------------------------------
// Private non-slot member functions
//-------------------------------------------
/**
 * Create the actions associated with this widget
 */
void ScriptOutputDock::initActions()
{
  // Copy action
  m_copy = new QAction(getQPixmap("copy_xpm"), "Copy", this);
  m_copy->setShortcut(tr("Ctrl+C"));
  connect(m_copy, SIGNAL(activated()), m_text_display, SLOT(copy()));
}

/**
 * Rest the font to default
 */
void ScriptOutputDock::resetFont()
{
  QFont f("Andale Mono");
  f.setFixedPitch(true);
  f.setPointSize(8);
  m_text_display->setCurrentFont(f);
  m_text_display->setMinimumWidth(5);
  m_text_display->setMinimumHeight(5);
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
 * @param env :: The scripting environment
 * @param parent :: The parent widget
 * @param flags :: Window flags passed to the base class
 */
ScriptingWindow::ScriptingWindow(ScriptingEnv *env,QWidget *parent, Qt::WindowFlags flags) : 
  QMainWindow(parent, flags), m_acceptClose(false)
{
  setObjectName("MantidScriptWindow");
  // Sub-widgets
  m_manager = new ScriptManagerWidget(env, this);
  setCentralWidget(m_manager);
  m_output_dock = new ScriptOutputDock(QString(), m_manager, this);
  m_output_dock->setScriptIsRunning(false);
  //Set the height to 10% of the height of the window
  addDockWidget(Qt::BottomDockWidgetArea, m_output_dock);
  int dock_width = m_output_dock->geometry().width();
  m_output_dock->resize(dock_width, this->geometry().height() * 0.01);
  
  connect(m_manager, SIGNAL(MessageToPrint(const QString&,bool,bool)), m_output_dock, 
	  SLOT(displayOutputMessage(const QString&, bool,bool)));
  connect(m_manager, SIGNAL(ScriptIsActive(bool)), m_output_dock, SLOT(setScriptIsRunning(bool)));


  QSettings settings;
  settings.beginGroup("/ScriptWindow");
  QString lastdir = settings.value("LastDirectoryVisited", "").toString();
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
  //get the recent scripts values from registry
  m_manager->setRecentScripts(settings.value("/RecentScripts").toStringList());
  settings.endGroup();

  // Create menus and actions
  initMenus();
  fileAboutToShow();
  editAboutToShow();

  // This connection must occur after the objects have been created and initialized
  connect(m_manager, SIGNAL(currentChanged(int)), this, SLOT(tabSelectionChanged()));

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
    this->hide();
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

  // Scripting language
  //m_file_menu->insertSeparator();
  //m_file_menu->addAction(m_scripting_lang);

  m_file_menu->insertSeparator();
  m_file_menu->addMenu(m_manager->m_recent_scripts);
  m_manager->updateRecentScriptList();

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

#ifdef SCRIPTING_DIALOG
  m_scripting_lang = new QAction(tr("Scripting &language"), this);
  connect(m_scripting_lang, SIGNAL(activated()), this, SIGNAL(chooseScriptingLanguage()));
#endif

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
  m_hide->setShortcut(tr("F3"));
  // Note that we channel the hide through the parent so that we can save the geometry state
  connect(m_hide, SIGNAL(activated()), this, SIGNAL(hideMe()));
  m_window_menu->addAction(m_hide);

  m_window_menu->insertSeparator();
  //Toggle output dock
  m_toggle_output = m_output_dock->toggleViewAction();
  m_toggle_output->setText("&Show Output");
  m_toggle_output->setChecked(true);
  m_window_menu->addAction(m_toggle_output);
  //Toggle progress
  m_window_menu->addAction(m_manager->m_toggle_progress);

  m_window_menu->insertSeparator();
  //Toggle folding
  m_window_menu->addAction(m_manager->m_toggle_folding);
  //Toggle code completion
  m_window_menu->addAction(m_manager->m_toggle_completion);
  //Toggle call tips
  m_window_menu->addAction(m_manager->m_toggle_calltips);
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
