#include "../ApplicationWindow.h"
#include "MantidLog.h"

/// Posts message text to QtiPlot Result Log
void MantidLog::log(const Poco::Message& msg)
{
    QString str = msg.getText().c_str();
    s_appWin->updateLog(str+"\n");
}

boost::shared_ptr<MantidLog> MantidLog::s_Instance;
ApplicationWindow* MantidLog::s_appWin = 0; 
