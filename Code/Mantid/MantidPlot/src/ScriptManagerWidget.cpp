//-----------------------------------------------------
// Includes
//-----------------------------------------------------
#include "ApplicationWindow.h"
#include "ScriptFileInterpreter.h"

#include "ScriptingEnv.h"
#include "ScriptingLangDialog.h"
#include "ScriptManagerWidget.h"

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
ScriptManagerWidget::ScriptManagerWidget(ScriptingEnv *env, QWidget *parent)
  : QTabWidget(parent), Scripted(env), m_last_dir(""),
    m_cursor_pos(), m_interpreter_mode(false), m_recentScriptList()
{
  //Create actions for this widget
  initActions();
  
  // Start with a blank tab
  newTab();
  if( m_interpreter_mode )
  {
//    tabBar()->hide();
//    setContextMenuPolicy(Qt::NoContextMenu);
//    ScriptEditor *editor = currentEditor();
//
//    connect(editor, SIGNAL(executeLine(const QString&)), this, SLOT(executeInterpreter(const QString &)));
//    connect(this, SIGNAL(MessageToPrint(const QString&, bool,bool)), editor,
//	    SLOT(displayOutput(const QString&,bool)));
//     connect(editor, SIGNAL(compile(const QString&)), this, SLOT(compile(const QString &)));
//      connect(editor, SIGNAL(executeMultiLine()), this, SLOT(executeMultiLine()));
 
  }
  
  // Settings
  QSettings settings;
  settings.beginGroup("ScriptWindow");
  m_toggle_folding->setChecked(settings.value("CodeFolding", true).toBool());
  m_toggle_completion->setChecked(settings.value("CodeCompletion", true).toBool());
  m_toggle_calltips->setChecked(settings.value("CallTips", true).toBool());
  settings.endGroup();

  connect(this, SIGNAL(currentChanged(int)), this, SLOT(tabSelectionChanged(int)));
}

/**
 * Destructor
 */
ScriptManagerWidget::~ScriptManagerWidget()
{
}

/**
 * Save the settings applicable here
 */
void ScriptManagerWidget::saveSettings()
{
  QString group("ScriptWindow");
  QSettings settings;
  settings.beginGroup(group);
  settings.setValue("/CodeFolding", m_toggle_folding->isChecked());
  settings.setValue("/CodeCompletion", m_toggle_completion->isChecked());
  settings.setValue("/CallTips", m_toggle_calltips->isChecked());
  settings.endGroup();
}

/// @return The current interpreter
ScriptFileInterpreter * ScriptManagerWidget::currentInterpreter()
{
  return interpreterAt(currentIndex());
}

/// @return Interpreter at given index
ScriptFileInterpreter * ScriptManagerWidget::interpreterAt(int index)
{
  return qobject_cast<ScriptFileInterpreter*>(widget(index));
}

// ----------------------- Menus --------------------------------------------
/**
 *  Add the required entries for the file menu in this context
 * @param fileMenu A pointer to the menu object to fill
 */
void ScriptManagerWidget::populateFileMenu(QMenu &fileMenu)
{
  fileMenu.addAction(m_new_tab);
  fileMenu.addAction(m_open_curtab);
  fileMenu.addAction(m_open_newtab);

  fileMenu.insertSeparator();

  if( count() > 0)
  {
    ScriptFileInterpreter *current = currentInterpreter();
    current->populateFileMenu(fileMenu);
  }

  fileMenu.insertSeparator();
  fileMenu.addMenu(m_recent_scripts);
  updateRecentScriptList();

  if( count() > 0)
  {
    fileMenu.insertSeparator();
    fileMenu.addAction(m_close_tab);
  }
}
/**
 *  Add the required entries for the edit menu in this context
 * @param editMenu A pointer to the menu object to fill
 */
void ScriptManagerWidget::populateEditMenu(QMenu &editMenu)
{
  if( count() > 0 )
  {
    ScriptFileInterpreter *current = currentInterpreter();
    current->populateEditMenu(editMenu);
  }
}
/**
 *  Add the required entries for the execute menu in this context
 * @param editMenu A pointer to the menu object to fill
 */
void ScriptManagerWidget::populateExecMenu(QMenu &execMenu)
{
  if( count() > 0 )
  {
    ScriptFileInterpreter *current = currentInterpreter();
    current->populateExecMenu(execMenu);
  }
}

/**
 * Is a script running in the environment
 */
bool ScriptManagerWidget::isScriptRunning()
{
  return scriptingEnv()->isRunning();
}


/// copy method
void ScriptManagerWidget::copy()
{
  QMessageBox::warning(this, "", "Copy not implemented");
}
  ///paste method
void ScriptManagerWidget::paste()
{
  QMessageBox::warning(this, "", "Paste not implemented");
  
}
//-------------------------------------------
// Public slots
//-------------------------------------------
/**
 * Create a new tab such that it is the specified index within the tab range.
 * @param index :: The index to give the new tab. If this is invalid the tab is simply appended
 * @param filename :: An optional filename
 */
void ScriptManagerWidget::newTab(int index, const QString & filename)
{
  QString tabTitle;
  if( filename.isEmpty() )
  {
    tabTitle = "New script";
  }
  else
  {
    tabTitle = QFileInfo(filename).fileName();
  }
//  if( m_interpreter_mode )
//  {
//    ScriptEditor *editor = new ScriptEditor(this, m_interpreter_mode, scriptingEnv()->createCodeLexer());
//    editor->setFileName(filename);
//    editor->setContextMenuPolicy(Qt::CustomContextMenu);
//    connect(editor, SIGNAL(customContextMenuRequested(const QPoint&)),
//      this, SLOT(editorContextMenu(const QPoint&)));
//    index = insertTab(index, editor, tabTitle);
//
//    // Store a script runner
//    Script * runner = createScriptRunner(editor);
//    m_script_runners.insert(editor, runner);
//
//    // Completion etc
//    setCodeCompletionBehaviour(editor, m_toggle_completion->isChecked());
//    setCallTipsBehaviour(editor, m_toggle_calltips->isChecked());
//    setCodeFoldingBehaviour(editor, m_toggle_folding->isChecked());
//    // Set the current editor to focus
//    setFocusProxy(editor);
//    editor->setFocus();
//    editor->setCursorPosition(0,0);
//    return editor;
//  }
  ScriptFileInterpreter *scriptRunner = new ScriptFileInterpreter(this);
  scriptRunner->setup(*scriptingEnv(), filename);
  connect(scriptRunner, SIGNAL(textChanged()), this, SLOT(markCurrentAsChanged()));
  index = insertTab(index, scriptRunner, tabTitle);
  setCurrentIndex(index);
  scriptRunner->setFocus();
  m_last_active_tab = index;
}

/**
 * Open a file in the current tab
 * @param filename :: An optional file name
 */
void ScriptManagerWidget::openInCurrentTab(const QString & filename)
{
  open(false, filename);
}

/**
 * Open a file in a new tab
 * @param filename :: An optional file name
 */
void ScriptManagerWidget::openInNewTab(const QString & filename)
{
  open(true, filename);
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
    ScriptFileInterpreter *interpreter = interpreterAt(index);
    QString s = interpreter->filename();
    if(!s.isEmpty())
    {
      fileNames+=s;
      fileNames+="\t";
    }
  }
  fileNames+="\n</scriptwindow>\n";
  return fileNames;
}
/**
 * Execute the highlighted code from the current tab
 */
void ScriptManagerWidget::executeAll()
{
  ScriptFileInterpreter *currentTab = qobject_cast<ScriptFileInterpreter*>(widget(currentIndex()));
  currentTab->executeAll();
}

/**
 * Execute the whole script
 */
void ScriptManagerWidget::executeSelection()
{
  ScriptFileInterpreter *currentTab = qobject_cast<ScriptFileInterpreter*>(widget(currentIndex()));
  currentTab->executeSelection();
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
 * @param code :: The chunk of code to execute
 */
void ScriptManagerWidget::executeInterpreter(const QString & code)
{
//  if( isScriptRunning() ) return;
//  ScriptEditor *editor = currentEditor();
//  if( !editor ) return;
//
//  int lineno,index;
//  editor->getCursorPosition(&lineno, &index);
//  runScriptCode(code,lineno);
//
//  editor->newInputLine();
//  setFocus();
}

/** 
 * Execute multi line code
 */
void ScriptManagerWidget::executeMultiLine()
{
//  ScriptEditor *editor = currentEditor();
//  if(!editor)
//  {
//    return;
//  }
//  int lineno,index;
//  editor->getCursorPosition(&lineno, &index);
//  runMultiLineCode(lineno);
//  editor->append("\n");
//  int marker_handle= editor->markerDefine(QsciScintilla::ThreeRightArrows);
//  editor->setMarkerHandle(marker_handle);
//  editor->newInputLine();
//  setFocus();
}

/**
 * Compile a piece of code in the current environment
 * @param code :: the code to compile
 */
void ScriptManagerWidget::compile(const QString & code)
{
//  ScriptEditor *editor = currentEditor();
//  if(!editor)
//  {
//    return ;
//  }// Get the correct script runner
//  Script * runner = currentRunner();
//  int lineno,index;
//  editor->getCursorPosition(&lineno, &index);
//  runner->setLineOffset(lineno);
//  runner->setCode(code);
//  emit ScriptIsActive(true);
//
//  bool success = runner->compile(true);
//  emit ScriptIsActive(false);
//
//  if(success)
//  {
//    editor->setCompilationStatus(true);
//  }
//  else
//  {
//    editor->setCompilationStatus(false);
//  }
 
}
/**Run the multi line code set to runner
 * @param line_offset :: offset of the line
 * @returns true if executed successfully
 */
bool ScriptManagerWidget::runMultiLineCode(int line_offset)
{
//  // Get the correct script runner
//  Script * runner = currentRunner();
//  emit ScriptIsActive(true);
//
//  runner->setLineOffset(line_offset);
//  bool success = runner->exec();
//
//  emit ScriptIsActive(false);
//  return success;
}

/**
 * Show the find dialog
 * If true then it is a find and replace dialog
 */
void ScriptManagerWidget::showFindDialog(bool replace)
{
  if( count() == 0 ) return;
//  if( !m_findrep_dlg )
//  {
//    m_findrep_dlg = new FindReplaceDialog(this, replace, this);
//    connect(this, SIGNAL(currentChanged(int)), m_findrep_dlg, SLOT(resetSearchFlag()));
//  }
//  if( !m_findrep_dlg->isVisible() )
//  {
//    m_findrep_dlg->show();
//  }

  QMessageBox::information(this, "MantidPlot", "Find not implemented");
}


//--------------------------------------------
// Private slots
//--------------------------------------------
/**
 * Handle a context menu request for one of the editors
 */
void ScriptManagerWidget::editorContextMenu(const QPoint &)
{
//  QMenu context(this);
//
//  if( !m_interpreter_mode )
//  {
//    //File actions
//    context.addAction(m_open_curtab);
//    context.addAction(m_save);
//    context.addAction(printAction());
//
//
//    context.insertSeparator();
//
//    //Evaluate and execute
//    context.addAction(m_exec);
//    context.addAction(m_exec_all);
//    if( scriptingEnv()->supportsEvaluation() )
//    {
//      context.addAction(m_eval);
//    }
//  }
//
//  // Edit actions
//   context.insertSeparator();
//   context.addAction(copyAction());
//   context.addAction(cutAction());
//   context.addAction(pasteAction());
//
//   context.insertSeparator();
//
//   context.addAction(zoomInAction());
//   context.addAction(zoomOutAction());
//
//   context.insertSeparator();
//
//  context.addAction(m_toggle_completion);
//  context.addAction(m_toggle_calltips);
//  context.exec(QCursor::pos());
}

/**
 * Close current tab
 */
int ScriptManagerWidget::closeCurrentTab()
{
  if( count() > 0 )
  {
    int index = currentIndex();
    closeTabAtIndex(index);
    return index;
  }
  return -1;
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
 * Mark the current tab as changed. The signal is disconnected
 * from the emitting widget so that multiple calls are not performed
 */
void ScriptManagerWidget::markCurrentAsChanged()
{
  int index = currentIndex();
  setTabText(index, tabText(index) + "*");
  // Disconnect signal so that this doesn't get run in the future
  ScriptFileInterpreter *currentWidget = qobject_cast<ScriptFileInterpreter*>(widget(index));
  disconnect(currentWidget, SIGNAL(textChanged()), this, SLOT(markCurrentAsChanged()));
}

/**
 * The current selection has changed
 * @param index The index of the new selection
 */
void ScriptManagerWidget::tabSelectionChanged(int index)
{
  if( count() > 0 )
  {
    ScriptFileInterpreter *currentWidget = qobject_cast<ScriptFileInterpreter*>(widget(index));
    setFocusProxy(currentWidget);
    currentWidget->setFocus();
  }
}

/**
 * Enable/disable script interaction based on script execution status
 * @param running :: The state of the script
 */
void ScriptManagerWidget::setScriptIsRunning(bool running)
{
}

/**
 * Toggle the progress arrow on/off
 * @param state :: The state of the option
 */
void ScriptManagerWidget::toggleProgressArrow(bool state)
{
  int index_end = count() - 1;
  for( int index = index_end; index >= 0; --index )
  {
    QMessageBox::warning(this, "", "Implement progress arrow");
  }
}

/**
 * Toggle code folding on/off
 * @param state :: The state of the option
 */
void ScriptManagerWidget::toggleCodeFolding(bool state)
{
  int index_end = count() - 1;
  for( int index = index_end; index >= 0; --index )
  {
    QMessageBox::warning(this, "", "Implement code folding");
  }
}
/**
 * Toggle code completion. Note that turning off code completion automatically turns off call tips
 * @param state :: The state of the option
 */
void ScriptManagerWidget::toggleCodeCompletion(bool state)
{
  int index_end = count() - 1;
  for( int index = index_end; index >= 0; --index )
  {
    QMessageBox::warning(this, "", "Implement code completion");
  }
}

/**
 * Toggle call tips.
 * @param state :: The state of the option
 */
void ScriptManagerWidget::toggleCallTips(bool state)
{
  int index_end = count() - 1;
  for( int index = index_end; index >= 0; --index )
  {
    QMessageBox::warning(this, "", "Implement call tips");
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


  //Close the current tab
  m_close_tab = new QAction(tr("&Close Tab"), this);
  m_close_tab->setShortcut(tr("Ctrl+W"));
  connect(m_close_tab,SIGNAL(activated()), this, SLOT(closeCurrentTab()));

  // recent scripts option
  m_recent_scripts = new QMenu(tr("&Recent Scripts"),this);
  connect(m_recent_scripts,SIGNAL(activated(int)),this,SLOT(openRecentScript(int)));
  
  // **Edit** actions
  m_find = new QAction(tr("&Find/Replace"), this);
  m_find->setShortcut(tr("Ctrl+F"));
  connect(m_find, SIGNAL(activated()), this, SLOT(showFindDialog()));

  // Toggle the progress arrow
  m_toggle_progress = new QAction(tr("Show &Progress Marker"), this);
  m_toggle_progress->setCheckable(true);
  if( m_interpreter_mode )
  {
    m_toggle_progress->setEnabled(false);
  }
  else
  {
    m_toggle_progress->setEnabled(scriptingEnv()->supportsProgressReporting());
  }
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
 * @param event :: The context menu event
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
 * @param event :: The custome event
 */
void ScriptManagerWidget::customEvent(QEvent *event)
{
  if( !isScriptRunning() && event->type() == SCRIPTING_CHANGE_EVENT )
  {    
    ScriptingChangeEvent *sce = static_cast<ScriptingChangeEvent*>(event);
    // This handles reference counting of the scripting environment
    Scripted::scriptingChangeEvent(sce);

    QMessageBox::warning(this, "", "Implement script changing");
  }
}

/**
 * Open a file
 * @param newtab :: If true, a new tab will be created
 * @param filename :: An optional file name
 */
void ScriptManagerWidget::open(bool newtab, const QString & filename)
{
  QString fileToOpen = filename;
  if( fileToOpen.isEmpty() )
  {
    QString filter = scriptingEnv()->fileFilter();
    filter += tr("Text") + " (*.txt *.TXT);;";
    filter += tr("All Files")+" (*)";
    fileToOpen = QFileDialog::getOpenFileName(this, tr("MantidPlot - Open a script from a file"),
        m_last_dir, filter);
    if( fileToOpen.isEmpty() )
    {
      return;
    }
  }
  /// remove the file name from script list
  m_recentScriptList.remove(fileToOpen);
  //add the script file to recent scripts list 
  m_recentScriptList.push_front(fileToOpen);
  //update the recent scripts menu 
  updateRecentScriptList();
 
  //Save last directory
  m_last_dir = QFileInfo(fileToOpen).absolutePath();

  int index(-1);
  if( !newtab ) index = closeCurrentTab();
  newTab(index, fileToOpen);
}

/**
 * Close a given tab
 * @param index :: The tab index
 */ 
void ScriptManagerWidget::closeTabAtIndex(int index)
{
  ScriptFileInterpreter *interpreter = qobject_cast<ScriptFileInterpreter*>(widget(index));
  interpreter->prepareToClose();
  removeTab(index);
}


/**
 * Close a tab at a given position
 * @param pos :: The tab at the given position
 */ 
void ScriptManagerWidget::closeTabAtPosition(const QPoint & pos)
{
  int index = tabBar()->tabAt(pos);
  //Index is checked in closeTab
  closeTabAtIndex(index);
}

///**
// * Set auto complete behaviour for the given editor
// * @param editor :: The editor widget to set the behaviour on
// * @param state :: The state required
// */
//void ScriptManagerWidget::setCodeCompletionBehaviour(ScriptEditor *editor, bool state)
//{
//  QsciScintilla::AutoCompletionSource api_source;
//  int threshold(-1);
//  if( state )
//  {
//    api_source = QsciScintilla::AcsAPIs;
//    threshold = 2;
//  }
//  else
//  {
//    api_source = QsciScintilla::AcsNone;
//    threshold = -1;
//  }
//
//  editor->setAutoCompletionThreshold(threshold);  // threshold characters before autocomplete kicks in
//  editor->setAutoCompletionSource(api_source);
//}
//
///**
// * Set call tips behaviour for the given editor
// * @param editor :: The editor widget to set the behaviour on
// * @param state :: The state required
// */
//void ScriptManagerWidget::setCallTipsBehaviour(ScriptEditor *editor, bool state)
//{
//  QsciScintilla::CallTipsStyle tip_style;
//  int nvisible(-1);
//  if( state )
//  {
//    tip_style = QsciScintilla::CallTipsNoAutoCompletionContext;
//    nvisible = 0; // This actually makes all of them visible at the same time
//  }
//  else
//  {
//    tip_style = QsciScintilla::CallTipsNone;
//    nvisible = -1;
//  }
//
//  editor->setCallTipsVisible(nvisible);
//  editor->setCallTipsStyle(tip_style);
//}
//
///**
// * Set code folding behaviour for the given editor
// * @param editor :: The editor widget to set the behaviour on
// * @param state :: The state required
// */
//void ScriptManagerWidget::setCodeFoldingBehaviour(ScriptEditor *editor, bool state)
//{
//  QsciScintilla::FoldStyle fold_option;
//  if( state && !m_interpreter_mode )
//  {
//    fold_option = QsciScintilla::BoxedTreeFoldStyle;
//  }
//  else
//  {
//    fold_option = QsciScintilla::NoFoldStyle;
//  }
//
//  editor->setFolding(fold_option);
//}

/** 
 * open the selected script from the File->Recent Scripts  in a new tab
 * @param index :: The index of the selected script
 */
void ScriptManagerWidget::openRecentScript(int index)
{
  QString scriptname = m_recent_scripts->text(index);
  scriptname.remove(0,3);
  scriptname.trimmed();
  openInNewTab(scriptname); 
}

/** 
* update the Recent Scripts menu items
* @param index :: The index of the selected script
*/
void ScriptManagerWidget::updateRecentScriptList()
{
  if (m_recentScriptList.isEmpty())
    return;

  while ((int)m_recentScriptList.size() > MaxRecentScripts)
    m_recentScriptList.pop_back();

  m_recent_scripts->clear();
  for (int i = 0; i<m_recentScriptList.size(); i++ )
  {
    m_recent_scripts->insertItem("&" + QString::number(i+1) + " " + m_recentScriptList[i]);
  }

}

/** 
* This method returns the recent scripts list
* @returns a list containing the name of the recent scripts.
*/
QStringList ScriptManagerWidget::recentScripts() 
{
  return m_recentScriptList;
}

/** 
 * sets the recent scripts list
 * @param rslist :: list containing the name of the recent scripts.
 */
void ScriptManagerWidget::setRecentScripts(const QStringList& rslist)
{
  m_recentScriptList=rslist;
}

//***************************************************************************
//
// FindReplaceDialog class
//
//***************************************************************************
//------------------------------------------------------
// Public member functions
//------------------------------------------------------
///**
// * Constructor
// */
//FindReplaceDialog::FindReplaceDialog(ScriptManagerWidget *manager, bool replace, QWidget* parent, Qt::WFlags fl )
//  : QDialog( parent, fl ), m_manager(manager), m_find_inprogress(false)
//{
//  setWindowTitle (tr("MantidPlot") + " - " + tr("Find"));
//  setSizeGripEnabled( true );
//
//  QGroupBox *gb1 = new QGroupBox();
//  QGridLayout *topLayout = new QGridLayout(gb1);
//
//  topLayout->addWidget( new QLabel(tr( "Find" )), 0, 0);
//  boxFind = new QComboBox();
//  boxFind->setEditable(true);
//  boxFind->setDuplicatesEnabled(false);
//  boxFind->setInsertPolicy( QComboBox::InsertAtTop );
//  boxFind->setAutoCompletion(true);
//  boxFind->setMaxCount ( 10 );
//  boxFind->setMaxVisibleItems ( 10 );
//  boxFind->setMinimumWidth(250);
//  boxFind->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
//  connect(boxFind, SIGNAL(editTextChanged(const QString &)), this, SLOT(resetSearchFlag()));
//
//  ScriptEditor *editor = m_manager->currentEditor();
//  if( editor->hasSelectedText() )
//  {
//    QString text = editor->selectedText();
//    boxFind->setEditText(text);
//    boxFind->addItem(text);
//  }
//
//  topLayout->addWidget(boxFind, 0, 1);
//
//  if( replace )
//  {
//    setWindowTitle (tr("MantidPlot") + " - " + tr("Find and Replace"));
//    topLayout->addWidget(new QLabel(tr( "Replace with" )), 1, 0);
//    boxReplace = new QComboBox();
//    boxReplace->setEditable(true);
//    boxReplace->setDuplicatesEnabled(false);
//    boxReplace->setInsertPolicy( QComboBox::InsertAtTop );
//    boxReplace->setAutoCompletion(true);
//    boxReplace->setMaxCount ( 10 );
//    boxReplace->setMaxVisibleItems ( 10 );
//    boxReplace->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
//    topLayout->addWidget( boxReplace, 1, 1);
//    topLayout->setColumnStretch(1, 10);
//  }
//
//  QGroupBox *gb2 = new QGroupBox();
//  QGridLayout * bottomLayout = new QGridLayout(gb2);
//  QButtonGroup *find_options = new QButtonGroup(this);
//  find_options->setExclusive(false);
//
//  boxCaseSensitive = new QCheckBox(tr("&Match case"));
//  boxCaseSensitive->setChecked(false);
//  bottomLayout->addWidget( boxCaseSensitive, 0, 0);
//  find_options->addButton(boxCaseSensitive);
//
//  boxWholeWords = new QCheckBox(tr("&Whole word"));
//  boxWholeWords->setChecked(false);
//  bottomLayout->addWidget(boxWholeWords, 1, 0);
//  find_options->addButton(boxWholeWords);
//
//  boxRegex = new QCheckBox(tr("&Regular expression"));
//  boxRegex->setChecked(false);
//  bottomLayout->addWidget(boxRegex, 2, 0);
//  find_options->addButton(boxRegex);
//
//  boxSearchBackwards = new QCheckBox(tr("&Search backwards"));
//  boxSearchBackwards->setChecked(false);
//  bottomLayout->addWidget(boxSearchBackwards, 0, 1);
//  find_options->addButton(boxSearchBackwards);
//
//  boxWrapAround = new QCheckBox(tr("&Wrap around"));
//  boxWrapAround->setChecked(true);
//  bottomLayout->addWidget(boxWrapAround, 1, 1);
//  find_options->addButton(boxWrapAround);
//
//  connect(find_options, SIGNAL(buttonClicked(int)), this, SLOT(resetSearchFlag()));
//
//  QVBoxLayout *vb1 = new QVBoxLayout();
//  vb1->addWidget(gb1);
//  vb1->addWidget(gb2);
//
//  QVBoxLayout *vb2 = new QVBoxLayout();
//
//  buttonNext = new QPushButton(tr("&Next"));
//  buttonNext->setShortcut(tr("Ctrl+F"));
//  buttonNext->setDefault(true);
//  vb2->addWidget(buttonNext);
//
//  if( replace )
//  {
//    buttonReplace = new QPushButton(tr("&Replace"));
//    connect(buttonReplace, SIGNAL(clicked()), this, SLOT(replace()));
//    vb2->addWidget(buttonReplace);
//
//    buttonReplaceAll = new QPushButton(tr("Replace &all"));
//    connect(buttonReplaceAll, SIGNAL(clicked()), this, SLOT(replaceAll()));
//    vb2->addWidget(buttonReplaceAll);
//  }
//
//  buttonCancel = new QPushButton(tr("&Close"));
//  vb2->addWidget(buttonCancel);
//  vb2->addStretch();
//
//  QHBoxLayout *hb = new QHBoxLayout(this);
//  hb->addLayout(vb1);
//  hb->addLayout(vb2);
//
//  connect(buttonNext, SIGNAL(clicked()), this, SLOT(findClicked()));
//  connect(buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));
//}
//
////------------------------------------------------------
//// Protected slot member functions
////------------------------------------------------------
///**
// * Find the current search term
// * @param backwards :: If true then the search procedes backwards from the cursor's current position
// * @returns A boolean indicating success/failure
// */
//bool FindReplaceDialog::find(bool backwards)
//{
//  QString searchString = boxFind->currentText();
//  if (searchString.isEmpty()){
//    QMessageBox::warning(this, tr("Empty Search Field"),
//			 tr("The search field is empty. Please enter some text and try again."));
//    boxFind->setFocus();
//    return false;
//  }
//
//  if(boxFind->findText(searchString) == -1)
//  {
//    boxFind->addItem(searchString);
//  }
//
//  if( m_find_inprogress )
//  {
//    m_find_inprogress = m_manager->currentEditor()->findNext();
//  }
//  else
//  {
//    bool cs = boxCaseSensitive->isChecked();
//    bool whole = boxWholeWords->isChecked();
//    bool wrap = boxWrapAround->isChecked();
//    bool regex = boxRegex->isChecked();
//    m_find_inprogress = m_manager->currentEditor()->findFirst(searchString, regex, cs, whole, wrap, !backwards);
//  }
//  return m_find_inprogress;
//}
//
///**
// * Replace the next occurrence of the search term with the replacement text
// */
//void FindReplaceDialog::replace()
//{
//  QString searchString = boxFind->currentText();
//  if (searchString.isEmpty()){
//    QMessageBox::warning(this, tr("Empty Search Field"),
//			 tr("The search field is empty. Please enter some text and try again."));
//    boxFind->setFocus();
//    return;
//  }
//
//  if (!m_manager->currentEditor()->hasSelectedText() || m_manager->currentEditor()->selectedText() != searchString){
//    find();//find and select next match
//    return;
//  }
//
//  QString replaceString = boxReplace->currentText();
//  m_manager->currentEditor()->replace(replaceString);
//  find();//find and select next match
//
//  if(boxReplace->findText(replaceString) == -1)
//    boxReplace->addItem(replaceString);
//}
//
///**
// * Replace all occurrences of the current search term with the replacement text
// */
//void FindReplaceDialog::replaceAll()
//{
//  QString searchString = boxFind->currentText();
//  if (searchString.isEmpty()){
//    QMessageBox::warning(this, tr("Empty Search Field"),
//			 tr("The search field is empty. Please enter some text and try again."));
//    boxFind->setFocus();
//    return;
//  }
//
//  if(boxFind->findText(searchString) == -1)
//  {
//    boxFind->addItem (searchString);
//  }
//
//  QString replaceString = boxReplace->currentText();
//  if(boxReplace->findText(replaceString) == -1)
//  {
//    boxReplace->addItem(replaceString);
//  }
//
//  ScriptEditor *editor =  m_manager->currentEditor();
//  int line(-1), index(-1), prevLine(-1), prevIndex(-1);
//  bool regex = boxRegex->isChecked();
//  bool cs = boxCaseSensitive->isChecked();
//  bool whole = boxWholeWords->isChecked();
//  bool wrap = boxWrapAround->isChecked();
//  bool backward = boxSearchBackwards->isChecked();
//  // Mark this as a set of actions that can be undone as one
//  editor->beginUndoAction();
//  bool found = editor->findFirst(searchString, regex, cs, whole, wrap, !backward, 0, 0);
//  // If find first fails then there is nothing to replace
//  if( !found )
//  {
//    QMessageBox::information(this, "MantidPlot - Find and Replace", "No matches found in current document.");
//  }
//
//  while( found )
//  {
//    editor->replace(replaceString);
//    editor->getCursorPosition(&prevLine, &prevIndex);
//    found = editor->findNext();
//    editor->getCursorPosition(&line, &index);
//    if( line < prevLine || ( line == prevLine && index <= prevIndex ) )
//    {
//      break;
//    }
//  }
//  editor->endUndoAction();
//}
//
///**
// * Find button clicked slot
// */
//void FindReplaceDialog::findClicked()
//{
//  // Forward to worker function
//  find(boxSearchBackwards->isChecked());
//}
//
///**
// * Flip the in-progress flag
// */
//void FindReplaceDialog::resetSearchFlag()
//{
//  if( ScriptEditor *editor = m_manager->currentEditor() )
//  {
//    m_find_inprogress = false;
//    editor->setSelection(-1, -1, -1, -1);
//  }
//}
