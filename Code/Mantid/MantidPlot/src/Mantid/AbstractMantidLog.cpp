#include "AbstractMantidLog.h"

#include "MantidKernel/SignalChannel.h"

#include <Poco/LoggingRegistry.h>
#include <boost/bind.hpp>

#include <QMessageBox>

void AbstractMantidLog::connect()
{
    try
    {
        Poco::SignalChannel *pChannel = dynamic_cast<Poco::SignalChannel*>(Poco::LoggingRegistry::defaultRegistry().channelForName("signalChannel"));
        if (!pChannel) 
        {
            QMessageBox::warning(0,"MantidLog","Channel is of wrong type");
            return;
        }
        pChannel->sig().connect(boost::bind(&AbstractMantidLog::log,this,_1));
    }
    catch(...)
    {
        QMessageBox::warning(0,"MantidLog","Channel signalChannel not found");
        return;
    }
}

void AbstractMantidLog::log(const Poco::Message& msg)
{
  (void) msg;
}

