//-------------------------------------------
// Includes
//-------------------------------------------
#include "MantidQtMantidWidgets/MessageDisplay.h"

#include "MantidKernel/Logger.h"

#include <QAction>
#include <QActionGroup>
#include <QHBoxLayout>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPoint>
#include <QScrollBar>
#include <QSettings>
#include <QSignalMapper>

#include <Poco/Logger.h>
#include <Poco/Message.h>
#include <Poco/SplitterChannel.h>


namespace MantidQt
{
  namespace MantidWidgets
  {
    using API::Message;

    //-------------------------------------------
    // Public member functions
    //-------------------------------------------
    /**
     * Constructs a widget that does not allow control over the global log level
     * @param parent An optional parent widget
     */
    MessageDisplay::MessageDisplay(QWidget *parent)
    : QWidget(parent), m_logLevelControl(DisableLogLevelControl), m_logChannel(new API::QtSignalChannel),m_filterChannel(new Poco::FilterChannel),
      m_textDisplay(new QPlainTextEdit(this)), m_formats(), m_loglevels(new QActionGroup(this)), m_logLevelMapping(new QSignalMapper(this)),
      m_error(new QAction(tr("&Error"), this)), m_warning(new QAction(tr("&Warning"), this)),
      m_notice(new QAction(tr("&Notice"), this)), m_information(new QAction(tr("&Information"), this)),
      m_debug(new QAction(tr("&Debug"), this))
    {
      initActions();
      initFormats();
      setupTextArea();
    }

    /**
     * @param logLevelControl Controls whether this display shows the right-click option to change
     * the global log level
     * @param parent An optional parent widget
     */
    MessageDisplay::MessageDisplay(LogLevelControl logLevelControl, QWidget *parent)
    : QWidget(parent), m_logLevelControl(logLevelControl), m_logChannel(new API::QtSignalChannel),m_filterChannel(new Poco::FilterChannel),
      m_textDisplay(new QPlainTextEdit(this)), m_loglevels(new QActionGroup(this)), m_logLevelMapping(new QSignalMapper(this)),
      m_error(new QAction(tr("&Error"), this)), m_warning(new QAction(tr("&Warning"), this)),
      m_notice(new QAction(tr("&Notice"), this)), m_information(new QAction(tr("&Information"), this)),
      m_debug(new QAction(tr("&Debug"), this))
    {
      initActions();
      initFormats();
      setupTextArea();
    }

    /**
     */
    MessageDisplay::~MessageDisplay()
    {
      QSettings settings;
      settings.setValue("MessageDisplayPriority",
                        static_cast<int>(m_filterChannel->getPriority()));
      // The Channel class is ref counted and will
      // delete itself when required
      m_filterChannel->release();
      m_logChannel->release();
      delete m_textDisplay;
    }

    /**
     * Attaches the Mantid logging framework
     * (Note the ConfigService must have already been started)
     */
    void MessageDisplay::attachLoggingChannel()
    {
      // Setup logging
      auto & rootLogger = Poco::Logger::root();
      auto * rootChannel = Poco::Logger::root().getChannel();
      // The root channel might be a SplitterChannel
      if(auto *splitChannel = dynamic_cast<Poco::SplitterChannel*>(rootChannel))
      {
        splitChannel->addChannel(m_filterChannel);
      }
      else
      {
        Poco::Logger::setChannel(rootLogger.name(), m_filterChannel);
      }
      m_filterChannel->addChannel(m_logChannel);
      
      QSettings settings;
      int priority =
          settings
              .value("MessageDisplayPriority", Message::Priority::PRIO_NOTICE)
              .toInt();
      m_filterChannel->setPriority(priority);

      connect(m_logChannel, SIGNAL(messageReceived(const Message&)),
          this, SLOT(append(const Message &)));
    }

    /**
     * @param source A string specifying the required source for messages
     * that will be emitted
     */
    void MessageDisplay::setSource(const QString & source)
    {
      m_logChannel->setSource(source);
    }

    //----------------------------------------------------------------------------------------
    // Public slots
    //----------------------------------------------------------------------------------------
    /**
     * @param text The text string to append at PRIO_FATAL level
     */
    void MessageDisplay::appendFatal(const QString & text)
    {
      this->append(Message(text, Message::Priority::PRIO_FATAL));
    }

    /**
     * @param text The text string to append at PRIO_ERROR level
     */
    void MessageDisplay::appendError(const QString & text)
    {
      this->append(Message(text, Message::Priority::PRIO_ERROR));
    }

    /**
     * @param text The text string to append at PRIO_WARNING level
     */
    void MessageDisplay::appendWarning(const QString & text)
    {
      this->append(Message(text, Message::Priority::PRIO_WARNING));
    }

    /**
     * @param text The text string to append at PRIO_NOTICE level
     */
    void MessageDisplay::appendNotice(const QString & text)
    {
      this->append(Message(text, Message::Priority::PRIO_NOTICE));
    }

    /**
     * @param text The text string to append at PRIO_INFORMATION level
     */
    void MessageDisplay::appendInformation(const QString & text)
    {
      this->append(Message(text, Message::Priority::PRIO_INFORMATION));
    }

    /**
     * @param text The text string to append at PRIO_DEBUG level
     */
    void MessageDisplay::appendDebug(const QString & text)
    {
      this->append(Message(text, Message::Priority::PRIO_DEBUG));
    }

    /**
     * @param msg A message that is echoed to the display after the
     * current text
     */
    void MessageDisplay::append(const Message & msg)
    {
      QTextCursor cursor = moveCursorToEnd();
      cursor.insertText(msg.text(), format(msg.priority()));
      moveCursorToEnd();

      if(msg.priority() <= Message::Priority::PRIO_ERROR ) emit errorReceived(msg.text());
      if(msg.priority() <= Message::Priority::PRIO_WARNING ) emit warningReceived(msg.text());
    }

    /**
     * @param msg Replace the current contents with this message
     */
    void MessageDisplay::replace(const Message & msg)
    {
      clear();
      append(msg.text());
    }

    /**
     * Clear all of the text
     */
    void MessageDisplay::clear()
    {
      m_textDisplay->clear();
    }

    /**
     * @returns The cursor at the end of the text
     */
    QTextCursor MessageDisplay::moveCursorToEnd()
    {
      QTextCursor cursor( m_textDisplay->textCursor());
      cursor.movePosition(QTextCursor::End);
      m_textDisplay->setTextCursor(cursor);
      return cursor;
    }

    /**
     * @return True if scroll bar is at bottom, false otherwise
     */
    bool MessageDisplay::isScrollbarAtBottom() const
    {
      return m_textDisplay->verticalScrollBar()->value() == m_textDisplay->verticalScrollBar()->maximum();
    }

    /**
     * Moves the cursor to the top of the document
     */
    void MessageDisplay::scrollToTop()
    {
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
    void MessageDisplay::scrollToBottom()
    {
      // Code taken from QtCreator source
      m_textDisplay->verticalScrollBar()->setValue(m_textDisplay->verticalScrollBar()->maximum());
      // QPlainTextEdit destroys the first calls value in case of multiline
      // text, so make sure that the scroll bar actually gets the value set.
      // Is a noop if the first call succeeded.
      m_textDisplay->verticalScrollBar()->setValue(m_textDisplay->verticalScrollBar()->maximum());
    }

    //----------------------------------------------------------------------------------------
    // Protected members
    //----------------------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    // Private slot member functions
    //-----------------------------------------------------------------------------

    void MessageDisplay::showContextMenu(const QPoint & mousePos)
    {
      QMenu * menu = m_textDisplay->createStandardContextMenu();
      if(!m_textDisplay->document()->isEmpty()) menu->addAction("Clear",m_textDisplay, SLOT(clear()));

      if(m_logLevelControl == MessageDisplay::EnableLogLevelControl)
      {
        menu->addSeparator();
        QMenu *logLevelMenu = menu->addMenu("&Log Level");
        logLevelMenu->addAction(m_error);
        logLevelMenu->addAction(m_warning);
        logLevelMenu->addAction(m_notice);
        logLevelMenu->addAction(m_information);
        logLevelMenu->addAction(m_debug);

        //check the right level
        int level = m_filterChannel->getPriority(); 
        //get the root logger logging level
        int rootLevel = Poco::Logger::root().getLevel();
        if (rootLevel < level) {
          level = rootLevel;
        }

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
      }

      menu->exec(this->mapToGlobal(mousePos));
      delete menu;
    }

    /*
     * @param priority An integer that must match the Poco::Message priority
     * enumeration
     */
    void MessageDisplay::setGlobalLogLevel(int priority)
    {
      //set Local filter level
      m_filterChannel->setPriority(priority);
      //if this is higher than the global level then that will have to be lowered to work
      int rootLevel = Poco::Logger::root().getLevel();
      if (rootLevel < priority)
      {
         Mantid::Kernel::Logger::setLevelForAll(priority);
      }
    }

    //-----------------------------------------------------------------------------
    // Private non-slot member functions
    //-----------------------------------------------------------------------------
    /*
     */
    void MessageDisplay::initActions()
    {
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

      connect(m_error, SIGNAL(activated()), m_logLevelMapping, SLOT (map()));
      connect(m_warning, SIGNAL(activated()), m_logLevelMapping, SLOT (map()));
      connect(m_notice, SIGNAL(activated()), m_logLevelMapping, SLOT (map()));
      connect(m_information, SIGNAL(activated()), m_logLevelMapping, SLOT (map()));
      connect(m_debug, SIGNAL(activated()), m_logLevelMapping, SLOT (map()));

      connect(m_logLevelMapping, SIGNAL(mapped(int)), this, SLOT(setGlobalLogLevel(int)));
    }

    /**
     * Sets up the internal map of text formatters for each log level
     */
    void MessageDisplay::initFormats()
    {
      m_formats.clear();
      QTextCharFormat format;

      format.setForeground(Qt::red);
      m_formats[Message::Priority::PRIO_ERROR] = format;

      format.setForeground(QColor::fromRgb(255, 100, 0));
      m_formats[Message::Priority::PRIO_WARNING] = format;

      format.setForeground(Qt::gray);
      m_formats[Message::Priority::PRIO_INFORMATION] = format;

      format.setForeground(Qt::darkBlue);
      m_formats[Message::Priority::PRIO_NOTICE] = format;
    }

    /**
     * Set the properties of the text display, i.e read-only
     * and make it occupy the whole widget
     */
    void MessageDisplay::setupTextArea()
    {
      m_textDisplay->setReadOnly(true);
      m_textDisplay->ensureCursorVisible();
      m_textDisplay->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
      m_textDisplay->setMouseTracking(true);
      m_textDisplay->setUndoRedoEnabled(false);

      this->setLayout(new QHBoxLayout(this));
      QLayout *layout = this->layout();
      layout->setContentsMargins(0,0,0,0);
      layout->addWidget(m_textDisplay);

      this->setFocusProxy(m_textDisplay);
      m_textDisplay->setContextMenuPolicy(Qt::CustomContextMenu);
      connect(m_textDisplay, SIGNAL(customContextMenuRequested(const QPoint&)),
          this, SLOT(showContextMenu(const QPoint&)));
    }

    /**
     * @param priority An enumeration for the log level
     * @return format for given log level
     */
    QTextCharFormat MessageDisplay::format(const API::Message::Priority priority) const
    {
      return m_formats.value(priority,QTextCharFormat());
    }

  }
}
