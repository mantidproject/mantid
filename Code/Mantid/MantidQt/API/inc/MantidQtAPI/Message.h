#ifndef MESSAGE_H_
#define MESSAGE_H_

//----------------------------------
// Includes
//----------------------------------
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

      /// Construct a message from a string with a given priority (default=notice)
      Message(const QString & text, Priority priority=Priority::PRIO_NOTICE)
        : m_text(text), m_priority(priority)
      {}

    public:

      /// @returns The message text
      inline QString text() const {return m_text;}
      /// @returns The message priority
      inline Priority priority() const {return m_priority;}

    private:
      DISABLE_DEFAULT_CONSTRUCT(Message);

      QString m_text;
      Priority m_priority;
    };

  }
}

#endif //MESSAGE_H_
