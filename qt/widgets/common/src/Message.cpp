// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
Message::Message()
    : QObject(), m_text(), m_priority(Priority::PRIO_NOTICE),
      m_frameworkMsg(true) {}

/**
 * @param text A QString containing the message text
 * @param priority A enumeration indicating the priority
 */
Message::Message(const QString &text, Priority priority, bool frameworkMsg)
    : QObject(), m_text(text), m_priority(priority),
      m_frameworkMsg(frameworkMsg) {}

/**
 * @param text A std::string containing the message text
 * @param priority A enumeration indicating the priority
 */
Message::Message(const std::string &text, Priority priority, bool frameworkMsg)
    : QObject(), m_text(QString::fromStdString(text)), m_priority(priority),
      m_frameworkMsg(frameworkMsg) {}

/**
 * @param text A c-style string containing the message text
 * @param priority A enumeration indicating the priority
 */
Message::Message(const char *text, Priority priority, bool frameworkMsg)
    : QObject(), m_text(text), m_priority(priority),
      m_frameworkMsg(frameworkMsg) {}

/**
 * Construct a message from another object
 */
Message::Message(const Message &msg)
    : QObject(), m_text(msg.text()), m_priority(msg.priority()),
      m_frameworkMsg(msg.frameworkMsg()) {}
} // namespace MantidWidgets
} // namespace MantidQt
