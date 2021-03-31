// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//--------------------------------------------
// Includes
//--------------------------------------------
#include "MantidKernel/DllConfig.h"

#include <Poco/LogStream.h>
#include <Poco/Message.h>
#include <Poco/Thread.h>

#include <iosfwd>
#include <map>
#include <mutex>
#include <string>

// Forward Declare
namespace Poco {
class Logger;
}

namespace Mantid {
namespace Kernel {
/**

   This class implements a threadsafe version of the POCO buffer interface to a
   Logger's stream object. The
   buffer uses OpenMP API calls to both protect shared memory from access by
   multiple threads and updates
   log messages is such a way that they are not mangled by separate threads.

   @author Martyn Gigg, Tessella Support Services plc
   @date 13/04/2010
*/
class MANTID_KERNEL_DLL ThreadSafeLogStreamBuf : public Poco::LogStreamBuf {
public:
  /// Constructor
  ThreadSafeLogStreamBuf(Poco::Logger &logger, Poco::Message::Priority priority);
  int overflow(char c);
  using Poco::LogStreamBuf::overflow;
  void accumulate(const std::string &message);
  std::string flush();

private:
  /// Overridden from base to write to the device in a thread-safe manner.
  int writeToDevice(char c) override;

private:
  /// Store a map of thread indices to messages
  std::map<Poco::Thread::TID, std::string> m_messages;
  /// Store a map of thread indices to accummulators of messages
  std::map<Poco::Thread::TID, std::string> m_accumulator;
  /// mutex protecting logstream
  std::mutex m_mutex;
};

/**
  The base class for ThreadSafeLogStream.

  From Poco/Foundation/LogStream.h - This class is needed to ensure the correct
  initialization
  order of the stream buffer and base classes.
 */
class MANTID_KERNEL_DLL ThreadSafeLogIOS : public virtual std::ios {
public:
  /// Constructor
  ThreadSafeLogIOS(Poco::Logger &logger, Poco::Message::Priority priority);
  // Return a pointer to the stream buffer object
  Poco::LogStreamBuf *rdbuf();

protected:
  /// The log stream buffer object
  ThreadSafeLogStreamBuf m_buf;
};

/**
  The main log stream class implementing an ostream interface to a Logger.
  Nearly identical to the Poco::LogStream class but instead uses thread-safe
  buffering required for Mantid

  The stream's buffer appends all characters written to it
  to a string. As soon as a CR or LF (std::endl) is written,
  the string is sent to the Poco::Logger, with the current
  priority.

  Usage example:
      ThreadSafeLogStream ls(somePoco::Logger);
      ls << "Some informational message\n";
      ls.error() << "Some error message\n";
 */
class MANTID_KERNEL_DLL ThreadSafeLogStream : public ThreadSafeLogIOS, public std::ostream {
public:
  /// Creates the ThreadSafeLogStream, using the given logger and priority.
  ThreadSafeLogStream(Poco::Logger &logger, Poco::Message::Priority priority = Poco::Message::PRIO_INFORMATION);

  /// Creates the ThreadSafeLogStream, using the logger identified
  /// by loggerName, and sets the priority.
  ThreadSafeLogStream(const std::string &loggerName,
                      Poco::Message::Priority priority = Poco::Message::PRIO_INFORMATION);
  /// Sets the priority for log messages to Poco::Message::PRIO_FATAL.
  ThreadSafeLogStream &fatal();
  /// Sets the priority for log messages to Poco::Message::PRIO_FATAL
  /// and writes the given message.
  ThreadSafeLogStream &fatal(const std::string &message);
  /// Sets the priority for log messages to Poco::Message::PRIO_CRITICAL.
  ThreadSafeLogStream &critical();
  /// Sets the priority for log messages to Poco::Message::PRIO_CRITICAL
  /// and writes the given message.
  ThreadSafeLogStream &critical(const std::string &message);
  /// Sets the priority for log messages to Poco::Message::PRIO_ERROR.
  ThreadSafeLogStream &error();
  /// Sets the priority for log messages to Poco::Message::PRIO_ERROR
  /// and writes the given message.
  ThreadSafeLogStream &error(const std::string &message);
  /// Sets the priority for log messages to Poco::Message::PRIO_WARNING.
  ThreadSafeLogStream &warning();
  /// Sets the priority for log messages to Poco::Message::PRIO_WARNING
  /// and writes the given message.
  ThreadSafeLogStream &warning(const std::string &message);
  /// Sets the priority for log messages to Poco::Message::PRIO_NOTICE.
  ThreadSafeLogStream &notice();
  /// Sets the priority for log messages to Poco::Message::PRIO_NOTICE
  /// and writes the given message.
  ThreadSafeLogStream &notice(const std::string &message);
  /// Sets the priority for log messages to Poco::Message::PRIO_INFORMATION.
  ThreadSafeLogStream &information();
  /// Sets the priority for log messages to Poco::Message::PRIO_INFORMATION
  /// and writes the given message.
  ThreadSafeLogStream &information(const std::string &message);
  /// Sets the priority for log messages to Poco::Message::PRIO_DEBUG.
  ThreadSafeLogStream &debug();
  /// Sets the priority for log messages to Poco::Message::PRIO_DEBUG
  /// and writes the given message.
  ThreadSafeLogStream &debug(const std::string &message);
  /// Sets the priority for log messages.
  ThreadSafeLogStream &priority(Poco::Message::Priority priority);
  /// accumulates the message to the accummulator buffer
  ThreadSafeLogStream &accumulate(const std::string &message);
  /// Returns and flushes the accumulated messages
  std::string flush();
};
} // namespace Kernel
} // namespace Mantid
