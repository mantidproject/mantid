#ifndef SCRIPTRUNNERWIDGET_H_
#define SCRIPTRUNNERWIDGET_H_

#include <QWidget>
#include <QTextEdit>
#include <QPoint>

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class Script;
class ScriptingEnv;
class ScriptEditor;
class ScriptOutputDisplay;

/**
 * Defines a widget that uses a ScriptEditor, a Script object and a
 * text display widget to give a single widget that can
 * edit, execute and display script code
 *
 */
class ScriptFileInterpreter : public QWidget
{
  Q_OBJECT

public:
  /// Construct the object
  ScriptFileInterpreter(QWidget *parent = NULL);
  /// Destroy the object
  ~ScriptFileInterpreter();
  /// Make sure we are in a safe state to delete the widget
  virtual void prepareToClose();
  /// Setup from a script environment
  virtual void setup(const ScriptingEnv & environ, const QString & identifier);

  /// Return the filename of the script in the editor
  virtual QString filename() const;
  /// Has the script text been modified
  virtual bool isScriptModified() const;

public slots:
  /// Save to the currently stored name
  virtual void saveToCurrentFile();
  /// Save to a different name
  virtual void saveAs();
  /// Save to the given filename
  virtual void saveScript(const QString & filename);
  /// Save the current output
  virtual void saveOutput(const QString & filename);
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
  /// Find in editor
  void findInScript();

  /// Execute the whole script.
  virtual void executeAll();
  /// Execute the current selection
  virtual void executeSelection();

  /// Zoom in on script
  virtual void zoomInOnScript();
  /// Zoom out on script
  virtual void zoomOutOnScript();

signals:
  /// Emits a signal when any text in the editor changes
  void textChanged();

private slots:
  // Popup a context menu
  void showContextMenu(const QPoint & clickPoint);

private:
  Q_DISABLE_COPY(ScriptFileInterpreter);
  void setupEditor(const ScriptingEnv & environ, const QString & identifier);
  void setupScriptRunner(const ScriptingEnv & environ, const QString & identifier);

  bool readFileIntoEditor(const QString & filename);
  void executeCode(const QString & code);


  ScriptEditor *m_editor;
  ScriptOutputDisplay *m_messages;
  QSharedPointer<Script> m_runner;
};

/**
 * A specialised NullScriptFileInterpreter class that
 * implements the Null object pattern to return a object of
 * this type that does nothing
 */
class NullScriptFileInterpreter : public ScriptFileInterpreter
{
  Q_OBJECT

public:
  /// Constructor
  NullScriptFileInterpreter() :
    ScriptFileInterpreter(NULL) {}

  /// Make sure we are in a safe state to delete the widget
  void prepareToClose() {};
  /// Setup from a script environment
  void setup(const ScriptingEnv &, const QString &) {};

  /// Return the filename of the script in the editor
  QString filename() const { return QString(); }
  /// Has the script text been modified
  bool isScriptModified() const { return false; }

private slots:
  /// Undo
  void undo() {}
  /// Redo
  void redo() {}
  /// Copy from the editor
  void copy() {}
  /// Cut from the editor
  void cut() {}
  /// Paste into the editor
  void paste() {}
  /// Find in editor
  void findInScript() {};

  /// Execute the whole script.
  virtual void executeAll() {}
  /// Execute the current selection
  virtual void executeSelection() {}

  /// Save to the currently stored name
  virtual void saveToCurrentFile() {}
  /// Save to a different name
  virtual void saveAs() {}
  /// Save to the given filename
  virtual void saveScript(const QString &) {};
  /// Save the current output
  virtual void saveOutput(const QString &) {};
  /// Print the script
  virtual void printScript() {}
  /// Print the script
  virtual void printOutput() {}

};

#endif /* SCRIPTRUNNERWIDGET_H_ */
