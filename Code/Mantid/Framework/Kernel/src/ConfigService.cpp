//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/FilterChannel.h"
#include "MantidKernel/SignalChannel.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"

#include <Poco/Util/LoggingConfigurator.h>
#include <Poco/Util/SystemConfiguration.h>
#include <Poco/Util/PropertyFileConfiguration.h>
#include <Poco/LoggingFactory.h>
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/StringTokenizer.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/Notification.h>
#include <Poco/Environment.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/join.hpp>

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

namespace Mantid
{
/**
 * Get the welcome message for Mantid.
 * @returns A string containing the welcome message for Mantid.
 */
std::string welcomeMessage()
{
  return "Welcome to Mantid - Manipulation and Analysis Toolkit for Instrument Data";
}

namespace Kernel
{

/** Inner templated class to wrap the poco library objects that have protected
 *  destructors and expose them as public.
 */
template<typename T>
class ConfigServiceImpl::WrappedObject: public T
{
public:
  /// The template type of class that is being wrapped
  typedef T element_type;
  /// Simple constructor
  WrappedObject() :
    T()
  {
    m_pPtr = static_cast<T*> (this);
  }

  /** Constructor with a class to wrap
   *  @param F :: The object to wrap
   */
  template<typename Field>
  WrappedObject(Field& F) :
    T(F)
  {
    m_pPtr = static_cast<T*> (this);
  }

  /// Copy constructor
  WrappedObject(const WrappedObject<T>& A) :
    T(A)
  {
    m_pPtr = static_cast<T*> (this);
  }

  /// Virtual destructor
  virtual ~WrappedObject()
  {
  }

  /// Overloaded * operator returns the wrapped object pointer
  const T& operator*() const
  {
    return *m_pPtr;
  }
  /// Overloaded * operator returns the wrapped object pointer
  T& operator*()
  {
    return m_pPtr;
  }
  /// Overloaded -> operator returns the wrapped object pointer
  const T* operator->() const
  {
    return m_pPtr;
  }
  /// Overloaded -> operator returns the wrapped object pointer
  T* operator->()
  {
    return m_pPtr;
  }

private:
  /// Private pointer to the wrapped class
  T* m_pPtr;
};

//Back to the ConfigService class itself...

//-------------------------------
// Private member functions
//-------------------------------

/// Private constructor for singleton class
ConfigServiceImpl::ConfigServiceImpl() :
  m_pConf(NULL), m_pSysConfig(NULL), g_log(Logger::get("ConfigService")), m_changed_keys(),
      m_ConfigPaths(), m_AbsolutePaths(), m_strBaseDir(""), m_PropertyString(""),
      m_properties_file_name("Mantid.properties"),
      m_user_properties_file_name("Mantid.user.properties"), m_DataSearchDirs(), m_UserSearchDirs(),
      m_instr_prefixes()
{
  //getting at system details
  m_pSysConfig = new WrappedObject<Poco::Util::SystemConfiguration> ;
  m_pConf = 0;

  //Register the FilterChannel with the Poco logging factory
  Poco::LoggingFactory::defaultFactory().registerChannelClass("FilterChannel", new Poco::Instantiator<
      Poco::FilterChannel, Poco::Channel>);

  //Register the SignalChannel with the Poco logging factory
  Poco::LoggingFactory::defaultFactory().registerChannelClass("SignalChannel", new Poco::Instantiator<
      Poco::SignalChannel, Poco::Channel>);

  // Define the directory to search for the Mantid.properties file.
  Poco::File f;

  // First directory: the current working
  m_strBaseDir = Poco::Path::current();
  f = Poco::File(m_strBaseDir + m_properties_file_name);
  if (!f.exists())
  {
    // Check the executable directory to see if it includes a mantid.properties file
    m_strBaseDir = Mantid::Kernel::getDirectoryOfExecutable();
    f = Poco::File(m_strBaseDir + m_properties_file_name);
    if (!f.exists())
    {
      // Last, use the MANTIDPATH environment var
      if (Poco::Environment::has("MANTIDPATH"))
      {
        // Here we have to follow the convention of the rest of this code and add a trailing slash.
        // Note: adding it to the MANTIDPATH itself will make other parts of the code crash.
        m_strBaseDir = Poco::Environment::get("MANTIDPATH") + "/";
      }
    }
  }

  //Fill the list of possible relative path keys that may require conversion to absolute paths
  m_ConfigPaths.insert(std::make_pair("plugins.directory", true));
  m_ConfigPaths.insert(std::make_pair("mantidqt.plugins.directory", true));
  m_ConfigPaths.insert(std::make_pair("instrumentDefinition.directory", true));
  m_ConfigPaths.insert(std::make_pair("parameterDefinition.directory", true));
  m_ConfigPaths.insert(std::make_pair("requiredpythonscript.directories", true));
  m_ConfigPaths.insert(std::make_pair("pythonscripts.directory", true));
  m_ConfigPaths.insert(std::make_pair("pythonscripts.directories", true));
  m_ConfigPaths.insert(std::make_pair("ManagedWorkspace.FilePath", true));
  m_ConfigPaths.insert(std::make_pair("defaultsave.directory", false));
  m_ConfigPaths.insert(std::make_pair("datasearch.directories", true));
  m_ConfigPaths.insert(std::make_pair("pythonalgorithms.directories", true));
  m_ConfigPaths.insert(std::make_pair("icatDownload.directory", true));
  m_ConfigPaths.insert(std::make_pair("mantidqt.python_interfaces_directory", true));

  //attempt to load the default properties file that resides in the directory of the executable
  std::string propertiesFilesList;
  updateConfig(getPropertiesDir() + m_properties_file_name, false, false);
  propertiesFilesList = getPropertiesDir() + m_properties_file_name;

  if (Poco::Environment::has("MANTIDPROPERTIES"))
  {
    //and then append the user properties
    updateConfig(getUserFilename(), true, false);
    propertiesFilesList += ", " + getUserFilename();
    //and the extra one from the environment
    updateConfig(Poco::Environment::get("MANTIDPROPERTIES"), true, true);
    propertiesFilesList += ", " + Poco::Environment::get("MANTIDPROPERTIES");
  }
  else
  {
    // Just do the user properties
    updateConfig(getUserFilename(), true, true);
    propertiesFilesList += ", " + getUserFilename();
  }

  updateFacilities();

  g_log.debug() << "ConfigService created." << std::endl;
  g_log.debug() << "Configured Mantid.properties directory of application as " << getPropertiesDir()
      << std::endl;
  g_log.information() << "This is Mantid Version " << MantidVersion::version() << std::endl;
  g_log.information() << "Properties file(s) loaded: " << propertiesFilesList << std::endl;
  g_log.information() << "Logging to: " << m_logFilePath << std::endl;

  // Make sure the log path is shown somewhere.
  //std::cout << "Logging to: " << m_logFilePath << std::endl;

}

/** Private Destructor
 *  Prevents client from calling 'delete' on the pointer handed out by Instance
 */
ConfigServiceImpl::~ConfigServiceImpl()
{
  //std::cerr << "ConfigService destroyed." << std::endl;
  Kernel::Logger::shutdown();
  delete m_pSysConfig;
  delete m_pConf; // potential double delete???
  for (std::vector<FacilityInfo*>::iterator it = m_facilities.begin(); it != m_facilities.end(); ++it)
  {
    delete *it;
  }
  m_facilities.clear();
}

/** Loads the config file provided.
 *  If the file contains logging setup instructions then these will be used to setup the logging framework.
 *
 *  @param filename :: The filename and optionally path of the file to load
 *  @param append :: If false (default) then any previous configuration is discarded, otherwise the new keys are added, and repeated keys will override existing ones.
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
    //slurp in entire file
    std::string temp;
    bool good = readFile(filename, temp);

    // check if we have failed to open the file
    if ((!good) || (temp == ""))
    {
      if (filename == getUserPropertiesDir() + m_user_properties_file_name)
      {
        //write out a fresh file
        createUserPropertiesFile();
      }
      else
      {
        throw Exception::FileError("Cannot open file", filename);
      }
    }

    //store the property string
    if ((append) && (m_PropertyString != ""))
    {
      m_PropertyString = m_PropertyString + "\n" + temp;
    }
    else
    {
      m_PropertyString = temp;
    }

  } catch (std::exception& e)
  {
    //there was a problem loading the file - it probably is not there
    std::cerr << "Problem loading the configuration file " << filename << " " << e.what() << std::endl;

    if (!append)
    {
      // if we have no property values then take the default
      m_PropertyString = defaultConfig();
    }
  }

  //use the cached property string to initialise the POCO property file
  std::istringstream istr(m_PropertyString);
  m_pConf = new WrappedObject<Poco::Util::PropertyFileConfiguration> (istr);
}

/**
 * Read a file and place its contents into the given string
 * @param filename :: The filename of the file to read
 * @param contents :: The file contents will be placed here
 * @returns A boolean indicating whether opening the file was successful
 */
bool ConfigServiceImpl::readFile(const std::string& filename, std::string & contents) const
{
  std::ifstream propFile(filename.c_str(), std::ios::in);
  bool good = propFile.good();
  if (!good)
  {
    contents = "";
    propFile.close();
    return good;
  }

  //slurp in entire file - extremely unlikely delimiter used as an alternate to \n
  contents.clear();
  getline(propFile, contents, '`');
  propFile.close();
  return good;
}

/** Configures the Poco logging and starts it up
 *
 */
void ConfigServiceImpl::configureLogging()
{
  try
  {
    //Ensure that the logging directory exists
    m_logFilePath = getString("logging.channels.fileChannel.path");
    Poco::Path logpath(m_logFilePath);

    // Undocumented way to override the mantid.log path
    if (Poco::Environment::has("MANTIDLOGPATH"))
    {
      logpath = Poco::Path(Poco::Environment::get("MANTIDLOGPATH"));
      logpath = logpath.absolute();
      m_logFilePath = logpath.toString();
    }

    // An absolute path makes things simpler
    logpath = logpath.absolute();


    // First, try the logpath given
    if (!m_logFilePath.empty())
    {
      try
      {
        // Save it for later
        m_logFilePath = logpath.toString();

        //make this path point to the parent directory and create it if it does not exist
        Poco::Path parent = logpath;
        parent.makeParent();
        Poco::File(parent).createDirectories();

        // Try to create or append to the file. If it fails, use the default
        FILE *fp = fopen(m_logFilePath.c_str(), "a+");
        if (fp == NULL)
        {
          std::cerr << "Error writing to log file path given in properties file: \"" << m_logFilePath
              << "\". Will use a default path instead." << std::endl;
          // Clear the path; this will make it use the default
          m_logFilePath = "";
        }
        else
          fclose(fp);
      } catch (std::exception &)
      {
        std::cerr << "Error writing to log file path given in properties file: \"" << m_logFilePath
            << "\". Will use a default path instead." << std::endl;
        // ERROR! Maybe the file is not writable!
        // Clear the path; this will make it use the default
        m_logFilePath = "";
      }
    }

    // The path given was invalid somehow? Use a default
    if (m_logFilePath.empty())
    {
      m_logFilePath = getUserPropertiesDir() + "mantid.log";
      logpath.assign(m_logFilePath);
      logpath = logpath.absolute();
      m_logFilePath = logpath.toString();
    }
    // Set the line in the configuration properties.
    //  this'll be picked up by LoggingConfigurator (somehow)
    m_pConf->setString("logging.channels.fileChannel.path", m_logFilePath);

    //make this path point to the parent directory and create it if it does not exist
    logpath.makeParent();
    if (!logpath.toString().empty())
    {
      Poco::File(logpath).createDirectories(); // Also creates all necessary directories
    }

    // Configure the logging framework
    Poco::Util::LoggingConfigurator configurator;
    configurator.configure(m_pConf);
  } catch (std::exception& e)
  {
    std::cerr << "Trouble configuring the logging framework " << e.what() << std::endl;
  }

}

/**
 * Searches the stored list for keys that have been loaded from the config file and may contain
 * relative paths. Any it find are converted to absolute paths and stored separately
 */
void ConfigServiceImpl::convertRelativeToAbsolute()
{
  if (m_ConfigPaths.empty())
    return;

  m_AbsolutePaths.clear();
  std::map<std::string, bool>::const_iterator send = m_ConfigPaths.end();
  for (std::map<std::string, bool>::const_iterator sitr = m_ConfigPaths.begin(); sitr != send; ++sitr)
  {
    std::string key = sitr->first;
    if (!m_pConf->hasProperty(key))
      continue;

    std::string value(m_pConf->getString(key));
    value = makeAbsolute(value, key);
    m_AbsolutePaths.insert(std::make_pair(key, value));
  }
}

/**
 * Make a relative path or a list of relative paths into an absolute one.
 * @param dir :: The directory to convert
 * @param key :: The key variable this relates to
 * @returns A string containing an aboluste path by resolving the relative directory with the executable directory
 */
std::string ConfigServiceImpl::makeAbsolute(const std::string & dir, const std::string & key) const
{
  std::string converted;
  // If we have a list, chop it up and convert each one
  if (dir.find_first_of(";,") != std::string::npos)
  {
    int options = Poco::StringTokenizer::TOK_TRIM + Poco::StringTokenizer::TOK_IGNORE_EMPTY;
    Poco::StringTokenizer tokenizer(dir, ";,", options);
    Poco::StringTokenizer::Iterator iend = tokenizer.end();
    for (Poco::StringTokenizer::Iterator itr = tokenizer.begin(); itr != iend;)
    {
      std::string absolute = makeAbsolute(*itr, key);
      if (absolute.empty())
      {
        ++itr;
      }
      else
      {
        converted += absolute;
        if (++itr != iend)
        {
          converted += ";";
        }
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
  } catch (Poco::PathSyntaxException&)
  {
    g_log.warning() << "Malformed path detected in the \"" << key << "\" variable, skipping \"" << dir
        << "\"\n";
    return "";
  }
  if (is_relative)
  {
    const std::string propFileDir(getPropertiesDir());
    converted = Poco::Path(propFileDir).resolve(dir).toString();
  }
  else
  {
    converted = dir;
  }
  converted = Poco::Path(converted).makeDirectory().toString();

  // C++ doesn't have a const version of operator[] for maps so I can't call that here
  std::map<std::string, bool>::const_iterator it = m_ConfigPaths.find(key);
  bool required = false;
  if (it != m_ConfigPaths.end())
  {
    required = it->second;
  }
  try
  {
    if (required && !Poco::File(converted).exists())
    {
      g_log.warning() << "Required properties path \"" << converted << "\" in the \"" << key
          << "\" variable does not exist.\n";
      converted = "";
    }
  } catch ( Poco::FileException & )
  {
    g_log.warning() << "Required properties path \"" << converted << "\" in the \"" << key
        << "\" variable does not exist.\n";
    converted = "";
  }
  
  // Backward slashes cannot be allowed to go into our properties file
  // Note this is a temporary fix for ticket #2445. 
  // Ticket #2460 prompts a review of our path handling in the config service.
  boost::replace_all(converted, "\\", "/");
  return converted;
}

/**
 * Create the store of data search paths from the 'datasearch.directories' key within the Mantid.properties file.
 * The value of the key should be a semi-colon separated list of directories
 */
void ConfigServiceImpl::cacheDataSearchPaths()
{
  m_DataSearchDirs.clear();
  std::string paths = getString("datasearch.directories");
  //Nothing to do
  if (paths.empty())
    return;
  int options = Poco::StringTokenizer::TOK_TRIM + Poco::StringTokenizer::TOK_IGNORE_EMPTY;
  Poco::StringTokenizer tokenizer(paths, ";,", options);
  Poco::StringTokenizer::Iterator iend = tokenizer.end();
  m_DataSearchDirs.reserve(tokenizer.count());
  for (Poco::StringTokenizer::Iterator itr = tokenizer.begin(); itr != iend; ++itr)
  {
    m_DataSearchDirs.push_back(*itr);
  }
}

/**
 * Create the store of user search paths from the 'usersearch.directories' key within the Mantid.properties file.
 * The value of the key should be a semi-colon separated list of directories
 */
void ConfigServiceImpl::cacheUserSearchPaths()
{
  m_UserSearchDirs.clear();
  std::string paths = getString("usersearch.directories");
  //Nothing to do
  if (paths.empty())
    return;
  int options = Poco::StringTokenizer::TOK_TRIM + Poco::StringTokenizer::TOK_IGNORE_EMPTY;
  Poco::StringTokenizer tokenizer(paths, ";,", options);
  Poco::StringTokenizer::Iterator iend = tokenizer.end();
  m_UserSearchDirs.reserve(tokenizer.count());
  for (Poco::StringTokenizer::Iterator itr = tokenizer.begin(); itr != iend; ++itr)
  {
    m_UserSearchDirs.push_back(*itr);
  }
}

/**
 *  The path that is passed should be as returned by makeAbsolute() and
 *  this function will return true if that path is in the list
 *  @param path :: the absolute path name to search for
 *  @return true if the path was found
 */
bool ConfigServiceImpl::isInDataSearchList(const std::string & path) const
{
  std::vector<std::string>::const_iterator it = std::find_if(m_DataSearchDirs.begin(),
      m_DataSearchDirs.end(), std::bind2nd(std::equal_to<std::string>(), path));
  return (it != m_DataSearchDirs.end());
}

/**
 * writes a basic placeholder user.properties file to disk
 * any errors are caught and logged, but not propagated
 */
void ConfigServiceImpl::createUserPropertiesFile() const
{
  try
  {
    std::fstream filestr((getUserPropertiesDir() + m_user_properties_file_name).c_str(),
        std::fstream::out);

    filestr << "# This file can be used to override any properties for this installation." << std::endl;
    filestr
        << "# Any properties found in this file will override any that are found in the Mantid.Properties file"
        << std::endl;
    filestr
        << "# As this file will not be replaced with futher installations of Mantid it is a safe place to put "
        << std::endl;
    filestr << "# properties that suit your particular installation." << std::endl;
    filestr << "" << std::endl;
    filestr << "#for example" << std::endl;
    filestr
        << "#uncommenting the line below will set the number of algorithms to retain interim results for to be 90"
        << std::endl;
    filestr << "#overriding any value set in the Mantid.properties file" << std::endl;
    filestr << "#algorithms.retained = 90" << std::endl;

    filestr.close();
  } catch (std::runtime_error& ex)
  {
    g_log.warning() << "Unable to write out user.properties file to " << getUserPropertiesDir()
        << m_user_properties_file_name << " error: " << ex.what() << std::endl;
  }

}

/**
 * Provides a default Configuration string to use if the config file cannot be loaded.
 * @returns The string value of default properties
 */
std::string ConfigServiceImpl::defaultConfig() const
{
  std::string propFile = "# logging configuration"
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

/** Updates and existing configuration and restarts the logging
 *  @param filename :: The filename and optionally path of the file to load
 *  @param append ::   If false (default) then any previous configuration is discarded,
 *                  otherwise the new keys are added, and repeated keys will override existing ones.
 *  @param update_caches :: If true(default) then the various property caches are updated
 */
void ConfigServiceImpl::updateConfig(const std::string& filename, const bool append,
    const bool update_caches)
{
  //std::cout << "Properties file loaded: " <<  filename << std::endl;
  loadConfig(filename, append);
  if (update_caches)
  {
    // Only configure logging once
    configureLogging();
    //Ensure that any relative paths given in the configuration file are relative to the correct directory
    convertRelativeToAbsolute();
    //Configure search paths into a specially saved store as they will be used frequently
    cacheDataSearchPaths();
    cacheUserSearchPaths();
  }
}

/**
 * Save the configuration to the user file
 * @param filename :: The filename for the saved configuration
 * @throw std::runtime_error if the file cannot be opened
 */
void ConfigServiceImpl::saveConfig(const std::string & filename) const
{
  if (m_changed_keys.empty())
    return;

  // Open and read the user properties file
  std::string updated_file("");

  std::ifstream reader(filename.c_str(), std::ios::in);
  if (reader.bad())
  {
    g_log.error() << "Error reading current user properties file. Cannot save updated configuration.\n";
    throw std::runtime_error("Error opening user properties file. Cannot save updated configuration.");
  }

  std::string file_line(""), output("");
  bool line_continuing(false);
  while (std::getline(reader, file_line))
  {
    if (!file_line.empty())
    {
      char last = *(file_line.end() - 1);
      if (last == '\\')
      {
        // If we are not in line continuation mode then need
        // a fresh start line
        if (!line_continuing)
          output = "";
        line_continuing = true;
        output += file_line + "\n";
        continue;
      }
      else if (line_continuing)
      {
        output += file_line;
        line_continuing = false;
      }
      else
      {
        output = file_line;
      }
    }
    else
    {
      output = "";
      updated_file += "\n";
      continue;
    }
    std::set<std::string>::iterator iend = m_changed_keys.end();
    std::set<std::string>::iterator itr = m_changed_keys.begin();
    for (; itr != iend; ++itr)
    {
      if (output.find(*itr) != std::string::npos)
      {
        break;
      }
    }

    if (itr == iend)
    {
      updated_file += output;
    }
    else
    {
      std::string key = *itr;
      std::string value = getString(*itr, false);
      updated_file += key + "=" + value;
      //Remove the key from the changed key list
      m_changed_keys.erase(itr);
    }
    updated_file += "\n";

  }

  // Any remaining keys within the changed key store weren't present in the current user properties so append them
  if (!m_changed_keys.empty())
  {
    updated_file += "\n";
    std::set<std::string>::iterator key_end = m_changed_keys.end();
    for (std::set<std::string>::iterator key_itr = m_changed_keys.begin(); key_itr != key_end;)
    {
      updated_file += *key_itr + "=";
      updated_file += getString(*key_itr, false);
      if (++key_itr != key_end)
      {
        updated_file += "\n";
      }
    }
    m_changed_keys.clear();
  }

  // Write out the new file
  std::ofstream writer(filename.c_str(), std::ios_base::trunc);
  if (writer.bad())
  {
    writer.close();
    g_log.error() << "Error writing new user properties file. Cannot save current configuration.\n";
    throw std::runtime_error(
        "Error writing new user properties file. Cannot save current configuration.");
  }

  writer.write(updated_file.c_str(), updated_file.size());
  writer.close();
}

/** Searches for a string within the currently loaded configuaration values and
 *  returns the value as a string. If the key is one of those that was a possible relative path
 *  then the local store is searched first.
 *
 *  @param keyName :: The case sensitive name of the property that you need the value of.
 *  @param use_cache :: If true, the local cache of directory names is queried first.
 *  @returns The string value of the property, or an empty string if the key cannot be found
 */
std::string ConfigServiceImpl::getString(const std::string& keyName, bool use_cache) const
{
  if (use_cache)
  {
    std::map<std::string, std::string>::const_iterator mitr = m_AbsolutePaths.find(keyName);
    if (mitr != m_AbsolutePaths.end())
    {
      return (*mitr).second;
    }
  }
  std::string retVal;
  try
  {
    retVal = m_pConf->getString(keyName);
  } catch (Poco::NotFoundException&)
  {
    g_log.debug() << "Unable to find " << keyName << " in the properties file" << std::endl;
    retVal = "";
  }
  return retVal;
}

/**
 * Set a configuration property. An existing key will have its value updated.
 * @param key :: The key to refer to this property
 * @param value :: The value of the property
 */
void ConfigServiceImpl::setString(const std::string & key, const std::string & value)
{
  std::string old;
  try
  {
    old = m_pConf->getString(key);
  } catch (Poco::NotFoundException &)
  {
    old = "";
  }

  //Ensure we keep a correct full path
  std::map<std::string, bool>::const_iterator itr = m_ConfigPaths.find(key);
  if (itr != m_ConfigPaths.end())
  {
    m_AbsolutePaths[key] = makeAbsolute(value, key);
  }

  if (key == "datasearch.directories")
  {
    cacheDataSearchPaths();
  }
  else if (key == "usersearch.directories")
  {
    cacheUserSearchPaths();
  }
  else if (key == "defaultsave.directory")
  {
    //Some recursion here! As this call calls the current function
    appendDataSearchDir(m_AbsolutePaths[key]);
  }

  // If this key exists within the loaded configuration then mark that its value will have
  // changed from the default
  if (m_pConf->hasProperty(key))
  {
    m_changed_keys.insert(key);
  }

  m_pConf->setString(key, value);

  if (value != old)
    m_notificationCenter.postNotification(new ValueChanged(key, value, old));
}

/** Searches for a string within the currently loaded configuaration values and
 *  attempts to convert the values to the template type supplied.
 *
 *  @param keyName :: The case sensitive name of the property that you need the value of.
 *  @param out ::     The value if found
 *  @returns A success flag - 0 on failure, 1 on success
 */
template<typename T>
int ConfigServiceImpl::getValue(const std::string& keyName, T& out)
{
  std::string strValue = getString(keyName);
  int result = Mantid::Kernel::Strings::convert(strValue, out);
  return result;
}

/**
 * Return the full filename of the user properties file
 * @returns A string containg the full path to the user file
 */
std::string ConfigServiceImpl::getUserFilename() const
{
  return getUserPropertiesDir() + m_user_properties_file_name;
}

/** Searches for the string within the environment variables and returns the
 *  value as a string.
 *
 *  @param keyName :: The name of the environment variable that you need the value of.
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
 * Gets the directory that we consider to be the directory containing the Mantid.properties file. 
 * Basically, this is the either the directory pointed to by MANTIDPATH or the directory of the current
 * executable if this is not set.
 * @returns The directory to consider as the base directory, including a trailing slash
 */
std::string ConfigServiceImpl::getPropertiesDir() const
{
  return m_strBaseDir;
}

/**
 * Return the directory that Mantid should use for writing any files it needs so that
 * this is kept separated to user saved files. A trailing slash is appended
 * so that filenames can more easily be concatenated with this
 * @return the directory that Mantid should use for writing files
 */
std::string ConfigServiceImpl::getUserPropertiesDir() const
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

/**
 * Return the list of search paths
 * @returns A vector of strings containing the defined search directories
 */
const std::vector<std::string>& ConfigServiceImpl::getDataSearchDirs() const
{
  return m_DataSearchDirs;
}

/**
 * Set a list of search paths via a vector
 * @param searchDirs :: A list of search directories
 */
void ConfigServiceImpl::setDataSearchDirs(const std::vector<std::string> &searchDirs)
{
  std::string searchPaths = boost::join(searchDirs, ";");
  setDataSearchDirs(searchPaths);
}

/**
 * Set a list of search paths via a string
 * @param searchDirs :: A string containing a list of search directories separated by a semi colon (;).
 */
void ConfigServiceImpl::setDataSearchDirs(const std::string &searchDirs)
{
  setString("datasearch.directories", searchDirs);
}

/**
 *  Adds the passed path to the end of the list of data search paths
 *  the path name must be absolute
 *  @param path :: the absolute path to add
 */
void ConfigServiceImpl::appendDataSearchDir(const std::string & path)
{
  if (!isInDataSearchList(path))
  {
    std::string newSearchString;
    std::vector<std::string>::const_iterator it = m_DataSearchDirs.begin();
    for (; it != m_DataSearchDirs.end(); ++it)
    {
      newSearchString.append(*it);
      newSearchString.append(";");
    }
    newSearchString.append(path);
    setString("datasearch.directories", newSearchString);
  }
}

/**
 * Return the list of user search paths
 * @returns A vector of strings containing the defined search directories
 */
const std::vector<std::string>& ConfigServiceImpl::getUserSearchDirs() const
{
  return m_UserSearchDirs;
}

/**
 * Return the search directory for XML instrument definition files (IDFs)
 * @returns Full path of instrument search directory
 */
const std::string ConfigServiceImpl::getInstrumentDirectory() const
{
  // Determine the search directory for XML instrument definition files (IDFs)
  std::string directoryName = getString("instrumentDefinition.directory");
  if (directoryName.empty())
  {
    // This is the assumed deployment directory for IDFs, where we need to be relative to the
    // directory of the executable, not the current working directory.
    directoryName = Poco::Path(getPropertiesDir()).resolve("../instrument").toString();
  }

  if (!Poco::File(directoryName).isDirectory())
  {
    g_log.error("Unable to locate instrument search directory at: " + directoryName);
  }

  return directoryName;
}

/**
 * Load facility information from instrumentDir/Facilities.xml file if fName parameter
 * is not set
 * @param fName :: An alternative file name for loading facilities information.
 */
void ConfigServiceImpl::updateFacilities(const std::string& fName)
{
  m_facilities.clear();

  std::string instrDir = getString("instrumentDefinition.directory");
  std::string fileName = fName.empty() ? instrDir + "Facilities.xml" : fName;

  // Set up the DOM parser and parse xml file
  Poco::XML::DOMParser pParser;
  Poco::XML::Document* pDoc;

  try
  {
    try
    {
      pDoc = pParser.parse(fileName);
    } catch (...)
    {
      throw Kernel::Exception::FileError("Unable to parse file:", fileName);
    }
    // Get pointer to root element
    Poco::XML::Element* pRootElem = pDoc->documentElement();
    if (!pRootElem->hasChildNodes())
    {
      pDoc->release();
      throw std::runtime_error("No root element in Facilities.xml file");
    }

    Poco::XML::NodeList* pNL_facility = pRootElem->getElementsByTagName("facility");
    unsigned long n = pNL_facility->length();

    for (unsigned long i = 0; i < n; ++i)
    {
      Poco::XML::Element* elem = dynamic_cast<Poco::XML::Element*> (pNL_facility->item(i));
      if (elem)
      {
        m_facilities.push_back(new FacilityInfo(elem));
      }
    }

    if (m_facilities.empty())
    {
      pNL_facility->release();
      pDoc->release();
      throw std::runtime_error("The facility definition file " + fileName + " defines no facilities");
    }

    pNL_facility->release();
    pDoc->release();
  } catch (std::exception& e)
  {
    g_log.error(e.what());
  }

}

/**
 * Returns instruments with given name
 * @param  iName Instrument name
 * @return the instrument information object
 * @throw NotFoundError if iName was not found
 */
const InstrumentInfo & ConfigServiceImpl::getInstrument(const std::string& instrumentName) const
{

  // Let's first search for the instrument in our default facility
  std::string defaultFacility = ConfigService::Instance().getFacility().name();

  if (!defaultFacility.empty())
  {
    try
    {
      g_log.debug() << "Looking for " << instrumentName << " at " << defaultFacility << "." << std::endl;
      return getFacility(defaultFacility).Instrument(instrumentName);
    }
    catch (Exception::NotFoundError e)
    {
      // Well the instName doesn't exist for this facility
      // Move along, there's nothing to see here...
    }
  }

  // Now let's look through the other facilities
  std::vector<FacilityInfo*>::const_iterator it = m_facilities.begin();
  for (; it != m_facilities.end(); ++it)
  {
    try
    {
      g_log.debug() << "Looking for " << instrumentName << " at " << (**it).name() << "." << std::endl;
      return (**it).Instrument(instrumentName);
    }
    catch (Exception::NotFoundError e)
    {
      // Well the instName doesn't exist for this facility...
      // Move along, there's nothing to see here...
    }
  }
  g_log.error("Instrument " + instrumentName + " not found");
  throw Exception::NotFoundError("Instrument", instrumentName);
}

/** Get the default facility
 * @return the facility information object
 */
const FacilityInfo& ConfigServiceImpl::getFacility() const
{
  std::string defFacility = getString("default.facility");
  if (defFacility.empty())
  {
    defFacility = "ISIS";
  }
  return getFacility(defFacility);
}

/**
 * Get a facility
 * @param fName :: Facility name
 * @return the facility information object
 * @throw NotFoundException if the facility is not found
 */
const FacilityInfo& ConfigServiceImpl::getFacility(const std::string& facilityName) const
{
  if (facilityName.empty())
    return this->getFacility();

  std::vector<FacilityInfo*>::const_iterator it = m_facilities.begin();
  for (; it != m_facilities.end(); ++it)
  {
    if ((**it).name() == facilityName)
    {
      return **it;
    }
  }
  g_log.error("Facility " + facilityName + " not found");
  throw Exception::NotFoundError("Facilities", facilityName);
}

/**
 * Set the default facility
 * @param facilityName the facility name
 * @throw NotFoundException if the facility is not found
 */
void ConfigServiceImpl::setFacility(const std::string &facilityName)
{
  bool found = false;
  // Look through the facilities for a matching one.
  std::vector<FacilityInfo*>::const_iterator it = m_facilities.begin();
  for (; it != m_facilities.end(); ++it)
  {
    if ((**it).name() == facilityName)
    {
      // Found the facility
      found = true;
      // So it's safe to set it as our default
      setString("default.facility", facilityName);
    }
  }
  if (found == false)
  {
    g_log.error("Failed to set default facility to be " + facilityName + ". Facility not found");
    throw Exception::NotFoundError("Facilities", facilityName);
  }

}

/**  Add an observer to a notification
 @param observer :: Reference to the observer to add
 */
void ConfigServiceImpl::addObserver(const Poco::AbstractObserver& observer) const
{
  m_notificationCenter.addObserver(observer);
}

/**  Remove an observer
 @param observer :: Reference to the observer to remove
 */
void ConfigServiceImpl::removeObserver(const Poco::AbstractObserver& observer) const
{
  m_notificationCenter.removeObserver(observer);
}

/// \cond TEMPLATE
template DLLExport int ConfigServiceImpl::getValue(const std::string&, double&);
template DLLExport int ConfigServiceImpl::getValue(const std::string&, std::string&);
template DLLExport int ConfigServiceImpl::getValue(const std::string&, int&);
template DLLExport int ConfigServiceImpl::getValue(const std::string&, std::size_t&);
/// \endcond TEMPLATE

} // namespace Kernel
} // namespace Mantid
