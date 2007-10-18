#include "../inc/Logger.h"
#include <Poco/Logger.h>

namespace Mantid
{
	Logger::Logger(const std::string& name): _log(Poco::Logger::get(_name))
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

	void Logger::notice(const std::string& msg)
	{
		_log.notice(msg);
	}

	void Logger::information(const std::string& msg)
	{
		_log.information(msg);
	}

	void Logger::debug(const std::string& msg)
	{
		_log.debug(msg);
	}

	void Logger::trace(const std::string& msg)
	{
		_log.trace(msg);
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
