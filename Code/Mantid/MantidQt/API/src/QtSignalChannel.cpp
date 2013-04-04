#include "MantidQtAPI/QtSignalChannel.h"
#include "MantidQtAPI/Message.h"
#include "MantidKernel/Logger.h"

#include <Poco/Message.h>

namespace MantidQt
{
  namespace API
  {

    /**
     */
    QtSignalChannel::QtSignalChannel()
      :  QObject(), Poco::Channel()
    {
    }

    /**
     */
    QtSignalChannel::~QtSignalChannel()
    {

    }

    /**
     * @param msg A Poco message object containing a priority & the string message
     */
    void QtSignalChannel::log(const Poco::Message& msg)
    {
      emit messageReceived(API::Message(QString::fromStdString(msg.getText()), msg.getPriority()));
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
