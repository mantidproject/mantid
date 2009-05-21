//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Support.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/FilterChannel.h"
#include "MantidKernel/SignalChannel.h"
#include "MantidKernel/Exception.h"
#include "Poco/Util/LoggingConfigurator.h"
#include "Poco/Util/SystemConfiguration.h"
#include "Poco/Util/PropertyFileConfiguration.h"
#include "Poco/LoggingFactory.h"
#include "Poco/Path.h"
#include "Poco/File.h"
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
		ConfigServiceImpl::ConfigServiceImpl() : 
		  g_log(Logger::get("ConfigService")), 
		  m_properties_file_name("Mantid.properties"),
		  m_user_properties_file_name("Mantid.user.properties")
		{
			//getting at system details
			m_pSysConfig = new WrappedObject<Poco::Util::SystemConfiguration>;
			m_pConf = 0;

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
			loadConfig( getBaseDir() + m_properties_file_name);
			//and then append the user properties
			loadConfig( getOutputDir() + m_user_properties_file_name, true);

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
        try {
          if( Poco::Path(value).isRelative() )
          {
            m_mAbsolutePaths.insert(std::make_pair(*sitr, Poco::Path(execdir).resolve(value).toString()));
          }
        }
        catch (Poco::PathSyntaxException &ex)
        {
          g_log.error() << ex.what() << " in .properties file: " << value << std::endl;
        }
			}
		}

		/**
		* writes a basic placeholder user.properties file to disk
		* any errors are caught and logged, but not propagated
		*/
		void ConfigServiceImpl::createUserPropertiesFile() const
		{
			try
			{
			        std::fstream filestr ((getOutputDir() + m_user_properties_file_name).c_str(), std::fstream::out);

				filestr << "# This file can be used to override any properties for this installation." << std::endl;
				filestr << "# Any properties found in this file will override any that are found in the Mantid.Properties file" << std::endl;
				filestr << "# As this file will not be replaced with futher installations of Mantid it is a safe place to put " << std::endl;
				filestr << "# properties that suit your particular installation." << std::endl;
				filestr << "" << std::endl;
				filestr << "#for example" << std::endl;
				filestr << "#uncommenting the line below will set the number of algorithms to retain interim results for to be 90" << std::endl;
				filestr << "#overriding any value set in the Mantid.properties file" << std::endl;
				filestr << "#algorithms.retained = 90" << std::endl;

				filestr.close();
			}
			catch (std::runtime_error ex)
			{
				g_log.error()<<"Unable to write out user.properties file to " << getOutputDir() << m_user_properties_file_name
					<< " error: " << ex.what() << std::endl;
			}

		}

		/**
		* Provides a default Configuration string to use if the config file cannot be loaded.
		* @returns The string value of default properties
		*/
		const std::string ConfigServiceImpl::defaultConfig() const
		{
				std::string propFile = 
					"# logging configuration"
					"# root level message filter (drop to debug for more messages)"
					"logging.loggers.root.level = debug"
					"# splitting the messages to many logging channels"
					"logging.loggers.root.channel.class = SplitterChannel"
					"logging.loggers.root.channel.channel1 = consoleChannel"
					"logging.loggers.root.channel.channel2 = fileFilterChannel"
					"logging.loggers.root.channel.channel3 = signalChannel"
					"# output to the console - primarily for console based apps"
					"logging.channels.consoleChannel.class = ConsoleChannel"
					"logging.channels.consoleChannel.formatter = f1"
					"# specfic filter for the file channel raising the level to warning (drop to debug for debugging)"
					"logging.channels.fileFilterChannel.class= FilterChannel"
					"logging.channels.fileFilterChannel.channel= fileChannel"
					"logging.channels.fileFilterChannel.level= warning"
					"# output to a file (For error capturing and debugging)"
					"logging.channels.fileChannel.class = debug"
					"logging.channels.fileChannel.path = ../logs/mantid.log"
					"logging.channels.fileChannel.formatter.class = PatternFormatter"
					"logging.channels.fileChannel.formatter.pattern = %Y-%m-%d %H:%M:%S,%i [%I] %p %s - %t"
					"logging.formatters.f1.class = PatternFormatter"
					"logging.formatters.f1.pattern = %s-[%p] %t"
					"logging.formatters.f1.times = UTC;"
					"# SignalChannel - Passes messages to the MantidPlot User interface"
					"logging.channels.signalChannel.class = SignalChannel";
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
				m_PropertyString = "";
			}
   
			try
			{
				std::ifstream propFile(filename.c_str(),std::ios::in);
				bool good = propFile.good();

				//slurp in entire file - extremely unlikely delimter used as an alternate to \n
				std::string temp; 
				getline(propFile,temp,'¬');
				propFile.close();
 
				// check if we have failed to open the file
				if ((!good) || (temp==""))
				{
				  if (filename == getOutputDir() + m_user_properties_file_name)
					{
						//write out a fresh file
						createUserPropertiesFile();
					}
					else
					{
						throw Exception::FileError("Cannot open file",filename);
					}
				}

				//store the property string
				if((append) && (m_PropertyString!=""))
				{
					m_PropertyString = m_PropertyString + "\n" + temp;
				}
				else
				{
					m_PropertyString = temp;
				}

			}
			catch (std::exception& e)
			{
				//there was a problem loading the file - it probably is not there
				std::cerr << "Problem loading the configuration file " << filename << " " << e.what() << std::endl;

				if(!append)
				{
					// if we have no property values then take the default
					m_PropertyString = defaultConfig();
				}
			}

			//use the cached property string to initialise the POCO property file
			std::istringstream istr(m_PropertyString);
			m_pConf = new WrappedObject<Poco::Util::PropertyFileConfiguration>(istr);

			try
			{
			        //Ensure that the logging directory exists
			        Poco::Path logpath(getString("logging.channels.fileChannel.path"));
				//make this path point to the parent directory and create it if it does not exist
				logpath.makeParent();
				if( !logpath.toString().empty() )
				{
				  Poco::File(logpath).createDirectory();
				}
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
			catch(Poco::NotFoundException&)
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
		std::string ConfigServiceImpl::getBaseDir() const
		{
			return m_strBaseDir;
		}
	        
	        /**
		 * Return the directory that Mantid should use for writing files. A trailing slash is appended
		 * so that filenames can more easily be concatenated with this
		 */
	        std::string ConfigServiceImpl::getOutputDir() const
		{
             #ifdef _WIN32 
		  return m_strBaseDir;
             #else
		  Poco::Path datadir(m_pSysConfig->getString("system.homeDir"));
		  datadir.append(".mantid");
		  // Create the directory if it doesn't already exist
		  Poco::File(datadir).createDirectory();
		  return datadir.toString() + "/";
             #endif
		}

		/// \cond TEMPLATE 

		template DLLExport int ConfigServiceImpl::getValue(const std::string&,double&);
		template DLLExport int ConfigServiceImpl::getValue(const std::string&,std::string&);
		template DLLExport int ConfigServiceImpl::getValue(const std::string&,int&);

		/// \endcond TEMPLATE

	} // namespace Kernel
} // namespace Mantid
