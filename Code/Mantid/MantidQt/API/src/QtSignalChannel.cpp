#include "MantidQtAPI/QtSignalChannel.h"
#include "MantidQtAPI/Message.h"
#include "MantidKernel/Logger.h"

#include <Poco/Message.h>

namespace MantidQt
{
  namespace API
  {

    /**
     * Creates a QtSignalChannel. This channel receives
     * Poco::Message objects and re-emits MantidQt Message objects
     * with the option to specify that only messages from a defined
     * source are emitted.
     * @param source A string specifying a source for the message
     */
    QtSignalChannel::QtSignalChannel(const QString & source)
      :  QObject(), Poco::Channel(), m_source(source)
    {
    }

    /**
     */
    QtSignalChannel::~QtSignalChannel()
    {
    }

    /**
     * @param source A string specifying the required source for messages
     * that will be emitted
     */
    void QtSignalChannel::setSource(const QString & source)
    {
      m_source = source;
    }

    /**
     * If the source is set then only messages with a matching source
     * cause a Qt signal to be emitted
     * @param msg A Poco message object containing a priority & the string message
     */
    void QtSignalChannel::log(const Poco::Message& msg)
    {
      if(m_source.isEmpty() || this->source() == msg.getSource().c_str())
      {
        emit messageReceived(API::Message(QString::fromStdString(msg.getText()), msg.getPriority()));
      }
    }

    /*
     * @param priority An integer that must match the Poco::Message priority enumeration
     */
    void QtSignalChannel::setGlobalLogLevel(int priority)
    {
      using Mantid::Kernel::Logger;
      Logger::setLevelForAll(priority);
    }

  }
}
