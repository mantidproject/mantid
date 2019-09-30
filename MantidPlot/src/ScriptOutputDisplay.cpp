// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ScriptOutputDisplay.h"
#include "TextFileIO.h"

#include "MantidQtWidgets/Common/pixmaps.h"

#include <QDateTime>
#include <QKeyEvent>
#include <QMenu>
#include <QPrintDialog>
#include <QPrinter>
#include <QWheelEvent>

using namespace MantidQt::API;

/**
 * Constructor
 * @param title :: The title
 * @param parent :: The parent widget
 * @param flags :: Window flags
 */
ScriptOutputDisplay::ScriptOutputDisplay(QWidget *parent)
    : QTextEdit(parent), m_copy(nullptr), m_clear(nullptr), m_save(nullptr),
      m_origFontSize(8), m_zoomLevel(0) {
#ifdef __APPLE__
  // Make all fonts 4 points bigger on the Mac because otherwise they're tiny!
  m_zoomLevel += 4;
#endif

  // the control is readonly, but if you set it read only then ctrl+c for
  // copying does not work
  // this approach allows ctrl+c and disables user editing through the use of
  // the KeyPress handler
  // and disabling drag and drop
  // also the mouseMoveEventHandler prevents dragging out of the control
  // affecting the text.
  setReadOnly(false);
  this->setAcceptDrops(false);

  setLineWrapMode(QTextEdit::WidgetWidth);
  setLineWrapColumnOrWidth(105);
  setAutoFormatting(QTextEdit::AutoNone);
  // Change to fix width font so that table formatting isn't screwed up
  resetFont();

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this,
          SLOT(showContextMenu(const QPoint &)));

  initActions();
}

/** Mouse move event handler - overridden to prevent dragging out of the control
 * affecting the text
 * it does this by temporarily setting the control to read only while the base
 * event handler operates
 * @param e the mouse move event
 */
void ScriptOutputDisplay::mouseMoveEvent(QMouseEvent *e) {
  this->setReadOnly(true);
  QTextEdit::mouseMoveEvent(e);
  this->setReadOnly(false);
}

/** Mouse move release handler - overridden to prevent middle mouse button
 * clicks from pasting on linux
 * @param e the mouse move event
 */
void ScriptOutputDisplay::mouseReleaseEvent(QMouseEvent *e) {
  this->setReadOnly(true);
  QTextEdit::mousePressEvent(e);
  this->setReadOnly(false);
}

void ScriptOutputDisplay::wheelEvent(QWheelEvent *e) {
  if (e->modifiers() & Qt::ControlModifier) {
    const int delta = e->delta();
    if (delta < 0) {
      zoom(-1);
      emit textZoomedOut(); // allows tracking
    } else if (delta > 0) {
      zoom(1);
      emit textZoomedIn(); // allows tracking
    }
    return;
  } else {
    QTextEdit::wheelEvent(e);
  }
}

void ScriptOutputDisplay::zoom(int range) {
  if ((range == 0) && (!isEmpty()))
    return;

  m_zoomLevel += range;
  // boundary protection
  if (m_zoomLevel < -10)
    m_zoomLevel = -10;
  if (m_zoomLevel > 20)
    m_zoomLevel = 20;

  QFont f = this->currentFont();
  int newSize = m_origFontSize + m_zoomLevel;

  if (newSize <= 0)
    newSize = 1;

  f.setPointSize(newSize);
  this->setCurrentFont(f);
  this->setFontPointSize(newSize);

  QTextCursor cursor = this->textCursor();
  this->selectAll();
  this->setFontPointSize(newSize);
  this->setTextCursor(cursor);
}

int ScriptOutputDisplay::zoomLevel() { return m_zoomLevel; }

void ScriptOutputDisplay::zoomUp() { zoom(1); }

void ScriptOutputDisplay::zoomDown() { zoom(-1); }

void ScriptOutputDisplay::setZoom(int value) {
  // 8 is the default font size
  QFont f = this->currentFont();
  zoom(value - m_zoomLevel);
}

/**
 * Is there anything here
 */
bool ScriptOutputDisplay::isEmpty() const { return document()->isEmpty(); }

/**
 * Populate a menu with editing actions
 * @param editMenu A new menu
 */
void ScriptOutputDisplay::populateEditMenu(QMenu &editMenu) {
  editMenu.addAction(m_clear);
}

/**
 * Capture key presses.
 * @param event A pointer to the QKeyPressEvent object
 */
void ScriptOutputDisplay::keyPressEvent(QKeyEvent *event) {
  if ((event->key() == Qt::Key_C) &&
      (event->modifiers() == Qt::KeyboardModifier::ControlModifier)) {
    this->copy();
  }
  // accept all key presses to prevent keyboard interaction
  event->accept();
}

/**
 *  Display an output message that is not an error
 *  @param msg :: The string message
 */
void ScriptOutputDisplay::displayMessage(const QString &msg) {
  prepareForNewMessage(ScriptOutputDisplay::Standard);
  appendText(msg);
}

/**
 *  Display an output message with a timestamp & border
 *  @param msg :: The string message
 */
void ScriptOutputDisplay::displayMessageWithTimestamp(const QString &msg) {
  prepareForNewMessage(ScriptOutputDisplay::Standard);
  QString timestamped = addTimestamp(msg);
  appendText(timestamped);
}

/**
 *  Display an error message
 *  @param msg :: The string message
 */
void ScriptOutputDisplay::displayError(const QString &msg) {
  prepareForNewMessage(ScriptOutputDisplay::Error);
  appendText(msg);
}

//-------------------------------------------
// Private slot member functions
//-------------------------------------------
/**
 * Display a context menu
 */
void ScriptOutputDisplay::showContextMenu(const QPoint &pos) {
  QMenu menu(this);
  menu.addAction(m_clear);
  menu.addAction(m_copy);
  menu.addAction(m_save);

  if (!isEmpty()) {
    QAction *print = new QAction(getQPixmap("fileprint_xpm"), "&Print", this);
    connect(print, SIGNAL(triggered()), this, SLOT(print()));
    menu.addAction(print);
  }

  menu.exec(mapToGlobal(pos));
}

/**
 * Print the window output
 */
void ScriptOutputDisplay::print() {
  QPrinter printer;
  QPrintDialog *print_dlg = new QPrintDialog(&printer, this);
  print_dlg->setWindowTitle(tr("Print Output"));
  if (print_dlg->exec() != QDialog::Accepted)
    return;
  QTextDocument document(toPlainText());
  document.print(&printer);
}

/**
 * Save script output to a file
 * @param filename :: The file name to save the output, if empty (default) then
 * a dialog is raised
 */
void ScriptOutputDisplay::saveToFile(const QString &filename) {
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
void ScriptOutputDisplay::prepareForNewMessage(const MessageType msgType) {
  // Ensure the cursor is in the correct position. This affects the font
  // unfortunately
  moveCursor(QTextCursor::End);
  resetFont();
  // zoom(0);
  if (msgType == ScriptOutputDisplay::Error) {
    setTextColor(Qt::red);
  } else {
    setTextColor(Qt::black);
  }
}

/**
 * Adds a border & timestamp to the message
 * @param msg
 * @return A new string with the required formatting
 */
QString ScriptOutputDisplay::addTimestamp(const QString &msg) {
  QString separator(75, '-');
  QString timestamped = "%1\n"
                        "%2: %3\n"
                        "%4\n";
  timestamped =
      timestamped.arg(separator, QDateTime::currentDateTime().toString(),
                      msg.trimmed(), separator);
  return timestamped;
}

/**
 * Append new text
 * @param txt :: The text to append
 */
void ScriptOutputDisplay::appendText(const QString &txt) {
  textCursor().insertText(txt);
  moveCursor(QTextCursor::End);
}

/**
 * Create the actions associated with this widget
 */
void ScriptOutputDisplay::initActions() {
  // Copy action
  m_copy = new QAction(getQPixmap("copy_xpm"), "Copy", this);
  m_copy->setShortcut(tr("Ctrl+C"));
  connect(m_copy, SIGNAL(triggered()), this, SLOT(copy()));

  // Clear action
  m_clear = new QAction("Clear Output", this);
  connect(m_clear, SIGNAL(triggered()), this, SLOT(clear()));

  // Save  action
  m_save = new QAction("Save Output", this);
  connect(m_save, SIGNAL(triggered()), this, SLOT(saveToFile()));
}

/**
 * Rest the font to default
 */
void ScriptOutputDisplay::resetFont() {
  QFont f("Andale Mono");
  f.setFixedPitch(true);
  int fontSize = m_origFontSize + m_zoomLevel;
  if (fontSize < 1)
    fontSize = 1;
  f.setPointSize(fontSize);
  setCurrentFont(f);
  setMinimumWidth(5);
  setMinimumHeight(5);
}
