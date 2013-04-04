#include "MantidQtAPI/QtSignalChannel.h"
#include "MantidKernel/RegistrationHelper.h"

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
      emit messageReceived(QString::fromStdString(msg.getText()));
    }

  }
}
