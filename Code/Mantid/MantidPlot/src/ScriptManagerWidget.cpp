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
#include <QMessageBox>
#include <QContextMenuEvent>
#include <QTabBar>
#include <QFileInfo>
#include <QFileDialog>

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
    m_cursor_pos(), m_recentScriptList(), m_nullScript(new NullScriptFileInterpreter),
    m_current(m_nullScript)
{
  // Start with a blank tab
  connect(this, SIGNAL(currentChanged(int)), this, SLOT(tabSelectionChanged(int)));
  newTab();
}

/**
 * Destructor
 */
ScriptManagerWidget::~ScriptManagerWidget()
{
  delete m_nullScript;
}

/// @return Interpreter at given index
ScriptFileInterpreter * ScriptManagerWidget::interpreterAt(int index)
{
  if(count() > 0)
  {
    return qobject_cast<ScriptFileInterpreter*>(widget(index));
  }
  else
  {
    return m_nullScript;
  }
}

/**
 * Is a script running in the environment
 */
bool ScriptManagerWidget::isExecuting()
{
  for(int i = 0; i <= count(); ++i)
  {
    if(interpreterAt(i)->isExecuting()) return true;
  }
  return false;
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
  ScriptFileInterpreter *scriptRunner = new ScriptFileInterpreter(this);
  scriptRunner->setup(*scriptingEnv(), filename);
  connect(scriptRunner, SIGNAL(editorModificationChanged(bool)),
          this, SLOT(currentEditorModified(bool)));
  index = insertTab(index, scriptRunner, "");
  setCurrentIndex(index);
  setTabTitle(scriptRunner, filename); // Make sure the tooltip is set
  scriptRunner->setFocus();
  emit undoAvailable(false);
  emit redoAvailable(false);
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
 * open the selected script from the File->Recent Scripts  in a new tab
 * @param index :: The index of the selected script
 */
void ScriptManagerWidget::openRecentScript(int index)
{
  if(index < m_recentScriptList.count())
  {
    QString filename = m_recentScriptList[index];
    openInNewTab(filename);
  }
}

/// Save current file
void ScriptManagerWidget::saveToCurrentFile()
{
  m_current->saveToCurrentFile();
  setTabTitle(m_current, m_current->filename());
}

/// Save to new file
void ScriptManagerWidget::saveAs()
{
  m_current->saveAs();
  setTabTitle(m_current, m_current->filename());
}

/// Print the current script
void ScriptManagerWidget::print()
{
  m_current->printScript();
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
  m_current = m_nullScript;
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
 * Show the find/replace dialog
 */
void ScriptManagerWidget::showFindReplaceDialog()
{
  m_current->showFindReplaceDialog();
}

/// undo
void ScriptManagerWidget::undo()
{
  m_current->undo();
}
/// redo
void ScriptManagerWidget::redo()
{
  m_current->redo();
}

/// cut current
void ScriptManagerWidget::cut()
{
  m_current->cut();
}

/// copy method
void ScriptManagerWidget::copy()
{
  m_current->copy();
}
  ///paste method
void ScriptManagerWidget::paste()
{
  m_current->paste();
}

/**
 * Execute the highlighted code from the current tab
 */
void ScriptManagerWidget::executeAll(const Script::ExecutionMode mode)
{
  m_current->executeAll(mode);
}

/**
 * Execute the whole script
 */
void ScriptManagerWidget::executeSelection(const Script::ExecutionMode mode)
{
  m_current->executeSelection(mode);
}

/**
 * Evaluate
 */
void ScriptManagerWidget::evaluate()
{
  QMessageBox::information(this, "MantidPlot", "Evaluate is not implemented yet.");
}

/// Increase font size
void ScriptManagerWidget::zoomIn()
{
  m_current->zoomInOnScript();
}
/// Decrease font size
void ScriptManagerWidget::zoomOut()
{
  m_current->zoomOutOnScript();
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
// Private slots
//--------------------------------------------

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
void ScriptManagerWidget::currentEditorModified(bool state)
{
  static const QString modifiedLabel("*");
  const int index = currentIndex();
  QString tabLabel = tabText(index);
  if(state)
  {
    tabLabel += modifiedLabel;
  }
  else
  {
    tabLabel.chop(modifiedLabel.length());
  }
  setTabText(index, tabLabel);
}

/**
 * The current selection has changed
 * @param index The index of the new selection
 */
void ScriptManagerWidget::tabSelectionChanged(int index)
{
  if( count() > 0 )
  {
    disconnect(m_current, SIGNAL(editorUndoAvailable(bool)), this, SIGNAL(undoAvailable(bool)));
    disconnect(m_current, SIGNAL(editorRedoAvailable(bool)), this, SIGNAL(redoAvailable(bool)));
    m_current = qobject_cast<ScriptFileInterpreter*>(widget(index));
    connect(m_current, SIGNAL(editorUndoAvailable(bool)), this, SIGNAL(undoAvailable(bool)));
    connect(m_current, SIGNAL(editorRedoAvailable(bool)), this, SIGNAL(redoAvailable(bool)));
    setFocusProxy(m_current);
    m_current->setFocus();
  }
  else
  {
    m_current = m_nullScript;
    emit undoAvailable(false);
    emit redoAvailable(false);
  }

}

/**
 * Enable/disable script interaction based on script execution status
 * @param running :: The state of the script
 */
void ScriptManagerWidget::setScriptIsRunning(bool)
{
}


//--------------------------------------------
// Private member functions (non-slot)
//--------------------------------------------

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
      connect(close, SIGNAL(triggered()), this, SLOT(closeClickedTab()));
      context.addAction(close);
    }
    // Close all tabs
    QAction *closeall = new QAction(tr("&Close All Tabs"), this);
    connect(closeall, SIGNAL(triggered()), this, SLOT(closeAllTabs()));
    context.addAction(closeall);

    context.insertSeparator();
  }

  QAction *newtab = new QAction(tr("&New Tab"), this);
  connect(newtab, SIGNAL(triggered()), this, SLOT(newTab()));
  context.addAction(newtab);


  context.exec(QCursor::pos());
}

/**
 * A custom event handler, which in this case monitors for ScriptChangeEvent signals
 * @param event :: The custome event
 */
void ScriptManagerWidget::customEvent(QEvent *event)
{
  if( !isExecuting() && event->type() == SCRIPTING_CHANGE_EVENT )
  {
    ScriptingChangeEvent *sce = static_cast<ScriptingChangeEvent*>(event);
    // This handles reference counting of the scripting environment
    Scripted::scriptingChangeEvent(sce);
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
  else
  {
    QFileInfo details(fileToOpen);
    fileToOpen = details.absoluteFilePath();
  }

  //Save last directory
  m_last_dir = QFileInfo(fileToOpen).absolutePath();

  int index(-1);
  if( !newtab ) index = closeCurrentTab();
  newTab(index, fileToOpen);

  //update the recent scripts menu 
  updateRecentScriptList(fileToOpen);

}

/**
 * Sets the tab title & tooltip from the filename
 * @param widget A pointer to the widget on the tab
 * @param filename The filename on the tab
 */
void ScriptManagerWidget::setTabTitle(QWidget *widget, const QString & filename)
{
  setTabLabel(widget, createTabTitle(filename));
  setTabToolTip(widget, filename);
}

/**
 * Returns the tab title for the given filename. If the filename
 * is empty the string "New Script" is returned
 * @param filename The filename of the script.
 * @return A string to use as the tab title
 */
QString ScriptManagerWidget::createTabTitle(const QString & filename) const
{
  QString title;
  if( filename.isEmpty() )
  {
    title = "New script";
  }
  else
  {
    title = QFileInfo(filename).fileName();
  }
  return title;
}

/**
 * Close a given tab
 * @param index :: The tab index
 */ 
void ScriptManagerWidget::closeTabAtIndex(int index)
{
  ScriptFileInterpreter *interpreter = interpreterAt(index);
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
 * Keeps the recent script list up to date
 */
void ScriptManagerWidget::updateRecentScriptList(const QString & filename)
{
  m_recentScriptList.remove(filename);
  m_recentScriptList.push_front(filename);
  if( m_recentScriptList.count() > MaxRecentScripts )
  {
    m_recentScriptList.pop_back();
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
  m_recentScriptList = rslist;
}
