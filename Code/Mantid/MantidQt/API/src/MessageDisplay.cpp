//-------------------------------------------
// Includes
//-------------------------------------------
#include "MantidQtAPI/MessageDisplay.h"

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
  namespace API
  {

    //-------------------------------------------
    // Public member functions
    //-------------------------------------------
    /**
     * @param parent An optional parent widget
     */
    MessageDisplay::MessageDisplay(QWidget *parent)
    : QWidget(parent), m_logChannel(NULL), m_textDisplay(new QTextEdit(this)),
      m_loglevels(new QActionGroup(this)), m_logLevelMapping(new QSignalMapper(this)),
      m_error(new QAction(tr("&Error"), this)), m_warning(new QAction(tr("&Warning"), this)),
      m_notice(new QAction(tr("&Notice"), this)), m_information(new QAction(tr("&Information"), this)),
      m_debug(new QAction(tr("&Debug"), this))
    {
      initActions();
      setupTextArea();
    }

    /**
     * Attaches the Mantid logging framework
     * (Note the ConfigService must have already been started)
     */
    void MessageDisplay::attachLoggingChannel()
    {
      // Setup logging
      m_logChannel = new QtSignalChannel;
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

      connect(m_logChannel, SIGNAL(messageReceived(const QString&)),
          this, SLOT(displayMessage(const QString &)));

    }


    /**
     * @param A message that is echoed to the display
     */
    void MessageDisplay::displayMessage(const QString & msg)
    {
      m_textDisplay->append(msg);
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

    //-----------------------------------------------------------------------------
    // Private slot member functions
    //-----------------------------------------------------------------------------

    void MessageDisplay::showContextMenu(const QPoint & mousePos)
    {
      QMenu * menu = m_textDisplay->createStandardContextMenu();
      if(!m_textDisplay->text().isEmpty()) menu->addAction("Clear",m_textDisplay, SLOT(clear()));

      // Change the log level
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

      menu->exec(this->mapToGlobal(mousePos));
      delete menu;

      //      QMenu *menu = results->createStandardContextMenu();
//      if(!menu) return;
//      if(results->text().isEmpty())
//      {
//        actionClearLogInfo->setEnabled(false);
//      }
//      else
//      {
//        actionClearLogInfo->setEnabled(true);
//      }
//
//      menu->addAction(actionClearLogInfo);
//      //Mantid log level changes
//      QMenu *logLevelMenu = menu->addMenu("&Log Level");
//      logLevelMenu->addAction(actionLogLevelError);
//      logLevelMenu->addAction(actionLogLevelWarning);
//      logLevelMenu->addAction(actionLogLevelNotice);
//      logLevelMenu->addAction(actionLogLevelInformation);
//      logLevelMenu->addAction(actionLogLevelDebug);
//
//      //check the right level
//      int level = Mantid::Kernel::Logger::get("").getLevel(); //get the root logger logging level
//      if (level == Poco::Message::PRIO_ERROR)
//        actionLogLevelError->setChecked(true);
//      if (level == Poco::Message::PRIO_WARNING)
//        actionLogLevelWarning->setChecked(true);
//      if (level == Poco::Message::PRIO_NOTICE)
//        actionLogLevelNotice->setChecked(true);
//      if (level == Poco::Message::PRIO_INFORMATION)
//        actionLogLevelInformation->setChecked(true);
//      if (level == Poco::Message::PRIO_DEBUG)
//        actionLogLevelDebug->setChecked(true);

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

      connect(m_logLevelMapping, SIGNAL(mapped(int)), this, SLOT(setGlobalLogLevel(int)));
    }

    /**
     * Set the properties of the text display, i.e read-only
     * and make it occupy the whole widget
     */
    void MessageDisplay::setupTextArea()
    {
      m_textDisplay->setReadOnly(true);

      this->setLayout(new QHBoxLayout(this));
      QLayout *layout = this->layout();
      layout->setContentsMargins(0,0,0,0);
      layout->addWidget(m_textDisplay);

      this->setFocusProxy(m_textDisplay);
      m_textDisplay->setContextMenuPolicy(Qt::CustomContextMenu);
      connect(m_textDisplay, SIGNAL(customContextMenuRequested(const QPoint&)),
          this, SLOT(showContextMenu(const QPoint&)));
    }


  }
}
