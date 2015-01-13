#ifndef SCRIPTINGWINDOW_H_
#define SCRIPTINGWINDOW_H_

//----------------------------------
// Includes
//----------------------------------
#include <QMainWindow>
#include "Script.h"

//----------------------------------------------------------
// Forward declarations
//---------------------------------------------------------
class MultiTabScriptInterpreter;
class ScriptingEnv;
class QTextEdit;
class QPoint;
class QMenu;
class QAction;
class QActionGroup;
class QCloseEvent;
class QShowEvent;
class QHideEvent;


/** @class ScriptingWindow    
    This class displays a seperate window for editing and executing scripts
*/
class ScriptingWindow : public QMainWindow
{
  ///Qt macro
  Q_OBJECT

public:
  ///Constructor
  ScriptingWindow(ScriptingEnv *env,bool capturePrint = true,QWidget *parent = 0, Qt::WindowFlags flags = 0);
  ///Destructor
  ~ScriptingWindow();
  /// Override the closeEvent
  void closeEvent(QCloseEvent *event);
  /// Override the showEvent
  void showEvent(QShowEvent *event);
  /// Is a script running?
  bool isExecuting() const;
  ///Save the current state of the script window for next time
  void saveSettings();
  /// Read settings from store
  void readSettings();
  /// Open a script in a new tab. Primarily useful for automatically
  /// opening a script
  void open(const QString & filename, bool newtab = true);
  /// Executes whatever is in the current tab. Primarily useful for automatically
  /// running a script loaded with open
  void executeCurrentTab(const Script::ExecutionMode mode);
  ///saves scripts file names to a string 
  QString saveToString();
  ///Set whether to accept/reject close events
  void acceptCloseEvent(const bool value);

signals:
  /// Show the scripting language dialog
  void chooseScriptingLanguage();
  /// Tell others we are closing
  void closeMe();
  /// Tell others we are hiding
  void hideMe();

protected:
  void dropEvent(QDropEvent *de);
  void dragMoveEvent(QDragMoveEvent *de);
  void dragEnterEvent(QDragEnterEvent *de);

private slots:
  /// Populate file menu
  void populateFileMenu();
  /// Ensure the list is up to date
  void populateRecentScriptsMenu();
  /// Populate edit menu
  void populateEditMenu();
  /// Populate execute menu
  void populateExecMenu();
  /// Populate window menu
  void populateWindowMenu();

  /// Update window flags
  void updateWindowFlags();
  /// Update menus based on current tab counts
  void setMenuStates(int nTabs);
  /// Sets the execution actions based on the flag
  void setEditActionsDisabled(bool state);
  /// Sets the execution actions based on the flag
  void setExecutionActionsDisabled(bool state);

  /// Finds the script corresponding to the action and
  /// asks the manager to open it
  void openRecentScript(QAction*);

  /// Execute all using the current mode option
  void executeAll();
  ///Execute selection using the current mode option
  void executeSelection();
  /// Clear out any previous variable definitions in the current script
  void clearScriptVariables();

private:
  /// Create menu bar
  void initMenus();

  /// Create all actions
  void initActions();
  /// Create the file actions
  void initFileMenuActions();
  /// Create the edit menu actions
  void initEditMenuActions();
  /// Create the execute menu actions
  void initExecMenuActions();
  /// Create the window menu actions
  void initWindowMenuActions();

  /// Returns the current execution mode
  Script::ExecutionMode getExecutionMode() const;

  /// Accept a custom defined event
  void customEvent(QEvent * event);

  /// Extract py files from urllist
  QStringList extractPyFiles(const QList<QUrl>& urlList) const;

private:
  /// The script editors' manager
  MultiTabScriptInterpreter *m_manager;

  /// File menu
  QMenu *m_fileMenu;
  /// File menu actions
  QAction *m_newTab, *m_openInCurTab, *m_openInNewTab, 
    *m_save, *m_saveAs, *m_print, *m_closeTab;
  QMenu *m_recentScripts;
  /// Edit menu
  QMenu *m_editMenu;
  /// Edit menu actions
  QAction *m_undo, *m_redo, *m_cut, *m_copy, *m_paste, *m_comment, *m_uncomment, 
    *m_tabsToSpaces, *m_spacesToTabs, *m_find;
  /// Run menu
  QMenu *m_runMenu;
  /// Execute menu actions
  QAction *m_execSelect, *m_execAll, *m_clearScriptVars;
  /// Execution mode menu
  QMenu *m_execModeMenu;
  /// Execute mode actions
  QAction *m_execParallel, *m_execSerial;
  /// Action group for execution mode
  QActionGroup *m_execModeGroup;
  /// Window menu
  QMenu *m_windowMenu;
  /// Window actions
  QAction *m_alwaysOnTop, *m_hide, *m_zoomIn, *m_zoomOut, *m_resetZoom,
    *m_toggleProgress, *m_toggleFolding, *m_toggleWhitespace, 
    *m_openConfigTabs, *m_selectFont;
  /// Change scripting language
  QAction *m_scripting_lang;
  /// Flag to define whether we should accept a close event
  bool m_acceptClose;

};

#endif //SCRIPTINGWINDOW_H_
