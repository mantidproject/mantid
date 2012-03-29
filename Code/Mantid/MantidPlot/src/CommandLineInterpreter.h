#ifndef COMMANDLINEINTERPRETER_H_
#define COMMANDLINEINTERPRETER_H_

#include "MantidQtMantidWidgets/ScriptEditor.h"
#include <QRegExp>

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class Script;
class ScriptingEnv;
class QKeyEvent;

/**
 * A class to handle multi-line input and test whether
 * it is complete and ready to be sent for execution
 */
class InputSplitter
{
public:
  InputSplitter(QSharedPointer<Script> compiler);
  /// Current indent level
  inline int currentIndent() const { return m_indentSpaces; }
  /// Push a line of code.
  bool push(const QString & line);
  /// Is the code complete
  bool pushCanAcceptMore() const;
  /// Return the current source formatted so that it can be executed
  QString getSource() const;
  /// Reset the state of the splitter to accept future input
  void reset();

private:
  InputSplitter();
  Q_DISABLE_COPY(InputSplitter);

  void store(const QString & line);
  void updateIndent(const QString & line);
  int numInitialSpaces(const QString & line);
  bool matchesDedent(const QString &) const;
  bool finalCharIsColon(const QString & str) const;

  /// Compiler
  QSharedPointer<Script> m_compiler;
  /// Indent level
  int m_indentSpaces;
  /// Whether the indent is now flush-left
  bool m_fullDedent;
  /// The full code as ready to be executed
  QString m_source;
  /// A list of input lines
  QStringList m_buffer;
  /// True if code forms complete statement
  bool m_complete;
};

/**
 * A Specialization of a ScriptEditor that combines it with a
 * Script object to define a command line environment with the script output
 * inline with the input.
 *
 */
class CommandLineInterpreter : public ScriptEditor
{
  Q_OBJECT

public:
  /// Construct
  CommandLineInterpreter(const ScriptingEnv & environ, QWidget *parent = NULL);

  /// Persist to store
  void saveSettings() const;

public slots:
  /// Paste needs to execute lines as it goes
  void paste();

private slots:
  /// Write output
  void displayOutput(const QString & messages);
  /// Write error
  void displayError(const QString & messages);
  /// Inserts a input prompt
  void insertInputPrompt();

private:
  Q_DISABLE_COPY(CommandLineInterpreter);

  /// Enumerate input mode
  enum InputMode { ReadWrite, ReadOnly };
  /// Status
  enum Status { Waiting, Executing };

  /// Setup with the scripting environment
  void setup(const ScriptingEnv & environ);
  /// Disable window editing keys
  void remapWindowEditingKeys();

  /// Returns the index of the last line
  /// @returns An index of the final line in the editor
  inline int lastLineIndex() const { return lines() - 1; }

  /// Intercept key presses
  void keyPressEvent(QKeyEvent* event);
  /// Attempt to handle the given key event
  bool handleKeyPress(const int key);
  /// Handles a history request
  void handleUpKeyPress();
  /// Handles a history request
  void handleDownKeyPress();
  /// Handle a return key press
  void handleReturnKeyPress();
    /// Try to execute the code in the current buffer.
  void tryExecute();
  /// Execute the given code
  void execute();
  /// Inserts a continuation prompt
  void insertContinuationPrompt();

  /// Capture mouse clicks to prevent moving the cursor to unwanted places
  void mousePressEvent(QMouseEvent *event);
  /// Set whether or not the current line(where the cursor is located) is editable
  void setEditingState(int line);

  ///this method checks the shortcut key for the copy command (Ctrl+C) pressed
  bool isCtrlCPressed(const int prevKey,const int curKey);
  /// this method checks the short cut key for the command cut(Ctrl+X) is pressed
  bool isCtrlXPressed(const int prevKey,const int curKey);


  QSharedPointer<Script> m_runner;
  CommandHistory m_history;
  InputMode m_inputmode;

  QSharedPointer<InputSplitter> m_inputBuffer;
  Status m_status;


  int m_promptKey;
  int m_continuationKey;
};

#endif /* COMMANDLINEINTERPRETER_H_ */
