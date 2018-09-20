#include "MantidQtWidgets/Common/QtSignalChannel.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/Common/Message.h"

#include <Poco/Message.h>

namespace MantidQt {
namespace MantidWidgets {

/**
 * Creates a QtSignalChannel. This channel receives
 * Poco::Message objects and re-emits MantidQt Message objects
 * with the option to specify that only messages from a defined
 * source are emitted.
 * @param source A string specifying a source for the message
 */
QtSignalChannel::QtSignalChannel(const QString &source)
    : QObject(), Poco::Channel(), m_source(source) {}

/**
 */
QtSignalChannel::~QtSignalChannel() {}

/**
 * @param source A string specifying the required source for messages
 * that will be emitted
 */
void QtSignalChannel::setSource(const QString &source) { m_source = source; }

/**
 * If the source is set then only messages with a matching source
 * cause a Qt signal to be emitted. A newline is appended as the
 * Poco log stream emits the message when a newline is received but doesn't
 * actually send a newline character
 * @param msg A Poco message object containing a priority & the string message
 */
void QtSignalChannel::log(const Poco::Message &msg) {
  if (m_source.isEmpty() || this->source() == msg.getSource().c_str()) {
    emit messageReceived(Message(QString::fromStdString(msg.getText() + "\n"),
                                 msg.getPriority()));
  }
}

/*
 * @param priority An integer that must match the Poco::Message priority
 * enumeration
 */
void QtSignalChannel::setGlobalLogLevel(int priority) {
  using Mantid::Kernel::Logger;
  Logger::setLevelForAll(priority);
}
} // namespace MantidWidgets
} // namespace MantidQt
