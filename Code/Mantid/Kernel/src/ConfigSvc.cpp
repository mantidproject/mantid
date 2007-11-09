//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "ConfigSvc.h"
#include "Support.h"
#include "Poco/Util/LoggingConfigurator.h"
#include "Poco/Util/SystemConfiguration.h"
#include "Poco/Util/PropertyFileConfiguration.h"
#include <sstream>
#include <iostream>
#include <string>

namespace Mantid
{
namespace Kernel
{
	// Initialise the instance pointer to zero
	ConfigSvc* ConfigSvc::m_instance=0;

  /** A static method which retrieves the single instance of the ConfigSvc
   *
   * @returns A pointer to the instance
   */
	ConfigSvc* ConfigSvc::Instance()
	{
		if (!m_instance) m_instance = new ConfigSvc;
		return m_instance;
	}

	/// Private constructor for singleton class
	ConfigSvc::ConfigSvc()
	{
		//getting at system details
		m_pSysConfig = new WrappedObject<Poco::Util::SystemConfiguration>;
		m_pConf = 0;
		//attempt to load the default properties filename
		loadConfig("Mantid.properties");
	}

  /** Private Destructor
   *  Prevents client from calling 'delete' on the pointer handed out by Instance
   */
	ConfigSvc::~ConfigSvc()
	{
		delete m_pSysConfig;
		delete m_pConf;                // potential double delete???
	}


  /** Loads the config file provided, any previous configuration is discarded.
   *  If the file contains logging setup instructions then these will be used to setup the logging framework.
   *
   *  @param filename The filename and optionally path of the file to load
   */
	void ConfigSvc::loadConfig(const std::string& filename)
	{
		delete m_pConf;

		try
		{
		  m_pConf = new WrappedObject<Poco::Util::PropertyFileConfiguration>(filename);
		}
		catch (std::exception& e)
		{
			//there was a problem loading the file - it probably is not there
			std::cerr << "Problem loading the logging file " << filename << " " << e.what() << std::endl;
			
			std::string propFile = 
				"logging.loggers.root.level = debug\n"
				"logging.loggers.root.channel.class = SplitterChannel\n"
				"logging.loggers.root.channel.channel1 = consoleChannel\n"
				"logging.loggers.root.channel.channel2 = fileChannel\n"
				"logging.channels.consoleChannel.class = ConsoleChannel\n"
				"logging.channels.consoleChannel.formatter = f1\n"
				"logging.channels.fileChannel.class = FileChannel\n"
				"logging.channels.fileChannel.path = mantid.log\n"
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
			
			//BUG? This line crashes the FrameworkManagerTest and ConfigSvcTest
			configurator.configure(m_pConf);
		}
		catch (std::exception& e)
		{
			std::cerr << "Trouble configuring the logging framework " << e.what()<<std::endl;
		}
	}
	
  /** Searches for a string within the currently loaded configuaration values and 
   *  returns the value as a string.
   *
   *  @param keyName The case sensitive name of the property that you need the value of.
   *  @returns The string value of the property
   */
	std::string ConfigSvc::getString(const std::string& keyName)
	{
		return m_pConf->getString(keyName);
	}

  /** Searches for a string within the currently loaded configuaration values and 
   *  attempts to convert the values to the template type supplied.
   *
   *  @param keyName The case sensitive name of the property that you need the value of.
   *  @param out     The value if found
   *  @returns A success flag - 0 on failure, 1 on success
   */
	template<typename T>
	int ConfigSvc::getValue(const std::string& keyName, T& out)
	{
		std::string strValue = getString(keyName);
		int result = StrFunc::convert(strValue,out);
		return result;
	}

  /** Searches for the string within the environment variables and returns the 
   *  value as a string.
   *
   *  @param keyName The name of the environment variable that you need the value of.
   *  @returns The string value of the property
   */
	std::string ConfigSvc::getEnvironment(const std::string& keyName)	
	{
		return m_pSysConfig->getString("system.env." + keyName);
	}
	
  /** Gets the name of the host operating system
   *
   *  @returns The name pf the OS version
   */
	std::string ConfigSvc::getOSName()
	{
		return m_pSysConfig->getString("system.osName");
	}

  /** Gets the name of the computer running Mantid
   *
   *  @returns The  name of the computer
   */
	std::string ConfigSvc::getOSArchitecture()
	{
		return m_pSysConfig->getString("system.osArchitecture");
	}
	
  /** Gets the name of the operating system Architecture
   *
   * @returns The operating system architecture
   */
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

	template DLLExport int ConfigSvc::getValue(const std::string&,double&);
	template DLLExport int ConfigSvc::getValue(const std::string&,std::string&);
	template DLLExport int ConfigSvc::getValue(const std::string&,int&);

/// \endcond TEMPLATE

} // namespace Kernel
} // namespace Mantid
