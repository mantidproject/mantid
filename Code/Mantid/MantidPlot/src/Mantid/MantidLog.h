#ifndef MANTIDLOG_H
#define MANTIDLOG_H

#include <QtCore>
#include "AbstractMantidLog.h"
#include <boost/shared_ptr.hpp>

class MantidUI;

class MantidLog: public AbstractMantidLog
{
	Q_OBJECT

public:
    static void connect(MantidUI* mui);
    static long count()
    {
        return s_Instance.use_count();
    }
signals:
    void messageReceived(const Poco::Message& msg);
protected:// slots:

    void log(const Poco::Message& msg);

private:

    static boost::shared_ptr<MantidLog> s_Instance;

};

Q_DECLARE_METATYPE(Poco::Message)

#endif /* MANTIDLOG_H */
