#ifndef MANTID_KERNEL_LOGGINGSERVICE_H_
#define MANTID_KERNEL_LOGGINGSERVICE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "System.h"
#include "MantidKernel/MultiThreaded.h"
#include <string>
#include <set>
#include <exception>
#include <ostream>
#include <streambuf>

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
/// @cond Exclude from doxygen documentation
namespace Poco
{
  class Logger;
  class NullOutputStream;
}
/// @endcond

namespace Mantid
{
namespace Kernel
{
class ThreadSafeLogStream;

/** @class Logger Logger.h Kernel/Logger.h

    The Logger class is in charge of the publishing messages from the framework through
    various channels. The static methods on the class are responsible for the creation
    of Logger objects on request. This class currently uses the Logging functionality
    provided through the POCO (portable components) library.

	Usage example:
	    Logger ls(someLogger);
	    ls.error("Some informational message");
	    ls.error() << "Some error message" << std::endl;

    @author Nicholas Draper, Tessella Support Services plc
    @date 12/10/2007

    Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport Logger
  {
  public:
    /// An enumeration of the priority levels of a log message.
    enum Priority
    {
      PRIO_FATAL = 1,		  ///< A fatal error. The application will most likely terminate. This is the highest priority.
      PRIO_ERROR = 3,       ///< An error. An operation did not complete successfully, but the application as a whole is not affected.
      PRIO_WARNING = 4,     ///< A warning. An operation completed with an unexpected result.
      PRIO_NOTICE = 5,      ///< An informational message, usually denoting the successful completion of an Algorithm, These are the headlines of what we should be reporting to the user.
      PRIO_INFORMATION = 6, ///< An informational message, usually denoting the successful completion of an operation.
      PRIO_DEBUG = 7        ///< A debugging message.This is the lowest priority.
    };

    /// Sets the Loggername to a new value.
    void setName(std::string newName);
    /// Logs at Fatal level	
    void fatal(const std::string& msg);
    /// Logs at error level
    void error(const std::string& msg);
    /// Logs at warning level
    void warning(const std::string& msg);
    /// Logs at notice level
    void notice(const std::string& msg);
    /// Logs at information level
    void information(const std::string& msg);
    /// Logs at debug level
    void debug(const std::string& msg);

    /// Logs at Fatal level
    std::ostream& fatal();
    /// Logs at error level			
    std::ostream& error();
    /// Logs at warning level			
    std::ostream& warning();
    /// Logs at notice level		
    std::ostream& notice();			
    /// Logs at information level
    std::ostream& information();
    /// Logs at debug level
    std::ostream& debug();			

    /// Logs the given message at debug level, followed by the data in buffer.
    void dump(const std::string& msg, const void* buffer, std::size_t length);

    /// Sets the Logger's log level.
    void setLevel(int level);
		
    /// Returns the Logger's log level.
    int getLevel() const;
		
    /// Sets the Logger's log level using a symbolic value.
    void setLevel(const std::string& level);

    ///returns true if the log is enabled
    bool getEnabled() const;

    ///set if the logging is enabled
    void setEnabled(const bool enabled);
		
    /// Returns true if at least the given log level is set.
    bool is(int level) const;

    /// releases resources and deletes this object
    void release();

    /// Returns a reference to the Logger with the given name.
    static Logger& get(const std::string& name);

    ///destroy the given logger and releases resources
    static void destroy(Logger& logger);

    /// Shuts down the logging framework and releases all Loggers.
    static void shutdown();


  protected:
    /// Protected constructor called by static get method
    Logger(const std::string& name);

    /// Protected destructor - call release instead
    ~Logger();
  private:
    Logger();

    /// Overload of = operator
    Logger& operator= (const Logger&);

    /// Return a log stream set with the given priority
    void log(const std::string message, Logger::Priority priority);

    /// Internal handle to third party logging objects
    Poco::Logger* m_log;
    /// A Log stream to allow streaming operations.  This pointer is owned by this class, initialized in the constructor and deleted in the destructor
    ThreadSafeLogStream* m_logStream;
    /// Name of this logging object
    std::string m_name;
    /// The state of this logger, disabled loggers send no messages
    bool m_enabled;
    /// Typdef for a container of logger pointers
    typedef std::set<Logger*> LoggerList;
    /// The container of logger pointers
    static LoggerList* m_loggerList;
    /// The null stream that is used when logging is disabled
    static Poco::NullOutputStream* m_nullStream;
    /// Mutex to make changing the static logger list threadsafe.
    static Mutex *mutexLoggerList;
  };

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_LOGGINGSERVICE_H_*/
