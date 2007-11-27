#ifndef MANTID_KERNEL_LOGGINGSERVICE_H_
#define MANTID_KERNEL_LOGGINGSERVICE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "System.h"
#include <string>
#include <exception>
#include <map>
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
			PRIO_FATAL = 1,		  /// A fatal error. The application will most likely terminate. This is the highest priority.
			PRIO_ERROR = 3,       /// An error. An operation did not complete successfully, but the application as a whole is not affected.
			PRIO_WARNING = 4,     /// A warning. An operation completed with an unexpected result.
			PRIO_INFORMATION = 6, /// An informational message, usually denoting the successful completion of an operation.
			PRIO_DEBUG = 7        /// A debugging message.This is the lowest priority.
		};

		void fatal(const std::string& msg);
		void error(const std::string& msg);
		void warning(const std::string& msg);
		void information(const std::string& msg);
		void debug(const std::string& msg);

		std::ostream& fatal();
		std::ostream& critical();
		std::ostream& error();
		std::ostream& warning();
		std::ostream& information();
		std::ostream& debug();

		/// Logs the given message at debug level, followed by the data in buffer.
		void dump(const std::string& msg, const void* buffer, std::size_t length);
			
		/// Returns true if at least the given log level is set.
		bool is(int level) const;

		/// Returns a reference to the Logger with the given name.
		static Logger& get(const std::string& name);

		/// Shuts down the logging framework and releases all Loggers.	
		static void shutdown();
		
		~Logger();

	protected:
		/// Protected constructor called by static get method
		Logger(const std::string& name);
		
		
	private:
		Logger();

		/// Overload of = operator
		Logger& operator= (const Logger&);

		/// Internal handle to third party logging objects
		Poco::Logger& _log;
		///This pointer is owned by this class, initialized in the constructor and deleted in the destructor
		Poco::LogStream* _logStream;
		
		/// Name of this logging object
		std::string _name;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_LOGGINGSERVICE_H_*/
