//-------------------------------------------
// Includes
//-------------------------------------------
#include "MantidQtMantidWidgets/MessageDisplay.h"

#include "MantidKernel/Logger.h"

#include <QAction>
#include <QActionGroup>
#include <QHBoxLayout>
#include <QMenu>
#include <QPoint>
#include <QSignalMapper>
#include <QTextEdit>

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
    : QWidget(parent), m_logLevelControl(DisableLogLevelControl), m_logChannel(new API::QtSignalChannel),
      m_textDisplay(new QTextEdit(this)), m_loglevels(new QActionGroup(this)), m_logLevelMapping(new QSignalMapper(this)),
      m_error(new QAction(tr("&Error"), this)), m_warning(new QAction(tr("&Warning"), this)),
      m_notice(new QAction(tr("&Notice"), this)), m_information(new QAction(tr("&Information"), this)),
      m_debug(new QAction(tr("&Debug"), this))
    {
      initActions();
      setupTextArea();
    }

    /**
     * @param logLevelControl Controls whether this display shows the right-click option to change
     * the global log level
     * @param parent An optional parent widget
     */
    MessageDisplay::MessageDisplay(LogLevelControl logLevelControl, QWidget *parent)
    : QWidget(parent), m_logLevelControl(logLevelControl), m_logChannel(new API::QtSignalChannel),
      m_textDisplay(new QTextEdit(this)), m_loglevels(new QActionGroup(this)), m_logLevelMapping(new QSignalMapper(this)),
      m_error(new QAction(tr("&Error"), this)), m_warning(new QAction(tr("&Warning"), this)),
      m_notice(new QAction(tr("&Notice"), this)), m_information(new QAction(tr("&Information"), this)),
      m_debug(new QAction(tr("&Debug"), this))
    {
      initActions();
      setupTextArea();
    }

    /**
     */
    MessageDisplay::~MessageDisplay()
    {
      // The Channel class is ref counted and will
      // delete itself when required
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
        splitChannel->addChannel(m_logChannel);
      }
      else
      {
        Poco::Logger::setChannel(rootLogger.name(), m_logChannel);
      }

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
      setTextColor(msg.priority());
      appendText(msg.text());
      //set the colour back to the default (black) for historical reasons
      setTextColor(Message::Priority::PRIO_INFORMATION);
      scrollToBottom();

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
     * Moves the cursor to the bottom of the document
     */
    void MessageDisplay::scrollToBottom()
    {
      QTextCursor cur = m_textDisplay->textCursor();
      cur.movePosition(QTextCursor::End);
      m_textDisplay->setTextCursor(cur);
    }

    //----------------------------------------------------------------------------------------
    // Protected members
    //----------------------------------------------------------------------------------------
    /**
     * @param event A QShowEvent object a parameterizing the event
     */
    void MessageDisplay::showEvent(QShowEvent * event)
    {
      Q_UNUSED(event);
      scrollToBottom();

      // Don't accept the event on purpose to allow parent to
      // do something if required.
    }

    //-----------------------------------------------------------------------------
    // Private slot member functions
    //-----------------------------------------------------------------------------

    void MessageDisplay::showContextMenu(const QPoint & mousePos)
    {
      QMenu * menu = m_textDisplay->createStandardContextMenu();
      if(!m_textDisplay->text().isEmpty()) menu->addAction("Clear",m_textDisplay, SLOT(clear()));

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
        int level = Mantid::Kernel::Logger::get("").getLevel(); //get the root logger logging level
        if (level == Poco::Message::PRIO_ERROR)
          m_error->setChecked(true);
        if (level == Poco::Message::PRIO_WARNING)
          m_warning->setChecked(true);
        if (level == Poco::Message::PRIO_NOTICE)
          m_notice->setChecked(true);
        if (level == Poco::Message::PRIO_INFORMATION)
          m_information->setChecked(true);
        if (level == Poco::Message::PRIO_DEBUG)
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
      Mantid::Kernel::Logger::setLevelForAll(priority);
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

      connect(m_logLevelMapping, SIGNAL(mapped(int)), m_logChannel, SLOT(setGlobalLogLevel(int)));
    }

    /**
     * Set the properties of the text display, i.e read-only
     * and make it occupy the whole widget
     */
    void MessageDisplay::setupTextArea()
    {
      m_textDisplay->setReadOnly(true);
      m_textDisplay->ensureCursorVisible();

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
     *  @param priority A priority enumeration
     */
    void MessageDisplay::setTextColor(const Message::Priority priority)
    {
      m_textDisplay->setTextColor(this->textColor(priority));
    }

    /**
     * @param priority A priority enumeration
     */
    QColor MessageDisplay::textColor(const Message::Priority priority) const
    {
      switch(priority)
      {
      case Message::Priority::PRIO_ERROR: return Qt::red;
        break;
      case Message::Priority::PRIO_WARNING: return QColor::fromRgb(255, 100, 0); //orange
        break;
      case Message::Priority::PRIO_INFORMATION: return Qt::gray;
        break;
      case Message::Priority::PRIO_NOTICE: return Qt::darkBlue;
        break;
      default: return Qt::black;
      }
    }

    /**
     * @param text Text to append
     */
    void MessageDisplay::appendText(const QString & text)
    {
      m_textDisplay->insertPlainText(text);
      QTextCursor cur = m_textDisplay->textCursor();
      cur.movePosition(QTextCursor::End);
      m_textDisplay->setTextCursor(cur);
    }
  }
}
