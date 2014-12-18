//-----------------------------------------------
// Includes
//-----------------------------------------------
#include "MantidKernel/ThreadSafeLogStream.h"
#include "MantidKernel/MultiThreaded.h"
#include <iostream>
#include <Poco/Thread.h>

using namespace Mantid::Kernel;

//************************************************************
// ThreadSafeLogStreamBuf
//************************************************************

/**
 * Constructor
 */
ThreadSafeLogStreamBuf::ThreadSafeLogStreamBuf(Poco::Logger &logger,
                                               Poco::Message::Priority priority)
    : Poco::LogStreamBuf(logger, priority), m_messages() {}

/**
 * Destructor
 */
ThreadSafeLogStreamBuf::~ThreadSafeLogStreamBuf() {}

int ThreadSafeLogStreamBuf::overflow(char c) {
  return Poco::UnbufferedStreamBuf::overflow(c);
}

/**
 * If the character is an EOL character then write the buffered messsage to the
 * chosen device(s). If
 * not, buffer the character
 * @param c :: The input character
 * @returns The ASCII code of the input character
 */
int ThreadSafeLogStreamBuf::writeToDevice(char c) {
  if (c == '\n' || c == '\r') {
    Poco::Message msg(logger().name(), m_messages[Poco::Thread::currentTid()],
                      getPriority());
    m_messages[Poco::Thread::currentTid()] = "";
    logger().log(msg);
  } else {
    Poco::FastMutex::ScopedLock lock(m_mutex);
    m_messages[Poco::Thread::currentTid()] += c;
  }
  return static_cast<int>(c);
}

//************************************************************
// ThreadSafeLogIOS
//************************************************************
/**
 * Constructor
 * @param logger :: A reference to the logger associated with this stream
 * @param priority :: The stream priority
 */
ThreadSafeLogIOS::ThreadSafeLogIOS(Poco::Logger &logger,
                                   Poco::Message::Priority priority)
    : m_buf(logger, priority) {
  poco_ios_init(&m_buf);
}

/**
 * Destructor
 */
ThreadSafeLogIOS::~ThreadSafeLogIOS() {}

/**
 * Return the underlying buffer for this stream
 * @returns The thread-safe buffer associated with this stream
 */
Poco::LogStreamBuf *ThreadSafeLogIOS::rdbuf() { return &m_buf; }

//************************************************************
// ThreadSafeLogStream
//************************************************************
/**
 * Constructor
 * @param logger :: A reference to the logger associated with this stream
 * @param priority :: The stream priority
 */
ThreadSafeLogStream::ThreadSafeLogStream(Poco::Logger &logger,
                                         Poco::Message::Priority priority)
    : ThreadSafeLogIOS(logger, priority), std::ostream(&m_buf) {}

/**
 * Constructor taking a name for a logger
 * @param loggerName :: A name for the logger stream
 * @param priority :: The stream priority
 */
ThreadSafeLogStream::ThreadSafeLogStream(const std::string &loggerName,
                                         Poco::Message::Priority priority)
    : ThreadSafeLogIOS(Poco::Logger::get(loggerName), priority),
      std::ostream(&m_buf) {}

/**
 * Destructor
 */
ThreadSafeLogStream::~ThreadSafeLogStream() {}

/**
 * Return a reference to the log stream with the priority set to fatal
 * @returns A reference to the log stream with fatal priority level
 */
ThreadSafeLogStream &ThreadSafeLogStream::fatal() {
  return priority(Poco::Message::PRIO_FATAL);
}

/**
 * Log a message as fatal and return a reference to the log stream with the
 * priority set to fatal
 * @param message :: The string to send to the logger
 * @returns A reference to the log stream with fatal priority level
 */
ThreadSafeLogStream &ThreadSafeLogStream::fatal(const std::string &message) {
  m_buf.logger().fatal(message);
  return priority(Poco::Message::PRIO_FATAL);
}

/**
 * Return a reference to the log stream with the priority set to critical
 * @returns A reference to the log stream with critical priority level
 */
ThreadSafeLogStream &ThreadSafeLogStream::critical() {
  return priority(Poco::Message::PRIO_CRITICAL);
}

/**
 * Log a message as critical and return a reference to the log stream with the
 * priority set to critical
 * @param message :: The string to send to the logger
 * @returns A reference to the log stream with critical priority level
 */
ThreadSafeLogStream &ThreadSafeLogStream::critical(const std::string &message) {
  m_buf.logger().critical(message);
  return priority(Poco::Message::PRIO_CRITICAL);
}

/**
 * Return a reference to the log stream with the priority set to error
 * @returns A reference to the log stream with error priority level
 */
ThreadSafeLogStream &ThreadSafeLogStream::error() {
  return priority(Poco::Message::PRIO_ERROR);
}

/**
 * Log a message as error and return a reference to the log stream with the
 * priority set to error
 * @param message :: The string to send to the logger
 * @returns A reference to the log stream with error priority level
 */
ThreadSafeLogStream &ThreadSafeLogStream::error(const std::string &message) {
  m_buf.logger().error(message);
  return priority(Poco::Message::PRIO_ERROR);
}

/**
 * Return a reference to the log stream with the priority set to warning
 * @returns A reference to the log stream with warning priority level
 */
ThreadSafeLogStream &ThreadSafeLogStream::warning() {
  return priority(Poco::Message::PRIO_WARNING);
}

/**
 * Log a message as a warning and return a reference to the log stream with the
 * priority set to warning
 * @param message :: The string to send to the logger
 * @returns A reference to the log stream with warning priority level
 */
ThreadSafeLogStream &ThreadSafeLogStream::warning(const std::string &message) {
  m_buf.logger().warning(message);
  return priority(Poco::Message::PRIO_WARNING);
}

/**
 * Return a reference tothe log stream with the priority set to notice
 * @returns A reference to the log stream with notice priority level
 */
ThreadSafeLogStream &ThreadSafeLogStream::notice() {
  return priority(Poco::Message::PRIO_NOTICE);
}

/**
 * Log a message as a notice and return a reference to the log stream with the
 * priority set to notice
 * @param message :: The string to send to the logger
 * @returns A reference to the log stream with notice priority level
 */
ThreadSafeLogStream &ThreadSafeLogStream::notice(const std::string &message) {
  m_buf.logger().notice(message);
  return priority(Poco::Message::PRIO_NOTICE);
}

/**
 * Return a reference to the log stream with the priority set to information
 * @returns A reference to the log stream with information priority level
 */
ThreadSafeLogStream &ThreadSafeLogStream::information() {
  return priority(Poco::Message::PRIO_INFORMATION);
}

/**
 * Log a message as information and return a reference to the log stream with
 * the priority set to information
 * @param message :: The string to send to the logger
 * @returns A reference to the log stream with information  priority level
 */
ThreadSafeLogStream &
ThreadSafeLogStream::information(const std::string &message) {
  m_buf.logger().information(message);
  return priority(Poco::Message::PRIO_INFORMATION);
}

/**
 * Return a reference to the log stream with the priority set to debug
 * @returns A reference to the log stream with debug priority level
 */
ThreadSafeLogStream &ThreadSafeLogStream::debug() {
  return priority(Poco::Message::PRIO_DEBUG);
}

/**
 * Log a message as debug  and return a reference to the log stream with the
 * priority set to debug
 * @param message :: The string to send to the logger
 * @returns A reference to the log stream with debug priority level
 */
ThreadSafeLogStream &ThreadSafeLogStream::debug(const std::string &message) {
  m_buf.logger().debug(message);
  return priority(Poco::Message::PRIO_DEBUG);
}

/**
 * Return a reference to the log stream with the priority set at the given level
 * @param priority :: The priority level
 * @returns A reference to the log stream with the given priority level
 */
ThreadSafeLogStream &
ThreadSafeLogStream::priority(Poco::Message::Priority priority) {
  m_buf.setPriority(priority);
  return *this;
}
