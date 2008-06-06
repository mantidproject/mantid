#ifndef MANTIDLOG_H
#define MANTIDLOG_H

#include "AbstractMantidLog.h"
#include <boost/shared_ptr.hpp>

class ApplicationWindow;

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
            s_Instance->AbstractMantidLog::connect();
        }
        if (w) s_appWin = w;
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

};


#endif /* MANTIDLOG_H */
