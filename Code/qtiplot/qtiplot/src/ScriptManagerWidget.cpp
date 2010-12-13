//-----------------------------------------------------
// Includes
//-----------------------------------------------------
#include "ScriptManagerWidget.h"
#include "ScriptEditor.h"
#include "ScriptingEnv.h"
#include "Script.h"
#include "ScriptingLangDialog.h"
#include "ApplicationWindow.h"
// Qt
#include <QPoint>
#include <QAction>
#include <QMenu>
#include <QFileInfo>
#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QPushButton>
#include <QContextMenuEvent>
#include <Qsci/qscilexer.h>
#include <QTabBar>
#include <QCheckBox>
#include <QComboBox>
#include <QRegExp>
#include <QLabel>
#include <QLayout>
#include <QGroupBox>
#include <QButtonGroup>
#include <QTextCursor>
//***************************************************************************
//
// ScriptManagerWidget class
//
//***************************************************************************
//-----------------------------------------------------
// Public member functions
//-----------------------------------------------------
/**
 * Constructor
 */
ScriptManagerWidget::ScriptManagerWidget(ScriptingEnv *env, QWidget *parent, bool interpreter_mode)
  : QTabWidget(parent), Scripted(env), m_last_dir(""), m_script_runners(),
    m_cursor_pos(), m_findrep_dlg(NULL), 
    m_interpreter_mode(interpreter_mode)
{
  //Create actions for this widget
  initActions();

  // Execution state change
  connect(this, SIGNAL(ScriptIsActive(bool)), this, SLOT(setScriptIsRunning(bool)));
  
  // Start with a blank tab
  newTab();
  QString group;
  if( interpreter_mode )
  {
    tabBar()->hide();
    setContextMenuPolicy(Qt::NoContextMenu);
    ScriptEditor *editor = currentEditor();
   
    connect(editor, SIGNAL(executeLine(const QString&)), this, SLOT(executeInterpreter(const QString &)));
    connect(this, SIGNAL(MessageToPrint(const QString&, bool,bool)), editor, 
	    SLOT(displayOutput(const QString&,bool)));
     connect(editor, SIGNAL(compile(const QString&)), this, SLOT(compile(const QString &)));
      connect(editor, SIGNAL(executeMultiLine()), this, SLOT(executeMultiLine()));
    group = "ScriptInterpreter";
 
  }
  else
  {
    group = "ScriptWindow";
  }
  
  // Settings
  QSettings settings;
  settings.beginGroup(group);
  m_toggle_folding->setChecked(settings.value("CodeFolding", true).toBool());
  m_toggle_completion->setChecked(settings.value("CodeCompletion", true).toBool());
  m_toggle_calltips->setChecked(settings.value("CallTips", true).toBool());
  settings.endGroup();

  setFocusPolicy(Qt::StrongFocus);
  setFocus();
}

/**
 * Destructor
 */
ScriptManagerWidget::~ScriptManagerWidget()
{
  if( m_findrep_dlg )
  {
    delete m_findrep_dlg;
  }
  
  QList<int> script_keys = m_script_runners.uniqueKeys();
  QListIterator<int> iter(script_keys);
  while( iter.hasNext() )
  {
    Script *code = m_script_runners.take(iter.next());
    delete code;
  }

}

/**
 * Save the settings applicable here
 */
void ScriptManagerWidget::saveSettings()
{
  QString group;
  if( m_interpreter_mode )
  {
    group = "ScriptInterpreter";
  }
  else
  {
    group = "ScriptWindow";
  }
    
  QSettings settings;
  settings.beginGroup(group);
  settings.setValue("/CodeFolding", m_toggle_folding->isChecked());
  settings.setValue("/CodeCompletion", m_toggle_completion->isChecked());
  settings.setValue("/CallTips", m_toggle_calltips->isChecked());
  settings.endGroup();
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
 * @param ok Indicate if the file read went ok
 * @returns The contents of the script
 */
QString ScriptManagerWidget::readScript(const QString& filename, bool *ok)
{
  QFile file(filename);
  QString script_txt;
  if( !file.open(QIODevice::ReadOnly|QIODevice::Text) )
  {
    QMessageBox::critical(this, tr("MantidPlot - File error"), 
			  tr("Could not open file \"%1\" for reading.").arg(filename));
    *ok = false;
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
  *ok = true;
  return script_txt;
}

/**
 * Is a script running in the environment
 */
bool ScriptManagerWidget::isScriptRunning()
{
  return scriptingEnv()->isRunning();
}

/**
 * Return the current editor
 */
ScriptEditor* ScriptManagerWidget::currentEditor() const
{
  if( count() == 0 ) return NULL;
  return qobject_cast<ScriptEditor*>(currentWidget());
}

/**
 * Undo action for the current editor
 */
QAction* ScriptManagerWidget::undoAction() const
{
  if( ScriptEditor *editor = currentEditor() )
  {
    return editor->undoAction();
  }
  else return NULL;
}
/**
 * Redo action for the current editor
 */
QAction* ScriptManagerWidget::redoAction() const
{
  if( ScriptEditor *editor = currentEditor() )
  {
    return editor->redoAction();
  }
  else return NULL;
}

/**
 * Cut action for the current editor
 */
QAction* ScriptManagerWidget::cutAction() const
{
  if( ScriptEditor *editor = currentEditor() )
  {
    return editor->cutAction();
  }
  else return NULL;
}

/** 
 * Copy action for the current editor
 */
QAction* ScriptManagerWidget::copyAction() const
{
  if( ScriptEditor *editor = currentEditor() )
  {
    return editor->copyAction();
  }
  else return NULL;
}

/**
 * Paste action for the current editor
 */
QAction* ScriptManagerWidget::pasteAction() const
{
  if( ScriptEditor *editor = currentEditor() )
  {
    return editor->pasteAction();
  }
  else return NULL;
}

/**
 * Print action for the current editor
 */
QAction* ScriptManagerWidget::printAction() const
{
  if( ScriptEditor *editor = currentEditor() )
  {
    return editor->printAction();
  }
  else return NULL;
}

/**
 * Print action for the current editor
 */
QAction* ScriptManagerWidget::zoomInAction() const
{
  if( ScriptEditor *editor = currentEditor() )
  {
    return editor->zoomInAction();
  }
  else return NULL;
}

/**
 * Print action for the current editor
 */
QAction* ScriptManagerWidget::zoomOutAction() const
{
  if( ScriptEditor *editor = currentEditor() )
  {
    return editor->zoomOutAction();
  }
  else return NULL;
}

//-------------------------------------------
// Public slots
//-------------------------------------------
/**
 * Create a new tab such that it is the specified index within the tab range.
 * @param index The index to give the new tab. If this is invalid the tab is simply appended
 */
ScriptEditor* ScriptManagerWidget::newTab(int index)
{
  ScriptEditor *editor = new ScriptEditor(this, m_interpreter_mode, scriptingEnv()->createCodeLexer());
  if( !m_interpreter_mode )
  {
    connect(editor, SIGNAL(textChanged()), this, SLOT(markCurrentAsChanged()));
  }
  editor->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(editor, SIGNAL(customContextMenuRequested(const QPoint&)), 
	  this, SLOT(editorContextMenu(const QPoint&)));
  QString tab_title = "New script";
  index = insertTab(index, editor, tab_title);
  setCurrentIndex(index);
  // Store a script runner
  m_script_runners.insert(index, createScriptRunner(editor));

  // Completion etc
  setCodeCompletionBehaviour(editor, m_toggle_completion->isChecked());
  setCallTipsBehaviour(editor, m_toggle_calltips->isChecked());
  setCodeFoldingBehaviour(editor, m_toggle_folding->isChecked());
  // Set the current editor to focus
  setFocusProxy(editor);
  editor->setFocus();
  editor->setCursorPosition(0,0);
  m_last_active_tab = index;
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
  if( index == -1 ) index = currentIndex();
  ScriptEditor *editor = qobject_cast<ScriptEditor*>(widget(index));
  editor->setFileName(file_to_save);
  doSave(editor);
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
      saveAs(index);
    }
    else
    {
      doSave(editor);
    }
  }
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
 *  This method is useful for saving the currently opened script files to project file 
*/
QString ScriptManagerWidget::saveToString()
{
	QString fileNames;
	fileNames="<scriptwindow>\n";
	fileNames+="ScriptNames\t";
	 int ntabs = count();
	 //get the number of tabs and append  the script file name for each tab
	 //to a string
	 for( int index = 0; index < ntabs; ++index )
    {
      ScriptEditor *editor = static_cast<ScriptEditor*>(widget(index));
      QString s = editor->fileName();
	  if(!s.isEmpty())
	  {fileNames+=s;
	   fileNames+="\t";
	  }
	 }
	 fileNames+="\n</scriptwindow>\n";
	return fileNames;
}
/**
 * Execute the highlighted code from the current tab
 */
void ScriptManagerWidget::execute()
{
  if( isScriptRunning() ) return;
  //Current editor
  ScriptEditor *editor = currentEditor();
  if( !editor ) return;

  QString code = editor->selectedText();
  if( code.isEmpty() )
  {
    executeAll();
    return;
  }
  int lineFrom(0), indexFrom(0), lineTo(0), indexTo(0);
  //Qscintilla function
  editor->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
  runScriptCode(code, lineFrom);
}

/**
 * Execute the whole script
 */
void ScriptManagerWidget::executeAll()
{
  if( isScriptRunning() ) return;
  //Current editor
  ScriptEditor *editor = currentEditor();
  if( !editor ) return;

  QString script_txt = editor->text();
  if( script_txt.isEmpty() ) return;
  
  //Run the code
  runScriptCode(script_txt, 0);
}

/**
 * Evaluate
 */
void ScriptManagerWidget::evaluate()
{  
  QMessageBox::information(this, "MantidPlot", "Evaluate is not implemented yet.");
}

/** 
 * Execute an interpreter line
 * @param code The chunk of code to execute
 */
void ScriptManagerWidget::executeInterpreter(const QString & code)
{
  if( isScriptRunning() ) return;
  ScriptEditor *editor = currentEditor();
  if( !editor ) return;
  
  int lineno,index;
  editor->getCursorPosition(&lineno, &index);
  runScriptCode(code,lineno);
  
  editor->newInputLine();
  setFocus();  
}

/** 
 * Execute multi line code
 */
void ScriptManagerWidget::executeMultiLine()
{
  ScriptEditor *editor = currentEditor();
  if(!editor)
  {
    return;
  }
  int lineno,index;
  editor->getCursorPosition(&lineno, &index);
  runMultiLineCode(lineno);
  editor->append("\n");
  int marker_handle= editor->markerDefine(QsciScintilla::ThreeRightArrows);
  editor->setMarkerHandle(marker_handle);
  editor->newInputLine();
  setFocus(); 
}

/**
 * Run a piece of code in the current environment
 * @param code The chunk of code to execute
 * @param line_offset If this is a chunk of code from an editor, give offset from the start
 */
bool ScriptManagerWidget::runScriptCode(const QString & code, const int line_offset)
{
  // Get the correct script runner
  Script * runner = m_script_runners.value(this->currentIndex());
  runner->setLineOffset(line_offset);
  ScriptEditor *editor = currentEditor();
  if( editor ) 
  {
    connect(runner, SIGNAL(currentLineChanged(int, bool)), editor, SLOT(updateMarker(int, bool)));
  }
  runner->setCode(code);
  QString filename = "<input>";
  if( editor && !editor->fileName().isEmpty() )
  {
    filename = editor->fileName();
  }
  runner->setName(filename);
  emit ScriptIsActive(true);

  if( !m_interpreter_mode ) 
  {
    displayOutput("Script execution started.", true);
  }

  bool success = runner->exec();
  emit ScriptIsActive(false);
  if( !m_interpreter_mode && success )
  {
    displayOutput("Script execution completed successfully.", true);
  }
  if( editor )
  {
    disconnect(runner, SIGNAL(currentLineChanged(int, bool)), editor, 
	       SLOT(updateMarker(int, bool)));
  }
  return success;
}

/**
 * Compile a piece of code in the current environment
 * @param code the code to compile
 */
void ScriptManagerWidget::compile(const QString & code)
{
  ScriptEditor *editor = currentEditor();
  if(!editor)
  {
    return ;
  }// Get the correct script runner
  Script * runner = m_script_runners.value(this->currentIndex());
  runner->setCode(code);
  emit ScriptIsActive(true);
   
  bool success = runner->compile(true);
  emit ScriptIsActive(false);
 
  if(success)
  {
    editor->setCompilationStatus(true);
  }
  else
  { 
    editor->setCompilationStatus(false);
  }
 
}
/**Run the multi line code set to runner
 * @param line_offset offset of the line
 * @returns true if executed successfully
 */
bool ScriptManagerWidget::runMultiLineCode(int line_offset)
{
  // Get the correct script runner
  Script * runner = m_script_runners.value(this->currentIndex());
  if(!runner)
  {
    return 0;
  }
  emit ScriptIsActive(true);
  runner->setLineOffset(line_offset);
  bool success = runner->exec();
  emit ScriptIsActive(false);
  return success;
}
/** 
 * Display an output message
 * @param msg The message string
 * @param timestamp Whether to display a timestamp
 */
void ScriptManagerWidget::displayOutput(const QString & msg, bool timestamp)
{  
  //Forward to helper
  emit MessageToPrint(msg, false, timestamp);
}

/**
 * Display an error message
 * @param msg The message string
 * @param timestamp Whether to display a timestamp
 */
void ScriptManagerWidget::displayError(const QString & msg, bool timestamp)
{ 
  //Forward to helper
  emit MessageToPrint(msg, true, timestamp);
}

/**
 * Show the find dialog
 * If true then it is a find and replace dialog
 */
void ScriptManagerWidget::showFindDialog(bool replace)
{
  if( count() == 0 ) return;
  if( !m_findrep_dlg )
  {
    m_findrep_dlg = new FindReplaceDialog(this, replace, this);
    connect(this, SIGNAL(currentChanged(int)), m_findrep_dlg, SLOT(resetSearchFlag()));
  }
  if( !m_findrep_dlg->isVisible() )
  {
    m_findrep_dlg->show();
  }
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

  if( !m_interpreter_mode ) 
  {
    //File actions
    context.addAction(m_open_curtab);
    context.addAction(m_save);
    context.addAction(printAction());
    
    
    
    context.insertSeparator();
    
    //Evaluate and execute
    context.addAction(m_exec);
    context.addAction(m_exec_all);
    if( scriptingEnv()->supportsEvaluation() )
    {
      context.addAction(m_eval);
    }
  }
   
  // Edit actions
   context.insertSeparator();
   context.addAction(copyAction());
   context.addAction(cutAction());
   context.addAction(pasteAction());
 
   context.insertSeparator();

   context.addAction(zoomInAction());
   context.addAction(zoomOutAction());

   context.insertSeparator();

  context.addAction(m_toggle_completion);
  context.addAction(m_toggle_calltips);
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
 * Mark the current tab as changed
 */
void ScriptManagerWidget::markCurrentAsChanged()
{
  int index = currentIndex();
  setTabText(index, tabText(index) + "*");
  //Disconnect signal so that this doesn't get run in the future
  disconnect( currentEditor(), SIGNAL(textChanged()), this, SLOT(markCurrentAsChanged()));
}

/**
 * Enable/disable script interaction based on script execution status
 * @param running The state of the script
 */
void ScriptManagerWidget::setScriptIsRunning(bool running)
{
  // Enable/disable execute actions
  m_exec->setEnabled(!running);
  m_exec_all->setEnabled(!running);
  if( scriptingEnv()->supportsEvaluation() ) m_eval->setEnabled(!running);
}

/**
 * Toggle the progress arrow on/off
 * @param state The state of the option
 */
void ScriptManagerWidget::toggleProgressArrow(bool state)
{
  scriptingEnv()->reportProgress(state);
  if( state == false )
  {
    int index_end = count() - 1;
    for( int index = index_end; index >= 0; --index )
    {
      ScriptEditor *editor = static_cast<ScriptEditor*>(widget(index));
      if( editor ) editor->setMarkerState(state);
    }
  }
}

/**
 * Toggle code folding on/off
 * @param state The state of the option
 */
void ScriptManagerWidget::toggleCodeFolding(bool state)
{
  int index_end = count() - 1;
  for( int index = index_end; index >= 0; --index )
  {
    if( ScriptEditor *editor = qobject_cast<ScriptEditor*>(this->widget(index)) )
    { 
      setCodeFoldingBehaviour(editor, state);
    }
  }
}
/**
 * Toggle code completion. Note that turning off code completion automatically turns off call tips
 * @param state The state of the option
 */
void ScriptManagerWidget::toggleCodeCompletion(bool state)
{
  int index_end = count() - 1;
  for( int index = index_end; index >= 0; --index )
  {
    if( ScriptEditor *editor = qobject_cast<ScriptEditor*>(this->widget(index)) )
    {
      setCodeCompletionBehaviour(editor, state);
    }
  }
}

/**
 * Toggle call tips.
 * @param state The state of the option
 */
void ScriptManagerWidget::toggleCallTips(bool state)
{
  int index_end = count() - 1;
  for( int index = index_end; index >= 0; --index )
  {
    if( ScriptEditor *editor = qobject_cast<ScriptEditor*>(this->widget(index)) )
    {
      setCallTipsBehaviour(editor, state);
    }
  }
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
  m_find = new QAction(tr("&Find/Replace"), this);
  m_find->setShortcut(tr("Ctrl+F"));
  connect(m_find, SIGNAL(activated()), this, SLOT(showFindDialog()));

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

  // Toggle the progress arrow
  m_toggle_progress = new QAction(tr("Show &Progress Marker"), this);
  m_toggle_progress->setCheckable(true);
  m_toggle_progress->setEnabled(scriptingEnv()->supportsProgressReporting());
  connect(m_toggle_progress, SIGNAL(toggled(bool)), this, SLOT(toggleProgressArrow(bool)));

  // Toggle code folding
  m_toggle_folding = new QAction(tr("Code &Folding"), this);
  m_toggle_folding->setCheckable(true);
  connect(m_toggle_folding, SIGNAL(toggled(bool)), this, SLOT(toggleCodeFolding(bool)));

  // Toggle code completion
  m_toggle_completion = new QAction(tr("Code &Completion"), this);
  m_toggle_completion->setCheckable(true);
  connect(m_toggle_completion, SIGNAL(toggled(bool)), this, SLOT(toggleCodeCompletion(bool)));

  // Toggle call tips
  m_toggle_calltips = new QAction(tr("Call &Tips"), this);
  m_toggle_calltips->setCheckable(true);
  connect(m_toggle_calltips, SIGNAL(toggled(bool)), this, SLOT(toggleCallTips(bool)));
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
    Scripted::scriptingChangeEvent(sce);

    // Update the code lexers for each tab
    int ntabs = count();
    for( int index = 0; index < ntabs; ++index )
    {
      ScriptEditor *editor = static_cast<ScriptEditor*>(widget(index));
      editor->setLexer(scriptingEnv()->createCodeLexer());
      m_script_runners[index] = createScriptRunner(editor);
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
  if( !newtab ) askSave(currentIndex());
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
  //Save last directory
  m_last_dir = QFileInfo(file_to_open).absolutePath();
  
  bool ok(false);
  QString script_txt = readScript(file_to_open, &ok);
  if( !ok ) return;

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
  editor->update();
  editor->blockSignals(false);
  setTabText(currentIndex(), QFileInfo(file_to_open).fileName());
  editor->setFileName(file_to_open);
  editor->setCursorPosition(0,0);
  

  // Set last directory
  m_last_dir = QFileInfo(file_to_open).absolutePath();

  // Ensure the script runner knows about the file path
  Script *runner = m_script_runners.value(currentIndex());
  if( runner ) runner->updatePath(file_to_open);
}

/**
 * Create and return a new Script object, connecting up the relevant signals.
 * @param An optional ScriptEditor object
 */
Script * ScriptManagerWidget::createScriptRunner(ScriptEditor *editor)
{
  Script *script = scriptingEnv()->newScript("", this, "");
  // Connect the signals that print output and error messages to the formatting functions
  connect(script, SIGNAL(print(const QString &)), this, SLOT(displayOutput(const QString &)));
  connect(script, SIGNAL(error(const QString &, const QString&, int)), this, 
	  SLOT(displayError(const QString &)));
  if( editor )
  {
    connect(script, SIGNAL(keywordsChanged(const QStringList&)), editor, 
	    SLOT(updateCompletionAPI(const QStringList &)));
    /// Initialize the auto complete by evaluating some completely trivial code
    script->setCode("1");
    script->exec();

    

  }
  return script;
}

/**
 * Close a given tab
 * @param index The tab index
 */ 
void ScriptManagerWidget::closeTabAtIndex(int index)
{
  ScriptEditor *editor = qobject_cast<ScriptEditor*>(widget(index));
  if( !editor ) return;
  //Check if we need to save
  askSave(index);
  // Remove the path from the script runner
  Script *runner = m_script_runners.value(currentIndex());
  if( runner ) runner->updatePath(editor->fileName(), false);
  //Get the widget attached to the tab first as this is not deleted
  //when remove is called
  editor->deleteLater();
  //  If we are removing the final tab, close the find replace dialog if it exists and is visible
  if( m_findrep_dlg && m_findrep_dlg->isVisible() && count() == 1 )
  {
    m_findrep_dlg->close();
  }
  removeTab(index);
}


/**
 * Close a tab at a given position
 * @param pos The tab at the given position
 */ 
void ScriptManagerWidget::closeTabAtPosition(const QPoint & pos)
{
  int index = tabBar()->tabAt(pos);
  //Index is checked in closeTab
  closeTabAtIndex(index);
}

/** Writes the file to disk
 *  @param The editor tab to be saved
 */
void ScriptManagerWidget::doSave(ScriptEditor * editor)
{
  QString filename = editor->fileName();
  editor->saveScript(filename);
  setTabText(currentIndex(), QFileInfo(filename).fileName());
  editor->setModified(false);
  connect(editor, SIGNAL(textChanged()), this, SLOT(markCurrentAsChanged()));
}

/** 
 * Set auto complete behaviour for the given editor
 * @param editor The editor widget to set the behaviour on
 * @param state The state required
 */
void ScriptManagerWidget::setCodeCompletionBehaviour(ScriptEditor *editor, bool state)
{
  QsciScintilla::AutoCompletionSource api_source;
  int threshold(-1);
  if( state )
  {
    api_source = QsciScintilla::AcsAPIs;
    threshold = 2;
  }
  else
  {
    api_source = QsciScintilla::AcsNone;
    threshold = -1;
  }
  
  editor->setAutoCompletionThreshold(threshold);  // threshold characters before autocomplete kicks in
  editor->setAutoCompletionSource(api_source);
}

/** 
 * Set call tips behaviour for the given editor
 * @param editor The editor widget to set the behaviour on
 * @param state The state required
 */
void ScriptManagerWidget::setCallTipsBehaviour(ScriptEditor *editor, bool state)
{
  QsciScintilla::CallTipsStyle tip_style;
  int nvisible(-1);
  if( state )
  {
    tip_style = QsciScintilla::CallTipsNoAutoCompletionContext;
    nvisible = 0; // This actually makes all of them visible at the same time
  }
  else
  {
    tip_style = QsciScintilla::CallTipsNone;
    nvisible = -1;
  }

  editor->setCallTipsVisible(nvisible);
  editor->setCallTipsStyle(tip_style);
}

/** 
 * Set code folding behaviour for the given editor
 * @param editor The editor widget to set the behaviour on
 * @param state The state required
 */
void ScriptManagerWidget::setCodeFoldingBehaviour(ScriptEditor *editor, bool state)
{
  QsciScintilla::FoldStyle fold_option;
  if( state && !m_interpreter_mode )
  {
    fold_option = QsciScintilla::BoxedTreeFoldStyle;
  }
  else
  {
    fold_option = QsciScintilla::NoFoldStyle;
  }
  
  editor->setFolding(fold_option);
}


//***************************************************************************
//
// FindReplaceDialog class
//
//***************************************************************************
//------------------------------------------------------
// Public member functions
//------------------------------------------------------
/**
 * Constructor
 */
FindReplaceDialog::FindReplaceDialog(ScriptManagerWidget *manager, bool replace, QWidget* parent, Qt::WFlags fl )
  : QDialog( parent, fl ), m_manager(manager), m_find_inprogress(false)
{
  setWindowTitle (tr("MantidPlot") + " - " + tr("Find"));
  setSizeGripEnabled( true );

  QGroupBox *gb1 = new QGroupBox();
  QGridLayout *topLayout = new QGridLayout(gb1);

  topLayout->addWidget( new QLabel(tr( "Find" )), 0, 0);
  boxFind = new QComboBox();
  boxFind->setEditable(true);
  boxFind->setDuplicatesEnabled(false);
  boxFind->setInsertPolicy( QComboBox::InsertAtTop );
  boxFind->setAutoCompletion(true);
  boxFind->setMaxCount ( 10 );
  boxFind->setMaxVisibleItems ( 10 );
  boxFind->setMinimumWidth(250);
  boxFind->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
  connect(boxFind, SIGNAL(editTextChanged(const QString &)), this, SLOT(resetSearchFlag()));

  ScriptEditor *editor = m_manager->currentEditor();
  if( editor->hasSelectedText() )
  {
    QString text = editor->selectedText();
    boxFind->setEditText(text);
    boxFind->addItem(text);
  }

  topLayout->addWidget(boxFind, 0, 1);

  if( replace )
  {
    setWindowTitle (tr("MantidPlot") + " - " + tr("Find and Replace"));
    topLayout->addWidget(new QLabel(tr( "Replace with" )), 1, 0);
    boxReplace = new QComboBox();
    boxReplace->setEditable(true);
    boxReplace->setDuplicatesEnabled(false);
    boxReplace->setInsertPolicy( QComboBox::InsertAtTop );
    boxReplace->setAutoCompletion(true);
    boxReplace->setMaxCount ( 10 );
    boxReplace->setMaxVisibleItems ( 10 );
    boxReplace->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    topLayout->addWidget( boxReplace, 1, 1);
    topLayout->setColumnStretch(1, 10);
  }

  QGroupBox *gb2 = new QGroupBox();
  QGridLayout * bottomLayout = new QGridLayout(gb2);
  QButtonGroup *find_options = new QButtonGroup(this);
  find_options->setExclusive(false);

  boxCaseSensitive = new QCheckBox(tr("&Match case"));
  boxCaseSensitive->setChecked(false);
  bottomLayout->addWidget( boxCaseSensitive, 0, 0);
  find_options->addButton(boxCaseSensitive);

  boxWholeWords = new QCheckBox(tr("&Whole word"));
  boxWholeWords->setChecked(false);
  bottomLayout->addWidget(boxWholeWords, 1, 0);
  find_options->addButton(boxWholeWords);

  boxRegex = new QCheckBox(tr("&Regular expression"));
  boxRegex->setChecked(false);
  bottomLayout->addWidget(boxRegex, 2, 0);
  find_options->addButton(boxRegex);
 
  boxSearchBackwards = new QCheckBox(tr("&Search backwards"));
  boxSearchBackwards->setChecked(false);
  bottomLayout->addWidget(boxSearchBackwards, 0, 1);
  find_options->addButton(boxSearchBackwards);

  boxWrapAround = new QCheckBox(tr("&Wrap around"));
  boxWrapAround->setChecked(true);
  bottomLayout->addWidget(boxWrapAround, 1, 1);
  find_options->addButton(boxWrapAround);

  connect(find_options, SIGNAL(buttonClicked(int)), this, SLOT(resetSearchFlag()));

  QVBoxLayout *vb1 = new QVBoxLayout();
  vb1->addWidget(gb1);
  vb1->addWidget(gb2);

  QVBoxLayout *vb2 = new QVBoxLayout();

  buttonNext = new QPushButton(tr("&Next"));
  buttonNext->setShortcut(tr("Ctrl+F"));
  buttonNext->setDefault(true);
  vb2->addWidget(buttonNext);

  if( replace )
  {
    buttonReplace = new QPushButton(tr("&Replace"));
    connect(buttonReplace, SIGNAL(clicked()), this, SLOT(replace()));
    vb2->addWidget(buttonReplace);

    buttonReplaceAll = new QPushButton(tr("Replace &all"));
    connect(buttonReplaceAll, SIGNAL(clicked()), this, SLOT(replaceAll()));
    vb2->addWidget(buttonReplaceAll);
  }

  buttonCancel = new QPushButton(tr("&Close"));
  vb2->addWidget(buttonCancel);
  vb2->addStretch();

  QHBoxLayout *hb = new QHBoxLayout(this);
  hb->addLayout(vb1);
  hb->addLayout(vb2);

  connect(buttonNext, SIGNAL(clicked()), this, SLOT(findClicked()));
  connect(buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));
}

//------------------------------------------------------
// Protected slot member functions
//------------------------------------------------------
/**
 * Find the current search term
 * @param backwards If true then the search procedes backwards from the cursor's current position
 * @returns A boolean indicating success/failure
 */
bool FindReplaceDialog::find(bool backwards)
{
  QString searchString = boxFind->currentText();
  if (searchString.isEmpty()){
    QMessageBox::warning(this, tr("Empty Search Field"),
			 tr("The search field is empty. Please enter some text and try again."));
    boxFind->setFocus();
    return false;
  }

  if(boxFind->findText(searchString) == -1)
  {
    boxFind->addItem(searchString);
  }

  if( m_find_inprogress )
  {
    m_find_inprogress = m_manager->currentEditor()->findNext();
  }
  else
  {
    bool cs = boxCaseSensitive->isChecked();
    bool whole = boxWholeWords->isChecked();
    bool wrap = boxWrapAround->isChecked();
    bool regex = boxRegex->isChecked();
    m_find_inprogress = m_manager->currentEditor()->findFirst(searchString, regex, cs, whole, wrap, !backwards);
  }
  return m_find_inprogress;
}

/**
 * Replace the next occurrence of the search term with the replacement text
 */
void FindReplaceDialog::replace()
{
  QString searchString = boxFind->currentText();
  if (searchString.isEmpty()){
    QMessageBox::warning(this, tr("Empty Search Field"),
			 tr("The search field is empty. Please enter some text and try again."));
    boxFind->setFocus();
    return;
  }

  if (!m_manager->currentEditor()->hasSelectedText() || m_manager->currentEditor()->selectedText() != searchString){
    find();//find and select next match
    return;
  }

  QString replaceString = boxReplace->currentText();
  m_manager->currentEditor()->replace(replaceString);
  find();//find and select next match

  if(boxReplace->findText(replaceString) == -1)
    boxReplace->addItem(replaceString);
}

/**
 * Replace all occurrences of the current search term with the replacement text
 */
void FindReplaceDialog::replaceAll()
{
  QString searchString = boxFind->currentText();
  if (searchString.isEmpty()){
    QMessageBox::warning(this, tr("Empty Search Field"),
			 tr("The search field is empty. Please enter some text and try again."));
    boxFind->setFocus();
    return;
  }

  if(boxFind->findText(searchString) == -1)
  {
    boxFind->addItem (searchString);
  }

  QString replaceString = boxReplace->currentText();
  if(boxReplace->findText(replaceString) == -1)
  {
    boxReplace->addItem(replaceString);
  }

  ScriptEditor *editor =  m_manager->currentEditor();
  int line(-1), index(-1), prevLine(-1), prevIndex(-1);
  bool regex = boxRegex->isChecked();
  bool cs = boxCaseSensitive->isChecked();
  bool whole = boxWholeWords->isChecked();
  bool wrap = boxWrapAround->isChecked();
  bool backward = boxSearchBackwards->isChecked();
  // Mark this as a set of actions that can be undone as one
  editor->beginUndoAction();
  bool found = editor->findFirst(searchString, regex, cs, whole, wrap, !backward, 0, 0);
  // If find first fails then there is nothing to replace
  if( !found )
  {
    QMessageBox::information(this, "MantidPlot - Find and Replace", "No matches found in current document.");
  }

  while( found )
  {
    editor->replace(replaceString);
    editor->getCursorPosition(&prevLine, &prevIndex);
    found = editor->findNext();
    editor->getCursorPosition(&line, &index);
    if( line < prevLine || ( line == prevLine && index <= prevIndex ) )
    {
      break;
    }
  }
  editor->endUndoAction();
}

/**
 * Find button clicked slot
 */
void FindReplaceDialog::findClicked()
{
  // Forward to worker function
  find(boxSearchBackwards->isChecked());
}

/**
 * Flip the in-progress flag
 */
void FindReplaceDialog::resetSearchFlag()
{
  if( ScriptEditor *editor = m_manager->currentEditor() )
  {
    m_find_inprogress = false;
    editor->setSelection(-1, -1, -1, -1);
  }
}
