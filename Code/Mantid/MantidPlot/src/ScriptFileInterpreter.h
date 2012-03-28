#ifndef SCRIPTRUNNERWIDGET_H_
#define SCRIPTRUNNERWIDGET_H_

#include <QWidget>
#include <QTextEdit>

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class Script;
class ScriptingEnv;
class ScriptEditor;
class ScriptOutputDisplay;
class QAction;

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
  void prepareToClose();
  /// Setup from a script environment
  void setup(const ScriptingEnv & environ, const QString & identifier);

  /// Return the filename of the script in the editor
  QString filename() const;
  /// Has the script text been modified
  bool isScriptModified() const;

  /// Fill a edit menu
  void populateFileMenu(QMenu &fileMenu);
  /// Fill a edit menu
  void populateEditMenu(QMenu &editMenu);
  /// Fill exec menu
  void populateExecMenu(QMenu &execMenu);

public slots:
  /// Execute the whole script.
  void executeAll();
  /// Execute the current selection
  void executeSelection();

  /// Save the current script in the editor
  void saveScript(const QString & filename);
  /// Save the current output
  void saveOutput(const QString & filename);

signals:
  /// Emits a signal when any text in the editor changes
  void textChanged();

private:
  Q_DISABLE_COPY(ScriptFileInterpreter);
  void initActions();
  void setupEditor(const ScriptingEnv & environ, const QString & identifier);
  void setupScriptRunner(const ScriptingEnv & environ, const QString & identifier);

  bool readFileIntoEditor(const QString & filename);
  void executeCode(const QString & code);

  ScriptEditor *m_editor;
  ScriptOutputDisplay *m_messages;
  QSharedPointer<Script> m_runner;

  QAction *m_execAll;
  QAction *m_execSelect;
};

#endif /* SCRIPTRUNNERWIDGET_H_ */
