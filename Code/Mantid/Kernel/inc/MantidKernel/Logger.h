#ifndef MANTID_KERNEL_LOGGINGSERVICE_H_
#define MANTID_KERNEL_LOGGINGSERVICE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "System.h"
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
	class LogStream;
	class NullChannel;
	class Mutex;
}
/// @endcond

namespace Mantid
{
namespace Kernel
{
/** @class Logger Logger.h Kernel/Logger.h

    The Logger class is in charge of the publishing messages from the framwork through
    various channels. The static methods on the class are responsible for the creation
    of Logger objects on request. This class currently uses the Logging functionality
    provided through the POCO (portable components) library.

	Usage example:
	    Logger ls(someLogger);
	    ls.error("Some informational message");
	    ls.error() << "Some error message" << std::endl;

    @author Nicholas Draper, Tessella Support Services plc
    @date 12/10/2007

    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
		/// An emuration of the priority levels of a log message.
		enum Priority
		{
			PRIO_FATAL = 1,		  ///< A fatal error. The application will most likely terminate. This is the highest priority.
			PRIO_ERROR = 3,       ///< An error. An operation did not complete successfully, but the application as a whole is not affected.
			PRIO_WARNING = 4,     ///< A warning. An operation completed with an unexpected result.
			PRIO_NOTICE = 5,      ///< An informational message, usually denoting the successful completion of an Algorithm, These are the headlines of what we should be reporting to the user.
			PRIO_INFORMATION = 6, ///< An informational message, usually denoting the successful completion of an operation.
			PRIO_DEBUG = 7        ///< A debugging message.This is the lowest priority.
		};

		void fatal(const std::string& msg);
		void error(const std::string& msg);
		void warning(const std::string& msg);
		void notice(const std::string& msg);
		void information(const std::string& msg);
		void debug(const std::string& msg);

		std::ostream& fatal();
		std::ostream& error();
		std::ostream& warning();
		std::ostream& notice();
		std::ostream& information();
		std::ostream& debug();

		/// Logs the given message at debug level, followed by the data in buffer.
		void dump(const std::string& msg, const void* buffer, std::size_t length);

		/// Sets the Logger's log level.
		void setLevel(int level);
		
		/// Returns the Logger's log level.
		int getLevel() const;
		
		/// Sets the Logger's log level using a symbolic value.
		///
		/// Valid values are:
		///   - fatal
		///   - critical
		///   - error
		///   - warning
		///   - notice
		///   - information
		///   - debug
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

		//destroy the given logger and releases resources
		static void Logger::destroy(Logger& logger);

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

		/// Internal handle to third party logging objects
		Poco::Logger& _log;
		///A Log stream to allow streaming operations.  This pointer is owned by this class, initialized in the constructor and deleted in the destructor
		Poco::LogStream* _logStream;
		///A Null stream, used when the logger is disabled.  This pointer is owned by this class, initialized in the constructor and deleted in the destructor
		Poco::LogStream* _nullStream;
		/// a null channell used to create the null stream		
		Poco::NullChannel*  _nullChannel;

		///returns the correct stream depending on the enabled status
		Poco::LogStream* getStream();

		/// Name of this logging object
		std::string _name;
		/// The state of this logger, disabled loggers send no messages
		bool _enabled;

		typedef std::set<Logger*> LoggerList;
		static LoggerList*        m_LoggerList;
		static Poco::Mutex        m_ListMtx;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_LOGGINGSERVICE_H_*/
