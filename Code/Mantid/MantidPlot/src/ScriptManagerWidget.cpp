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
bool ScriptManagerWidget::isScriptRunning()
{
  return scriptingEnv()->isRunning();
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
}

/// Save to new file
void ScriptManagerWidget::saveAs()
{
  m_current->saveToCurrentFile();
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

void ScriptManagerWidget::findInScript()
{
  m_current->findInScript();
}

/**
 * Execute the highlighted code from the current tab
 */
void ScriptManagerWidget::executeAll()
{
  m_current->executeAll();
}

/**
 * Execute the whole script
 */
void ScriptManagerWidget::executeSelection()
{
  m_current->executeSelection();
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
void ScriptManagerWidget::markCurrentAsChanged()
{
  int index = currentIndex();
  setTabText(index, tabText(index) + "*");
  // Disconnect signal so that this doesn't get run in the future
  disconnect(m_current, SIGNAL(textChanged()), this, SLOT(markCurrentAsChanged()));
}

/**
 * The current selection has changed
 * @param index The index of the new selection
 */
void ScriptManagerWidget::tabSelectionChanged(int index)
{
  if( count() > 0 )
  {
    m_current = qobject_cast<ScriptFileInterpreter*>(widget(index));
    setFocusProxy(m_current);
    m_current->setFocus();
  }
  else
  {
    m_current = m_nullScript;
  }
}

/**
 * Enable/disable script interaction based on script execution status
 * @param running :: The state of the script
 */
void ScriptManagerWidget::setScriptIsRunning(bool running)
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

  //Save last directory
  m_last_dir = QFileInfo(fileToOpen).absolutePath();

  int index(-1);
  if( !newtab ) index = closeCurrentTab();
  newTab(index, fileToOpen);

  //update the recent scripts menu 
  updateRecentScriptList(fileToOpen);

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
