// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/ThreadSafeLogStream.h"

#include <Poco/Message.h>
#include <array>
#include <iosfwd>
#include <memory>
#include <string>

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
/// @cond Exclude from doxygen documentation
namespace Poco {
class Logger;
}
/// @endcond

namespace Mantid {
namespace Kernel {

/** @class Logger Logger.h Kernel/Logger.h

    The Logger class is in charge of the publishing messages from the framework
   through
    various channels. The static methods on the class are responsible for the
   creation
    of Logger objects on request. This class currently uses the Logging
   functionality
    provided through the POCO (portable components) library.

        Usage example:
            Logger ls(someLogger);
            ls.error("Some informational message");
            ls.error() << "Some error message\n";

    @author Nicholas Draper, Tessella Support Services plc
    @date 12/10/2007
*/
class MANTID_KERNEL_DLL Logger {
public:
  // Our logger's priority types are the same as POCO's Message's types.
  using Priority = Poco::Message::Priority;

  static const std::array<std::string, 9> PriorityNames;

  /// Constructor giving the logger name
  Logger(const std::string &name);
  /// Update the name of the logger
  void setName(const std::string &name);

  /// Logs at Fatal level
  void fatal(const std::string &msg);
  /// Logs at error level
  void error(const std::string &msg);
  /// Logs at warning level
  void warning(const std::string &msg);
  /// Logs at notice level
  void notice(const std::string &msg);
  /// Logs at information level
  void information(const std::string &msg);
  /// Logs at debug level
  void debug(const std::string &msg);
  /// accumulates a message
  void accumulate(const std::string &msg);
  /// flushes accumulated messages to the current level
  void flush();
  /// flushes accumulated messages to the given priority
  void flush(Priority);
  void flushDebug();
  void flushInformation();
  void flushNotice();
  void flushWarning();
  void flushError();
  void flushFatal();
  void purge();

  /// Logs at Fatal level
  std::ostream &fatal();
  /// Logs at error level
  std::ostream &error();
  /// Logs at warning level
  std::ostream &warning();
  /// Logs at notice level
  std::ostream &notice();
  /// Logs at information level
  std::ostream &information();
  /// Logs at debug level
  std::ostream &debug();

  /// Log a message at a given priority
  void log(const std::string &message, const Priority &priority);

  /// gets the correct log stream for a priority
  std::ostream &getLogStream(const Priority &priority);

  /// Logs the given message at debug level, followed by the data in buffer.
  void dump(const std::string &msg, const void *buffer, std::size_t length);

  /// Sets the Logger's log level.
  void setLevel(int level);

  /// Sets the Logger's log offset level.
  void setLevelOffset(int level);

  /// Gets the Logger's log offset level.
  int getLevelOffset() const;

  /// Returns the Logger's log level.
  int getLevel() const;

  std::string getLevelName() const;

  /// Sets the Logger's log level using a symbolic value.
  void setLevel(const std::string &level);

  /// returns true if the log is enabled
  bool getEnabled() const;

  /// set if the logging is enabled
  void setEnabled(const bool enabled);

  /// Returns true if at least the given log level is set.
  bool is(int level) const;

  /// Returns true if log level is at least debug
  bool isDebug() const;

  /// Sets the log level for all Loggers created so far, including the root
  /// logger.
  static void setLevelForAll(const int level);
  static void setLevelForAll(const std::string &level);

  /// Shuts down the logging framework and releases all Loggers.
  static void shutdown();

private:
  // Disable default constructor
  Logger();
  /// Disable copying
  Logger(const Logger &);
  /// Disable assignment
  Logger &operator=(const Logger &);

  /// Return a log stream set with the given priority
  Priority applyLevelOffset(Priority proposedLevel);

  /// Internal handle to third party logging objects
  Poco::Logger *m_log;
  /// Allows stream operators for a logger
  std::unique_ptr<ThreadSafeLogStream> m_logStream;

  /// The offset of the logger
  int m_levelOffset;
  /// The state of this logger, disabled loggers send no messages
  bool m_enabled;
};

} // namespace Kernel
} // namespace Mantid
