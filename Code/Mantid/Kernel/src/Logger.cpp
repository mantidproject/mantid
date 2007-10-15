#include "../inc/Logger.h"
#include <Poco/Logger.h>

namespace Mantid
{
	// Getting the logger instance from the underlying framework looks horribly ineffecient,
	// but it is not too bad, each instance is stored in a map by poco, so we are just doing a map lookup.
	// Doing it this way meant I could avoid including any mention of POCO in the header file.


	Logger::Logger(const std::string& name)
	{
		_name = name;
	}

	void Logger::fatal(const std::string& msg)
	{
		Poco::Logger& log = Poco::Logger::get(_name);
		log.fatal(msg); 
	}
		
	void Logger::critical(const std::string& msg)
	{
		Poco::Logger& log = Poco::Logger::get(_name);
		log.critical(msg);
	}

	void Logger::error(const std::string& msg)
	{
		Poco::Logger& log = Poco::Logger::get(_name);
		log.error(msg);
	}

	void Logger::warning(const std::string& msg)
	{
		Poco::Logger& log = Poco::Logger::get(_name);
		log.warning(msg);
	}

	void Logger::notice(const std::string& msg)
	{
		Poco::Logger& log = Poco::Logger::get(_name);
		log.notice(msg);
	}

	void Logger::information(const std::string& msg)
	{
		Poco::Logger& log = Poco::Logger::get(_name);
		log.information(msg);
	}

	void Logger::debug(const std::string& msg)
	{
		Poco::Logger& log = Poco::Logger::get(_name);
		log.debug(msg);
	}

	void Logger::trace(const std::string& msg)
	{
		Poco::Logger& log = Poco::Logger::get(_name);
		log.trace(msg);
	}

	bool Logger::is(int level) const
	{
		Poco::Logger& log = Poco::Logger::get(_name);
		return log.is(level);
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
