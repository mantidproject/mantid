// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//-------------------------------------------
// Includes
//-------------------------------------------
#include "MantidQtWidgets/Common/MessageDisplay.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/Common/NotificationService.h"

#include <QAction>
#include <QActionGroup>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPoint>
#include <QScrollBar>
#include <QSettings>
#include <QSignalMapper>

#include <Poco/Logger.h>
#include <Poco/Message.h>
#include <Poco/SplitterChannel.h>
#include <Poco/Version.h>

namespace {

int DEFAULT_LINE_COUNT_MAX = 8192;
const char *PRIORITY_KEY_NAME = "MessageDisplayPriority";
const char *LINE_COUNT_MAX_KEY_NAME = "MessageDisplayLineCountMax";

} // namespace

using Mantid::Kernel::ConfigService;

namespace MantidQt::MantidWidgets {

//-------------------------------------------
// Public member functions
//-------------------------------------------

/**
 * Load settings from the persistent store. The client is expected to call
 * this method with the QSettings object opened at the approriate group
 * @param storage A pointer to an existing QSettings instance opened
 * at the group containing the values
 */
void MessageDisplay::readSettings(const QSettings &storage) {
  const int logLevel = storage.value(PRIORITY_KEY_NAME, 0).toInt();
  if (logLevel > 0) {
    ConfigService::Instance().setLogLevel(logLevel, true);
  }
  setMaximumLineCount(storage.value(LINE_COUNT_MAX_KEY_NAME, DEFAULT_LINE_COUNT_MAX).toInt());
}

/**
 * Load settings from the persistent store. The client is expected to call
 * this method with the QSettings object opened at the approriate group
 * @param storage A pointer to an existing QSettings instance opened
 * at the group where the values should be stored.
 */
void MessageDisplay::writeSettings(QSettings &storage) const {
  storage.setValue(PRIORITY_KEY_NAME, Poco::Logger::root().getLevel());
  storage.setValue(LINE_COUNT_MAX_KEY_NAME, maximumLineCount());
}

/**
 * Construct a MessageDisplay with the default font
 * @param parent An optional parent widget
 */
MessageDisplay::MessageDisplay(QWidget *parent) : MessageDisplay(QFont(), parent) {}

/**
 * Construct a MessageDisplay using the given font
 * @param font A reference to a font object
 * @param parent An optional parent widget
 */
MessageDisplay::MessageDisplay(const QFont &font, QWidget *parent)
    : QWidget(parent), m_logChannel(new QtSignalChannel), m_textDisplay(new QPlainTextEdit(this)), m_formats(),
      m_loglevels(new QActionGroup(this)), m_logLevelMapping(new QSignalMapper(this)),
      m_error(new QAction(tr("&Error"), this)), m_warning(new QAction(tr("&Warning"), this)),
      m_notice(new QAction(tr("&Notice"), this)), m_information(new QAction(tr("&Information"), this)),
      m_debug(new QAction(tr("&Debug"), this)) {
  initActions();
  initFormats();
  setupTextArea(font);
}

MessageDisplay::~MessageDisplay() {
  // We only attach to splitter channels but we may not have been attached...
  auto rootChannel = Poco::Logger::root().getChannel();
#if POCO_VERSION > 0x01090400
  // getChannel changed to return an AutoPtr
  if (auto *splitChannel = dynamic_cast<Poco::SplitterChannel *>(rootChannel.get())) {
#else
  if (auto *splitChannel = dynamic_cast<Poco::SplitterChannel *>(rootChannel)) {
#endif
    splitChannel->removeChannel(m_logChannel);
  }
  // The Channel class is ref counted and will delete itself when required
  m_logChannel->release();
  delete m_textDisplay;
}

/**
 * Attaches the Mantid logging framework. Starts the ConfigService if
 * required
 * @param logLevel If > 0 then set the filter channel level to this. A
 * number =< 0 uses the channel
 */
void MessageDisplay::attachLoggingChannel(int logLevel) {
  // Setup logging. ConfigService needs to be started
  auto &configSvc = ConfigService::Instance();
  // The root channel might be a SplitterChannel
  auto rootChannel = Poco::Logger::root().getChannel();
#if POCO_VERSION > 0x01090400
  // getChannel changed to return an AutoPtr
  if (auto *splitChannel = dynamic_cast<Poco::SplitterChannel *>(rootChannel.get())) {
#else
  if (auto *splitChannel = dynamic_cast<Poco::SplitterChannel *>(rootChannel)) {
#endif
    splitChannel->addChannel(m_logChannel);
  } else {
    throw std::runtime_error("MessageDisplay requires the root logger to be configured with a SplitterChannel.\n"
                             "Set 'logging.loggers.root.channel.class = SplitterChannel' in properties file.");
  }
  connect(m_logChannel, SIGNAL(messageReceived(const Message &)), this, SLOT(append(const Message &)));
  if (logLevel > 0) {
    configSvc.setLogLevel(logLevel, true);
  }
}

/**
 * @param source A string specifying the required source for messages
 * that will be emitted
 */
void MessageDisplay::setSource(const QString &source) { m_logChannel->setSource(source); }

/**
 * Filter messages, either by Framework or all/individual scripts
 */
void MessageDisplay::filterMessages() {
  m_textDisplay->clear();
  for (auto &msg : getHistory()) {
    if (shouldBeDisplayed(msg)) {
      m_textDisplay->textCursor().insertText(msg.text(), format(msg.priority()));
    }
  }
  moveCursorToEnd();
}

/**
 * Method to be called when a file's path is modified. Each Message object
 * associated with the file has its scriptPath attribute updated.
 * @param oldPath The path of the file being renamed
 * @param newPath The new path of the file
 */
void MessageDisplay::filePathModified(const QString &oldPath, const QString &newPath) {
  for (auto &msg : m_messageHistory) {
    if (msg.scriptPath() == oldPath)
      msg.setScriptPath(newPath);
  }
}

/**
 * Append a message to the message history. If the length of the history exceeds
 * the max line count, remove entries from the start until it does not.
 * @param msg The Message object to append
 */
void MessageDisplay::appendToHistory(const Message &msg) {
  m_messageHistory.append(msg);
  while (m_messageHistory.size() > maximumLineCount() && maximumLineCount() > 0)
    // Use .removeAt(0) since .removeFirst asserts a !.isEmpty() check
    m_messageHistory.removeAt(0);
}

//----------------------------------------------------------------------------------------
// Public slots
//----------------------------------------------------------------------------------------
/**
 * @param text The text string to append at PRIO_FATAL level
 */
void MessageDisplay::appendFatal(const QString &text) { this->append(Message(text, Message::Priority::PRIO_FATAL)); }

/**
 * @param text The text string to append at PRIO_ERROR level
 */
void MessageDisplay::appendError(const QString &text) { this->append(Message(text, Message::Priority::PRIO_ERROR)); }

/**
 * @param text The text string to append at PRIO_WARNING level
 */
void MessageDisplay::appendWarning(const QString &text) {
  this->append(Message(text, Message::Priority::PRIO_WARNING));
}

/**
 * @param text The text string to append at PRIO_NOTICE level
 */
void MessageDisplay::appendNotice(const QString &text) { this->append(Message(text, Message::Priority::PRIO_NOTICE)); }

/**
 * @param text The text string to append at PRIO_INFORMATION level
 */
void MessageDisplay::appendInformation(const QString &text) {
  this->append(Message(text, Message::Priority::PRIO_INFORMATION));
}

/**
 * @param text The text string to append at PRIO_DEBUG level
 */
void MessageDisplay::appendDebug(const QString &text) { this->append(Message(text, Message::Priority::PRIO_DEBUG)); }

/**
 * @param msg A message that is echoed to the display after the
 * current text
 */
void MessageDisplay::append(const Message &msg) {
  appendToHistory(msg);
  if (shouldBeDisplayed(msg) || msg.priority() <= Message::Priority::PRIO_WARNING) {
    QTextCursor cursor = moveCursorToEnd();
    cursor.insertText(msg.text(), format(msg.priority()));
    moveCursorToEnd();

    if (msg.priority() <= Message::Priority::PRIO_ERROR) {
      NotificationService::showMessage(parentWidget() ? parentWidget()->windowTitle() : "Mantid",
                                       "Sorry, there was an error, please look at the message display for "
                                       "details.",
                                       NotificationService::MessageIcon::Critical);
      emit errorReceived(msg.text());
    }
    if (msg.priority() <= Message::Priority::PRIO_WARNING)
      emit warningReceived(msg.text());
  }
}

/**
 * Append a message to the message window, adding the script name associated
 * with the message.
 * @param text The message to display in the window
 * @param priority The priority level of the message
 * @param filePath The path of the Python script being executed
 */
void MessageDisplay::appendPython(const QString &text, const int &priority, const QString &filePath) {
  Message msg = Message(text, static_cast<Message::Priority>(priority), filePath);
  append(msg);
}

/**
 * @param msg Replace the current contents with this message
 */
void MessageDisplay::replace(const Message &msg) {
  clear();
  append(msg.text());
}

/**
 * Clear all of the text
 */
void MessageDisplay::clear() {
  m_textDisplay->clear();
  m_messageHistory.clear();
}

/**
 * @returns The cursor at the end of the text
 */
QTextCursor MessageDisplay::moveCursorToEnd() {
  QTextCursor cursor(m_textDisplay->textCursor());
  cursor.movePosition(QTextCursor::End);
  m_textDisplay->setTextCursor(cursor);
  return cursor;
}

/**
 * @return True if scroll bar is at bottom, false otherwise
 */
bool MessageDisplay::isScrollbarAtBottom() const {
  return m_textDisplay->verticalScrollBar()->value() == m_textDisplay->verticalScrollBar()->maximum();
}

/**
 * Moves the cursor to the top of the document
 */
void MessageDisplay::scrollToTop() {
  // Code taken from QtCreator source
  m_textDisplay->verticalScrollBar()->setValue(m_textDisplay->verticalScrollBar()->minimum());
  // QPlainTextEdit destroys the first calls value in case of multiline
  // text, so make sure that the scroll bar actually gets the value set.
  // Is a noop if the first call succeeded.
  m_textDisplay->verticalScrollBar()->setValue(m_textDisplay->verticalScrollBar()->minimum());
}

/**
 * Moves the cursor to the bottom of the document
 */
void MessageDisplay::scrollToBottom() {
  // Code taken from QtCreator source
  m_textDisplay->verticalScrollBar()->setValue(m_textDisplay->verticalScrollBar()->maximum());
  // QPlainTextEdit destroys the first calls value in case of multiline
  // text, so make sure that the scroll bar actually gets the value set.
  // Is a noop if the first call succeeded.
  m_textDisplay->verticalScrollBar()->setValue(m_textDisplay->verticalScrollBar()->maximum());
}

//-----------------------------------------------------------------------------
// Private slot member functions
//-----------------------------------------------------------------------------

void MessageDisplay::showContextMenu(const QPoint &mousePos) {
  QMenu *menu{generateContextMenu()};
  menu->exec(this->mapToGlobal(mousePos));
  delete menu;
}

/*
 * @param priority An integer that must match the Poco::Message priority
 * enumeration
 */
void MessageDisplay::setLogLevel(int priority) { ConfigService::Instance().setLogLevel(priority); }

/**
 * Set the maximum number of blocks kept by the text edit
 */
void MessageDisplay::setScrollbackLimit() {
  constexpr int minLineCountAllowed(-1);
  setMaximumLineCount(
      QInputDialog::getInt(this, "", "No. of lines\n(-1 keeps all content)", maximumLineCount(), minLineCountAllowed));
}

// The text edit works in blocks but it is not entirely clear what a block
// is defined as. Experiments showed setting a max block count=1 suppressed
// all output and a min(block count)==2 was required to see a single line.
// We have asked the user for lines so add 1 to get the behaviour they
// would expect. Equally we subtract 1 for the value we show them to
// keep it consistent

/**
 * @return The maximum number of lines displayed in the text edit
 */
int MessageDisplay::maximumLineCount() const { return m_textDisplay->maximumBlockCount() - 1; }

/**
 * The maximum number of lines that are to be displayed in the text edit
 * @param count The new maximum number of lines to retain.
 */
void MessageDisplay::setMaximumLineCount(int count) { m_textDisplay->setMaximumBlockCount(count + 1); }

//-----------------------------------------------------------------------------
// Private non-slot member functions
//-----------------------------------------------------------------------------
QMenu *MessageDisplay::generateContextMenu() {
  QMenu *menu = m_textDisplay->createStandardContextMenu();
  menu->addSeparator();
  if (!m_textDisplay->document()->isEmpty()) {
    menu->addAction("Clear All", this, SLOT(clear()));
    menu->addSeparator();
  }
  menu->addAction("&Scrollback limit", this, SLOT(setScrollbackLimit()));
  menu->addSeparator();

  QMenu *logLevelMenu = menu->addMenu("&Log Level");
  logLevelMenu->addAction(m_error);
  logLevelMenu->addAction(m_warning);
  logLevelMenu->addAction(m_notice);
  logLevelMenu->addAction(m_information);
  logLevelMenu->addAction(m_debug);

  // check the right level
  int level = Poco::Logger::root().getLevel();
  if (level == Poco::Message::PRIO_ERROR)
    m_error->setChecked(true);
  if (level == Poco::Message::PRIO_WARNING)
    m_warning->setChecked(true);
  if (level == Poco::Message::PRIO_NOTICE)
    m_notice->setChecked(true);
  if (level == Poco::Message::PRIO_INFORMATION)
    m_information->setChecked(true);
  if (level >= Poco::Message::PRIO_DEBUG)
    m_debug->setChecked(true);
  return menu;
}

void MessageDisplay::initActions() {
  m_error->setCheckable(true);
  m_warning->setCheckable(true);
  m_notice->setCheckable(true);
  m_information->setCheckable(true);
  m_debug->setCheckable(true);

  m_loglevels->addAction(m_error);
  m_loglevels->addAction(m_warning);
  m_loglevels->addAction(m_notice);
  m_loglevels->addAction(m_information);
  m_loglevels->addAction(m_debug);

  m_logLevelMapping->setMapping(m_error, Poco::Message::PRIO_ERROR);
  m_logLevelMapping->setMapping(m_warning, Poco::Message::PRIO_WARNING);
  m_logLevelMapping->setMapping(m_notice, Poco::Message::PRIO_NOTICE);
  m_logLevelMapping->setMapping(m_information, Poco::Message::PRIO_INFORMATION);
  m_logLevelMapping->setMapping(m_debug, Poco::Message::PRIO_DEBUG);

  connect(m_error, SIGNAL(triggered()), m_logLevelMapping, SLOT(map()));
  connect(m_warning, SIGNAL(triggered()), m_logLevelMapping, SLOT(map()));
  connect(m_notice, SIGNAL(triggered()), m_logLevelMapping, SLOT(map()));
  connect(m_information, SIGNAL(triggered()), m_logLevelMapping, SLOT(map()));
  connect(m_debug, SIGNAL(triggered()), m_logLevelMapping, SLOT(map()));

  connect(m_logLevelMapping, SIGNAL(mapped(int)), this, SLOT(setLogLevel(int)));
}

/**
 * Sets up the internal map of text formatters for each log level
 */
void MessageDisplay::initFormats() {
  m_formats.clear();
  QTextCharFormat textFormat;

  textFormat.setForeground(Qt::red);
  m_formats[Message::Priority::PRIO_ERROR] = textFormat;

  textFormat.setForeground(QColor::fromRgb(255, 100, 0));
  m_formats[Message::Priority::PRIO_WARNING] = textFormat;

  textFormat.setForeground(Qt::gray);
  m_formats[Message::Priority::PRIO_INFORMATION] = textFormat;

  textFormat.setForeground(Qt::darkBlue);
  m_formats[Message::Priority::PRIO_NOTICE] = textFormat;
}

/**
 * Set the properties of the text display, i.e read-only
 * and make it occupy the whole widget
 * @param font A reference to the font for the text area
 */
void MessageDisplay::setupTextArea(const QFont &font) {
  m_textDisplay->setFont(font);
  m_textDisplay->setReadOnly(true);
  m_textDisplay->ensureCursorVisible();
  setMaximumLineCount(DEFAULT_LINE_COUNT_MAX);
  m_textDisplay->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  m_textDisplay->setMouseTracking(true);
  m_textDisplay->setUndoRedoEnabled(false);

  auto layoutBox = new QHBoxLayout(this);
  layoutBox->setContentsMargins(0, 0, 0, 0);
  layoutBox->addWidget(m_textDisplay);

  this->setFocusProxy(m_textDisplay);
  m_textDisplay->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_textDisplay, SIGNAL(customContextMenuRequested(const QPoint &)), this,
          SLOT(showContextMenu(const QPoint &)));
}

/**
 * @param priority An enumeration for the log level
 * @return format for given log level
 */
QTextCharFormat MessageDisplay::format(const Message::Priority priority) const {
  return m_formats.value(priority, QTextCharFormat());
}

/** Returns true if the given message should be displayed given the current
 * settings.
 * @param msg A Message object
 */
bool MessageDisplay::shouldBeDisplayed(const Message &msg) {
  if (((msg.scriptPath().isEmpty() && showFrameworkOutput()) ||
       (!msg.scriptPath().isEmpty() && showAllScriptOutput()) ||
       (showActiveScriptOutput() && (msg.scriptPath() == activeScript()))) &&
      !QCoreApplication::closingDown())
    return true;
  return false;
}
} // namespace MantidQt::MantidWidgets
