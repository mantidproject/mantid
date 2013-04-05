#ifndef MESSAGE_H_
#define MESSAGE_H_

//----------------------------------
// Includes
//----------------------------------
#include <QMetaType>
#include <QObject>
#include <QString>

#include "MantidKernel/ClassMacros.h"
#include "MantidKernel/Logger.h" // So we can match the Logger priority

//----------------------------------------------------------
// Forward declarations
//----------------------------------------------------------

namespace MantidQt
{
  namespace API
  {
    /** @class Message
     * Provides a simple binding of a text message with a priority
     */
    class Message : public QObject
    {
      // No Q_Object macro by design

    public:
      /// Priority matches Mantid Logger priority
      typedef Mantid::Kernel::Logger::Priority Priority;

      /// Default constuctor required by Qt meta-type system
      Message();
      /// Construct a message from a string with a given priority (default=notice)
      Message(const QString & text, Priority priority=Priority::PRIO_NOTICE);
      /// Copy constructor
      Message(const Message & msg);

    public:
      /// @returns The message text
      inline QString text() const {return m_text;}
      /// @returns The message priority
      inline Priority priority() const {return m_priority;}

    private:
      QString m_text;
      Priority m_priority;
    };
  }
}

/// Required to operate in signals/slots
Q_DECLARE_METATYPE(MantidQt::API::Message);

#endif //MESSAGE_H_
