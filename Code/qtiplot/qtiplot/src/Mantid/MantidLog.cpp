#include "MantidLog.h"
#include "MantidUI.h"
#include <iostream>
using namespace std;

void MantidLog::connect(MantidUI* mui)
{
    if (!s_Instance)
    {
        qRegisterMetaType<Poco::Message>();
        s_Instance.reset(new MantidLog);
        QObject::connect(s_Instance.get(),SIGNAL(messageReceived(const Poco::Message&)),mui,SLOT(logMessage(const Poco::Message&)), Qt::QueuedConnection);
        s_Instance->AbstractMantidLog::connect();
    }
}

/// Posts message text to QtiPlot Result Log
void MantidLog::log(const Poco::Message& msg)
{
    emit messageReceived(msg);
}

boost::shared_ptr<MantidLog> MantidLog::s_Instance;