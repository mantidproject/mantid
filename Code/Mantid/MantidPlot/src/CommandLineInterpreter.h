#ifndef COMMANDLINEINTERPRETER_H_
#define COMMANDLINEINTERPRETER_H_

#include "MantidQtMantidWidgets/ScriptEditor.h"
#include <QRegExp>
#include <QTextStream>

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class Script;
class ScriptingEnv;
class FindDialog;

class QKeyEvent;
class QAction;

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
  /// Is any code executing
  inline bool isExecuting() const { return m_status == Executing; }

public slots:
  /// Shutdown the interpreter
  void shutdown();

  /// Paste needs to execute lines as it goes
  void paste();
  /// Cut can only edit text on the current input line
  void cut();
  /// Overrides base class to slot
  void saveAs();

private slots:
  /// Custom context menu
  void showContextMenu(const QPoint &);
  /// Write output
  void displayOutput(const QString & messages);
  /// Write error
  void displayError(const QString & messages);
  /// Inserts a input prompt
  void insertInputPrompt();
  /// Flag that code is executing
  void setStatusToExecuting();
  /// Flag that code is waiting
  void setStatusToWaiting();
  /// Process next line in paste queue
  void processNextPastedLine();

signals:
  /// Indicates that more input is required
  void moreInputRequired();

private:
  Q_DISABLE_COPY(CommandLineInterpreter);

  /// Status
  enum Status { Waiting, Executing };

  /// Setup with the scripting environment
  void setupEnvironment(const ScriptingEnv & environ);
  /// Setup the margin
  void setupMargin();
  /// Set the indentation policy
  void setupIndentation();
  /// Set the fonts used
  void setupFont();
  /// Create required actions
  void initActions();
  /// Disable window editing keys
  void remapWindowEditingKeys();
  /// Show find dialog
  void showFindDialog();
  /// Do nothing for find replace
  void showFindReplaceDialog() {}

  /// Returns the index of line the cursor is currently on
  int indexOfCursorLine() const;
  /// Returns the index of the last line
  /// @returns An index of the final line in the editor
  inline int indexOfLastLine() const { return lines() - 1; }
  /// Set the cursor position to the start of the current input line
  void moveCursorToStartOfLastLine();
  /// Set the cursor position to after the last character in the editor
  void moveCursorToEnd();
  /// Does the text contain newlines
  bool containsNewlines(const QString & text) const;

  /// Paste and execute multi-line code
  void processPastedCodeWithNewlines(const int offset);
  /// Sets text and generates a return key press
  void simulateUserInput(QString & text, const int offset = 0);

  /// Intercept key presses
  void keyPressEvent(QKeyEvent* keyPress);
  /// Attempt to handle the given key event
  bool handleKeyPress(QKeyEvent* event);
  /// Handles a history request
  void handleUpKeyPress();
  /// Handles a history request
  void handleDownKeyPress();
  /// Handle a return key press
  void handleReturnKeyPress();
  /// Handle a backspace key press
  bool handleBackspace();
  /// Try to execute the code in the current buffer.
  void tryExecute();
  /// Execute the given code
  void execute();
  /// Inserts a continuation prompt
  void insertContinuationPrompt();
  /// Capture mouse clicks to prevent moving the cursor to unwanted places
  void mousePressEvent(QMouseEvent *event);
  /// Capture mouse releases to prevent moving the cursor to unwanted places
  void mouseReleaseEvent(QMouseEvent *event);

  /// Write to the given device, commenting output lines
  virtual void writeToDevice(QIODevice & device) const;

  QSharedPointer<Script> m_runner;
  CommandHistory m_history;
  QSharedPointer<InputSplitter> m_inputBuffer;
  Status m_status;

  int m_promptKey;
  int m_continuationKey;
  int m_currentPromptLineIndex;

  QString m_pastedText;
  QTextStream m_pasteQueue;
  
  QAction *m_copy;
  QAction *m_cut;
  QAction *m_paste;
  QAction *m_saveAs;
  QAction *m_zoomIn;
  QAction *m_zoomOut;
};

#endif /* COMMANDLINEINTERPRETER_H_ */
