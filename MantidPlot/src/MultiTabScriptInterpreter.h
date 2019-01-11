// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SCRIPTMANAGERWIDGET_H_
#define SCRIPTMANAGERWIDGET_H_

//---------------------------------------------------------
// Includes
//---------------------------------------------------------
#include "Script.h"
#include "Scripted.h"
#include <QDialog>
#include <QTabWidget>

//---------------------------------------------------------
// Forward declarations
//--------------------------------------------------------
class QTabWidget;
class QPoint;
class QAction;
class QPushButton;
class QCheckBox;
class QComboBox;
class ScriptFileInterpreter;
class NullScriptFileInterpreter;

class QMenu;
class QStringList;

class ScriptingWindow;

/**
    This class manages ScriptEditor objects and displays them in a series
    of tabs. It is also the single point of entry for executing scripts
    with in the current ScriptingEnv

    @author Martyn Gigg, Tessella Support Services plc
    @date 19/08/2009
*/
class MultiTabScriptInterpreter : public QTabWidget, Scripted {
  Q_OBJECT

public:
  /// Constructor
  MultiTabScriptInterpreter(ScriptingEnv *env, QWidget *parent);
  /// Destructor
  ~MultiTabScriptInterpreter() override;

  /// Current interpreter
  ScriptFileInterpreter *currentInterpreter() { return m_current; };
  /// Interpreter at given index
  ScriptFileInterpreter *interpreterAt(int index);

  /// Is a script running in the environment
  bool isExecuting();

  /// Returns the global zoom level
  int globalZoomLevel() const { return m_globalZoomLevel; }

  /// this method appends the file names of scripts
  /// in different tabs to a string and returns
  QString saveToString();
  /// Saves Filenames associated with each tab to a QStringList
  QStringList fileNamesToQStringList();
  /// this method returns a list containing  recent scripts
  QStringList recentScripts();
  /// update the Recent Scripts menu items
  void updateRecentScriptList(const QString &filename);
  /// set the recent script list
  void setRecentScripts(const QStringList &scriptList);

signals:
  /// Signal that a tab has been created
  void newTabCreated(int);
  /// Signal that a tab is about to close, parametrised by the index
  void tabClosing(int);
  /// Signal that a tab has closed, parametrised by the index
  void tabClosed(int);
  /// Signal that the last tab has closed
  void lastTabClosed();
  /// Signal that the tab count has changed, giving the new count
  void tabCountChanged(int);
  /// Undo availability for current editor
  void undoAvailable(bool);
  /// Redo availability for current editor
  void redoAvailable(bool);
  /// Execution state changed
  void executionStateChanged(bool state);

public slots:
  /// Create a new tab for script editing with the text within the file imported
  /// and insert it at the index
  void newTab(int index = -1, const QString &filename = "");
  /// Open a file in the current tab
  void openInCurrentTab(const QString &filename = QString());
  /// Open a file in a new tab
  void openInNewTab(const QString &filename = QString());
  /// open recent scripts
  void openRecentScript(int index);
  /// Save current file
  void saveToCurrentFile();
  /// Save to new file
  void saveAs();
  /// Print the current script
  void print();
  /// Close current tab
  int closeCurrentTab();
  /// Close all tabs
  void closeAllTabs();
  /// Show the find dialog
  void showFindReplaceDialog();
  /// Comment a block of code
  void comment();
  /// Uncomment a block of code
  void uncomment();
  /// Convert tabs in selection to spaces
  void tabsToSpaces();
  /// Convert spaces in selection to tabs
  void spacesToTabs();
  /// undo
  void undo();
  /// redo
  void redo();
  /// cut current
  void cut();
  /// copy implementation
  void copy();
  /// paste implementation
  void paste();

  /** @name Execute members.*/
  //@{
  /// Execute all using the given mode
  bool executeAll(const Script::ExecutionMode mode);
  /// Execute selection using the given mode
  void executeSelection(const Script::ExecutionMode mode);
  /// Abort the current script
  void abortCurrentScript();
  /// Evaluate
  void evaluate();
  /// Clear out any previous variable definitions in the current script
  void clearScriptVariables();

  /// Tracks the global zoom level
  void trackZoomIn();
  /// Tracks the global zoom level
  void trackZoomOut();

  /// Increase font size
  void zoomIn();
  /// Increase font size on all tabs except that given
  void zoomInAllButCurrent();
  /// Decrease font size
  void zoomOut();
  /// Decrease font size on all tabs except that given
  void zoomOutAllButCurrent();
  /// Resets the zoom level
  void resetZoom();

  /// Toggle the progress reporting arrow
  void toggleProgressReporting(bool on);
  /// Toggle code folding
  void toggleCodeFolding(bool on);
  /// Toggle line wrapping
  void toggleLineWrapping(bool on);
  /// Toggle the whitespace reporting arrow
  void toggleWhitespace(bool state);
  /// Show configuration dialogue for tab whitespace
  void openConfigTabs();
  /// Toggle replacing tabs with whitespace
  void toggleReplaceTabs(bool state);
  /// Change whitespace count
  void changeWhitespaceCount(int value);
  /// Show select font dialog
  void showSelectFont();

private slots:
  /// Close a tab with a given index
  void closeTabAtIndex(int index);
  /// Close clicked tab
  void closeClickedTab();
  /// Current editor's modification status has changed
  void currentEditorModified(bool state);
  /// Current tab has changed
  void tabSelectionChanged(int index);
  /// Receive events regarding script started
  void sendScriptExecutingSignal();
  /// Receive events regarding script stopped state
  void sendScriptStoppedSignal();

private:
  /// A context menu event for the tab widget itself
  void contextMenuEvent(QContextMenuEvent *event) override;
  /// A custom defined event handler
  void customEvent(QEvent *event) override;
  /// Open a script
  void open(bool newtab, const QString &filename = QString());
  /// Sets the tab title & tooltip from the filename
  void setTabTitle(QWidget *widget, const QString &filename);
  /// Returns the tab title for the given filename
  QString createTabTitle(const QString &filename) const;
  /// Close a tab at a given position
  void closeTabAtPosition(const QPoint &pos);

private:
  friend class ScriptingWindow;

  /// The last directory visited with a file dialog
  QString m_last_dir;
  // The cursor position within the tab bar when the right-mouse button was last
  // clicked
  // I need this to ensure that the position of a call to tabBar()->tabAt() is
  // accurate
  // as Qt doesn't provide an action signal parameterised on a position
  QPoint m_cursor_pos;
  /// Current progress report state
  bool m_reportProgress;
  /// enum used for maximum of recent scripts size
  enum { MaxRecentScripts = 5 };
  /// List of recent scripts, with most recent at the top
  QStringList m_recentScriptList;
  /// A pointer to the Null object
  NullScriptFileInterpreter *m_nullScript;
  /// A pointer to the current interpreter
  ScriptFileInterpreter *m_current;
  /// Store the current global zoom level
  int m_globalZoomLevel;
  // Current whitespace visibility state
  bool m_showWhitespace;
  // Are tabs being inserted as whitespace
  bool m_replaceTabs;
  // Number of spaces to use for a tab
  int m_tabWhitespaceCount;
  // Font to use for script window
  QString m_fontFamily;
  // Save the code folding preference
  bool m_codeFolding;
  // Save the line wrapping preference
  bool m_LineWrapping;
};

#endif
