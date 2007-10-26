#include "../inc/ConfigSvc.h"
#include "../inc/Support.h"
#include "Poco/Util/LoggingConfigurator.h"
#include "Poco/Util/SystemConfiguration.h"
#include "Poco/Util/PropertyFileConfiguration.h"
#include <sstream>
#include <iostream>
#include <string>

namespace Mantid
{
	// Initialise the instance pointer to zero
	ConfigSvc* ConfigSvc::m_instance=0;

	//private constructor
	ConfigSvc::ConfigSvc()
	{
		//getting at system details
		m_pSysConfig = new WrappedObject<Poco::Util::SystemConfiguration>;
		
		m_pConf = 0;

		//attempt to load the default properties filename
		loadConfig("Mantid.Properties");
	}

	//destructor
	ConfigSvc::~ConfigSvc()
	{
		delete m_pSysConfig;
		delete m_pConf;
	}


	void ConfigSvc::loadConfig(const std::string& filename)
	{
		delete m_pConf;

		try
		{
			m_pConf = new WrappedObject<Poco::Util::PropertyFileConfiguration>(filename);
		}
		catch (std::exception e)
		{
			//there was a problem loading the file - it probably is not there
			std::cerr << "Problem loading the logging file " << filename << " " << e.what();
			
			std::string propFile = 
				"logging.loggers.root.level = debug\n"
				"logging.loggers.root.channel.class = SplitterChannel\n"
				"logging.loggers.root.channel.channel1 = consoleChannel\n"
				"logging.loggers.root.channel.channel2 = fileChannel\n"
				"logging.channels.consoleChannel.class = ConsoleChannel\n"
				"logging.channels.consoleChannel.formatter = f1\n"
				"logging.channels.fileChannel.class = FileChannel\n"
				"logging.channels.fileChannel.path = sample.log\n"
				"logging.channels.fileChannel.formatter.class = PatternFormatter\n"
				"logging.channels.fileChannel.formatter.pattern = %s: {%p} %t\n"
				"logging.formatters.f1.class = PatternFormatter\n"
				"logging.formatters.f1.pattern = %s-[%p] %t\n"
				"logging.formatters.f1.times = UTC\n";
		
			std::istringstream istr(propFile);
			m_pConf = new WrappedObject<Poco::Util::PropertyFileConfiguration>(istr);
		}

		try
		{
			//configure the logging framework
			Poco::Util::LoggingConfigurator configurator;
			configurator.configure(m_pConf);
		}
		catch (std::exception e)
		{
			std::cerr << "Trouble configuring the logging framework " << e.what();
		}
	}
	
	std::string ConfigSvc::getString(const std::string& keyName)
	{
		return m_pConf->getString(keyName);
	}

	template<typename T>
	int ConfigSvc::getValue(const std::string& keyName, T& out)
	{
		std::string strValue = getString(keyName);
		int result = Mantid::StrFunc::convert(strValue,out);
		return result;
	}

	std::string ConfigSvc::getEnvironment(const std::string& keyName)	
	{
		return m_pSysConfig->getString("system.env." + keyName);
	}

	std::string ConfigSvc::getOSName()
	{
		return m_pSysConfig->getString("system.osName");
	}

	std::string ConfigSvc::getOSArchitecture()
	{
		return m_pSysConfig->getString("system.osArchitecture");
	}
	
	std::string ConfigSvc::getComputerName()
	{
		return m_pSysConfig->getString("system.nodeName");
	}

/*	Removed as the use of these throughs a debug assertion about an invalid heap pointer
	File dbgheap.c
	Expression _CrtIsValidHeapPointer(pUserData)

	std::string ConfigSvc::getOSVersion()
	{
		return m_pSysConfig->getString("system.osVersion");
	}
	
	std::string ConfigSvc::getCurrentDir()
	{
		return m_pSysConfig->getString("system.currentDir");
	}
	
	std::string ConfigSvc::getHomeDir()
	{
		return m_pSysConfig->getString("system.homeDir");
	}
	
	std::string ConfigSvc::getTempDir()
	{
		return m_pSysConfig->getString("system.tempDir");
	}
*/

	
/// \cond TEMPLATE 

	template int ConfigSvc::getValue(const std::string&,double&);
	template int ConfigSvc::getValue(const std::string&,std::string&);
	template int ConfigSvc::getValue(const std::string&,int&);
}
