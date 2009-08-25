//-----------------------------------------------------
// Includes
//-----------------------------------------------------
#include "ScriptManagerWidget.h"
#include "ScriptEditor.h"
#include "ScriptingEnv.h"
#include "Script.h"

// Mantid
#include "MantidKernel/ConfigService.h"

// Qt
#include <QApplication>
#include <QPoint>
#include <QAction>
#include <QMenu>
#include <QFileInfo>
#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QPushButton>
#include <QDateTime>
#include <QContextMenuEvent>
#include <Qsci/qscilexer.h>
#include <QTabBar>

//-----------------------------------------------------
// Public member functions
//-----------------------------------------------------
/**
 * Constructor
 */
ScriptManagerWidget::ScriptManagerWidget(ScriptingEnv *env, QWidget *parent) : 
  QTabWidget(parent), scripted(env), m_last_dir(""), m_script_runner(NULL), m_script_executing(false), m_cursor_pos()
{
  // Set the last directory to the Mantid scripts directory
  m_last_dir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("pythonscripts.directory"));
  if( m_last_dir.isEmpty() )
  {
    m_last_dir = qApp->applicationDirPath();
  }

  //Create actions for this widget
  initActions();

  // Create and store the script object that will execute the code
  setNewScriptRunner();

  // Execution state change
  connect(this, SIGNAL(ScriptIsActive(bool)), this, SLOT(setScriptIsRunning(bool)));
  //Tab change
  connect(this, SIGNAL(currentChanged(int)), this, SLOT(newTabSelected()));
  
  // Start with a blank tab
  newTab();
  
  setFocusPolicy(Qt::StrongFocus);
  setFocus();
}

/**
 * Destructor
 */
ScriptManagerWidget::~ScriptManagerWidget()
{
  delete m_script_runner;
}

/**
 * Close all tabs
 */
void ScriptManagerWidget::closeAllTabs()
{
  int index_end = count() - 1;
  setCurrentIndex(index_end);
  for( int index = index_end; index >= 0; --index )
  {
    //This closes the tab at the end.
    closeTabAtIndex(index);
  }
}

/**
 * Ask whether we should save and then perform the correct action
 * @index The tab index
 */
void ScriptManagerWidget::askSave(int index)
{
  ScriptEditor *editor = qobject_cast<ScriptEditor*>(widget(index));
  if( !editor || !editor->isModified() ) return;

  QMessageBox msg_box(this);
  msg_box.setModal(true);
  msg_box.setWindowTitle("MantidPlot");
  msg_box.setText(tr("The current script has been modified."));
  msg_box.setInformativeText(tr("Save changes?"));
  msg_box.addButton(QMessageBox::Save);
  QPushButton *saveAsButton = msg_box.addButton("Save As...", QMessageBox::AcceptRole);
  msg_box.addButton(QMessageBox::Discard);
  int ret = msg_box.exec();
  if( msg_box.clickedButton() == saveAsButton )
  {
    saveAs(index);
  }
  else if( ret == QMessageBox::Save )
  {
    save(index);
  }
  else 
  { 
    editor->setModified(false);
  }
}

/**
 * Opens a script
 * @param filename An filename to use 
 * @returns The contents of the script
 */
QString ScriptManagerWidget::readScript(const QString& filename)
{
  QFile file(filename);
  QString script_txt;
  if( !file.open(QIODevice::ReadOnly) )
  {
    QMessageBox::critical(this, tr("MantidPlot - File error"), 
			  tr("Could not open file \"%1\" for reading.").arg(filename));
    return script_txt;
  }
  
  QTextStream reader(&file);
  reader.setEncoding(QTextStream::UnicodeUTF8);
  while( !reader.atEnd() )
  {
    // Read line trims off line endings automatically and we'll simply use '\n' throughout 
    script_txt.append(reader.readLine() + "\n");
  }
  file.close();  
  return script_txt;
}

/**
 * Run a piece of code in the current environment
 * @param code The chunk of code to execute
 */
bool ScriptManagerWidget::runScriptCode(const QString & code)
{
  m_script_runner->setCode(code);
  emit ScriptIsActive(true);
  formatMessage("Script execution started.", false);
  bool success = m_script_runner->exec();
  emit ScriptIsActive(false);
  if( success )
  {
    formatMessage("Script execution completed successfully.", false);
  }
  return success;
}

//-------------------------------------------
// Public slots
//-------------------------------------------
/**
 * Create a new tab such that it is the specified index within the tab range.
 * @param index The index to give the new tab
 */
ScriptEditor* ScriptManagerWidget::newTab(int index)
{
  ScriptEditor *editor = new ScriptEditor(this);
  editor->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(editor, SIGNAL(customContextMenuRequested(const QPoint&)), 
	  this, SLOT(editorContextMenu(const QPoint&)));
  connect(editor, SIGNAL(textChanged()), this, SLOT(markCurrentAsChanged()));
  QString tab_title = "New script";
  setCurrentIndex(insertTab(index, editor, tab_title));
  //Set the current code lexer, the editor takes ownership
  editor->setLexer(scriptingEnv()->scriptCodeLexer());
  editor->setAutoIndent(true);
  editor->setMarginLineNumbers(1,true);
  editor->setMarginWidth(1, 38);
  // Set the current editor to focus
  setFocusProxy(editor);
  return editor;
}

/**
 * Open a file in the current tab
 * @param filename An optional file name
 */
void ScriptManagerWidget::openInCurrentTab(const QString & filename)
{
  // Redirect work
  open(false, filename);
}

/**
 * Open a file in a new tab
 * @param filename An optional file name
 */
void ScriptManagerWidget::openInNewTab(const QString & filename)
{
  // Redirect work
  open(true, filename);
}

/**
 * Save script under a different file name
 * @param The index of the tab to save
 */
QString ScriptManagerWidget::saveAs(int index)
{
  QString filter = scriptingEnv()->fileFilter();
  filter += tr("Text") + " (*.txt *.TXT);;";
  filter += tr("All Files")+" (*)";
  QString selected_filter;
  QString file_to_save = QFileDialog::getSaveFileName(this, tr("MantidPlot - Save script"), 
						      m_last_dir, filter, &selected_filter);
  if( file_to_save.isEmpty() ) return QString();

  // Set last directory
  m_last_dir = QFileInfo(file_to_save).absolutePath();
  ScriptEditor *editor = qobject_cast<ScriptEditor*>(currentWidget());
  editor->setFileName(file_to_save);
  save(index);
  return file_to_save;
}

/**
 * Save the tab text given by the index
 * @param The index of the tab to save
 */
void ScriptManagerWidget::save(int index)
{
  if( index == -1 ) index = currentIndex();    
  ScriptEditor *editor = qobject_cast<ScriptEditor*>(widget(index));
  if( editor && editor->isModified() )
  {
    QString filename = editor->fileName();
    //Open dialog if necessary
    if( filename.isEmpty() )
    {
      if( saveAs().isEmpty() ) return;
    }
    else
    {
      editor->saveScript(filename);
      setTabText(currentIndex(), QFileInfo(filename).fileName());
      editor->setModified(false);
      connect(editor, SIGNAL(textChanged()), this, SLOT(markCurrentAsChanged()));
    }
  }
}

/**
 * Execute the highlighted code from the current tab
 */
void ScriptManagerWidget::execute()
{
  //Current editor
  ScriptEditor *editor = qobject_cast<ScriptEditor*>(currentWidget());

  QString code = editor->selectedText();
  if( code.isEmpty() )
  {
    executeAll();
    return;
  }
  int lineFrom(0), indexFrom(0), lineTo(0), indexTo(0);
  //Qscintilla function
  editor->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
  scriptingEnv()->setFirstLineNumber(lineFrom);
  
  runScriptCode(code);
}

/**
 * Execute the whole script
 */
void ScriptManagerWidget::executeAll()
{
  //Current editor
  ScriptEditor *editor = qobject_cast<ScriptEditor*>(currentWidget());
  QString script_txt = editor->text();
  if( script_txt.isEmpty() ) return;
  scriptingEnv()->setFirstLineNumber(0);

  //Run the code
  runScriptCode(script_txt);
}

/**
 * Evaluate
 */
void ScriptManagerWidget::evaluate()
{  
  QMessageBox::information(this, "MantidPlot", "Evaluate is not implemented yet.");
}

/** 
 * Format an output message
 * @param msg The message string
 */
void ScriptManagerWidget::formatOutput(const QString & msg)
{
  //Forward to helper
  formatMessage(msg, false);
}

/**
 * Format an error message
 * @param msg The message string
 */
void ScriptManagerWidget::formatError(const QString & msg)
{
  //Forward to helper
  formatMessage(msg, true);
}

//--------------------------------------------
// Private slots
//--------------------------------------------
/**
 * Handle a context menu request for one of the editors
 */
void ScriptManagerWidget::editorContextMenu(const QPoint &)
{
  QMenu context(this);

  //File actions
  context.addAction(m_open_curtab);
  context.addAction(m_save);
  context.insertSeparator();
  // Edit actions
  context.addAction(m_copy);
  context.addAction(m_cut);
  context.addAction(m_paste);

  context.insertSeparator();
  
  //Evaluate and execute
  context.addAction(m_exec);
  context.addAction(m_exec_all);
  if( scriptingEnv()->supportsEvaluation() )
  {
    context.addAction(m_eval);
  }
  
  context.exec(QCursor::pos());
}

/**
 * Close current tab
 */
int ScriptManagerWidget::closeCurrentTab()
{
  int index = currentIndex();
  closeTabAtIndex(index);
  return index;
}

/**
 * Close clicked tab. Qt cannot give the position where an action is clicked so this just gets 
 * the current cursor position and calls the closeAtPosition function
 */
void ScriptManagerWidget::closeClickedTab()
{
  closeTabAtPosition(m_cursor_pos);
}

/**
 * Update actions based on new tab selection
 * @param index The index of the new tab
 */
void ScriptManagerWidget::newTabSelected()
{
  // Rewire all editing actions so that they refer to the new editor
  ScriptEditor *editor = qobject_cast<ScriptEditor*>(currentWidget());
  if( !editor ) return;
  m_undo = editor->undoAction();
  m_redo = editor->redoAction();
  m_cut = editor->cutAction();
  m_copy = editor->copyAction();
  m_paste = editor->pasteAction();  
}

/**
 * Mark the current tab as changed
 */
void ScriptManagerWidget::markCurrentAsChanged()
{
  int index = currentIndex();
  setTabText(index, tabText(index) + "*");
  //Disconnect signal so that this doesn't get run in the future
  //Note that qobject_cast is quite fast
  ScriptEditor *editor = qobject_cast<ScriptEditor*>(currentWidget());
  disconnect(editor, SIGNAL(textChanged()), this, SLOT(markCurrentAsChanged()));
}

/**
 * Enable/disable script interaction based on script execution status
 * @param running The state of the script
 */
void ScriptManagerWidget::setScriptIsRunning(bool running)
{
  m_script_executing = running;
  // Enable/disable execute actions
  m_exec->setEnabled(!running);
  m_exec_all->setEnabled(!running);
  if( scriptingEnv()->supportsEvaluation() ) m_eval->setEnabled(!running);
}

//--------------------------------------------
// Private member functions (non-slot)
//--------------------------------------------
/**
 * Initalize the actions relevant for this object
 */
void ScriptManagerWidget::initActions()
{
  // **File** actions
  // New tab
  m_new_tab = new QAction(tr("&New Tab"), this);
  m_new_tab->setShortcut(tr("Ctrl+N"));
  connect(m_new_tab, SIGNAL(activated()), this, SLOT(newTab()));
  // Open script in current tab
  m_open_curtab = new QAction(tr("&Open"), this);
  m_open_curtab->setShortcut(tr("Ctrl+O"));
  connect(m_open_curtab, SIGNAL(activated()), this, SLOT(openInCurrentTab()));
  //Open in new tab
  m_open_newtab = new QAction(tr("&Open in New Tab"), this);
  m_open_newtab->setShortcut(tr("Ctrl+Shift+O"));
  connect(m_open_newtab, SIGNAL(activated()), this, SLOT(openInNewTab()));
  //Save a script
  m_save = new QAction(tr("&Save"), this);
  m_save->setShortcut(tr("Ctrl+S"));
  connect(m_save, SIGNAL(activated()), this , SLOT(save()));
  //Save a script under a new file name
  m_saveas = new QAction(tr("&Save As"), this);
  connect(m_saveas, SIGNAL(activated()), this , SLOT(saveAs()));
  m_saveas->setShortcut(tr("Ctrl+Shift+S"));
  //Close the current tab
  m_close_tab = new QAction(tr("&Close Tab"), this);
  m_close_tab->setShortcut(tr("Ctrl+W"));
  connect(m_close_tab,SIGNAL(activated()), this, SLOT(closeCurrentTab()));
  
  // **Edit** actions
  //Undo placeholder
   m_undo = new QAction(tr("&Undo"), this);
   m_undo->setEnabled(false);

  // **Execute** actions
  m_exec = new QAction(tr("E&xecute"), this);
  m_exec->setShortcut(tr("Ctrl+Return"));
  connect(m_exec, SIGNAL(activated()), this, SLOT(execute()));
  m_exec_all = new QAction(tr("Execute &All"), this);
  m_exec_all->setShortcut(tr("Ctrl+Shift+Return"));
  connect(m_exec_all, SIGNAL(activated()), this, SLOT(executeAll()));
  m_eval = new QAction(tr("&Evaluate Expression"), this);
  m_eval->setShortcut( tr("Ctrl+E") );
  connect(m_eval, SIGNAL(activated()), this, SLOT(evaluate()));
  m_eval->setEnabled(scriptingEnv()->supportsEvaluation());
}

/**
 * A context menu event for the tab widget itself
 * @param event The context menu event
 */  
void ScriptManagerWidget::contextMenuEvent(QContextMenuEvent *event)
{
  QMenu context(this);

  m_cursor_pos = event->pos(); //in widget coordinates
  
  if( count() > 0 )
  {
    if( tabBar()->tabAt(m_cursor_pos) >= 0 )
    {
      QAction *close = new QAction(tr("&Close Tab"), this);
      connect(close, SIGNAL(activated()), this, SLOT(closeClickedTab()));
      context.addAction(close);
    }
    // Close all tabs
    QAction *closeall = new QAction(tr("&Close All Tabs"), this);
    connect(closeall, SIGNAL(activated()), this, SLOT(closeAllTabs()));
    context.addAction(closeall);

    context.insertSeparator();
  }

  QAction *newtab = new QAction(tr("&New Tab"), this);
  connect(newtab, SIGNAL(activated()), this, SLOT(newTab()));
  context.addAction(newtab);


  context.exec(QCursor::pos());
}

/**
 * A custom event handler, which in this case monitors for ScriptChangeEvent signals
 * @param event The custome event
 */
void ScriptManagerWidget::customEvent(QEvent *event)
{
  if( !isScriptRunning() && event->type() == SCRIPTING_CHANGE_EVENT )
  {    
    ScriptingChangeEvent *sce = static_cast<ScriptingChangeEvent*>(event);
    // This handles reference counting of the scripting environment
    scripted::scriptingChangeEvent(sce);

    setNewScriptRunner();

    // Update the code lexers for each tab
    int ntabs = count();
    for( int index = 0; index < ntabs; ++index )
    {
      ScriptEditor *editor = static_cast<ScriptEditor*>(widget(index));
      QsciLexer *old_lexer = editor->lexer();
      editor->setLexer(scriptingEnv()->scriptCodeLexer());
      delete old_lexer;
    }
  }
}

/**
 * Open a file
 * @param newtab If true, a new tab will be created
 * @param filename An optional file name
 */
void ScriptManagerWidget::open(bool newtab, const QString & filename)
{
  // If we want to keep in current tab check if we need to save
  if( !newtab ) askSave(currentIndex());

  // Qstrings are implicitly shared so this just copies a pointer
  QString file_to_open = filename;

  if( filename.isEmpty() )
  {
    QString filter = scriptingEnv()->fileFilter();
    filter += tr("Text") + " (*.txt *.TXT);;";
    filter += tr("All Files")+" (*)";
    file_to_open = QFileDialog::getOpenFileName(this, tr("MantidPlot - Open a script from a file"), 
						m_last_dir, filter);
    if( file_to_open.isEmpty() ) 
    {
      return;
    }
  }
  QString script_txt = readScript(file_to_open);
  if( script_txt.isEmpty() ) return;

  int index(-1);
  if( !newtab )
  {
    // This asks about saving again but since it's already taken care of
    // then it'll be quick
    index = closeCurrentTab();
  }
  
  ScriptEditor *editor = newTab(index);
  editor->blockSignals(true);
  editor->append(script_txt);
  editor->blockSignals(false);
  editor->setFileName(file_to_open);
  setTabText(currentIndex(), QFileInfo(file_to_open).fileName());

  // Set last directory
  m_last_dir = QFileInfo(file_to_open).absolutePath();
}

/**
 * Format a message and emit a signal that it is ready to be printed
 * @param msg The message
 * @param error If this is an error message
 */
void ScriptManagerWidget::formatMessage(const QString & msg, bool error)
{
  // I want to control end of line formatting my self
  QString  msg_to_print = msg.trimmed();
  if( msg_to_print.isEmpty() ) return;

  QString hashes(20, '#');
  //Signal that the text is ready to print
  emit MessageToPrint(hashes + " " + QDateTime::currentDateTime().toString() + "  " + 
    hashes + "\n" + msg_to_print + "\n", error);
}


/**
 * Create a new Script object, connect up the relevant signals and store the pointer. Note
 * that this will always delete the current scripting running object
 */
void ScriptManagerWidget::setNewScriptRunner()
{
  if( m_script_runner ) delete m_script_runner;
  
  m_script_runner = scriptingEnv()->newScript("", this, "");
  // Connect the signals that print output and error messages to the formatting functions
  connect(m_script_runner, SIGNAL(print(const QString &)), this, SLOT(formatOutput(const QString &)));
  connect(m_script_runner, SIGNAL(error(const QString &, const QString&, int)), this, 
	  SLOT(formatError(const QString &)));
  
}

/**
 * Close a given tab
 * @param index The tab index
 */ 
void ScriptManagerWidget::closeTabAtIndex(int index)
{
  QWidget *editor = widget(index);
  if( !editor ) return;
  //Check if we need to save
  askSave(index);
  //Get the widget attached to the tab first as this is not deleted
  //when remove is called
  editor->deleteLater();
  m_undo = NULL; m_redo = NULL;
  m_cut = NULL; m_copy = NULL;
  m_paste = NULL;
  removeTab(index);
}


/**
 * Close a tab at a given position
 * @param pos The tab at the given position
 */ 
void ScriptManagerWidget::closeTabAtPosition(const QPoint & pos)
{
  int index = tabBar()->tabAt(pos);
  std::cerr << "close index " << index << "\n";
  //Index is checked in closeTab
  closeTabAtIndex(index);
}
