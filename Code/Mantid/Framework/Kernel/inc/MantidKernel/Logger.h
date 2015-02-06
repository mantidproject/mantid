#ifndef MANTID_KERNEL_LOGGINGSERVICE_H_
#define MANTID_KERNEL_LOGGINGSERVICE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"

#include <Poco/Message.h>

#include <iosfwd>
#include <set>
#include <string>

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
/// @cond Exclude from doxygen documentation
namespace Poco {
class FastMutex;
class Logger;
class NullOutputStream;
}
/// @endcond

namespace Mantid {
namespace Kernel {
class ThreadSafeLogStream;

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
            ls.error() << "Some error message" << std::endl;

    @author Nicholas Draper, Tessella Support Services plc
    @date 12/10/2007

    Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_KERNEL_DLL Logger {
public:
  // Our logger's priority types are the same as POCO's Message's types.
  typedef Poco::Message::Priority Priority;

  /// Constructor giving the logger name
  Logger(const std::string &name);
  /// Destructor
  ~Logger();
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

  /// Sets the Logger's log level using a symbolic value.
  void setLevel(const std::string &level);

  /// returns true if the log is enabled
  bool getEnabled() const;

  /// set if the logging is enabled
  void setEnabled(const bool enabled);

  /// Returns true if at least the given log level is set.
  bool is(int level) const;

  /// Sets the log level for all Loggers created so far, including the root
  /// logger.
  static void setLevelForAll(const int level);

  /// Shuts down the logging framework and releases all Loggers.
  static void shutdown();

private:
  // Disable default constructor
  Logger();
  /// Disable copying
  Logger(const Logger &);
  /// Disable assignment
  Logger &operator=(const Logger &);

  /// Log a message at a given priority
  void log(const std::string &message, Logger::Priority priority);
  /// gets the correct log stream for a priority
  std::ostream &getLogStream(Logger::Priority priority);
  /// Return a log stream set with the given priority
  Priority applyLevelOffset(Priority proposedLevel);

  /// Internal handle to third party logging objects
  Poco::Logger *m_log;
  /// Allows stream operators for a logger
  ThreadSafeLogStream *m_logStream;

  /// The offset of the logger
  int m_levelOffset;
  /// The state of this logger, disabled loggers send no messages
  bool m_enabled;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_LOGGINGSERVICE_H_*/
