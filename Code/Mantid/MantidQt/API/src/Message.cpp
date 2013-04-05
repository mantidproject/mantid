//-------------------------------------------
// Includes
//-------------------------------------------
#include "MantidQtAPI/Message.h"

namespace MantidQt
{
  namespace API
  {
    //-----------------------------------------------------------------------------------
    // Public member functions
    //-----------------------------------------------------------------------------------
    /**
     * Constructs a blank message with notice priority
     * (here to satisfy Qt meta-type system)
     */
    Message::Message() : QObject(), m_text(), m_priority(Priority::PRIO_NOTICE)
    {}

    /// Construct a message from a string with a given priority (default=notice)
    Message::Message(const QString & text, Priority priority)
      : QObject(), m_text(text), m_priority(priority)
    {}

    /**
     * Construct a message from another object
     */
    Message::Message(const Message & msg)
      : QObject(), m_text(msg.text()), m_priority(msg.priority())
    {
    }


  }
}
