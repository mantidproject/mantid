// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
  ScriptFileInterpreter(QWidget *parent = nullptr,
                        const QString &settingsGroup = "");
  /// Destroy the object
  ~ScriptFileInterpreter() override;
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

  const Script &getRunner() const { return *m_runner.data(); }

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
  virtual bool
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
  bool executeCode(const ScriptCode &code, const Script::ExecutionMode mode);

  void toggleComment(bool addComment);

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
  NullScriptFileInterpreter() : ScriptFileInterpreter(nullptr) {}

  /// Does nothing
  bool shouldClose() override { return false; }
  /// Does nothing
  void setup(const ScriptingEnv &, const QString &) override {}

  /// Does nothing
  QString filename() const override { return QString(); }
  /// Does nothing
  bool isScriptModified() const override { return false; }

private slots:
  /// Does nothing
  void undo() override {}
  /// Does nothing
  void redo() override {}
  /// Does nothing
  void copy() override {}
  /// Does nothing
  void cut() override {}
  /// Does nothing
  void paste() override {}
  /// Does nothing
  void showFindReplaceDialog() override {}

  /// Does nothing
  bool executeAll(const Script::ExecutionMode) override { return true; }
  /// Does nothing
  void executeSelection(const Script::ExecutionMode) override {}
  /// Does nothing
  void abort() override {}
  /// Does nothing
  void clearVariables() override {}

  /// Does nothing
  virtual void zoomInOnScript() {}
  /// Does nothing
  virtual void zoomOutOnScript() {}
  /// Does nothing
  void toggleProgressReporting(bool) override {}
  /// Does nothing
  void toggleCodeFolding(bool) override {}

  /// Does nothing
  void saveToCurrentFile() override {}
  /// Does nothing
  void saveAs() override {}
  /// Does nothing
  void saveScript(const QString &) override {}
  /// Does nothing
  void saveOutput(const QString &) override {}
  /// Does nothing
  void printScript() override {}
  /// Does nothing
  void printOutput() override {}
};

//-----------------------------------------------------------------------------
// ScriptCloseDialog - Specific message for closing the widget
//-----------------------------------------------------------------------------
class ScriptCloseDialog : public QWidget {
  Q_OBJECT

public:
  ScriptCloseDialog(ScriptFileInterpreter &interpreter,
                    QWidget *parent = nullptr);

  bool shouldScriptClose();

private:
  QMessageBox *m_msgBox;
  ScriptFileInterpreter &m_interpreter;
};

#endif /* SCRIPTFILEINTERPRETER_H_ */
