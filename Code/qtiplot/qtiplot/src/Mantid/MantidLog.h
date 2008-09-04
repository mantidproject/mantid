#ifndef MANTIDLOG_H
#define MANTIDLOG_H

#include "AbstractMantidLog.h"
#include <boost/shared_ptr.hpp>

#include <QMutex>

class ApplicationWindow;
class QTextEdit;

class MantidLog: public AbstractMantidLog
{
	//Q_OBJECT

public:

    static void connect(ApplicationWindow* w)
    {
        if (!s_Instance)
        {
            s_Instance.reset(new MantidLog);
            s_appWin = w;
            s_logEdit = 0;
            s_Instance->AbstractMantidLog::connect();
        }
        if (w) s_appWin = w;
    }
    static void connect(QTextEdit* te)
    {
        if (!s_Instance)
        {
            s_Instance.reset(new MantidLog);
            s_appWin = 0;
            s_logEdit = te;
            s_Instance->AbstractMantidLog::connect();
        }
        if (te) s_logEdit = te;
    }
    static long count()
    {
        return s_Instance.use_count();
    }

protected:// slots:

    void log(const Poco::Message& msg);

private:

    static boost::shared_ptr<MantidLog> s_Instance;
    static ApplicationWindow* s_appWin;
    static QTextEdit* s_logEdit;
    static QMutex s_mutex;

};


#endif /* MANTIDLOG_H */
