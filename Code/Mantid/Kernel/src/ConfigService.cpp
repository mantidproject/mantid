//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Support.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/FilterChannel.h"
#include "MantidKernel/SignalChannel.h"
#include "MantidKernel/exception.h"
#include "Poco/Util/LoggingConfigurator.h"
#include "Poco/Util/SystemConfiguration.h"
#include "Poco/Util/PropertyFileConfiguration.h"
#include "Poco/LoggingFactory.h"
#include "Poco/Path.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

namespace Mantid
{
	namespace Kernel
	{

		//-------------------------------
		// Private member functions
		//-------------------------------

		/// Private constructor for singleton class
		ConfigServiceImpl::ConfigServiceImpl() : g_log(Logger::get("ConfigService"))
		{
			//getting at system details
			m_pSysConfig = new WrappedObject<Poco::Util::SystemConfiguration>;
			m_pConf = 0;
			m_pPropertyString = 0;

			//Register the FilterChannel with the Poco logging factory
			Poco::LoggingFactory::defaultFactory().registerChannelClass("FilterChannel",new Poco::Instantiator<Poco::FilterChannel, Poco::Channel>);

			//Register the SignalChannel with the Poco logging factory
			Poco::LoggingFactory::defaultFactory().registerChannelClass("SignalChannel",new Poco::Instantiator<Poco::SignalChannel, Poco::Channel>);

			//Determine how we are running mantid
			if( Mantid::Kernel::getPathToExecutable().rfind("python") != std::string::npos )
			{
				m_strBaseDir = Poco::Path::current();
			}
			else
			{
				m_strBaseDir = Mantid::Kernel::getDirectoryOfExecutable();
			}

			//attempt to load the default properties file that resides in the directory of the executable
			loadConfig( getBaseDir() + "Mantid.properties");
			//and then append the user properties
			loadConfig( getBaseDir() + "Mantid.user.properties", true);

			//Fill the list of possible relative path keys that may require conversion to absolute paths
			m_vConfigPaths.clear();
			m_vConfigPaths.push_back("plugins.directory");
			m_vConfigPaths.push_back("instrumentDefinition.directory");
			m_vConfigPaths.push_back("pythonscripts.directory");
			m_vConfigPaths.push_back("ManagedWorkspace.FilePath");

			convertRelativePaths();

			g_log.debug() << "ConfigService created." << std::endl;
			g_log.debug() << "Configured base directory of application as " << getBaseDir() << std::endl;

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
			delete m_pPropertyString;
			//		g_log.debug() << "ConfigService destroyed." << std::endl;
		}

		/**
		* Searches the stored list for keys that have been loaded from the config file and may contain
		* relative paths. Any it find are converted to absolute paths and stored separately
		*/
		void ConfigServiceImpl::convertRelativePaths()
		{
			if( m_vConfigPaths.empty() ) return;

			std::string execdir(getBaseDir());

			std::vector<std::string>::const_iterator send = m_vConfigPaths.end();
			for( std::vector<std::string>::const_iterator sitr = m_vConfigPaths.begin(); sitr != send; ++sitr )
			{
				if( !m_pConf->hasProperty(*sitr) ) continue;

				std::string value(m_pConf->getString(*sitr));
				if( Poco::Path(value).isRelative() )
				{
					m_mAbsolutePaths.insert(std::make_pair(*sitr, Poco::Path(execdir).resolve(value).toString()));
				}
			}
		}

		/**
		* Provides a default Configuration string to use if the config file cannot be loaded.
		*/
		const std::string ConfigServiceImpl::defaultConfig() const
		{
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
				return propFile;
		}


		//-------------------------------
		// Public member functions
		//-------------------------------

		/** Loads the config file provided.
		*  If the file contains logging setup instructions then these will be used to setup the logging framework.
		*
		*  @param filename The filename and optionally path of the file to load
		*  @param append If false (default) then any previous configuration is discarded, otherwise the new keys are added, and repeated keys will override existing ones.
		*/
		void ConfigServiceImpl::loadConfig(const std::string& filename, const bool append)
		{
			delete m_pConf;
			if (!append)
			{
				//remove the previous property string
				delete m_pPropertyString;
				m_pPropertyString = 0;
			}
   
			try
			{
				std::ifstream propFile(filename.c_str(),std::ios::in);
				bool good = propFile.good();

				//slurp in entire file - extremely unlikely delimter used as an alternate to \n
				std::string temp; 
				getline(propFile,temp,'¿');
				propFile.close();
 
				if ((!good) || (temp==""))
				{
					throw Exception::FileError("Cannot open file",filename);
				}

				//store the property string
				if((append) && (m_pPropertyString!=0))
				{
					m_pPropertyString = new std::string(*m_pPropertyString + "\n" + temp);
				}
				else
				{
					m_pPropertyString = new std::string(temp);
				}


				//m_pConf = new WrappedObject<Poco::Util::PropertyFileConfiguration>(filename);
			}
			catch (std::exception& e)
			{
				//there was a problem loading the file - it probably is not there
				std::cerr << "Problem loading the configuration file " << filename << " " << e.what() << std::endl;

				if(!append)
				{
					// if we have no property values then take the default
					m_pPropertyString = new std::string(defaultConfig());
				}
			}
			//use the cached property string to initialise the POCO property file
			std::istringstream istr(*m_pPropertyString);
			m_pConf = new WrappedObject<Poco::Util::PropertyFileConfiguration>(istr);

			try
			{
				//configure the logging framework
				Poco::Util::LoggingConfigurator configurator;
				configurator.configure(m_pConf);
			}
			catch (std::exception& e)
			{
				std::cerr << "Trouble configuring the logging framework " << e.what()<<std::endl;
			}
		}


		/** Searches for a string within the currently loaded configuaration values and 
		*  returns the value as a string. If the key is one of those that was a possible relative path
		*  then the local store is searched first.
		*
		*  @param keyName The case sensitive name of the property that you need the value of.
		*  @returns The string value of the property, or an empty string if the key cannot be found
		*/
		std::string ConfigServiceImpl::getString(const std::string& keyName)
		{
			std::map<std::string, std::string>::const_iterator mitr = m_mAbsolutePaths.find(keyName);
			if( mitr != m_mAbsolutePaths.end() ) 
			{
				return (*mitr).second;
			}
			std::string retVal;
			try
			{
				retVal = m_pConf->getString(keyName);
			}
			catch(Poco::NotFoundException& ex)
			{
				g_log.debug()<<"Unable to find " << keyName << " in the properties file" << std::endl;
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

		/**
		* Gets the directory that we consider to be the bse directory. Basically, this is the 
		* executable directory when running normally or the current directory on startup when
		* running through Python on the command line
		* @returns The directory to consider as the base directory, including a trailing slash
		*/
		std::string ConfigServiceImpl::getBaseDir()
		{
			return m_strBaseDir;
		}


		/// \cond TEMPLATE 

		template DLLExport int ConfigServiceImpl::getValue(const std::string&,double&);
		template DLLExport int ConfigServiceImpl::getValue(const std::string&,std::string&);
		template DLLExport int ConfigServiceImpl::getValue(const std::string&,int&);

		/// \endcond TEMPLATE

	} // namespace Kernel
} // namespace Mantid
