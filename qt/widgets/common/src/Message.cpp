//-------------------------------------------
// Includes
//-------------------------------------------
#include "MantidQtWidgets/Common/Message.h"

namespace MantidQt {
namespace MantidWidgets {
//-----------------------------------------------------------------------------------
// Public member functions
//-----------------------------------------------------------------------------------
/**
 * Constructs a blank message with notice priority
 * (here to satisfy Qt meta-type system)
 */
Message::Message() : QObject(), m_text(), m_priority(Priority::PRIO_NOTICE) {}

/**
 * @param text A QString containing the message text
 * @param priority A enumeration indicating the priority
 */
Message::Message(const QString &text, Priority priority)
    : QObject(), m_text(text), m_priority(priority) {}

/**
 * @param text A std::string containing the message text
 * @param priority A enumeration indicating the priority
 */
Message::Message(const std::string &text, Priority priority)
    : QObject(), m_text(QString::fromStdString(text)), m_priority(priority) {}

/**
 * @param text A c-style string containing the message text
 * @param priority A enumeration indicating the priority
 */
Message::Message(const char *text, Priority priority)
    : QObject(), m_text(text), m_priority(priority) {}

/**
 * Construct a message from another object
 */
Message::Message(const Message &msg)
    : QObject(), m_text(msg.text()), m_priority(msg.priority()) {}
} // namespace MantidWidgets
} // namespace MantidQt
