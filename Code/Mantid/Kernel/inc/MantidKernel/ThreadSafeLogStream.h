#ifndef MANTID_KERNEL_THREADSAFELOGSTREAM
#define MANTID_KERNEL_THREADSAFELOGSTREAM

//--------------------------------------------
// Includes
//--------------------------------------------
#include "MantidKernel/System.h"
#include "Poco/Logger.h"
#include "Poco/LogStream.h"
#include "Poco/UnbufferedStreamBuf.h"
#include <istream>

namespace Mantid 
{

namespace Kernel
{
/**

   This class implements a threadsafe version of the POCO buffer interface to a Logger's stream object. The
   buffer uses OpenMP API calls to both protect shared memory from access by multiple threads and updates
   log messages is such a way that they are not mangled by separate threads.

   @author Martyn Gigg, Tessella Support Services plc
   @date 13/04/2010
   
   Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
   
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
   
   File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
   Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport ThreadSafeLogStreamBuf: public Poco::LogStreamBuf
{
public:
  /// Constructor
  ThreadSafeLogStreamBuf(Poco::Logger& logger, Poco::Message::Priority priority);
  /// Destructor
  ~ThreadSafeLogStreamBuf();
		
private:
  ///Overridden fron base to write to the device in a thread-safe manner.
  int writeToDevice(char c);
  
private:
  /// Store a map of thread indices to messages
  std::map<int, std::string> m_messages;
};


/**
  The base class for ThreadSafeLogStream.

  From Poco/Foundation/LogStream.h - This class is needed to ensure the correct initialization
  order of the stream buffer and base classes.
 */
class DLLExport ThreadSafeLogIOS: public virtual std::ios
{
public:
  ///Constructor
  ThreadSafeLogIOS(Poco::Logger& logger, Poco::Message::Priority priority);
  ///Destructor
  ~ThreadSafeLogIOS();
  // Return a pointer to the stream buffer object
  Poco::LogStreamBuf* rdbuf();
  
protected:
  /// The log stream buffer object
  ThreadSafeLogStreamBuf m_buf;
};

/**
  The main log stream class implementing an ostream interface to a Logger.
  Nearly identical to the Poco::LogStream class but instead uses thread-safe buffering required for Mantid
 
  The stream's buffer appends all characters written to it
  to a string. As soon as a CR or LF (std::endl) is written,
  the string is sent to the Poco::Logger, with the current
  priority.
 
  Usage example:
      ThreadSafeLogStream ls(somePoco::Logger);
      ls << "Some informational message" << std::endl;
      ls.error() << "Some error message" << std::endl;
 */
class DLLExport ThreadSafeLogStream : public ThreadSafeLogIOS, public std::ostream
{
public:
  /// Creates the ThreadSafeLogStream, using the given logger and priority.
  ThreadSafeLogStream(Poco::Logger& logger, Poco::Message::Priority priority = Poco::Message::PRIO_INFORMATION);

  /// Creates the ThreadSafeLogStream, using the logger identified
  /// by loggerName, and sets the priority.
  ThreadSafeLogStream(const std::string& loggerName, Poco::Message::Priority priority = Poco::Message::PRIO_INFORMATION);
  /// Destroys the ThreadSafeLogStream.
  ~ThreadSafeLogStream();
  /// Sets the priority for log messages to Poco::Message::PRIO_FATAL.		
  ThreadSafeLogStream& fatal();
  /// Sets the priority for log messages to Poco::Message::PRIO_FATAL
  /// and writes the given message.
  ThreadSafeLogStream& fatal(const std::string& message);
  /// Sets the priority for log messages to Poco::Message::PRIO_CRITICAL.  
  ThreadSafeLogStream& critical();
  /// Sets the priority for log messages to Poco::Message::PRIO_CRITICAL
  /// and writes the given message.
  ThreadSafeLogStream& critical(const std::string& message);
  /// Sets the priority for log messages to Poco::Message::PRIO_ERROR.  
  ThreadSafeLogStream& error();
  /// Sets the priority for log messages to Poco::Message::PRIO_ERROR
  /// and writes the given message.
  ThreadSafeLogStream& error(const std::string& message);
  /// Sets the priority for log messages to Poco::Message::PRIO_WARNING.  
  ThreadSafeLogStream& warning();
  /// Sets the priority for log messages to Poco::Message::PRIO_WARNING
  /// and writes the given message.
  ThreadSafeLogStream& warning(const std::string& message);
  /// Sets the priority for log messages to Poco::Message::PRIO_NOTICE.  
  ThreadSafeLogStream& notice();
  /// Sets the priority for log messages to Poco::Message::PRIO_NOTICE
  /// and writes the given message.
  ThreadSafeLogStream& notice(const std::string& message);
  /// Sets the priority for log messages to Poco::Message::PRIO_INFORMATION.
  ThreadSafeLogStream& information();
  /// Sets the priority for log messages to Poco::Message::PRIO_INFORMATION
  /// and writes the given message.
  ThreadSafeLogStream& information(const std::string& message);
  /// Sets the priority for log messages to Poco::Message::PRIO_DEBUG.
  ThreadSafeLogStream& debug();
  /// Sets the priority for log messages to Poco::Message::PRIO_DEBUG
  /// and writes the given message.
  ThreadSafeLogStream& debug(const std::string& message);
  /// Sets the priority for log messages.
  ThreadSafeLogStream& priority(Poco::Message::Priority priority);
};

}

}

#endif //MANTID_KERNEL_THREADSAFELOGSTREAM
