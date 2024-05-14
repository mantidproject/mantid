// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/Logger.h"

#include <Poco/Logger.h>
#include <Poco/NullStream.h>

#include <algorithm>
#include <exception>
#include <iostream>
#include <sstream>

namespace Mantid::Kernel {
namespace {
// We only need a single NullStream object
Poco::NullOutputStream NULL_STREAM;

int fixLevel(const int level) {
  if (level < 0)
    return 0;
  else if (level >= int(Logger::PriorityNames.size()))
    return int(Logger::PriorityNames.size()) - 1;
  else
    return level;
}

int toLevel(const std::string &level) {
  // convert the string to uppercase
  std::string lowercase(level); // make sure space is allocated
  std::transform(level.cbegin(), level.cend(), lowercase.begin(), [](unsigned char c) { return std::tolower(c); });

  // see if it is an alias for "none"
  if (lowercase == "not_used") {
    lowercase = "none";
  }

  // look for it in the list already
  const auto iter = std::find(Logger::PriorityNames.cbegin(), Logger::PriorityNames.cend(), lowercase);
  if (iter != Logger::PriorityNames.cend()) {
    return int(std::distance(Logger::PriorityNames.cbegin(), iter));
  }

  // didn't find it so error out
  std::stringstream msg;
  msg << "Do not know how to convert \"" << level << "\" to an integer level";
  throw std::runtime_error(msg.str());
}

} // anonymous namespace

// "none" effectively turns off logging
const std::array<std::string, 9> Logger::PriorityNames{"none",   "fatal",       "critical", "error", "warning",
                                                       "notice", "information", "debug",    "trace"};

/** Constructor
 * @param name :: The class name invoking this logger
 */
Logger::Logger(const std::string &name)
    : m_log(&Poco::Logger::get(name)), m_logStream(std::make_unique<ThreadSafeLogStream>(*m_log)), m_levelOffset(0),
      m_enabled(true) {}

/**
 * @param name The new name
 */
void Logger::setName(const std::string &name) {
  auto *logger = &Poco::Logger::get(name);
  auto logStream = std::make_unique<ThreadSafeLogStream>(*logger); // don't swap if this throws

  using std::swap;
  swap(m_log, logger);
  swap(m_logStream, logStream);
}

/** Returns true if the log is enabled
 *
 *  @retval true - logging is enabled
 *  @retval false - all messages are ignored.
 */
bool Logger::getEnabled() const { return m_enabled; }

/** set if the logging is enabled
 *
 *  @param enabled ::  true - logging is enabled, false - all messages are
 *ignored.
 */
void Logger::setEnabled(const bool enabled) { m_enabled = enabled; }

/** If the Logger's log level is at least Poco::Message::PRIO_FATAL, creates a
 *Message with
 *  priority Poco::Message::PRIO_FATAL and the given message text and sends it
 *to the attached channel.
 *
 *  @param msg :: The message to log.
 */
void Logger::fatal(const std::string &msg) { log(msg, Poco::Message::PRIO_FATAL); }

/** If the Logger's log level is at least Poco::Message::PRIO_ERROR, creates a
 *Message with priority
 *  Poco::Message::PRIO_ERROR and the given message text and sends it to the
 *attached channel.
 *
 *  @param msg :: The message to log.
 */
void Logger::error(const std::string &msg) { log(msg, Poco::Message::PRIO_ERROR); }

/** If the Logger's log level is at least Poco::Message::PRIO_WARNING, creates a
 *Message with
 *  priority Poco::Message::PRIO_WARNING and the given message text and sends it
 *to the attached channel.
 *
 *  @param msg :: The message to log.
 */
void Logger::warning(const std::string &msg) { log(msg, Poco::Message::PRIO_WARNING); }

/** If the Logger's log level is at least Poco::Message::PRIO_NOTICE, creates a
 *Message with
 *  priority Poco::Message::PRIO_NOTICE and the given message text and sends it
 *to the attached channel.
 *
 *  @param msg :: The message to log.
 */
void Logger::notice(const std::string &msg) { log(msg, Poco::Message::PRIO_NOTICE); }

/** If the Logger's log level is at least Poco::Message::PRIO_INFORMATION,
 *creates a Message with
 *  priority Poco::Message::PRIO_INFORMATION and the given message text and
 *sends it to the
 *  attached channel.
 *
 *  @param msg :: The message to log.
 */
void Logger::information(const std::string &msg) { log(msg, Poco::Message::PRIO_INFORMATION); }

/** If the Logger's log level is at least Poco::Message::PRIO_DEBUG, creates a
 *Message with priority
 *  Poco::Message::PRIO_DEBUG and the given message text and sends it to the
 *attached channel.
 *
 *  @param msg :: The message to log.
 */
void Logger::debug(const std::string &msg) { log(msg, Poco::Message::PRIO_DEBUG); }

/** Logs the given message at debug level, followed by the data in buffer.
 *
 *  The data in buffer is written in canonical hex+ASCII form:
 *  Offset (4 bytes) in hexadecimal, followed by sixteen space-separated,
 *  two column, hexadecimal bytes, followed by the same sixteen bytes as
 *  ASCII characters.
 *  For bytes outside the range 32 .. 127, a dot is printed.
 *  Note all Dump messages go out at Debug message level
 *
 *  @param msg :: The message to log
 *  @param buffer :: the binary data to log
 *  @param length :: The length of the binaary data to log
 */
void Logger::dump(const std::string &msg, const void *buffer, std::size_t length) {
  if (m_enabled) {
    try {
      m_log->dump(msg, buffer, length);
    } catch (std::exception &e) {
      // failures in logging are not allowed to throw exceptions out of the
      // logging class
      std::cerr << e.what();
    }
  }
}

/** Returns true if at least the given log level is set.
 *  @param level :: The logging level it is best to use the Logger::Priority
 * enum (7=debug, 6=information, 4=warning, 3=error, 2=critical, 1=fatal)
 *  @return true if at least the given log level is set.
 */
bool Logger::is(int level) const {
  bool retVal = false;
  try {
    retVal = m_log->is(level);
  } catch (std::exception &e) {
    // failures in logging are not allowed to throw exceptions out of the
    // logging class
    std::cerr << e.what();
  }
  return retVal;
}

bool Logger::isDebug() const { return this->is(Priority::PRIO_DEBUG); }

void Logger::setLevel(int level) {
  try {
    const int levelActual = fixLevel(level);
    m_log->setLevel(levelActual);
  } catch (std::exception &e) {
    // failures in logging are not allowed to throw exceptions out of the
    // logging class
    std::cerr << e.what() << "\n";
  }
}

/// Sets the Logger's log level using a symbolic value.
///
/// @param level :: Valid values are: fatal, critical, error, warning, notice,
/// information, debug, trace
void Logger::setLevel(const std::string &level) {
  try {
    const int int_level = toLevel(level);
    m_log->setLevel(int_level);
  } catch (std::exception &e) {
    // failures in logging are not allowed to throw exceptions out of the
    // logging class
    std::cerr << e.what() << "\n";
  }
}

int Logger::getLevel() const { return m_log->getLevel(); }

std::string Logger::getLevelName() const {
  const auto level = std::size_t(this->getLevel());
  return Logger::PriorityNames[level];
}

/** This class implements an ostream interface to the Logger for fatal messages.
 *
 * The stream's buffer appends all characters written to it
 * to a string. As soon as a CR or LF (std::endl) is written,
 * the string is sent to the Logger.
 * @returns an std::ostream reference.
 */
std::ostream &Logger::fatal() { return getLogStream(Priority::PRIO_FATAL); }

/** This class implements an ostream interface to the Logger for error messages.
 *
 * The stream's buffer appends all characters written to it
 * to a string. As soon as a CR or LF (std::endl) is written,
 * the string is sent to the Logger.
 * @returns an std::ostream reference.
 */
std::ostream &Logger::error() { return getLogStream(Priority::PRIO_ERROR); }

/** This class implements an ostream interface to the Logger for warning
 *messages.
 *
 * The stream's buffer appends all characters written to it
 * to a string. As soon as a CR or LF (std::endl) is written,
 * the string is sent to the Logger.
 * @returns an std::ostream reference.
 */
std::ostream &Logger::warning() { return getLogStream(Priority::PRIO_WARNING); }

/** This class implements an ostream interface to the Logger for notice
 *messages.
 *
 * The stream's buffer appends all characters written to it
 * to a string. As soon as a CR or LF (std::endl) is written,
 * the string is sent to the Logger.
 * @returns an std::ostream reference.
 */
std::ostream &Logger::notice() { return getLogStream(Priority::PRIO_NOTICE); }

/** This class implements an ostream interface to the Logger for information
 *messages.
 *
 * The stream's buffer appends all characters written to it
 * to a string. As soon as a CR or LF (std::endl) is written,
 * the string is sent to the Logger.
 * @returns an std::ostream reference.
 */
std::ostream &Logger::information() { return getLogStream(Priority::PRIO_INFORMATION); }

/** This class implements an ostream interface to the Logger for debug messages.
 *
 * The stream's buffer appends all characters written to it
 * to a string. As soon as a CR or LF (std::endl) is written,
 * the string is sent to the Logger.
 * @returns an std::ostream reference.
 */
std::ostream &Logger::debug() { return getLogStream(Priority::PRIO_DEBUG); }

/**
 * accumulates a message to the buffer
 * @param msg the log message
 */
void Logger::accumulate(const std::string &msg) { m_logStream->accumulate(msg); }

/**
 * Flushes the accumulated message to the current channel
 */
void Logger::flush() { log(m_logStream->flush(), Priority(getLevel())); }

/**
 * Flushes the accumulated message to the given priority
 * @param priority the log level priority
 */
void Logger::flush(Priority priority) { log(m_logStream->flush(), priority); }

/// flushes the accumulated message with debug priority
void Logger::flushDebug() { flush(Poco::Message::PRIO_DEBUG); }

/// flushes the accumulated message with information priority
void Logger::flushInformation() { flush(Poco::Message::PRIO_INFORMATION); }

/// flushes the accumulated message with notice priority
void Logger::flushNotice() { flush(Poco::Message::PRIO_NOTICE); }

/// flushes the accumulated message with warning priority
void Logger::flushWarning() { flush(Poco::Message::PRIO_WARNING); }

/// flushes the accumulated message with error priority
void Logger::flushError() { flush(Poco::Message::PRIO_ERROR); }

/// flushes the accumulated message with fatal priority
void Logger::flushFatal() { flush(Poco::Message::PRIO_FATAL); }

/// flushes the accumulated messages without logging them
void Logger::purge() { m_logStream->flush(); }

/** Shuts down the logging framework and releases all Loggers.
 * Static method.
 */
void Logger::shutdown() {
  try {
    // Release the POCO loggers
    Poco::Logger::shutdown();
  } catch (std::exception &e) {
    // failures in logging are not allowed to throw exceptions out of the
    // logging class
    std::cerr << e.what();
  }
}

/** Sets the log level for all Loggers created so far, including the root
 * logger.
 * @param level :: the priority level to set for the loggers
 */
void Logger::setLevelForAll(const int level) {
  const int levelActual = fixLevel(level);

  try {
    // "" is the root logger
    Poco::Logger::setLevel("", levelActual);
  } catch (std::exception &e) {
    // failures in logging are not allowed to throw exceptions out of the
    // logging class
    std::cerr << e.what();
  }
}

void Logger::setLevelForAll(const std::string &level) {
  int intLevel = toLevel(level);
  setLevelForAll(intLevel);
}

/**
 * @param message :: The message to log
 * @param priority :: The priority level
 */
void Logger::log(const std::string &message, const Logger::Priority &priority) {
  if (!m_enabled)
    return;

  try {
    switch (applyLevelOffset(priority)) {
    case Poco::Message::PRIO_FATAL:
      m_log->fatal(message);
      break;
    case Poco::Message::PRIO_CRITICAL:
      m_log->critical(message);
      break;
    case Poco::Message::PRIO_ERROR:
      m_log->error(message);
      break;
    case Poco::Message::PRIO_WARNING:
      m_log->warning(message);
      break;
    case Poco::Message::PRIO_NOTICE:
      m_log->notice(message);
      break;
    case Poco::Message::PRIO_INFORMATION:
      m_log->information(message);
      break;
    case Poco::Message::PRIO_DEBUG:
      m_log->debug(message);
      break;
    case Poco::Message::PRIO_TRACE:
      m_log->trace(message);
      break;
    default:
      break;
    }
  } catch (std::exception &e) {
    // Failures in logging are not allowed to throw exceptions out of the
    // logging class
    std::cerr << "Error in logging framework: " << e.what();
  }
}

/**
 * Log a given message at a given priority
 * @param priority :: The priority level
 * @return :: the stream
 */
std::ostream &Logger::getLogStream(const Logger::Priority &priority) {
  if (!m_enabled)
    return NULL_STREAM;

  switch (applyLevelOffset(priority)) {
  case Poco::Message::PRIO_FATAL:
    return m_logStream->fatal();
    break;
  case Poco::Message::PRIO_CRITICAL:
    return m_logStream->critical();
    break;
  case Poco::Message::PRIO_ERROR:
    return m_logStream->error();
    break;
  case Poco::Message::PRIO_WARNING:
    return m_logStream->warning();
    break;
  case Poco::Message::PRIO_NOTICE:
    return m_logStream->notice();
    break;
  case Poco::Message::PRIO_INFORMATION:
    return m_logStream->information();
    break;
  case Poco::Message::PRIO_DEBUG:
    return m_logStream->debug();
    break;
  default:
    return NULL_STREAM;
  }
}

/**
 * Adjust a log priority level based off the m_levelOffset
 * @param proposedLevel :: The proposed level
 * @returns The offseted level
 */
Logger::Priority Logger::applyLevelOffset(Logger::Priority proposedLevel) {
  int retVal = proposedLevel;
  // fast exit if offset is 0
  if (m_levelOffset == 0) {
    return proposedLevel;
  } else {
    retVal += m_levelOffset;
    if (retVal < static_cast<int>(Priority::PRIO_FATAL)) {
      retVal = Priority::PRIO_FATAL;
    } else if (retVal > static_cast<int>(Priority::PRIO_TRACE)) {
      retVal = Priority::PRIO_TRACE;
    }
  }
  // Logger::Priority p(retVal);
  return static_cast<Logger::Priority>(retVal);
}

/**
 * Sets the Logger's log offset level.
 * @param level :: The  level offset to use
 */
void Logger::setLevelOffset(int level) { m_levelOffset = level; }

/**
 * Gets the Logger's log offset level.
 * @returns The offset level
 */ /// Gets the Logger's log offset level.
int Logger::getLevelOffset() const { return m_levelOffset; }

} // namespace Mantid::Kernel
