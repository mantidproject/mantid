#ifndef SCRIPTFILEINTERPRETER_H_
#define SCRIPTFILEINTERPRETER_H_

#include "Script.h"

#include <QPoint>
#include <QSplitter>
#include <QStatusBar>
#include <QTextEdit>

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class QMessageBox;
class ScriptCloseDialog;
class ScriptingEnv;
class ScriptEditor;
class ScriptOutputDisplay;

//-----------------------------------------------------------------------------
// ScriptFileInterpreter
//-----------------------------------------------------------------------------
/**
 * Defines a widget that uses a ScriptEditor, a Script object and a
 * text display widget to give a single widget that can
 * edit, execute and display script code
 *
 */
class ScriptFileInterpreter : public QWidget {
  Q_OBJECT

public:
  /// Construct the object
  ScriptFileInterpreter(QWidget *parent = NULL,
                        const QString &settingsGroup = "");
  /// Destroy the object
  ~ScriptFileInterpreter();
  /// Determine if the script is ready to be closed
  virtual bool shouldClose();
  /// Setup from a script envment
  virtual void setup(const ScriptingEnv &env, const QString &identifier);

  /// Return the filename of the script in the editor
  virtual QString filename() const;
  ///
  inline ScriptEditor *editor() const { return m_editor; }
  ///
  inline ScriptOutputDisplay *messages() const { return m_messages; }
  /// Has the script text been modified
  virtual bool isScriptModified() const;
  /// Is the script running
  virtual bool isExecuting() const;

public slots:
  /// Save to the currently stored name
  virtual void saveToCurrentFile();
  /// Save to a different name
  virtual void saveAs();
  /// Save to the given filename
  virtual void saveScript(const QString &filename);
  /// Save the current output
  virtual void saveOutput(const QString &filename);
  /// Print the script
  virtual void printScript();
  /// Print the script
  virtual void printOutput();

  /// Undo
  virtual void undo();
  /// Redo
  virtual void redo();
  /// Copy from the editor
  virtual void copy();
  /// Cut from the editor
  virtual void cut();
  /// Paste into the editor
  virtual void paste();
  /// Find/replace in editor
  virtual void showFindReplaceDialog();
  /// Comment block of code
  virtual void comment();
  /// Uncomment block of code
  virtual void uncomment();
  /// Convert tabs in selection to spaces
  virtual void tabsToSpaces();
  /// Convert spaces in selection to tabs
  virtual void spacesToTabs();

  /// Execute the whole script.
  virtual void
  executeAll(const Script::ExecutionMode mode = Script::Asynchronous);
  /// Execute the current selection
  virtual void
  executeSelection(const Script::ExecutionMode mode = Script::Asynchronous);
  /// Request that the script execution be aborted
  virtual void abort();
  /// Clear the script variable cache
  virtual void clearVariables();

  /// Toggles the progress reports on/off
  virtual void toggleProgressReporting(bool state);
  /// Toggles the code folding on/off
  virtual void toggleCodeFolding(bool state);
  /// Toggles soft wrapping of text on/off;
  virtual void toggleLineWrapping(bool state);
  /// Toggles the whitespace visibility
  virtual void toggleWhitespace(bool state);
  /// Toggle replacing tabs with whitespace
  virtual void toggleReplaceTabs(bool state);
  /// Number of spaces to insert for a tab
  virtual void setTabWhitespaceCount(int count);
  /// Set a font
  virtual void setFont(const QString &fontFamily);

signals:
  /// Emits a signal when any text in the editor changes
  void textChanged();
  /// Emits a signal whenever the modification state of the editor changes
  void editorModificationChanged(bool);
  /// Emitted when the undo availability changes
  void editorUndoAvailable(bool);
  /// Emitted when the redo availability changes
  void editorRedoAvailable(bool);
  /// Emitted when a script starts executing
  void executionStarted();
  /// Emitted when a script stops executing
  void executionStopped();
  /// Emitted when a zoom in has occurred
  void textZoomedIn();
  /// Emitted when a zoom out has occurred
  void textZoomedOut();

private slots:
  /// Popup a context menu
  void showContextMenu(const QPoint &clickPoint);
  /// Update the status bar while the script is executing
  void setExecutingStatus();
  /// Update the status bar when the script has stopped
  void setStoppedStatus();
  // capture zoom in signals from either widget an emit our own
  void emitZoomIn();
  // capture zoom out signals from either widget an emit our own
  void emitZoomOut();

private:
  friend class ScriptCloseDialog;

  Q_DISABLE_COPY(ScriptFileInterpreter)
  void setupChildWidgets();

  void setupEditor(const ScriptingEnv &env, const QString &identifier);
  void setupScriptRunner(const ScriptingEnv &env, const QString &identifier);

  bool readFileIntoEditor(const QString &filename);
  void executeCode(const ScriptCode &code, const Script::ExecutionMode mode);

  void toggleComment(bool addComment);
  // Replaces the currently selected text in the editor
  inline void replaceSelectedText(const ScriptEditor *editor,
                                  const QString &text);

  QSplitter *m_splitter;
  ScriptEditor *m_editor;
  ScriptOutputDisplay *m_messages;
  QStatusBar *m_status;
  QSharedPointer<Script> m_runner;
};

/**
 * A specialised NullScriptFileInterpreter class that
 * implements the Null object pattern to return a object of
 * this type that does nothing
 */
class NullScriptFileInterpreter : public ScriptFileInterpreter {
  Q_OBJECT

public:
  /// Constructor
  NullScriptFileInterpreter() : ScriptFileInterpreter(NULL) {}

  /// Does nothing
  bool shouldClose() { return false; }
  /// Does nothing
  void setup(const ScriptingEnv &, const QString &) {}

  /// Does nothing
  QString filename() const { return QString(); }
  /// Does nothing
  bool isScriptModified() const { return false; }

private slots:
  /// Does nothing
  void undo() {}
  /// Does nothing
  void redo() {}
  /// Does nothing
  void copy() {}
  /// Does nothing
  void cut() {}
  /// Does nothing
  void paste() {}
  /// Does nothing
  void showFindReplaceDialog() {}

  /// Does nothing
  virtual void executeAll(const Script::ExecutionMode) {}
  /// Does nothing
  virtual void executeSelection(const Script::ExecutionMode) {}
  /// Does nothing
  virtual void abort() {}
  /// Does nothing
  virtual void clearVariables() {}

  /// Does nothing
  virtual void zoomInOnScript() {}
  /// Does nothing
  virtual void zoomOutOnScript() {}
  /// Does nothing
  virtual void toggleProgressReporting(bool) {}
  /// Does nothing
  virtual void toggleCodeFolding(bool) {}

  /// Does nothing
  virtual void saveToCurrentFile() {}
  /// Does nothing
  virtual void saveAs() {}
  /// Does nothing
  virtual void saveScript(const QString &) {}
  /// Does nothing
  virtual void saveOutput(const QString &) {}
  /// Does nothing
  virtual void printScript() {}
  /// Does nothing
  virtual void printOutput() {}
};

//-----------------------------------------------------------------------------
// ScriptCloseDialog - Specific message for closing the widget
//-----------------------------------------------------------------------------
class ScriptCloseDialog : public QWidget {
  Q_OBJECT

public:
  ScriptCloseDialog(ScriptFileInterpreter &interpreter, QWidget *parent = NULL);

  bool shouldScriptClose();

private:
  QMessageBox *m_msgBox;
  ScriptFileInterpreter &m_interpreter;
};

#endif /* SCRIPTFILEINTERPRETER_H_ */
