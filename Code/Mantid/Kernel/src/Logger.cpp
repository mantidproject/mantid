#include "../inc/Logger.h"
#include <Poco/Logger.h>
#include <Poco/Message.h>

namespace Mantid
{
	Logger::Logger(const std::string& name): _log(Poco::Logger::get(name))
	{  
		_name = name;
	}

	void Logger::fatal(const std::string& msg)
	{
		_log.fatal(msg); 
	}
		
	void Logger::critical(const std::string& msg)
	{
		_log.critical(msg);
	}

	void Logger::error(const std::string& msg)
	{
		_log.error(msg);
	}

	void Logger::warning(const std::string& msg)
	{
		_log.warning(msg);
	}

	void Logger::information(const std::string& msg)
	{
		_log.information(msg);
	}

	void Logger::debug(const std::string& msg)
	{
		_log.debug(msg);
	}

	void Logger::dump(const std::string& msg, const void* buffer, std::size_t length)
	{
		_log.dump(msg,buffer,length);
	}
		

	bool Logger::is(int level) const
	{
		return _log.is(level);
	}

	void Logger::shutdown()
	{
		Poco::Logger::shutdown();
	}

	Logger& Logger::get(const std::string& name)
	{
		Logger* pLogger = new Logger(name);
		return *pLogger;
	}


	
}
