//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Support.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/FilterChannel.h"
#include "MantidKernel/SignalChannel.h"
#include "Poco/Util/LoggingConfigurator.h"
#include "Poco/Util/SystemConfiguration.h"
#include "Poco/Util/PropertyFileConfiguration.h"
#include "Poco/LoggingFactory.h"
#include <sstream>
#include <iostream>
#include <string>

namespace Mantid
{
namespace Kernel
{

  /// Private constructor for singleton class
	ConfigServiceImpl::ConfigServiceImpl() : g_log(Logger::get("ConfigService"))
	{
		//getting at system details
		m_pSysConfig = new WrappedObject<Poco::Util::SystemConfiguration>;
		m_pConf = 0;
        
        //Register the FilterChannel with the Poco logging factory
        Poco::LoggingFactory::defaultFactory().registerChannelClass("FilterChannel",new Poco::Instantiator<Poco::FilterChannel, Poco::Channel>);

        //Register the SignalChannel with the Poco logging factory
        Poco::LoggingFactory::defaultFactory().registerChannelClass("SignalChannel",new Poco::Instantiator<Poco::SignalChannel, Poco::Channel>);

        //attempt to load the default properties filename
		loadConfig("Mantid.properties");
		g_log.debug() << "ConfigService created." << std::endl;
	}

  /// Private copy constructor for singleton class
	ConfigServiceImpl::ConfigServiceImpl(const ConfigServiceImpl&) : g_log(Logger::get("ConfigService"))
	{
	}

  /** Private Destructor
   *  Prevents client from calling 'delete' on the pointer handed out by Instance
   */
	ConfigServiceImpl::~ConfigServiceImpl()
	{
		delete m_pSysConfig;
		delete m_pConf;                // potential double delete???
//		g_log.debug() << "ConfigService destroyed." << std::endl;
	}


  /** Loads the config file provided, any previous configuration is discarded.
   *  If the file contains logging setup instructions then these will be used to setup the logging framework.
   *
   *  @param filename The filename and optionally path of the file to load
   */
	void ConfigServiceImpl::loadConfig(const std::string& filename)
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
				"logging.loggers.root.channel.channel1 = consoleChannelFilter\n"
				"logging.loggers.root.channel.channel2 = fileChannel\n"
				"logging.channels.consoleChannelFilter.class = FilterChannel\n"
				"logging.channels.consoleChannelFilter.channel = ConsoleChannel\n"
				"logging.channels.consoleChannelFilter.level = information\n"
				"logging.channels.consoleChannel.class = ConsoleChannel\n"
				"logging.channels.consoleChannel.formatter = f1\n"
				"logging.channels.fileChannel.class = FileChannel\n"
				"logging.channels.fileChannel.path = mantid.log\n"
				"logging.channels.fileChannel.formatter.class = PatternFormatter\n"
				"logging.channels.fileChannel.formatter.pattern = %Y-%m-%d %H:%M:%S,%i [%I] %p %s - %t\n"
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
			
			//BUG? This line crashes the FrameworkManagerTest and ConfigServiceImplTest
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
   *  @returns The string value of the property, or an empty string if the key cannot be found
   */
	std::string ConfigServiceImpl::getString(const std::string& keyName)
	{
    std::string retVal;
    try
    {
      retVal = m_pConf->getString(keyName);
    }
    catch(Poco::NotFoundException& ex)
    {
      g_log.warning()<<"Unable to find " << keyName << " in the properties file" << std::endl;
      retVal = "";
    }
		return retVal;
	}

  /** Searches for a string within the currently loaded configuaration values and 
   *  attempts to convert the values to the template type supplied.
   *
   *  @param keyName The case sensitive name of the property that you need the value of.
   *  @param out     The value if found
   *  @returns A success flag - 0 on failure, 1 on success
   */
	template<typename T>
	int ConfigServiceImpl::getValue(const std::string& keyName, T& out)
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
	std::string ConfigServiceImpl::getEnvironment(const std::string& keyName)	
	{
		return m_pSysConfig->getString("system.env." + keyName);
	}
	
  /** Gets the name of the host operating system
   *
   *  @returns The name pf the OS version
   */
	std::string ConfigServiceImpl::getOSName()
	{
		return m_pSysConfig->getString("system.osName");
	}

  /** Gets the name of the computer running Mantid
   *
   *  @returns The  name of the computer
   */
	std::string ConfigServiceImpl::getOSArchitecture()
	{
		return m_pSysConfig->getString("system.osArchitecture");
	}
	
  /** Gets the name of the operating system Architecture
   *
   * @returns The operating system architecture
   */
	std::string ConfigServiceImpl::getComputerName()
	{
		return m_pSysConfig->getString("system.nodeName");
	}

  /** Gets the name of the operating system version
   *
   * @returns The operating system version
   */
	std::string ConfigServiceImpl::getOSVersion()
	{
		return m_pSysConfig->getString("system.osVersion");
	}
	
  /** Gets the absolute path of the current directory containing the dll
   *
   * @returns The absolute path of the current directory containing the dll
   */
	std::string ConfigServiceImpl::getCurrentDir()
	{
		return m_pSysConfig->getString("system.currentDir");
	}
	  	
  /** Gets the absolute path of the temp directory 
   *
   * @returns The absolute path of the temp directory 
   */
	std::string ConfigServiceImpl::getTempDir()
	{
		return m_pSysConfig->getString("system.tempDir");
	}


	
/// \cond TEMPLATE 

	template DLLExport int ConfigServiceImpl::getValue(const std::string&,double&);
	template DLLExport int ConfigServiceImpl::getValue(const std::string&,std::string&);
	template DLLExport int ConfigServiceImpl::getValue(const std::string&,int&);

/// \endcond TEMPLATE

} // namespace Kernel
} // namespace Mantid
