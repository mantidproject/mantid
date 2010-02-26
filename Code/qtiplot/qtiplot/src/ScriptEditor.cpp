//---------------------------------------------
// Includes
//-----------------------------------------------
#include "ScriptEditor.h"

// Qt
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QAction>
#include <QPrintDialog>
#include <QPrinter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QClipboard>
#include <QShortcut>

// std
#include <cmath>
#include <iostream>

//***************************************************************************
//
// CommandHistory struct
//
//***************************************************************************
/**
 * Add a command to the store. 
 */
void CommandHistory::add(QString cmd)
{
  //Ignore duplicates that are the same as the last command entered and just reset the pointer
  int ncmds = m_commands.count();
  if( ncmds > 1 && m_commands.lastIndexOf(cmd) == ncmds - 2 ) 
  {
    //Reset the index pointer
    m_current = ncmds - 1;
    return;
  }

  // If the stack is full, then remove the oldest
  if( ncmds == m_hist_maxsize + 1)
  {
    m_commands.removeFirst();
  }
  if( !m_commands.isEmpty() )
  {
    //Remove blankline
    m_commands.removeLast();
  }
  // Add the command and an extra blank line
  m_commands.append(cmd);
  m_commands.append("");
  
  //Reset the index pointer
  m_current = m_commands.count() - 1;
}

/** 
 * Is there a previous command
 * @returns A boolean indicating whether there is command on the left of the current index
 */
bool CommandHistory::hasPrevious() const
{
  if( !m_commands.isEmpty() &&  m_current > 0 ) return true;
  else return false;
}

/**
 * Get the item pointed to by the current index and move it back one
 */
QString CommandHistory::getPrevious() const
{
  return m_commands.value(--m_current);
}

/** 
 * Is there a command next on the stack
 */
bool CommandHistory::hasNext() const
{
  if( !m_commands.isEmpty() &&  m_current < m_commands.count() - 1 ) return true;
  else return false;
}

/** 
 * Get the item pointed to by the current index and move it down one
 */
QString CommandHistory::getNext() const
{
  return m_commands.value(++m_current);
}

//***************************************************************************
//
// ScriptEditor class
//
//***************************************************************************
// The colour for a success marker
QColor ScriptEditor::g_success_colour = QColor("lightgreen");
// The colour for an error marker
QColor ScriptEditor::g_error_colour = QColor("red");

//------------------------------------------------
// Public member functions
//------------------------------------------------
/**
 * Constructor
 * @param parent The parent widget (can be NULL)
 */
ScriptEditor::ScriptEditor(QWidget *parent, bool interpreter_mode) : 
  QsciScintilla(parent), m_filename(""), m_marker_handle(-1), m_interpreter_mode(interpreter_mode),
  m_history(), m_read_only(false), m_need_newline(false)
{
  // Undo action
  m_undo = new QAction(tr("&Undo"), this);
  m_undo->setShortcut(tr("Ctrl+Z"));
  connect(m_undo, SIGNAL(activated()), this, SLOT(undo()));
  connect(this, SIGNAL(undoAvailable(bool)), m_undo, SLOT(setEnabled(bool)));
  // Redo action
  m_redo = new QAction(tr("&Redo"), this);
  m_redo->setShortcut(tr("Ctrl+Y"));
  connect(m_redo, SIGNAL(activated()), this, SLOT(redo()));
  connect(this, SIGNAL(redoAvailable(bool)), m_redo, SLOT(setEnabled(bool)));

  //Cut
  m_cut = new QAction(tr("C&ut"), this);
  m_cut->setShortcut(tr("Ctrl+X"));
  connect(m_cut, SIGNAL(activated()), this, SLOT(cut()));
  connect(this, SIGNAL(copyAvailable(bool)), m_cut, SLOT(setEnabled(bool)));

  //Copy
  m_copy = new QAction(tr("&Copy"), this);
  m_copy->setShortcut(tr("Ctrl+C"));
  connect(m_copy, SIGNAL(activated()), this, SLOT(copy()));
  connect(this, SIGNAL(copyAvailable(bool)), m_copy, SLOT(setEnabled(bool)));

  //Paste
  m_paste = new QAction(tr("&Paste"), this);
  m_paste->setShortcut(tr("Ctrl+V"));
  connect(m_paste, SIGNAL(activated()), this, SLOT(paste()));

  //Print
  m_print = new QAction(tr("&Print script"), this);
  m_print->setShortcut(tr("Ctrl+P"));
  connect(m_print, SIGNAL(activated()), this, SLOT(print()));

  if( interpreter_mode )
  {
    m_marker_handle = markerDefine(QsciScintilla::ThreeRightArrows);
    setMarginLineNumbers(1,false);
    //Editor properties
    setAutoIndent(false); 
    markerAdd(0, m_marker_handle); 
    setMarginWidth(1, 14);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // Need to disable some default key bindings that Scintilla provides as they don't really
    // fit here
    remapWindowEditingKeys();
    QShortcut *shortcut = new QShortcut(m_paste->shortcut(), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(paste()));
  }
  else
  {
    m_marker_handle = markerDefine(QsciScintilla::RightArrow);
    setMarginLineNumbers(1,true);
    //Editor properties
    setAutoIndent(true);
    // Update for a text change
    connect(this, SIGNAL(textChanged()), this, SLOT(update()));
    //Update the editor
    update();
  }
  
}

/**
 * Destructor
 */
ScriptEditor::~ScriptEditor()
{
}

/**
 * Default size hint
 */
QSize ScriptEditor::sizeHint() const
{
  if( m_interpreter_mode )
  {
    return QSize(0,50);
  }
  else
  {
    return QSize(600, 500);
  }
}

/**
 * Save the text to the given filename
 * @param filename The filename to use
 */
bool ScriptEditor::saveScript(const QString & filename)
{
  if( filename.isEmpty() )
  {
    return false;
  }
  
  QFile file(filename);
  if( !file.open(QIODevice::WriteOnly) )
  {
    QMessageBox::critical(this, tr("MantidPlot - File error"), 
			  tr("Could not open file \"%1\" for writing.").arg(filename));
    return false;
  }

  QTextStream writer(&file);
  writer.setEncoding(QTextStream::UnicodeUTF8);
  QApplication::setOverrideCursor(Qt::WaitCursor);
  writer << text();
  QApplication::restoreOverrideCursor();
  file.close();

  return true;
}

/**
 * Set the text on the given line, something I feel is missing from the QScintilla API. Note
 * that like QScintilla line numbers start from 0
 * @param lineno A zero-based index representing the linenumber, 
 * @param text The text to insert at the given line
 */
void ScriptEditor::setText(int lineno, const QString& txt)
{
  int line_length = txt.length();
  // Index is max of the length of current/new text
  setSelection(lineno, 0, lineno, qMax(line_length, this->text(lineno).length()));
  removeSelectedText();
  insertAt(txt, lineno, 0);
  setCursorPosition(lineno, line_length);
}

/** 
 * Capture key presses and if in interpeter mode use Up and Down arrow keys to search the
 * command history
 * @event A pointer to the QKeyPressEvent object
 */
void ScriptEditor::keyPressEvent(QKeyEvent* event)
{
  if( !m_interpreter_mode || isListActive() )
  {
    return QsciScintilla::keyPressEvent(event);
  }
   // Check if we have flagged to mark the line as read only
  if( m_read_only ) return;

  int key = event->key();
  int last_line = lines() - 1;
  if( key == Qt::Key_Return || key == Qt::Key_Enter )
  {
    executeCodeAtLine(last_line);
    return;
  }
  else if( key == Qt::Key_Up )
  {
    if( m_history.hasPrevious() )
    {
      QString cmd = m_history.getPrevious();
      setText(last_line, cmd);
    }
    return;
  }
  else if( key == Qt::Key_Down )
  {
    if( m_history.hasNext() )
    {
      QString cmd = m_history.getNext();
      setText(last_line, cmd);
    }
    return;
  }
  //At the start of a line we don't want to go back to the previous
  else if( key == Qt::Key_Left || key == Qt::Key_Backspace )
  {
    int index(-1), dummy(-1);
    getCursorPosition(&dummy, &index);
    if( index == 0 ) return;
  }
  else
  {
    return QsciScintilla::keyPressEvent(event);
  }
  return QsciScintilla::keyPressEvent(event);
}

/**
 * Set whether or not the current line(where the cursor is located) is editable
 */
void ScriptEditor::setEditingState(int line)
{
  m_read_only = (line != lines() - 1);
}

/**
 * Capture mouse click events to prevent moving the cursor to unwanted places
 */
void ScriptEditor::mousePressEvent(QMouseEvent *event)
{  
  QsciScintilla::mousePressEvent(event);
  if( m_interpreter_mode )
  {
    int line(-1), dummy(-1);
    getCursorPosition(&line, &dummy);
    setEditingState(line);
  }
}

/**
* Create a new input line
*/
void ScriptEditor::newInputLine()
{
  int cursorline = lines();
  // Work out if we need a new line or not
  if( !text().endsWith('\n') )
  {
    append("\n");
  }
  else
  {
    cursorline -= 1;
  }

  markerAdd(cursorline, m_marker_handle);
  setCursorPosition(cursorline, 0);
}

//-----------------------------------------------
// Public slots
//-----------------------------------------------
/**
 * Update the editor
 */
void ScriptEditor::update()
{
  emit undoAvailable(isUndoAvailable());
  emit redoAvailable(isRedoAvailable());
  int width = 38;
  int ntens = static_cast<int>(std::log10(static_cast<double>(lines())));
  if( ntens > 1 )
  {
    width += 5*ntens;
  }
  setMarginWidth( 1, width );
}

/**
 * Set the marker state
 * @param enable If true then the progress arrow is enabled
 */
void ScriptEditor::setMarkerState(bool enabled)
{
  if( enabled )
  {
    setMarkerBackgroundColor(QColor("gray"), m_marker_handle);
    markerAdd(0, m_marker_handle);
  }
  else
  {
    markerDeleteAll(); 
  }
}

/**
 * Update the arrow marker to point to the correct line and colour it depending on the error state
 * @param lineno The line to place the marker at. A negative number will clear all markers
 * @param success If false, the marker will turn red
 */
void ScriptEditor::updateMarker(int lineno, bool success)
{
  if( success )
  {
    setMarkerBackgroundColor(g_success_colour, m_marker_handle);
  }
  else
  {
    setMarkerBackgroundColor(g_error_colour, m_marker_handle);
  }

  if( lineno < 0 ) return;
  markerDeleteAll();

  ensureLineVisible(lineno);
  markerAdd(lineno - 1, m_marker_handle);
}

/**
 * Print the current text
 */
void ScriptEditor::print()
{
  QPrinter printer(QPrinter::HighResolution);
  QPrintDialog *print_dlg = new QPrintDialog(&printer, this);
  print_dlg->setWindowTitle(tr("Print Script"));
  if (print_dlg->exec() != QDialog::Accepted) 
  {
    return;
  }
  QTextDocument document(text());
  document.print(&printer);
}

/**
 * Display the output from a script that has been run in interpeter mode
 * @param msg The output string
 * @param error If this is an error
 */
void ScriptEditor::displayOutput(const QString& msg, bool error)
{
  if( m_need_newline )
  { 
    append("\n");      
    m_need_newline = false;
  }
  if( !error )
  {
    append(msg);
  }
  else
  {
    append("\"" + msg.trimmed() + "\"");
  }
}

/// Overrride the paste command when in interpreter mode
void ScriptEditor::paste()
{
  if( m_interpreter_mode )
  {
    QString txt = QApplication::clipboard()->text();
    if( txt.isEmpty() )
    {
      return;
    }
    // Split by line and send each line that requires executing separately to the console
    QStringList code_lines = txt.split('\n');
    QStringListIterator itr(code_lines);
    while( itr.hasNext() )
    {
      int line_index = this->lines() - 1;
      QString txt = itr.next();
      this->setText(line_index, txt.remove('\r').remove('\n'));
      executeCodeAtLine(line_index);
    }
  }
  else
  {
    QsciScintilla::paste();
  }
}

//------------------------------------------------
// Private member functions
//------------------------------------------------
/**
* Execute a line of code
* @param lineno The line number of the code to execute
*/
void ScriptEditor::executeCodeAtLine(int lineno)
{
  QString cmd = text(lineno).remove('\r').remove('\n');
  // I was seeing strange behaviour with the first line marker disappearing after
  // entering some text, removing it and retyping then pressing enter
  
  if( cmd.isEmpty() )
  {
    return;
  }

  m_history.add(cmd);
  if( lineno == 0 ) markerAdd(lineno, m_marker_handle); 
  m_need_newline = true;
  emit executeLine(cmd);
}

/**
 * Disable several default key bindings, such as Ctrl+A, that Scintilla provides.
 */
void ScriptEditor::remapWindowEditingKeys()
{
  //Down
  int keyDef = SCK_DOWN + (0 << 16);
  SendScintilla(SCI_CLEARCMDKEY, keyDef);
  //Up
  keyDef = SCK_UP + (0 << 16);
  SendScintilla(SCI_CLEARCMDKEY, keyDef);
  //Select all 
  keyDef = 'A' + (SCMOD_CTRL << 16);
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
