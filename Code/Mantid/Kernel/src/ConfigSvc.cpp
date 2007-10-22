#include "../inc/ConfigSvc.h"
#include "Poco/Util/LoggingConfigurator.h"
#include "Poco/Util/SystemConfiguration.h"
#include "Poco/Util/PropertyFileConfiguration.h"

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
		m_pConf = new WrappedObject<Poco::Util::PropertyFileConfiguration>(filename);

		//configure the logging framework
		Poco::Util::LoggingConfigurator configurator;
		configurator.configure(m_pConf);
	}
	
	std::string ConfigSvc::getString(const std::string& keyName)
	{
		return m_pConf->getString(keyName);
	}
	
	int ConfigSvc::getInt(const std::string& keyName)
	{
		return 1;
	}
	
	double ConfigSvc::getDouble(const std::string& keyName)
	{
		return 1.0;
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

/*	Removed as the use of these throughs a debug assertion about an invlid heap pointer
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
}
