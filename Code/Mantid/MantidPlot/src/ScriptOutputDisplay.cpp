#include "ScriptOutputDisplay.h"
#include "TextFileIO.h"

#include "pixmaps.h"

#include <QDateTime>
#include <QMenu>
#include <QFileDialog>
#include <QPrinter>
#include <QPrintDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QApplication>

/**
 * Constructor
 * @param title :: The title
 * @param parent :: The parent widget
 * @param flags :: Window flags
 */
ScriptOutputDisplay::ScriptOutputDisplay(QWidget * parent) :
  QTextEdit(parent), m_copy(NULL), m_clear(NULL), m_save(NULL)
{
  setReadOnly(true);
  setLineWrapMode(QTextEdit::FixedColumnWidth);
  setLineWrapColumnOrWidth(105);
  setAutoFormatting(QTextEdit::AutoNone);
  // Change to fix width font so that table formatting isn't screwed up
  resetFont();

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this,
    SLOT(showContextMenu(const QPoint&)));

  initActions();
}

/**
 * Is there anything here
 */
bool ScriptOutputDisplay::isEmpty() const
{
  return document()->isEmpty();
}

/**
 * Populate a menu with editing actions
 * @param editMenu A new menu
 */
void ScriptOutputDisplay::populateEditMenu(QMenu &editMenu)
{
  editMenu.addAction(m_clear);
}

/**
 * Clear the text area
 */
void ScriptOutputDisplay::clear()
{
  clear();
}

/**
 * Change the title based on the script's execution state
 * @param running :: The current state of the script environment
 */
void ScriptOutputDisplay::setScriptIsRunning(bool running)
{
//  QString title("Script Output - Status: ");
//  if( running )
//  {
//    title += "Running ...";
//  }
//  else
//  {
//    title += "Stopped";
//  }
//  setWindowTitle(title);
}

/**
 *  Display an output message that is not an error
 *  @param msg :: The string message
 */
void ScriptOutputDisplay::displayMessage(const QString & msg)
{
  prepareForNewMessage(ScriptOutputDisplay::Standard);
  appendText(msg);
}

/**
 *  Display an output message with a timestamp & border
 *  @param msg :: The string message
 */
void ScriptOutputDisplay::displayMessageWithTimestamp(const QString & msg)
{
  prepareForNewMessage(ScriptOutputDisplay::Standard);
  QString timestamped = addTimestamp(msg);
  appendText(timestamped);
}

/**
 *  Display an error message
 *  @param msg :: The string message
 */
void ScriptOutputDisplay::displayError(const QString & msg)
{
  prepareForNewMessage(ScriptOutputDisplay::Error);
  appendText(msg);
}


//-------------------------------------------
// Private slot member functions
//-------------------------------------------
/**
 * Display a context menu
 */
void ScriptOutputDisplay::showContextMenu(const QPoint & pos)
{
  QMenu menu(this);
  menu.addAction(m_clear);
  menu.addAction(m_copy);
  menu.addAction(m_save);

  if( !isEmpty() )
  {
    QAction* print = new QAction(getQPixmap("fileprint_xpm"), "&Print", this);
    connect(print, SIGNAL(activated()), this, SLOT(print()));
    menu.addAction(print);
  }

  menu.exec(mapToGlobal(pos));
}

/**
 * Print the window output
 */
void ScriptOutputDisplay::print()
{
  QPrinter printer;
  QPrintDialog *print_dlg = new QPrintDialog(&printer, this);
  print_dlg->setWindowTitle(tr("Print Output"));
  if (print_dlg->exec() != QDialog::Accepted)
    return;
  QTextDocument document(text());
  document.print(&printer);
}

/**
 * Save script output to a file
 * @param filename :: The file name to save the output, if empty (default) then a dialog is raised
 */
void ScriptOutputDisplay::saveToFile(const QString & filename)
{
  QStringList filters;
  filters.append(tr("Text") + " (*.txt *.TXT)");
  filters.append(tr("All Files") + " (*)");
  TextFileIO fileIO(filters);
  fileIO.save(this->toPlainText(), filename);
}

//-------------------------------------------
// Private non-slot member functions
//-------------------------------------------
/**
 * Prepares the display for the next message
 * @param msgType :: One of the predefined message types
 */
void ScriptOutputDisplay::prepareForNewMessage(const MessageType msgType)
{
  if(msgType == ScriptOutputDisplay::Error)
  {
    setTextColor(Qt::red);
  }
  else
  {
    setTextColor(Qt::black);
  }
  // Ensure the cursor is in the correct position. This affects the font unfortunately
  moveCursor(QTextCursor::End);
  resetFont();
}

/**
 * Adds a border & timestamp to the message
 * @param msg
 * @return A new string with the required formatting
 */
QString ScriptOutputDisplay::addTimestamp(const QString & msg)
{
  QString separator(75, '-');
  QString timestamped =
      "%1\n"
      "%2: %3\n"
      "%4\n";
  timestamped = timestamped.arg(separator, QDateTime::currentDateTime().toString(),
                                msg.trimmed(), separator);
  return timestamped;
}

/**
 * Append new text
 * @param txt :: The text to append
 */
void ScriptOutputDisplay::appendText(const QString & txt)
{
  textCursor().insertText(txt);
  moveCursor(QTextCursor::End);
}

/**
 * Create the actions associated with this widget
 */
void ScriptOutputDisplay::initActions()
{
  // Copy action
  m_copy = new QAction(getQPixmap("copy_xpm"), "Copy", this);
  m_copy->setShortcut(tr("Ctrl+C"));
  connect(m_copy, SIGNAL(activated()), this, SLOT(copy()));

  // Clear action
  m_clear = new QAction("Clear Output", this);
  connect(m_clear, SIGNAL(activated()), this, SLOT(clear()));

  // Save  action
  m_save = new QAction("Save Output", this);
  connect(m_save, SIGNAL(activated()), this, SLOT(saveToFile()));
}

/**
 * Rest the font to default
 */
void ScriptOutputDisplay::resetFont()
{
  QFont f("Andale Mono");
  f.setFixedPitch(true);
  f.setPointSize(8);
  setCurrentFont(f);
  setMinimumWidth(5);
  setMinimumHeight(5);
}

