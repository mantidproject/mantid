#include "CommandLineInterpreter.h"
#include "ScriptingEnv.h"
#include "MantidQtMantidWidgets/FindDialog.h"

#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMenu>

#include <Qsci/qscilexer.h>

#include <iostream>
#include <stdexcept>

//-----------------------------------------------------------------------------
// InputSplitter class
//-----------------------------------------------------------------------------

InputSplitter::InputSplitter(QSharedPointer<Script> compiler)
  : m_compiler(compiler), m_indentSpaces(0), m_fullDedent(false), m_source(),
    m_buffer(), m_complete(false)
{
}

/**
 * Push a line of code, returning true if code is now complete
 */
bool InputSplitter::push(const QString & line)
{
  store(line);
  QString source = m_source;
  if(source.endsWith("\\"))
  {
    return true;
  }

  updateIndent(line);
  m_complete = m_compiler->compilesToCompleteStatement(source);
  return m_complete;
}

/**
 * Returns true if push can accept more input.
 * Push will not accept more input when a syntax error is raised
 * or:
 *     The input compiles to a complete AND
 *     The indentation is flush-left
 *     There is a single extra-line of whitespace
 * @return True if the above conditions are met
 */
bool InputSplitter::pushCanAcceptMore() const
{
  // Not complete statement, definitely need more
  if(!m_complete)
  {
    return true;
  }

  if(m_indentSpaces == 0)
  {
    if(!m_fullDedent)
    {
      return false;
    }
  }

  QString lastLine = m_buffer.back();
  if(lastLine.simplified().isEmpty()) // All whitespace
  {
    return false;
  }
  else
  {
    return true;
  }
}

/**
 * Returns the complete source
 * @return String containing the complete code
 */
QString InputSplitter::getSource() const
{
  return m_source;
}

/**
 * Reset the state of the splitter to accept future input
 */
void InputSplitter::reset()
{
  m_indentSpaces = 0;
  m_buffer.clear();
  m_source = "";
  m_complete = false;
  m_fullDedent = false;
}

/**
 * Store the given line, appending a new line if necessary
 * @param line
 */
void InputSplitter::store(const QString & line)
{
  if(line.endsWith('\n'))
  {
    m_buffer.append(line);
  }
  else
  {
    m_buffer.append(line + "\n");
  }
  m_source = m_buffer.join("");
}

/**
 * Update the indent level for the given line
 * @param line
 */
void InputSplitter::updateIndent(const QString & line)
{
  int indentSpaces = m_indentSpaces;
  bool fullDedent = m_fullDedent;

  int initialSpaces = numInitialSpaces(line);
  if(initialSpaces < indentSpaces)
  {
    indentSpaces = initialSpaces;
    if( indentSpaces <= 0 )
    {
      fullDedent = true;
    }
  }

  if(finalCharIsColon(line))
  {
    indentSpaces += 4;
  }
  else if(matchesDedent(line))
  {
    indentSpaces -= 4;
    if( indentSpaces <= 0 )
    {
      fullDedent = true;
    }
  }
  if( indentSpaces < 0 ) indentSpaces = 0;

  m_indentSpaces = indentSpaces;
  m_fullDedent = fullDedent;
}
/**
 * Return the number of initial spaces on the line, tabs count as 1
 * @param line
 * @return A number of whitespace characters at the start
 */
int InputSplitter::numInitialSpaces(const QString & line)
{
  static QRegExp iniSpacesRegex("^([ \t\r\f\v]+)");
  int index = iniSpacesRegex.indexIn(line);
  if( index < 0 )
  {
    return 0;
  }
  else
  {
    return iniSpacesRegex.matchedLength();
  }
}

/**
 * Returns a reference to regex for unindented code
 * @return
 */
bool InputSplitter::matchesDedent(const QString & str) const
{
  static QRegExp dedentRegex(
      "^\\s+raise(\\s.*)?$" // raise statement (+ space + other stuff, maybe)
      "^\\s+raise\\([^\\)]*\\).*$" // wacky raise with immediate open paren
      "^\\s+return(\\s.*)?$" // normal return (+ space + other stuff, maybe)
      "^\\s+return\\([^\\)]*\\).*$" // wacky return with immediate open paren
      "^\\s+pass\\s*$" // pass (optionally followed by trailing spaces)
      );
  int start = dedentRegex.indexIn(str);
  if(start < 0) return false;
  else return true;
}

/**
 * Is the final character, ignoring trailing whitespace, a colon
 *
 */
bool InputSplitter::finalCharIsColon(const QString & str) const
{
  QChar singleChar;
  int index = str.size() - 1;
  for (; index >= 0; --index)
  {
    singleChar = str.at(index);
    if(singleChar.isSpace()) continue;
    else break;
  }
  return (singleChar == ':');
}

//-----------------------------------------------------------------------------
// CommandLineInterpreter class
//-----------------------------------------------------------------------------

/**
 * Construct an object with the given parent
 */
CommandLineInterpreter::CommandLineInterpreter(const ScriptingEnv & environ, QWidget *parent)
  : ScriptEditor(parent,environ.createCodeLexer()), m_runner(), m_history(),
    m_inputBuffer(),m_status(Waiting),
    m_promptKey(markerDefine(QsciScintilla::ThreeRightArrows)),
    m_continuationKey(markerDefine(QsciScintilla::ThreeDots)),
    m_currentPromptLineIndex(0),
    m_pastedText(), m_pasteQueue(),
    m_copy(NULL), m_cut(NULL), m_paste(NULL), m_saveAs(NULL),
    m_zoomIn(NULL), m_zoomOut(NULL)
{
  enableAutoCompletion();
  setupEnvironment(environ);
  setupMargin();
  setupIndentation();
  setupFont();

  initActions();

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
          this, SLOT(showContextMenu(const QPoint &)));
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  // Need to disable some default key bindings that Scintilla provides as they don't really
  // fit here
  remapWindowEditingKeys();
}

/**
 * Persist the current settings to the store
 */
void CommandLineInterpreter::saveSettings() const
{

}

/**
 * Shutdown the interpreter
 * The script runner objects may need to perform
 * shutdown operations that require the environment to be
 * running. The destructor is not controlled enough for this
 */
void CommandLineInterpreter::shutdown()
{
  m_inputBuffer.clear();
  m_runner.clear();
}

/**
 * Paste in code and execute as new lines are encountered
 */
void CommandLineInterpreter::paste()
{
  const QClipboard *clipboard = QApplication::clipboard();
  m_pastedText = clipboard->text();
  if(m_pastedText.isEmpty()) return;

  const int lastLineIndex = indexOfLastLine();
  if(indexOfCursorLine() != lastLineIndex)
  {
    moveCursorToEnd();
  }

  int dummy(-1), offset(0);
  getCursorPosition(&dummy, &offset);

  if(containsNewlines(m_pastedText))
  {
    processPastedCodeWithNewlines(offset);
  }
  else
  {
    // If no newlines just insert the text at the current position
    setText(lastLineIndex,m_pastedText,offset);
  }
}

/**
 * Copies the selected code and if the cursor is on the current
 * input line then removes the text
 */
void CommandLineInterpreter::cut()
{
  if(indexOfCursorLine() == indexOfLastLine())
  {
    ScriptEditor::cut();
  }
  else
  {
    copy();
  }
}

/**
 * Overrides base class to slot. Calls base class and catches
 * possible cancellation exception
 */
void CommandLineInterpreter::saveAs()
{
  try
  {
    ScriptEditor::saveAs();
  }
  catch(ScriptEditor::SaveCancelledException&)
  {
  }
}

/**
 * Display the context menu
 * @param clickPoint The point in widget coordinates where the mouse was clicked
 */
void CommandLineInterpreter::showContextMenu(const QPoint & clickPoint)
{
  QMenu context;
  context.addAction(m_copy);
  context.addAction(m_cut);
  context.addAction(m_paste);

  context.addSeparator();
  context.addAction(m_saveAs);

  context.addSeparator();
  context.addAction(m_zoomIn);
  context.addAction(m_zoomOut);

  context.exec(this->mapToGlobal(clickPoint));
}


/**
 * Write the output to the interpreter
 * @param messages
 */
void CommandLineInterpreter::displayOutput(const QString & messages)
{
  append(messages);
}

/**
 * Write the output to the interpreter
 * @param messages
 */
void CommandLineInterpreter::displayError(const QString & messages)
{
  append(messages);

  //disconnect front pasting if we encounter an error
  disconnect(m_runner.data(), SIGNAL(finished(const QString &)), this, SLOT(processNextPastedLine()));
  disconnect(this, SIGNAL(moreInputRequired()), this, SLOT(processNextPastedLine()));
}

/**
 *  Inserts a input prompt at the end of the document
 */
void CommandLineInterpreter::insertInputPrompt()
{
  const int prevPromptLineIndex = m_currentPromptLineIndex;
  if(!text().endsWith("\n")) append("\n"); // If the text is already on a new line don't bother with another
  moveCursorToStartOfLastLine();
  m_currentPromptLineIndex = indexOfLastLine();
  // Order is important. Qscintilla tries to make the markers
  // follow the cursor so we have to add the new one
  // then restore the original
  markerAdd(m_currentPromptLineIndex, m_promptKey);
  markerAdd(prevPromptLineIndex, m_promptKey);
}

/**
 * Flag that code is executing
 */
void CommandLineInterpreter::setStatusToExecuting()
{
  m_status = Executing;
}

/**
 * Flag that code is waiting
 */
void CommandLineInterpreter::setStatusToWaiting()
{
  m_status = Waiting;
}

/**
 * Process the next line in the paste queue
 */
void CommandLineInterpreter::processNextPastedLine()
{
  QString nextLine = m_pasteQueue.readLine();


  if(m_pasteQueue.atEnd())
  {
    disconnect(m_runner.data(), SIGNAL(finished(const QString &)), this, SLOT(processNextPastedLine()));
    disconnect(this, SIGNAL(moreInputRequired()), this, SLOT(processNextPastedLine()));
  }
  simulateUserInput(nextLine);
}

/**
 * Setup with a scripting environment
 * @param environ A reference to a scripting environment
 */
void CommandLineInterpreter::setupEnvironment(const ScriptingEnv & environ)
{
  m_runner = QSharedPointer<Script>(environ.newScript("__main__",this,Script::Interactive));
  connect(m_runner.data(), SIGNAL(autoCompleteListGenerated(const QStringList &)),
          this, SLOT(updateCompletionAPI(const QStringList &)));
  m_runner->generateAutoCompleteList();

  connect(m_runner.data(), SIGNAL(started(const QString &)), this, SLOT(setStatusToExecuting()));
  connect(m_runner.data(), SIGNAL(print(const QString &)), this, SLOT(displayOutput(const QString &)));
  connect(m_runner.data(), SIGNAL(finished(const QString &)), this, SLOT(insertInputPrompt()));
  connect(m_runner.data(), SIGNAL(finished(const QString &)), this, SLOT(setStatusToWaiting()));

  /// Order here is important so that the error signal reaches the widget first
  connect(m_runner.data(), SIGNAL(error(const QString &, const QString &, int)),
          this, SLOT(displayError(const QString &)));
  connect(m_runner.data(), SIGNAL(error(const QString &, const QString &, int)), this, SLOT(insertInputPrompt()));
  connect(m_runner.data(), SIGNAL(error(const QString &, const QString &, int)), this, SLOT(setStatusToWaiting()));

  m_inputBuffer = QSharedPointer<InputSplitter>(new InputSplitter(m_runner));

  connect(&environ, SIGNAL(shuttingDown()), this, SLOT(shutdown()));
}

/**
 * Setup the margin to have no line numbers and a reasonable width
 */
void CommandLineInterpreter::setupMargin()
{
  markerAdd(0, m_promptKey);
  setMarginLineNumbers(1,false);
  setMarginWidth(1, 14);
}

/**
 *  Set the indentation policy to no autoindent, spaces for tabs
 *  and tab width = 4
 */
void CommandLineInterpreter::setupIndentation()
{
  setAutoIndent(false);
  setIndentationsUseTabs(false);
  setTabWidth(4);
}

/**
 *  Set the fonts used to be fixed width
 */
void CommandLineInterpreter::setupFont()
{
  // Use a fixed width font
  QFont f("Courier");
  f.setFixedPitch(true);
  f.setPointSize(10);
  if(lexer())
  {
    lexer()->setFont(f);
  }
  else
  {
    setFont(f);
  }
}

/**
 * Create the required actions for this class
 */
void CommandLineInterpreter::initActions()
{
  m_copy = new QAction(tr("&Copy"), this);
  connect(m_copy, SIGNAL(triggered()), this, SLOT(copy()));

  m_cut = new QAction(tr("C&ut"), this);
  connect(m_cut, SIGNAL(triggered()), this, SLOT(cut()));

  m_paste = new QAction(tr("&Paste"), this);
  connect(m_paste, SIGNAL(triggered()), this, SLOT(paste()));

  m_saveAs = new QAction(tr("Save &As"), this);
  connect(m_saveAs, SIGNAL(triggered()), this, SLOT(saveAs()));

  m_zoomIn = new QAction(("Increase font size"), this);
  // Setting two shortcuts makes it work for both the plus on the keypad and one above an =
  // Despite the Qt docs advertising the use of QKeySequence::ZoomIn as the solution to this,
  // it doesn't seem to work for me
  m_zoomIn->setShortcut(Qt::SHIFT+Qt::CTRL+Qt::Key_Equal);
  m_zoomIn->setShortcut(Qt::CTRL+Qt::Key_Plus);
  connect(m_zoomIn, SIGNAL(triggered()), this, SLOT(zoomIn()));

  m_zoomOut = new QAction(("Decrease font size"), this);
  m_zoomOut->setShortcut(QKeySequence::ZoomOut);
  connect(m_zoomOut, SIGNAL(triggered()), this, SLOT(zoomOut()));

}

/**
 * Disable several default key bindings, such as Ctrl+A, that Scintilla provides.
 */
void CommandLineInterpreter::remapWindowEditingKeys()
{
  SendScintilla(SCI_SETUNDOCOLLECTION, false);
}

/**
 * Show find dialog
 */
void CommandLineInterpreter::showFindDialog()
{
  //m_findDialog->show();
}

/**
 * @return The index of the line that the cursor is currently sitting on
 */
int CommandLineInterpreter::indexOfCursorLine() const
{
  int lineIndex(-1), offset(-1);
  getCursorPosition(&lineIndex, &offset);
  return lineIndex;
}


/// Set the cursor position to the start of the current input line
void CommandLineInterpreter::moveCursorToStartOfLastLine()
{
  setCursorPosition(indexOfLastLine(), 0);
}

/**
 * Moves the cursor to after the last character in the input
 */
void CommandLineInterpreter::moveCursorToEnd()
{
  setCursorPosition(indexOfLastLine(), length()+1);
}

/**
 * Does the text contain any newline characters
 * @return True if the text contains any newline characters, false otherwise
 */
bool CommandLineInterpreter::containsNewlines(const QString & text) const
{
  static QRegExp newlinesExp("[(\n)(\r\n)(\r)]+");
  return text.contains(newlinesExp);
}

/**
 * Paste and interpret multi-line code as we go
 * @param offset A starting offset on the current line
 */
void CommandLineInterpreter::processPastedCodeWithNewlines(const int offset)
{
  m_pasteQueue.setString(&m_pastedText, QIODevice::ReadOnly);
  QString firstLine = m_pasteQueue.readLine();

  // Execute the first line and connect the finished signal to a function to process the next line.
  // This chains the processing together while avoiding blocking the GUI
  connect(m_runner.data(), SIGNAL(finished(const QString &)), this, SLOT(processNextPastedLine()));
  connect(this, SIGNAL(moreInputRequired()), this, SLOT(processNextPastedLine()));
  simulateUserInput(firstLine, offset);
}

/**
 * Simulates user input by setting a line of text and generating a return key press
 * @param text A line of text to set
 * @param offset An offset of where to insert the text on the input line
 */
void CommandLineInterpreter::simulateUserInput(QString & text, const int offset)
{
  setText(indexOfLastLine(), text, offset);
  QKeyEvent enterKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
  this->keyPressEvent(&enterKeyEvent);
}

/**
 * Intercept key presses
 * @param keyPress A Qt key event describing the key press
 */
void CommandLineInterpreter::keyPressEvent(QKeyEvent* keyPress)
{
  // If the autocomplete box is active we don't want to touch the events.
  if(!isListActive() && handleKeyPress(keyPress))
  {
    keyPress->accept();
  }
  else
  {
    ScriptEditor::keyPressEvent(keyPress);
  }
}

bool CommandLineInterpreter::handleKeyPress(QKeyEvent* event)
{
  if(isExecuting())
  {
    return true;
  }

  const int key = event->key();
  bool handled(false);
  bool cursorToEnd(true);
  if(event->matches(QKeySequence::Copy))
  {
    handled = true;
    cursorToEnd = false;
    copy();
  }
  else if(event->matches(QKeySequence::Paste))
  {
    handled = true;
    cursorToEnd = true;
    paste();
  }
  else if(event->matches(QKeySequence::Cut))
  {
    handled = true;
    cursorToEnd = false;
    cut();
  }
  else if(event->matches(QKeySequence::Find))
  {
    handled = true;
    cursorToEnd = false;
    showFindDialog();
  }
  else if(key == Qt::Key_Return || key == Qt::Key_Enter)
  {
    handled = true;
    handleReturnKeyPress();
  }
  else if(key == Qt::Key_Left || key == Qt::Key_Direction_L || 
          key == Qt::Key_Backspace)
  {
    cursorToEnd = false;
    int index(-1), dummy(-1);
    getCursorPosition(&dummy, &index);
    handled = handleBackspace();
  }
  else if(key == Qt::Key_Right || key == Qt::Key_Direction_R)
  {
    cursorToEnd = false;
  }
  else if (key == Qt::Key_Up)
  {
    handled = true;
    handleUpKeyPress();
  }
  else if (key == Qt::Key_Down)
  {
    handled = true;
    handleDownKeyPress();
  }
  else if (key == Qt::Key_Delete)
  {
    cursorToEnd = false;
  }
  if(cursorToEnd && key != Qt::Key_Control &&
      (indexOfCursorLine() != indexOfLastLine() || event->matches(QKeySequence::Paste)))
  {
    moveCursorToEnd();
  }
  return handled;
}

bool CommandLineInterpreter::handleBackspace()
{
  if(hasSelectedText())
  {
    int lineFrom(-1), lineTo(-1), indexFrom(-1), indexTo(-1);
    int maxLine = lines()-1;
    getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);

    //check selection is only on the last line
    return (lineFrom < maxLine);
  }
  else
  {
    int index(-1), dummy(-1);
    getCursorPosition(&dummy, &index);

    //check if the cursor is > the start of the line
    return (index == 0);
  }
}

/**
 * Handle an up key press
 */
void CommandLineInterpreter::handleUpKeyPress()
{
  if(indexOfCursorLine() == indexOfLastLine() && m_history.hasPrevious())
  {
    QString cmd = m_history.getPrevious();
    setText(indexOfLastLine(), cmd);
  }
}

/**
 * Handle a down key press
 */
void CommandLineInterpreter::handleDownKeyPress()
{
  if(indexOfCursorLine() == indexOfLastLine() && m_history.hasNext())
  {
    QString cmd = m_history.getNext();
    setText(indexOfLastLine(), cmd);
  }
}

/**
 * Handle a return key press
 */
void CommandLineInterpreter::handleReturnKeyPress()
{
  moveCursorToEnd();
  tryExecute();
}

/**
 * Try and execute the code in the current buffer. If it
 * is incomplete then ask for more input
 */
void CommandLineInterpreter::tryExecute()
{
  m_inputBuffer->push(text(indexOfLastLine()));
  const bool needMore = m_inputBuffer->pushCanAcceptMore();
  if(needMore)
  {
    insertContinuationPrompt();
    emit moreInputRequired();
  }
  else
  {
    // Move cursor to start of fresh line to guarantee output is on fresh line
    append("\n");
    execute();
  }
}

/**
 * Execute the code currently in the buffer
 */
void CommandLineInterpreter::execute()
{
  const QString code = m_inputBuffer->getSource().trimmed();
  m_inputBuffer->reset();
  if(code.isEmpty())
  {
    insertInputPrompt();
  }
  else
  {
    try
    {
      m_runner->executeAsync(code);
    }
    catch(std::runtime_error &exc)
    {
      QMessageBox::warning(this, "MantidPlot", exc.what());
    }
    m_history.addCode(code);
  }
}

/**
 * Inserts a continuation prompt preserving the current indent level
 */
void CommandLineInterpreter::insertContinuationPrompt()
{
  append("\n");
  const int index = lines() - 1;
  int indentLevel = m_inputBuffer->currentIndent();
  indentLevel /= 4;
  for(int i = 0;i < indentLevel; ++i)
  {
    this->indent(index);
  }
  markerAdd(index, m_continuationKey);
  moveCursorToEnd();
}

/**
 * Capture mouse click events to prevent moving the cursor to unwanted places
 */
void CommandLineInterpreter::mousePressEvent(QMouseEvent *event)
{
  ScriptEditor::mousePressEvent(event);
}

/**
 * Capture mouse click events to prevent moving the cursor to unwanted places
 */
void CommandLineInterpreter::mouseReleaseEvent(QMouseEvent *event)
{
  ScriptEditor::mouseReleaseEvent(event);
}

/**
 * Writes all of the lines to a file with the output commented out. The device
 * is left open
 * @param device
 */
void CommandLineInterpreter::writeToDevice(QIODevice & device) const
{
  const int nlines(lines());
  for(int i = 0; i < nlines; ++i)
  {
    unsigned markerMask = markersAtLine(i);
    if(markerMask == 0)
    {
      device.write("# Output: ");
    }
    device.writeBlock(text(i).toAscii());
  }
}

