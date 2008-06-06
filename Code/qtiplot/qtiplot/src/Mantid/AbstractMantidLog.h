#ifndef ABSTRACT_MANTIDLOG_H
#define ABSTRACT_MANTIDLOG_H

#ifndef QT_NO_KEYWORDS
#define QT_NO_KEYWORDS
#define DEFINE_SLOTS_AGAIN
#endif

#include <QObject>
#include "Poco/Message.h"

/** 

   class AbstractMantidLog connects to Mantid's SignalChannel
   Method log() receives the message from SignalChannel

*/
class AbstractMantidLog : public QObject
{
	//Q_OBJECT

public:

    /// Makes connection to SignalChannel.
    /// Channel's name must be signalChannel
	void connect();

protected:// Q_SLOTS:

    /// Receives message from SignalChannel.
    /// Does nothing, must be overriden in a child class
    virtual void log(const Poco::Message& msg);
};

#ifdef DEFINE_SLOTS_AGAIN
#undef QT_NO_KEYWORDS
//#define slots //  this is not very nice, a Qt's include file should do it
//#include <qobjectdefs.h>
#undef DEFINE_SLOTS_AGAIN
#endif

#endif /* ABSTRACT_MANTIDLOG_H */
