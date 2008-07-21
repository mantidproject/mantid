#include "../ApplicationWindow.h"
#include "MantidLog.h"

/// Posts message text to QtiPlot Result Log
void MantidLog::log(const Poco::Message& msg)
{
    QString str = msg.getText().c_str();
    if (s_appWin)
        s_appWin->updateLog(str+"\n");
    if (s_logEdit)
    {
        if (msg.getPriority() < Poco::Message::PRIO_WARNING)
            s_logEdit->setTextColor(Qt::red);
        else
            s_logEdit->setTextColor(Qt::black);
        s_logEdit->insertPlainText(str+"\n");
        QTextCursor cur = s_logEdit->textCursor();
        cur.movePosition(QTextCursor::End);
        s_logEdit->setTextCursor(cur);
    }
}

boost::shared_ptr<MantidLog> MantidLog::s_Instance;
ApplicationWindow* MantidLog::s_appWin = 0; 
QTextEdit* MantidLog::s_logEdit = 0; 
