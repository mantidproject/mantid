#ifndef MANTID_LOGGINGSERVICE_H_
#define MANTID_LOGGINGSERVICE_H_
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <string>
#include <exception>
#include <map>

namespace Mantid
{
/** @class Logger Logger.h Kernel/Logger.h

    The Logger class is in charge of the publishing messages from the framwork through various channels.
	The static methods on the class are responsible for the creation of Logger objects on request.
	This class currently uses the Logging functionality provided through the POCO (portable components library).
    
    @author Nicholas Draper, Tessella Support Services plc
    @date 12/10/2007
    
    Copyright ? 2007 ???RAL???

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
*/
	class Logger
	{
	public:	
		void fatal(const std::string& msg);
			/// If the Logger's log level is at least PRIO_FATAL,
			/// creates a Message with priority PRIO_FATAL
			/// and the given message text and sends it
			/// to the attached channel.

		void critical(const std::string& msg);
			/// If the Logger's log level is at least PRIO_CRITICAL,
			/// creates a Message with priority PRIO_CRITICAL
			/// and the given message text and sends it
			/// to the attached channel.

		void error(const std::string& msg);
			/// If the Logger's log level is at least PRIO_ERROR,
			/// creates a Message with priority PRIO_ERROR
			/// and the given message text and sends it
			/// to the attached channel.

		void warning(const std::string& msg);
			/// If the Logger's log level is at least PRIO_WARNING,
			/// creates a Message with priority PRIO_WARNING
			/// and the given message text and sends it
			/// to the attached channel.

		void notice(const std::string& msg);
			/// If the Logger's log level is at least PRIO_NOTICE,
			/// creates a Message with priority PRIO_NOTICE
			/// and the given message text and sends it
			/// to the attached channel.

		void information(const std::string& msg);
			/// If the Logger's log level is at least PRIO_INFORMATION,
			/// creates a Message with priority PRIO_INFORMATION
			/// and the given message text and sends it
			/// to the attached channel.

		void debug(const std::string& msg);
			/// If the Logger's log level is at least PRIO_DEBUG,
			/// creates a Message with priority PRIO_DEBUG
			/// and the given message text and sends it
			/// to the attached channel.

		void trace(const std::string& msg);
			/// If the Logger's log level is at least PRIO_TRACE,
			/// creates a Message with priority PRIO_TRACE
			/// and the given message text and sends it
			/// to the attached channel.
			
		/*void dump(const std::string& msg, const void* buffer, std::size_t length, Message::Priority prio = Message::PRIO_DEBUG);
			/// Logs the given message, followed by the data in buffer.
			///
			/// The data in buffer is written in canonical hex+ASCII form:
			/// Offset (4 bytes) in hexadecimal, followed by sixteen 
			/// space-separated, two column, hexadecimal bytes,
			/// followed by the same sixteen bytes as ASCII characters.
			/// For bytes outside the range 32 .. 127, a dot is printed.
*/
		bool is(int level) const;
			/// Returns true if at least the given log level is set.
		
		static Logger& get(const std::string& name);
			/// Returns a reference to the Logger with the given name.
			/// If the Logger does not yet exist, it is created, based
			/// on its parent logger.
	
		static void shutdown();
			/// Shuts down the logging framework and releases all
			/// Loggers.
		
		static const std::string ROOT; /// The name of the root logger ("").	
		
	protected:
		Logger(const std::string& name);
		~Logger();

	private:
		Logger();
		Logger(const Logger&);
		Logger& operator = (const Logger&);
		
		std::string _name;
};


  
}

#endif /*MANTID_LOGGINGSERVICE_H_*/
