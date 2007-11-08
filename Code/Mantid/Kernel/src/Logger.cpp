#include "../inc/Logger.h"
#include <Poco/Logger.h>
#include <Poco/Message.h>
#include <iostream>

namespace Mantid
{
namespace Kernel
{
	Logger::Logger(const std::string& name): _log(Poco::Logger::get(name))
	{  
		_name = name;
	}

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

	Logger& Logger::get(const std::string& name)
	{
		Logger* pLogger = new Logger(name);
		return *pLogger;
	}

} // namespace Kernel
} // Namespace Mantid

