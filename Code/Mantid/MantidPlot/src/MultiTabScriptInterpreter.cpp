//-----------------------------------------------------
// Includes
//-----------------------------------------------------
#include "ApplicationWindow.h"
#include "ScriptFileInterpreter.h"

#include "ScriptingEnv.h"
#include "ScriptingLangDialog.h"
#include "MultiTabScriptInterpreter.h"

#include "MantidQtMantidWidgets/ScriptEditor.h"

// Qt
#include <QPoint>
#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QContextMenuEvent>
#include <QTabBar>
#include <QFileInfo>
#include <QFileDialog>

// std
#include <stdexcept>

//***************************************************************************
//
// MultiTabScriptInterpreter class
//
//***************************************************************************
//-----------------------------------------------------
// Public member functions
//-----------------------------------------------------
/**
 * Constructor
 */
MultiTabScriptInterpreter::MultiTabScriptInterpreter(ScriptingEnv *env, QWidget *parent)
  : QTabWidget(parent), Scripted(env), m_last_dir(""),
    m_cursor_pos(), m_reportProgress(false), m_recentScriptList(), m_nullScript(new NullScriptFileInterpreter),
    m_current(m_nullScript), m_globalZoomLevel(0)
{
  connect(this, SIGNAL(currentChanged(int)), this, SLOT(tabSelectionChanged(int)));
}

/**
 * Destructor
 */
MultiTabScriptInterpreter::~MultiTabScriptInterpreter()
{
  delete m_nullScript;
}

/// @return Interpreter at given index
ScriptFileInterpreter * MultiTabScriptInterpreter::interpreterAt(int index)
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
bool MultiTabScriptInterpreter::isExecuting()
{
  for(int i = 0; i < count(); ++i)
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
void MultiTabScriptInterpreter::newTab(int index, const QString & filename)
{
  ScriptFileInterpreter *scriptRunner = new ScriptFileInterpreter(this,"ScriptWindow");
  scriptRunner->setup(*scriptingEnv(), filename);
  scriptRunner->toggleProgressReporting(m_reportProgress);
  connect(scriptRunner, SIGNAL(editorModificationChanged(bool)),
          this, SLOT(currentEditorModified(bool)));
  index = insertTab(index, scriptRunner, "");
  setCurrentIndex(index);
  setTabTitle(scriptRunner, filename); // Make sure the tooltip is set
  scriptRunner->setFocus();
  scriptRunner->editor()->zoomIn(globalZoomLevel());
  connect(scriptRunner->editor(), SIGNAL(textZoomedIn()), this, SLOT(zoomInAllButCurrent()));
  connect(scriptRunner->editor(), SIGNAL(textZoomedIn()), this, SLOT(trackZoomIn()));
  connect(scriptRunner->editor(), SIGNAL(textZoomedOut()), this, SLOT(zoomOutAllButCurrent()));
  connect(scriptRunner->editor(), SIGNAL(textZoomedOut()), this, SLOT(trackZoomOut()));

  emit newTabCreated(index);
  emit tabCountChanged(count());
}

/**
 * Open a file in the current tab
 * @param filename :: An optional file name
 */
void MultiTabScriptInterpreter::openInCurrentTab(const QString & filename)
{
  open(false, filename);
}

/**
 * Open a file in a new tab
 * @param filename :: An optional file name
 */
void MultiTabScriptInterpreter::openInNewTab(const QString & filename)
{
  open(true, filename);
}

/**
 * open the selected script from the File->Recent Scripts  in a new tab
 * @param index :: The index of the selected script
 */
void MultiTabScriptInterpreter::openRecentScript(int index)
{
  if(index < m_recentScriptList.count())
  {
    QString filename = m_recentScriptList[index];
    openInNewTab(filename);
  }
}

/// Save current file
void MultiTabScriptInterpreter::saveToCurrentFile()
{
  try
  {
    m_current->saveToCurrentFile();
    setTabTitle(m_current, m_current->filename());
  }
  catch(ScriptEditor::SaveCancelledException&) {}
  catch(std::runtime_error& exc)
  {
    QMessageBox::critical(this, tr("MantidPlot"), tr(exc.what()));
  }
}

/// Save to new file
void MultiTabScriptInterpreter::saveAs()
{
  try
  {
    m_current->saveAs();
    setTabTitle(m_current, m_current->filename());
  }
  catch(ScriptEditor::SaveCancelledException&) {}
  catch(std::runtime_error& exc)
  {
    QMessageBox::critical(this, tr("MantidPlot"), tr(exc.what()));
  }
}

/// Print the current script
void MultiTabScriptInterpreter::print()
{
  m_current->printScript();
}

/**
 * Close current tab
 */
int MultiTabScriptInterpreter::closeCurrentTab()
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
void MultiTabScriptInterpreter::closeAllTabs()
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
QString MultiTabScriptInterpreter::saveToString()
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
void MultiTabScriptInterpreter::showFindReplaceDialog()
{
  m_current->showFindReplaceDialog();
}

/// undo
void MultiTabScriptInterpreter::undo()
{
  m_current->undo();
}
/// redo
void MultiTabScriptInterpreter::redo()
{
  m_current->redo();
}

/// cut current
void MultiTabScriptInterpreter::cut()
{
  m_current->cut();
}

/// copy method
void MultiTabScriptInterpreter::copy()
{
  m_current->copy();
}
  ///paste method
void MultiTabScriptInterpreter::paste()
{
  m_current->paste();
}

/**
 * Execute the highlighted code from the current tab
 * *@param mode :: The mode used to execute
 */
void MultiTabScriptInterpreter::executeAll(const Script::ExecutionMode mode)
{
  m_current->executeAll(mode);
}

/* Execute the highlighted code from the current tab using the
 * given execution mode
 * @param mode :: The mode used to execute
 */
void MultiTabScriptInterpreter::executeSelection(const Script::ExecutionMode mode)
{
  m_current->executeSelection(mode);
}

/**
 * Evaluate
 */
void MultiTabScriptInterpreter::evaluate()
{
  QMessageBox::information(this, "MantidPlot", "Evaluate is not implemented.");
}

/// Tracks the global zoom level
void MultiTabScriptInterpreter::trackZoomIn()
{
  ++m_globalZoomLevel;
}

/// Tracks the global zoom level
void MultiTabScriptInterpreter::trackZoomOut()
{
  --m_globalZoomLevel;
}

/// Increase font size on all tabs
void MultiTabScriptInterpreter::zoomIn()
{
  for(int index = 0; index < count(); ++index)
  {
    interpreterAt(index)->editor()->zoomIn();
  }
}

/**
 * @param skipIndex The tab that should be skipped
 */
void MultiTabScriptInterpreter::zoomInAllButCurrent()
{
  int skipIndex = this->currentIndex();
  for(int i = 0; i < count(); ++i)
  {
    if(i != skipIndex) interpreterAt(i)->editor()->zoomIn();
  }
}

/// Decrease font size on all tabs
void MultiTabScriptInterpreter::zoomOut()
{
  for(int i = 0; i < count(); ++i)
  {
    interpreterAt(i)->editor()->zoomOut();
  }
}

/**
 * @param skipIndex The tab that should be skipped
 */
void MultiTabScriptInterpreter::zoomOutAllButCurrent()
{
  int skipIndex = this->currentIndex();
  for(int i = 0; i < count(); ++i)
  {
    if(i != skipIndex) interpreterAt(i)->editor()->zoomOut();
  }
}


/**
 * Toggle the progress arrow on/off
 * @param state :: The state of the option
 */
void MultiTabScriptInterpreter::toggleProgressReporting(bool state)
{
  m_reportProgress = state;
  int index_end = count() - 1;
  for( int index = index_end; index >= 0; --index )
  {
    interpreterAt(index)->toggleProgressReporting(state);
  }
}

/**
 * Toggle code folding on/off
 * @param state :: The state of the option
 */
void MultiTabScriptInterpreter::toggleCodeFolding(bool state)
{
  int index_end = count() - 1;
  for( int index = index_end; index >= 0; --index )
  {
    interpreterAt(index)->toggleCodeFolding(state);
  }
}

//--------------------------------------------
// Private slots
//--------------------------------------------

/**
 * Close clicked tab. Qt cannot give the position where an action is clicked so this just gets 
 * the current cursor position and calls the closeAtPosition function
 */
void MultiTabScriptInterpreter::closeClickedTab()
{
  closeTabAtPosition(m_cursor_pos);
}

/**
 * Mark the current tab as changed. True means that the editor has
 * modifications
 */
void MultiTabScriptInterpreter::currentEditorModified(bool state)
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
void MultiTabScriptInterpreter::tabSelectionChanged(int index)
{
  m_current->disconnect(SIGNAL(executionStarted()));
  m_current->disconnect(SIGNAL(executionStopped()));
  if( count() > 0 )
  {
    m_current = interpreterAt(index);
    connect(m_current, SIGNAL(executionStarted()), this, SLOT(sendScriptExecutingSignal()));
    connect(m_current, SIGNAL(executionStopped()), this, SLOT(sendScriptStoppedSignal()));
    emit executionStateChanged(m_current->isExecuting());
    setFocusProxy(m_current);
    m_current->setFocus();
  }
  else
  {
    m_current = m_nullScript;
  }
}

/**
 * Emits the executionStateChanged(true) signal
 */
void MultiTabScriptInterpreter::sendScriptExecutingSignal()
{
  emit executionStateChanged(true);
}

/**
 * Emits the executionStateChanged(false) signal
 */
void MultiTabScriptInterpreter::sendScriptStoppedSignal()
{
  emit executionStateChanged(false);
}

//--------------------------------------------
// Private member functions (non-slot)
//--------------------------------------------

/**
 * A context menu event for the tab widget itself
 * @param event :: The context menu event
 */  
void MultiTabScriptInterpreter::contextMenuEvent(QContextMenuEvent *event)
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
void MultiTabScriptInterpreter::customEvent(QEvent *event)
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
void MultiTabScriptInterpreter::open(bool newtab, const QString & filename)
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
void MultiTabScriptInterpreter::setTabTitle(QWidget *widget, const QString & filename)
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
QString MultiTabScriptInterpreter::createTabTitle(const QString & filename) const
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
void MultiTabScriptInterpreter::closeTabAtIndex(int index)
{
  ScriptFileInterpreter *interpreter = interpreterAt(index);
  interpreter->prepareToClose();
  emit tabClosing(index);
  removeTab(index);
  emit tabClosed(index);
  const int nTabs = count();
  emit tabCountChanged(nTabs);
  if(nTabs == 0) emit lastTabClosed();
}


/**
 * Close a tab at a given position
 * @param pos :: The tab at the given position
 */ 
void MultiTabScriptInterpreter::closeTabAtPosition(const QPoint & pos)
{
  int index = tabBar()->tabAt(pos);
  //Index is checked in closeTab
  closeTabAtIndex(index);
}

/** 
 * Keeps the recent script list up to date
 */
void MultiTabScriptInterpreter::updateRecentScriptList(const QString & filename)
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
QStringList MultiTabScriptInterpreter::recentScripts() 
{
  return m_recentScriptList;
}

/** 
 * sets the recent scripts list
 * @param rslist :: list containing the name of the recent scripts.
 */
void MultiTabScriptInterpreter::setRecentScripts(const QStringList& rslist)
{
  m_recentScriptList = rslist;
}
