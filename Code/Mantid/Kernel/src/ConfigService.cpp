//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/MantidVersion.h"
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
#include "Poco/StringTokenizer.h"

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
    m_pConf(NULL), m_pSysConfig(NULL),
    g_log(Logger::get("ConfigService")), 
    m_ConfigPaths(), m_AbsolutePaths(),
    m_strBaseDir(""), m_PropertyString(""),
    m_properties_file_name("Mantid.properties"),
    m_user_properties_file_name("Mantid.user.properties"),
    m_DataSearchDirs()
  {
    //getting at system details
    m_pSysConfig = new WrappedObject<Poco::Util::SystemConfiguration>;
    m_pConf = 0;

    //Register the FilterChannel with the Poco logging factory
    Poco::LoggingFactory::defaultFactory().registerChannelClass("FilterChannel",new Poco::Instantiator<Poco::FilterChannel, Poco::Channel>);

    //Register the SignalChannel with the Poco logging factory
    Poco::LoggingFactory::defaultFactory().registerChannelClass("SignalChannel",new Poco::Instantiator<Poco::SignalChannel, Poco::Channel>);

    // Define a directory that serves as the 'application directory'. This is where we expect to find the Mantid.properties file
    // It cannot simply be the current directory since the application may be called from a different place, .i.e.
    // on Linux where the bin directory is in the path and the app is run from a different location.
    // We have two scenarios:
    //  1) The framework is compiled into an executable and its directory is then considered as the base or,
    //  2) The framework is in a stand-alone library and is created from within another executing application
    //     e.g. Python or Matlab (only two at the moment). We can only use the current directory here as there
    //     is no programmatic way of determing where the library that contains this class is. 

    // A MANTID environmental variable might solve all of this??

    std::string callingApplication = Poco::Path(getPathToExecutable()).getFileName();
    // the cases used in the names varies on different systems so we do this case insensitive
    std::transform(callingApplication.begin(), callingApplication.end(),
      callingApplication.begin(), tolower);
    if ( callingApplication.find("python") != std::string::npos || callingApplication.find("matlab") != std::string::npos)
    {
      m_strBaseDir = Poco::Path::current();
    }
    else
    {
      m_strBaseDir = Mantid::Kernel::getDirectoryOfExecutable();
    }

    //Fill the list of possible relative path keys that may require conversion to absolute paths
    m_ConfigPaths.insert(std::make_pair("plugins.directory", true));
    m_ConfigPaths.insert(std::make_pair("instrumentDefinition.directory", true));
    m_ConfigPaths.insert(std::make_pair("requiredpythonscript.directories", true));
    m_ConfigPaths.insert(std::make_pair("pythonscripts.directory", false));
    m_ConfigPaths.insert(std::make_pair("pythonscripts.directories", false));
    m_ConfigPaths.insert(std::make_pair("ManagedWorkspace.FilePath", false));
    m_ConfigPaths.insert(std::make_pair("defaultsave.directory", false));
    m_ConfigPaths.insert(std::make_pair("datasearch.directories",false));
    m_ConfigPaths.insert(std::make_pair("pythonalgorithms.directories",false));

    //attempt to load the default properties file that resides in the directory of the executable
    loadConfig( getBaseDir() + m_properties_file_name);
    //and then append the user properties
    loadConfig( getOutputDir() + m_user_properties_file_name, true);

    g_log.debug() << "ConfigService created." << std::endl;
    g_log.debug() << "Configured base directory of application as " << getBaseDir() << std::endl;
    g_log.information() << "This is Mantid Version " << MANTID_VERSION << std::endl;
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
    Kernel::Logger::shutdown();
    delete m_pSysConfig;
    delete m_pConf;                // potential double delete???
  }

  /**
  * Searches the stored list for keys that have been loaded from the config file and may contain
  * relative paths. Any it find are converted to absolute paths and stored separately
  */
  void ConfigServiceImpl::convertRelativeToAbsolute()
  {
    if( m_ConfigPaths.empty() ) return;

    std::string execdir(getBaseDir());
    m_AbsolutePaths.clear();

    std::map<std::string, bool>::const_iterator send = m_ConfigPaths.end();
    for( std::map<std::string, bool>::const_iterator sitr = m_ConfigPaths.begin(); sitr != send; ++sitr )
    {
      std::string key = sitr->first;
      if( !m_pConf->hasProperty(key) ) continue;

      std::string value(m_pConf->getString(key));
      value = makeAbsolute(value, key);
      if( !value.empty() )
      {
        m_AbsolutePaths.insert(std::make_pair(key, value));
      }
    }
  }

  /**
  * Make a relative path or a list of relative paths into an absolute one.
  * @param dir The directory to convert
  * @param key The key variable this relates to
  * @returns A string containing an aboluste path by resolving the relative directory with the executable directory
  */
  std::string ConfigServiceImpl::makeAbsolute(const std::string & dir, const std::string & key) const
  {
    std::string converted;
    const std::string execdir(getBaseDir());
    // If we have a list, chop it up and convert each one
    if( dir.find_first_of(";,") != std::string::npos )
    {
      int options = Poco::StringTokenizer::TOK_TRIM + Poco::StringTokenizer::TOK_IGNORE_EMPTY;
      Poco::StringTokenizer tokenizer(dir, ";,", options);
      Poco::StringTokenizer::Iterator iend = tokenizer.end();
      for( Poco::StringTokenizer::Iterator itr = tokenizer.begin(); itr != iend; )
      {
        std::string absolute = makeAbsolute(*itr, key);
        if( !absolute.empty() )
        {
          converted += absolute;
        }
        if( ++itr != iend )
        {
          converted += ";";
        }
      }
      return converted;
    }

    // MG 05/10/09: When the Poco::FilePropertyConfiguration object reads its key/value pairs it 
    // treats a backslash as the start of an escape sequence. If the next character does not
    // form a valid sequence then the backslash is removed from the stream. This has the effect
    // of giving malformed paths when using Windows-style directories. E.g C:\Mantid ->C:Mantid
    // and Poco::Path::isRelative throws an exception on this
    bool is_relative(false);
    try
    {
      is_relative = Poco::Path(dir).isRelative();
    }
    catch(Poco::PathSyntaxException &)
    {
      g_log.error() << "Malformed path detected in the \"" << key << "\" variable, skipping \"" << dir << "\"\n";
      return "";
    }
    if( is_relative )
    {
      converted = Poco::Path(execdir).resolve(dir).toString();
    }
    else
    {
      converted = dir;
    }
    converted = Poco::Path(converted).makeDirectory().toString();
    
    // C++ doesn't have a const version of operator[] for maps so I can't call that here
    std::map<std::string,bool>::const_iterator it = m_ConfigPaths.find(key);
    bool required = false;
    if( it != m_ConfigPaths.end() )
    {
      required = it->second;
    }
    if( required && !Poco::File(converted).exists() )
    {
     g_log.error() << "Required properties path \"" << converted << "\" in the \"" << key << "\" variable does not exist.\n";
     converted = "";
    }
    return converted;
 }


  /**
  * Create the store of data search paths from the 'datasearch.directories' key within the Mantid.properties file.
  * The value of the key should be a semi-colon separated list of directories
  */
  void ConfigServiceImpl::defineDataSearchPaths()
  {
    m_DataSearchDirs.clear();
    std::string paths = getString("datasearch.directories");
    //Nothing to do
    if( paths.empty() ) return;
    int options = Poco::StringTokenizer::TOK_TRIM + Poco::StringTokenizer::TOK_IGNORE_EMPTY;
    Poco::StringTokenizer tokenizer(paths, ";,", options);
    Poco::StringTokenizer::Iterator iend = tokenizer.end();
    m_DataSearchDirs.reserve(tokenizer.count());
    for( Poco::StringTokenizer::Iterator itr = tokenizer.begin(); itr != iend; ++itr )
    {
      m_DataSearchDirs.push_back(*itr);
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
      if( logpath.toString().empty() || getOutputDir() != getBaseDir() )
      {
        std::string logfile = getOutputDir() + "mantid.log";
        logpath.assign(logfile);
        m_pConf->setString("logging.channels.fileChannel.path", logfile);
      }
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

    //Ensure that any relative paths given in the configuration file are relative to the current directory
    convertRelativeToAbsolute();
    //Configure search paths into a specially saved store as they will be used frequently
    defineDataSearchPaths();
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
    std::map<std::string, std::string>::const_iterator mitr = m_AbsolutePaths.find(keyName);
    if( mitr != m_AbsolutePaths.end() ) 
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

  const std::vector<std::string>& ConfigServiceImpl::getDataSearchDirs() const
  {
    return m_DataSearchDirs;
  }

  /// \cond TEMPLATE 

  template DLLExport int ConfigServiceImpl::getValue(const std::string&,double&);
  template DLLExport int ConfigServiceImpl::getValue(const std::string&,std::string&);
  template DLLExport int ConfigServiceImpl::getValue(const std::string&,int&);

  /// \endcond TEMPLATE

  } // namespace Kernel
} // namespace Mantid
