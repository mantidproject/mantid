#include "CommandLineInterpreter.h"
#include "ScriptingEnv.h"

#include <QKeyEvent>
#include <QApplication>
#include <QClipboard>
#include <Qsci/qscilexer.h>

#include <iostream>

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
  : ScriptEditor(parent,NULL), m_runner(), m_history(), m_inputmode(ReadWrite),
    m_inputBuffer(),m_status(Waiting),
    m_promptKey(markerDefine(QsciScintilla::ThreeRightArrows)),
    m_continuationKey(markerDefine(QsciScintilla::ThreeDots))
{
  setup(environ);

  markerAdd(0, m_promptKey);
  setMarginLineNumbers(1,false);

  setAutoIndent(false);
  setIndentationsUseTabs(false);
  setTabWidth(4);

  setMarginWidth(1, 14);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  // Need to disable some default key bindings that Scintilla provides as they don't really
  // fit here
  remapWindowEditingKeys();

  // Use a fixed width font
  QFont f("Courier");
  f.setFixedPitch(true);
  f.setPointSize(10);
  if( lexer() )
  {
    lexer()->setFont(f);
  }
  else
  {
    setFont(f);
  }
}


/**
 * Persist the current settings to the store
 */
void CommandLineInterpreter::saveSettings() const
{

}

/**
 * Paste in code and execute as new lines are encountered
 */
void CommandLineInterpreter::paste()
{
//  // Check if we have flagged to mark the line as read only
//  if( m_inputmode == ReadOnly )
//  {
//    ///set the cursor at the current input line
//    setCursorPosition(lastLineIndex(),length()+1);
//    /// now set the editing state of the current line
//    setEditingState(lastLineIndex());
//  }
//  QString txt = QApplication::clipboard()->text();
//  if( txt.isEmpty() )
//  {
//    return;
//  }
//  // Split by line and send each line that requires executing separately to the console
//  QStringList code_lines = txt.split('\n');
//  QStringListIterator itr(code_lines);
//  while( itr.hasNext() )
//  {
//    int line_index = this->lines() - 1;
//    QString txt = itr.next();
//    this->setText(line_index, txt.remove('\r').remove('\n'),length()-1);
//    setCursorPosition(line_index,length()+1);
//    //if there are lines ahead of this line ( multilines) then execute the line
//    if(itr.hasNext())
//    {
//      executeCodeAtLine(line_index);
//    }
//  }
}

/**
 * Write the output to the interpreter
 * @param messages
 */
void CommandLineInterpreter::displayOutput(const QString & messages)
{
  if(!text().endsWith("\n")) append("\n");
  if(messages != "\n") append(messages);
}

/**
 * Write the output to the interpreter
 * @param messages
 */
void CommandLineInterpreter::displayError(const QString & messages)
{
  if(!text().endsWith("\n")) append("\n");
  append(messages);
}

/**
 *  Inserts a input prompt at the end of the document
 */
void CommandLineInterpreter::insertInputPrompt()
{
  append("\n");
  markerAdd(lastLineIndex(), m_promptKey);
  setCursorPosition(lastLineIndex(), 0);
}

/**
 * Setup with a scripting environment
 * @param environ A reference to a scripting environment
 */
void CommandLineInterpreter::setup(const ScriptingEnv & environ)
{
  m_runner = QSharedPointer<Script>(environ.newScript("<commandline>",this,Script::Interactive));
  connect(m_runner.data(), SIGNAL(print(const QString &)), this, SLOT(displayOutput(const QString &)));
  connect(m_runner.data(), SIGNAL(finished(const QString &)), this, SLOT(insertInputPrompt()));
  /// Order here is important so that the error signal reaches the widget first
  connect(m_runner.data(), SIGNAL(error(const QString &, const QString &, int)),
          this, SLOT(displayError(const QString &)));
  connect(m_runner.data(), SIGNAL(error(const QString &, const QString &, int)), this, SLOT(insertInputPrompt()));

  setLexer(environ.createCodeLexer());
  m_inputBuffer = QSharedPointer<InputSplitter>(new InputSplitter(m_runner));

}

/**
 * Disable several default key bindings, such as Ctrl+A, that Scintilla provides.
 */
void CommandLineInterpreter::remapWindowEditingKeys()
{
  //Select all
  int keyDef = 'A' + (SCMOD_CTRL << 16);
  SendScintilla(SCI_CLEARCMDKEY, keyDef);
  //Undo
  keyDef = 'Z' + (SCMOD_CTRL << 16);
  SendScintilla(SCI_CLEARCMDKEY, keyDef);
  //  Other key combination
  keyDef = SCK_BACK + (SCMOD_ALT << 16);
  SendScintilla(SCI_CLEARCMDKEY, keyDef);
  //Redo
  keyDef = 'Y' + (SCMOD_CTRL << 16);
  SendScintilla(SCI_CLEARCMDKEY, keyDef);
  //Paste
  keyDef = 'V' + (SCMOD_CTRL << 16);
  SendScintilla(SCI_CLEARCMDKEY, keyDef);
}

/**
 * Intercept key presses
 * @param event
 */
void CommandLineInterpreter::keyPressEvent(QKeyEvent* event)
{
  if(handleKeyPress(event->key()))
  {
    return;
  }
  else
  {
    ScriptEditor::keyPressEvent(event);
  }
  //int key = event->key();
//  // Check if we have flagged to mark the line as read only
//  if( m_inputmode == ReadOnly )
//  {
//  // if control or Ctrl+C or Ctrl+X  pressed
//    if(key==Qt::Key_Control || isCtrlCPressed(m_previousKey,key )|| isCtrlXPressed(m_previousKey,key ) )
//    {
//      forwardKeyPressToBase(event);
//       m_previousKey=key;
//      return;
//    }
//   ///set the cursor at the current input line
//    setCursorPosition(lines() - 1, length()-1);
//    /// now set the editing state of the current line
//    setEditingState(lines() - 1);
//  }
//  m_previousKey=key;
//
//  bool handled(false);
//  int last_line = lines() - 1;
//
//  if( key == Qt::Key_Return || key == Qt::Key_Enter )
//  {
//
//  }
//  else if( key == Qt::Key_Up )
//  {
//
//    if( m_history.hasPrevious() )
//    {
//      QString cmd = m_history.getPrevious();
//      setText(last_line, cmd);
//    }
//    handled = true;
//  }
//  else if( key == Qt::Key_Down )
//  {
//    if( m_history.hasNext() )
//    {
//      QString cmd = m_history.getNext();
//      setText(last_line, cmd);
//    }
//    handled = true;
//  }
//  //At the start of a line we don't want to go back to the previous
//  else if( key == Qt::Key_Left || key == Qt::Key_Backspace )
//  {
//    int index(-1), dummy(-1);
//    getCursorPosition(&dummy, &index);
//    if( index == 0 ) handled = true;
//  }
//  else
//  {
//  }
//
//  if( handled ) return;
//  forwardKeyPressToBase(event);
//  return;
}

bool CommandLineInterpreter::handleKeyPress(const int key)
{
  bool handled(false);
  if(key == Qt::Key_Return || key == Qt::Key_Enter)
  {
    handled = true;
    handleReturnKeyPress();
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

  return handled;
}

/**
 * Handle an up key press
 */
void CommandLineInterpreter::handleUpKeyPress()
{
  if( m_history.hasPrevious() )
  {
    QString cmd = m_history.getPrevious();
    setText(lastLineIndex(), cmd);
  }
}

/**
 * Handle a down key press
 */
void CommandLineInterpreter::handleDownKeyPress()
{
  if( m_history.hasNext() )
  {
    QString cmd = m_history.getNext();
    setText(lastLineIndex(), cmd);
  }
}

/**
 * Handle a return key press
 */
void CommandLineInterpreter::handleReturnKeyPress()
{
  tryExecute();
}

/**
 * Try and execute the code in the current buffer. If it
 * is incomplete then ask for more input
 */
void CommandLineInterpreter::tryExecute()
{
  m_inputBuffer->push(text(lines() - 1));
  const bool needMore = m_inputBuffer->pushCanAcceptMore();
  if(needMore)
  {
    insertContinuationPrompt();
  }
  else
  {
    execute();
  }
}

/**
 * Execute the given code
 * @param code Run the code asynchronously and return
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
    m_runner->executeAsync(code);
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
  setCursorPosition(lines() - 1,length()+1);
}

/**
 * Capture mouse click events to prevent moving the cursor to unwanted places
 */
void CommandLineInterpreter::mousePressEvent(QMouseEvent *event)
{
  ScriptEditor::mousePressEvent(event);
  int line(-1), dummy(-1);
  getCursorPosition(&line, &dummy);
  setEditingState(line);
}

/**
 * Set whether or not the current line(where the cursor is located) is editable
 */
void CommandLineInterpreter::setEditingState(int line)
{
  if(line != lines() - 1)
  {
    m_inputmode = ReadOnly;
  }
  else
  {
    m_inputmode = ReadWrite;
  }
}

/**
 * checks the shortcut key for copy pressed
 * @param prevKey :: -code corresponding to the previous key
 * @param curKey :: -code corresponding to the current key
 * @returns bool returns true if the keys pressed are Ctrl+C
 */
bool CommandLineInterpreter::isCtrlCPressed(const int prevKey,const int curKey)
{
   return ((curKey==Qt::Key_C  && prevKey==Qt::Key_Control )?true:false);
}

/**
 * checks the shortcut key for cut pressed
 * @param prevKey :: -code corresponding to the previous key
 * @param curKey :: -code corresponding to the current key
 * @returns bool returns true if the keys pressed are Ctrl+X
 */
bool CommandLineInterpreter::isCtrlXPressed(const int prevKey,const int curKey)
{
   return ((curKey==Qt::Key_X  && prevKey==Qt::Key_Control )?true:false);
}

