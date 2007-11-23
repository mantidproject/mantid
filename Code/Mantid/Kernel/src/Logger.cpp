#include "MantidKernel/Logger.h"
#include <Poco/Logger.h>
#include <Poco/Message.h>
#include <iostream>

namespace Mantid
{
namespace Kernel
{
  /// No argument constructor
  Logger::Logger(const std::string& name): _log(Poco::Logger::get(name))
	{  
		_name = name;
	}

  /** If the Logger's log level is at least PRIO_FATAL, creates a Message with 
   *  priority PRIO_FATAL and the given message text and sends it to the attached channel.
   * 
   *  @param msg The message to log.
   */
	void Logger::fatal(const std::string& msg)
	{
		try
		{
			_log.fatal(msg); 
		} 
		catch (std::exception& e)
		{
			//failures in logging are not allowed to throw exceptions out of the logging class
			std::cerr << e.what();
		}
	}
		
  /** If the Logger's log level is at least PRIO_CRITICAL, creates a Message with priority
   *  PRIO_CRITICAL and the given message text and sends it to the attached channel.
   * 
   *  @param msg The message to log.
   */
	void Logger::critical(const std::string& msg)
	{
		try
		{
			_log.critical(msg);
		} 
		catch (std::exception& e)
		{
			//failures in logging are not allowed to throw exceptions out of the logging class
			std::cerr << e.what();
		}
	}

  /** If the Logger's log level is at least PRIO_ERROR, creates a Message with priority
   *  PRIO_ERROR and the given message text and sends it to the attached channel.
   * 
   *  @param msg The message to log.
   */
	void Logger::error(const std::string& msg)
	{
		try
		{
			_log.error(msg);
		} 
		catch (std::exception& e)
		{
			//failures in logging are not allowed to throw exceptions out of the logging class
			std::cerr << e.what();
		}
		
	}

  /** If the Logger's log level is at least PRIO_WARNING, creates a Message with 
   *  priority PRIO_WARNING and the given message text and sends it to the attached channel.
   * 
   *  @param msg The message to log.
   */
	void Logger::warning(const std::string& msg)
	{
		try
		{
			_log.warning(msg);
		} 
		catch (std::exception& e)
		{
			//failures in logging are not allowed to throw exceptions out of the logging class
			std::cerr << e.what();
		}
	}

  /** If the Logger's log level is at least PRIO_INFORMATION, creates a Message with 
   *  priority PRIO_INFORMATION and the given message text and sends it to the 
   *  attached channel.
   * 
   *  @param msg The message to log.
   */
	void Logger::information(const std::string& msg)
	{
		try
		{
			_log.information(msg);
		} 
		catch (std::exception& e)
		{
			//failures in logging are not allowed to throw exceptions out of the logging class
			std::cerr << e.what();
		}
	}

  /** If the Logger's log level is at least PRIO_DEBUG, creates a Message with priority
   *  PRIO_DEBUG and the given message text and sends it to the attached channel.
   * 
   *  @param msg The message to log.
   */
	void Logger::debug(const std::string& msg)
	{
		try
		{
			_log.debug(msg);
		} 
		catch (std::exception& e)
		{
			//failures in logging are not allowed to throw exceptions out of the logging class
			std::cerr << e.what();
		}
	}

  /** Logs the given message at debug level, followed by the data in buffer.
   * 
   *  The data in buffer is written in canonical hex+ASCII form:
   *  Offset (4 bytes) in hexadecimal, followed by sixteen space-separated, 
   *  two column, hexadecimal bytes, followed by the same sixteen bytes as 
   *  ASCII characters.
   *  For bytes outside the range 32 .. 127, a dot is printed.  
   *  Note all Dump messages go out at Debug message level
   * 
   *  @param msg The message to log
   *  @param buffer the binary data to log
   *  @param length The length of the binaary data to log
   */
	void Logger::dump(const std::string& msg, const void* buffer, std::size_t length)
	{
		try
		{
			_log.dump(msg,buffer,length);
		} 
		catch (std::exception& e)
		{
			//failures in logging are not allowed to throw exceptions out of the logging class
			std::cerr << e.what();
		}
	}
		
  /** Returns true if at least the given log level is set.
   *  @param level The logging level it is best to use the Logger::Priority enum (7=debug, 6=information, 4=warning, 3=error, 2=critical, 1=fatal)
   */
	bool Logger::is(int level) const
	{
		bool retVal = false;
		try
		{
			retVal = _log.is(level);
		} 
		catch (std::exception& e)
		{
			//failures in logging are not allowed to throw exceptions out of the logging class
			std::cerr << e.what();
		}
		return retVal;
	}

  /// Shuts down the logging framework and releases all Loggers.  
	void Logger::shutdown()
	{
		try
		{
			Poco::Logger::shutdown();
		} 
		catch (std::exception& e)
		{
			//failures in logging are not allowed to throw exceptions out of the logging class
			std::cerr << e.what();
		}
	}

  /** Returns a reference to the Logger with the given name.
   *  If the Logger does not yet exist, it is created, based on its parent logger.
   * 
   *  @param name The name of the logger to use - this is usually the class namename. 
   */
	Logger& Logger::get(const std::string& name)
	{
		Logger* pLogger = new Logger(name);
		return *pLogger;
	}

} // namespace Kernel
} // Namespace Mantid

