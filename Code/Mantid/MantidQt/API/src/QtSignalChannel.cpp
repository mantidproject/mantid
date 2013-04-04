#include "MantidQtAPI/QtSignalChannel.h"
#include "MantidKernel/Logger.h"

#include <Poco/Message.h>

#include <sstream>

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
      emit messageReceived(QString::fromStdString(msg.getText()));
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
