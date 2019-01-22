// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SCRIPTINGWINDOW_H_
#define SCRIPTINGWINDOW_H_

//----------------------------------
// Includes
//----------------------------------
#include "MantidQtWidgets/Common/IProjectSerialisable.h"
#include "Script.h"

#include <QMainWindow>
#include <boost/optional.hpp>

//----------------------------------------------------------
// Forward declarations
//---------------------------------------------------------
class ScriptFileInterpreter;
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
    This class displays a separate window for editing and executing scripts
*/
class ScriptingWindow : public QMainWindow {
  /// Qt macro
  Q_OBJECT

public:
  /// Constructor
  ScriptingWindow(ScriptingEnv *env, bool capturePrint = true,
                  QWidget *parent = nullptr, Qt::WindowFlags flags = nullptr);
  /// Destructor
  ~ScriptingWindow() override;
  /// Override the closeEvent
  void closeEvent(QCloseEvent *event) override;
  /// Override the showEvent
  void showEvent(QShowEvent *event) override;
  /// Is a script running?
  bool isExecuting() const;
  /// Save the current state of the script window for next time
  void saveSettings();
  /// Read settings from store
  void readSettings();
  /// Open a script in a new tab. Primarily useful for automatically
  /// opening a script
  void open(const QString &filename, bool newtab = true);
  /// Executes whatever is in the current tab. Primarily useful for
  /// automatically
  /// running a script loaded with open
  void executeCurrentTab(const Script::ExecutionMode mode);
  /// Set whether to accept/reject close events
  void acceptCloseEvent(const bool value);
  /// Opens a script providing a copy is not already open
  void openUnique(QString filename);
  /// Saves the open script names to the current project
  std::string saveToProject(ApplicationWindow *app);
  /// Loads the open script names for the current project
  void loadFromProject(const std::string &lines, ApplicationWindow *app,
                       const int fileVersion);
  // Loads the scripts from a list of filenames
  void loadFromFileList(const QStringList &files);

  /// Sets a flag which is set to true if synchronous execution fails
  // We set a flag on failure to avoid problems with Async not returning success
  bool getSynchronousErrorFlag() { return m_failureFlag; }

  /// Get a reference to the runner of the current script on the current tab
  const Script &getCurrentScriptRunner();

signals:
  /// Show the scripting language dialog
  void chooseScriptingLanguage();
  /// Tell others we are closing
  void closeMe();
  /// Tell others we are hiding
  void hideMe();

protected:
  /// Accept a custom defined event
  void customEvent(QEvent *event) override;
  void dragEnterEvent(QDragEnterEvent *de) override;
  void dragMoveEvent(QDragMoveEvent *de) override;
  void dropEvent(QDropEvent *de) override;

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
  /// Populate help menu
  void populateHelpMenu();

  /// Update window flags
  void updateWindowFlags();
  /// Update menus based on current tab counts
  void setMenuStates(int nTabs);
  /// Sets the execution actions based on the flag
  void setEditActionsDisabled(bool off);
  /// Sets the execution actions based on the flag
  void setExecutionActionsDisabled(bool off);
  /// Sets the abort actions based on the flag or the environment
  void setAbortActionsDisabled(bool off);

  /// Finds the script corresponding to the action and
  /// asks the manager to open it
  void openRecentScript(QAction *);

  /// Execute all using the current mode option
  void executeAll();
  /// Execute selection using the current mode option
  void executeSelection();
  /// Abort the current script
  void abortCurrent();
  /// Clear out any previous variable definitions in the current script
  void clearScriptVariables();

  /// Opens help page for scripting window
  void showHelp();
  /// Opens help page for Python API
  void showPythonHelp();

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
  /// Create the help menu actions
  void initHelpMenuActions();

  /// Should we enable abort functionality
  bool shouldEnableAbort() const;
  /// Opens tabs based on QStringList
  void openPreviousTabs(const QStringList &tabsToOpen);
  /// Returns the current execution mode
  Script::ExecutionMode getExecutionMode() const;

private:
  /// The script editors' manager
  MultiTabScriptInterpreter *m_manager;

  /// File menu
  QMenu *m_fileMenu;
  /// File menu actions
  QAction *m_newTab, *m_openInCurTab, *m_openInNewTab, *m_save, *m_saveAs,
      *m_print, *m_closeTab;
  QMenu *m_recentScripts;
  /// Edit menu
  QMenu *m_editMenu;
  /// Edit menu actions
  QAction *m_undo, *m_redo, *m_cut, *m_copy, *m_paste, *m_comment, *m_uncomment,
      *m_tabsToSpaces, *m_spacesToTabs, *m_find;
  /// Run menu
  QMenu *m_runMenu;
  /// Execute menu actions
  QAction *m_execSelect, *m_execAll, *m_abortCurrent, *m_clearScriptVars;
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
      *m_toggleProgress, *m_toggleFolding, *m_toggleWrapping,
      *m_toggleWhitespace, *m_openConfigTabs, *m_selectFont;
  /// Help menu
  QMenu *m_helpMenu;
  /// Help actions
  QAction *m_showHelp, *m_showPythonHelp;
  /// Change scripting language
  QAction *m_scripting_lang;
  /// Flag to define whether we should accept a close event
  bool m_acceptClose;

  bool m_failureFlag{false};
};

#endif // SCRIPTINGWINDOW_H_
