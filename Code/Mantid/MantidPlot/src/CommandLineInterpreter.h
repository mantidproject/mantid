#ifndef COMMANDLINEINTERPRETER_H_
#define COMMANDLINEINTERPRETER_H_

#include <QWidget>

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class Script;
class ScriptingEnv;
class ScriptEditor;

/**
 * Defines a widget that uses a ScriptEditor and a Script object
 * to define a command line environment with the script output
 * inline with the input
 *
 */
class CommandLineInterpreter : public QWidget
{
  Q_OBJECT

public:
  /// Construct
  CommandLineInterpreter(QWidget *parent = NULL);
  /// Setup with a scripting environment
  void setup(const ScriptingEnv & environ);

  /// Persist to store
  void saveSettings() const;

  //----------------------- Actions -------------------------------------------
  /// Copy to clipboard
  void copy();
  /// Paste from clipboard
  void paste();

private:
  Q_DISABLE_COPY(CommandLineInterpreter);
  ScriptEditor *m_editor;
  QSharedPointer<Script> m_runner;
};

#endif /* COMMANDLINEINTERPRETER_H_ */
