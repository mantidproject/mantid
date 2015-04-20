//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/ParaViewVersion.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/FilterChannel.h"
#include "MantidKernel/StdoutChannel.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/NetworkProxy.h"

#include <Poco/Util/LoggingConfigurator.h>
#include <Poco/Util/SystemConfiguration.h>
#include <Poco/Util/PropertyFileConfiguration.h>
#include <Poco/LoggingFactory.h>
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/StringTokenizer.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/Environment.h>
#include <Poco/Process.h>
#include <Poco/URI.h>
#ifdef _WIN32
#pragma warning(disable : 4250)
#endif
#include <Poco/PipeStream.h>
#include <Poco/StreamCopier.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/regex.hpp>

#include <fstream>
#include <iostream>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

namespace Mantid {
/**
 * Get the welcome message for Mantid.
 * @returns A string containing the welcome message for Mantid.
 */
std::string welcomeMessage() {
  return "Welcome to Mantid " +
         std::string(Mantid::Kernel::MantidVersion::version()) +
         "\nPlease cite: " + Mantid::Kernel::MantidVersion::paperCitation() +
         " and this release: " + Mantid::Kernel::MantidVersion::doi();
}

namespace Kernel {

namespace { // anonymous namespace for some utility functions

/// static Logger object
Logger g_log("ConfigService");

/**
 * Split the supplied string on semicolons.
 *
 * @param path The path to split.
 * @param splitted vector to put the splitted path into.
 */
void splitPath(const std::string &path, std::vector<std::string> &splitted) {
  if (path.find(";") == std::string::npos) { // don't bother tokenizing
    splitted.push_back(path);
    return;
  }

  int options =
      Poco::StringTokenizer::TOK_TRIM + Poco::StringTokenizer::TOK_IGNORE_EMPTY;

  splitted.clear();
  Poco::StringTokenizer tokenizer(path, ";,", options);
  Poco::StringTokenizer::Iterator iend = tokenizer.end();
  splitted.reserve(tokenizer.count());
  for (Poco::StringTokenizer::Iterator itr = tokenizer.begin(); itr != iend;
       ++itr) {
    if (!itr->empty()) {
      splitted.push_back(*itr);
    }
  }
}

} // end of anonymous namespace

/** Inner templated class to wrap the poco library objects that have protected
 *  destructors and expose them as public.
 */
template <typename T> class ConfigServiceImpl::WrappedObject : public T {
public:
  /// The template type of class that is being wrapped
  typedef T element_type;
  /// Simple constructor
  WrappedObject() : T() { m_pPtr = static_cast<T *>(this); }

  /** Constructor with a class to wrap
   *  @param F :: The object to wrap
   */
  template <typename Field> WrappedObject(Field &F) : T(F) {
    m_pPtr = static_cast<T *>(this);
  }

  /// Copy constructor
  WrappedObject(const WrappedObject<T> &A) : T(A) {
    m_pPtr = static_cast<T *>(this);
  }

  /// Virtual destructor
  virtual ~WrappedObject() {}

  /// Overloaded * operator returns the wrapped object pointer
  const T &operator*() const { return *m_pPtr; }
  /// Overloaded * operator returns the wrapped object pointer
  T &operator*() { return m_pPtr; }
  /// Overloaded -> operator returns the wrapped object pointer
  const T *operator->() const { return m_pPtr; }
  /// Overloaded -> operator returns the wrapped object pointer
  T *operator->() { return m_pPtr; }

private:
  /// Private pointer to the wrapped class
  T *m_pPtr;
};

// Back to the ConfigService class itself...

//-------------------------------
// Private member functions
//-------------------------------

/// Private constructor for singleton class
ConfigServiceImpl::ConfigServiceImpl()
    : m_pConf(NULL), m_pSysConfig(NULL), m_changed_keys(), m_ConfigPaths(),
      m_AbsolutePaths(), m_strBaseDir(""), m_PropertyString(""),
      m_properties_file_name("Mantid.properties"),
#ifdef MPI_BUILD
      // Use a different user properties file for an mpi-enabled build to avoid
      // confusion if both are used on the same filesystem
      m_user_properties_file_name("Mantid-mpi.user.properties"),
#else
      m_user_properties_file_name("Mantid.user.properties"),
#endif
      m_DataSearchDirs(), m_UserSearchDirs(), m_InstrumentDirs(),
      m_instr_prefixes(), m_removedFlag("@@REMOVED@@"), m_proxyInfo(),
      m_isProxySet(false) {
  // getting at system details
  m_pSysConfig = new WrappedObject<Poco::Util::SystemConfiguration>;
  m_pConf = 0;

  // Register the FilterChannel with the Poco logging factory
  Poco::LoggingFactory::defaultFactory().registerChannelClass(
      "FilterChannel",
      new Poco::Instantiator<Poco::FilterChannel, Poco::Channel>);
  // Register StdChannel with Poco
  Poco::LoggingFactory::defaultFactory().registerChannelClass(
      "StdoutChannel",
      new Poco::Instantiator<Poco::StdoutChannel, Poco::Channel>);

  // Define the directory to search for the Mantid.properties file.
  Poco::File f;

  // First directory: the current working
  m_strBaseDir = Poco::Path::current();
  f = Poco::File(m_strBaseDir + m_properties_file_name);
  if (!f.exists()) {
    // Check the executable directory to see if it includes a mantid.properties
    // file
    m_strBaseDir = getDirectoryOfExecutable();
    f = Poco::File(m_strBaseDir + m_properties_file_name);
    if (!f.exists()) {
      // Last, use the MANTIDPATH environment var
      if (Poco::Environment::has("MANTIDPATH")) {
        // Here we have to follow the convention of the rest of this code and
        // add a trailing slash.
        // Note: adding it to the MANTIDPATH itself will make other parts of the
        // code crash.
        m_strBaseDir = Poco::Environment::get("MANTIDPATH") + "/";
      }
    }
  }

  // Assert that the appdata and the instrument subdirectory exists
  std::string appDataDir = getAppDataDir();
  Poco::Path path(appDataDir);
  path.pushDirectory("instrument");
  Poco::File file(path);
  // createdirectories will fail gracefully if it is already present
  file.createDirectories();

  // Fill the list of possible relative path keys that may require conversion to
  // absolute paths
  m_ConfigPaths.insert(
      std::make_pair("mantidqt.python_interfaces_directory", true));
  m_ConfigPaths.insert(std::make_pair("plugins.directory", true));
  m_ConfigPaths.insert(std::make_pair("pvplugins.directory", true));
  m_ConfigPaths.insert(std::make_pair("mantidqt.plugins.directory", true));
  m_ConfigPaths.insert(std::make_pair("instrumentDefinition.directory", true));
  m_ConfigPaths.insert(std::make_pair("groupingFiles.directory", true));
  m_ConfigPaths.insert(std::make_pair("maskFiles.directory", true));
  m_ConfigPaths.insert(std::make_pair("colormaps.directory", true));
  m_ConfigPaths.insert(
      std::make_pair("requiredpythonscript.directories", true));
  m_ConfigPaths.insert(std::make_pair("pythonscripts.directory", true));
  m_ConfigPaths.insert(std::make_pair("pythonscripts.directories", true));
  m_ConfigPaths.insert(std::make_pair("python.plugins.directories", true));
  m_ConfigPaths.insert(std::make_pair("user.python.plugins.directories", true));
  m_ConfigPaths.insert(std::make_pair("datasearch.directories", true));
  m_ConfigPaths.insert(std::make_pair("icatDownload.directory", true));

  // attempt to load the default properties file that resides in the directory
  // of the executable
  std::string propertiesFilesList;
  updateConfig(getPropertiesDir() + m_properties_file_name, false, false);
  propertiesFilesList = getPropertiesDir() + m_properties_file_name;

  // Load the local (machine) properties file, if it exists
  Poco::File localFile(getLocalFilename());
  if (localFile.exists()) {
    updateConfig(getLocalFilename(), true, false);
    propertiesFilesList += ", " + getLocalFilename();
  }

  if (Poco::Environment::has("MANTIDPROPERTIES")) {
    // and then append the user properties
    updateConfig(getUserFilename(), true, false);
    propertiesFilesList += ", " + getUserFilename();
    // and the extra one from the environment
    updateConfig(Poco::Environment::get("MANTIDPROPERTIES"), true, true);
    propertiesFilesList += ", " + Poco::Environment::get("MANTIDPROPERTIES");
  } else {
    // Just do the user properties
    updateConfig(getUserFilename(), true, true);
    propertiesFilesList += ", " + getUserFilename();
  }

  updateFacilities();

  g_log.debug() << "ConfigService created." << std::endl;
  g_log.debug() << "Configured Mantid.properties directory of application as "
                << getPropertiesDir() << std::endl;
  g_log.information() << "This is Mantid version " << MantidVersion::version()
                      << " revision " << MantidVersion::revision() << std::endl;
  g_log.information() << "Properties file(s) loaded: " << propertiesFilesList
                      << std::endl;
#ifndef MPI_BUILD // There is no logging to file by default in MPI build
  g_log.information() << "Logging to: " << m_logFilePath << std::endl;
#endif
}

/** Private Destructor
 *  Prevents client from calling 'delete' on the pointer handed out by Instance
 */
ConfigServiceImpl::~ConfigServiceImpl() {
  // std::cerr << "ConfigService destroyed." << std::endl;
  Kernel::Logger::shutdown();
  delete m_pSysConfig;
  delete m_pConf; // potential double delete???
  clearFacilities();
}

/** Loads the config file provided.
 *  If the file contains logging setup instructions then these will be used to
 *setup the logging framework.
 *
 *  @param filename :: The filename and optionally path of the file to load
 *  @param append :: If false (default) then any previous configuration is
 *discarded, otherwise the new keys are added, and repeated keys will override
 *existing ones.
 */
void ConfigServiceImpl::loadConfig(const std::string &filename,
                                   const bool append) {
  delete m_pConf;
  if (!append) {
    // remove the previous property string
    m_PropertyString = "";
    m_changed_keys.clear();
  }

  try {
    // slurp in entire file
    std::string temp;
    bool good = readFile(filename, temp);

    // check if we have failed to open the file
    if ((!good) || (temp == "")) {
      if (filename == getUserPropertiesDir() + m_user_properties_file_name) {
        // write out a fresh file
        createUserPropertiesFile();
      } else {
        throw Exception::FileError("Cannot open file", filename);
      }
    }

    // store the property string
    if ((append) && (m_PropertyString != "")) {
      m_PropertyString = m_PropertyString + "\n" + temp;
    } else {
      m_PropertyString = temp;
    }
  }
  catch (std::exception &e) {
    // there was a problem loading the file - it probably is not there
    std::cerr << "Problem loading the configuration file " << filename << " "
              << e.what() << std::endl;
    if (!append) {
      // if we have no property values then take the default
      m_PropertyString = defaultConfig();
    }
  }

  // use the cached property string to initialise the POCO property file
  std::istringstream istr(m_PropertyString);
  m_pConf = new WrappedObject<Poco::Util::PropertyFileConfiguration>(istr);
}

/**
 * Read a file and place its contents into the given string
 * @param filename :: The filename of the file to read
 * @param contents :: The file contents will be placed here
 * @returns A boolean indicating whether opening the file was successful
 */
bool ConfigServiceImpl::readFile(const std::string &filename,
                                 std::string &contents) const {
  std::ifstream propFile(filename.c_str(), std::ios::in);
  bool good = propFile.good();
  if (!good) {
    contents = "";
    propFile.close();
    return good;
  }

  // slurp in entire file - extremely unlikely delimiter used as an alternate to
  // \n
  contents.clear();
  getline(propFile, contents, '`');
  propFile.close();
  return good;
}

/** Configures the Poco logging and starts it up
 *
 */
void ConfigServiceImpl::configureLogging() {
  try {
    // Ensure that the logging directory exists
    m_logFilePath = getString("logging.channels.fileChannel.path");
    Poco::Path logpath(m_logFilePath);

    // Undocumented way to override the mantid.log path
    if (Poco::Environment::has("MANTIDLOGPATH")) {
      logpath = Poco::Path(Poco::Environment::get("MANTIDLOGPATH"));
      logpath = logpath.absolute();
      m_logFilePath = logpath.toString();
    }

    // An absolute path makes things simpler
    logpath = logpath.absolute();

    // First, try the logpath given
    if (!m_logFilePath.empty()) {
      try {
        // Save it for later
        m_logFilePath = logpath.toString();

        // make this path point to the parent directory and create it if it does
        // not exist
        Poco::Path parent = logpath;
        parent.makeParent();
        Poco::File(parent).createDirectories();

        // Try to create or append to the file. If it fails, use the default
        FILE *fp = fopen(m_logFilePath.c_str(), "a+");
        if (fp == NULL) {
          std::cerr
              << "Error writing to log file path given in properties file: \""
              << m_logFilePath << "\". Will use a default path instead."
              << std::endl;
          // Clear the path; this will make it use the default
          m_logFilePath = "";
        } else
          fclose(fp);
      }
      catch (std::exception &) {
        std::cerr
            << "Error writing to log file path given in properties file: \""
            << m_logFilePath << "\". Will use a default path instead."
            << std::endl;
        // ERROR! Maybe the file is not writable!
        // Clear the path; this will make it use the default
        m_logFilePath = "";
      }
    }

    // The path given was invalid somehow? Use a default
    if (m_logFilePath.empty()) {
      m_logFilePath = getUserPropertiesDir() + "mantid.log";
      // Check whether the file can be written. The Poco::File::canWrite method
      // does not work
      // for files that don't exist, it throws an exception. It also can't be
      // used to check for
      // directory access as the Windows API doesn't return this information
      // correctly for
      // directories.
      FILE *fp = fopen(m_logFilePath.c_str(), "a+");
      if (!fp) {
        // if we cannot write to the default directory then set use the system
        // temp
        logpath = Poco::Path::temp() + "mantid.log";
        m_logFilePath = logpath.toString();
        std::cerr << "Error writing to log file path to default location: \""
                  << m_logFilePath
                  << "\". Will use a system temp path instead: \""
                  << m_logFilePath << "\"" << std::endl;
      } else
        fclose(fp);
    }
    // Set the line in the configuration properties.
    //  this'll be picked up by LoggingConfigurator (somehow)
    m_pConf->setString("logging.channels.fileChannel.path", m_logFilePath);

    // make this path point to the parent directory and create it if it does not
    // exist
    logpath.makeParent();
    if (!logpath.toString().empty()) {
      Poco::File(logpath)
          .createDirectories(); // Also creates all necessary directories
    }

    // Configure the logging framework
    Poco::Util::LoggingConfigurator configurator;
    configurator.configure(m_pConf);
  }
  catch (std::exception &e) {
    std::cerr << "Trouble configuring the logging framework " << e.what()
              << std::endl;
  }
}

/**
 * Searches the stored list for keys that have been loaded from the config file
 * and may contain
 * relative paths. Any it find are converted to absolute paths and stored
 * separately
 */
void ConfigServiceImpl::convertRelativeToAbsolute() {
  if (m_ConfigPaths.empty())
    return;

  m_AbsolutePaths.clear();
  std::map<std::string, bool>::const_iterator send = m_ConfigPaths.end();
  for (std::map<std::string, bool>::const_iterator sitr = m_ConfigPaths.begin();
       sitr != send; ++sitr) {
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
 * @returns A string containing an absolute path by resolving the relative
 * directory with the executable directory
 */
std::string ConfigServiceImpl::makeAbsolute(const std::string &dir,
                                            const std::string &key) const {
  if (dir.empty()) {
    // Don't do anything for an empty value
    return dir;
  }
  std::string converted;
  // If we have a list, chop it up and convert each one
  if (dir.find_first_of(";,") != std::string::npos) {
    std::vector<std::string> splitted;
    splitPath(dir, splitted);
    std::vector<std::string>::const_iterator iend = splitted.end();
    for (std::vector<std::string>::const_iterator itr = splitted.begin();
         itr != iend;) {
      std::string absolute = makeAbsolute(*itr, key);
      if (absolute.empty()) {
        ++itr;
      } else {
        converted += absolute;
        if (++itr != iend) {
          converted += ";";
        }
      }
    }
    return converted;
  }

  // MG 05/10/09: When the Poco::FilePropertyConfiguration object reads its
  // key/value pairs it
  // treats a backslash as the start of an escape sequence. If the next
  // character does not
  // form a valid sequence then the backslash is removed from the stream. This
  // has the effect
  // of giving malformed paths when using Windows-style directories. E.g
  // C:\Mantid ->C:Mantid
  // and Poco::Path::isRelative throws an exception on this
  bool is_relative(false);
  try {
    is_relative = Poco::Path(dir).isRelative();
  }
  catch (Poco::PathSyntaxException &) {
    g_log.warning() << "Malformed path detected in the \"" << key
                    << "\" variable, skipping \"" << dir << "\"\n";
    return "";
  }
  if (is_relative) {
    const std::string propFileDir(getPropertiesDir());
    converted = Poco::Path(propFileDir).resolve(dir).toString();
  } else {
    converted = dir;
  }
  converted = Poco::Path(converted).makeDirectory().toString();

  // C++ doesn't have a const version of operator[] for maps so I can't call
  // that here
  std::map<std::string, bool>::const_iterator it = m_ConfigPaths.find(key);
  bool required = false;
  if (it != m_ConfigPaths.end()) {
    required = it->second;
  }
  try {
    if (required && !Poco::File(converted).exists()) {
      g_log.debug() << "Required properties path \"" << converted
                    << "\" in the \"" << key << "\" variable does not exist.\n";
      converted = "";
    }
  }
  catch (Poco::FileException &) {
    g_log.debug() << "Required properties path \"" << converted
                  << "\" in the \"" << key << "\" variable does not exist.\n";
    converted = "";
  }

  // Backward slashes cannot be allowed to go into our properties file
  // Note this is a temporary fix for ticket #2445.
  // Ticket #2460 prompts a review of our path handling in the config service.
  boost::replace_all(converted, "\\", "/");
  return converted;
}

/**
 * Create the store of data search paths from the 'datasearch.directories' key
 * within the Mantid.properties file.
 * The value of the key should be a semi-colon separated list of directories
 */
void ConfigServiceImpl::cacheDataSearchPaths() {
  m_DataSearchDirs.clear();
  std::string paths = getString("datasearch.directories");
  // Nothing to do
  if (paths.empty())
    return;
  splitPath(paths, m_DataSearchDirs);
}

/**
 * Create the store of user search paths from the 'usersearch.directories' key
 * within the Mantid.properties file.
 * The value of the key should be a semi-colon separated list of directories
 */
void ConfigServiceImpl::cacheUserSearchPaths() {
  m_UserSearchDirs.clear();
  std::string paths = getString("usersearch.directories");
  // Nothing to do
  if (paths.empty())
    return;
  splitPath(paths, m_UserSearchDirs);
}

/**
 *  The path that is passed should be as returned by makeAbsolute() and
 *  this function will return true if that path is in the list
 *  @param path :: the absolute path name to search for
 *  @return true if the path was found
 */
bool ConfigServiceImpl::isInDataSearchList(const std::string &path) const {
  // the path produced by poco will have \ on windows, but the searchdirs will
  // always have /
  std::string correctedPath = path;
  replace(correctedPath.begin(), correctedPath.end(), '\\', '/');

  std::vector<std::string>::const_iterator it =
      std::find_if(m_DataSearchDirs.begin(), m_DataSearchDirs.end(),
                   std::bind2nd(std::equal_to<std::string>(), correctedPath));
  return (it != m_DataSearchDirs.end());
}

/**
 * writes a basic placeholder user.properties file to disk
 * any errors are caught and logged, but not propagated
 */
void ConfigServiceImpl::createUserPropertiesFile() const {
  try {
    std::fstream filestr(
        (getUserPropertiesDir() + m_user_properties_file_name).c_str(),
        std::fstream::out);

    filestr << "# This file can be used to override any properties for this "
               "installation." << std::endl;
    filestr << "# Any properties found in this file will override any that are "
               "found in the Mantid.Properties file" << std::endl;
    filestr << "# As this file will not be replaced with futher installations "
               "of Mantid it is a safe place to put " << std::endl;
    filestr << "# properties that suit your particular installation."
            << std::endl;
    filestr << "#" << std::endl;
    filestr << "# See here for a list of possible options:" << std::endl;
    filestr << "# "
               "http://www.mantidproject.org/"
               "Properties_File#Mantid.User.Properties" << std::endl;
    filestr << std::endl;
    filestr << "##" << std::endl;
    filestr << "## GENERAL" << std::endl;
    filestr << "##" << std::endl;
    filestr << std::endl;
    filestr << "## Set the number of algorithm properties to retain"
            << std::endl;
    filestr << "#algorithms.retained=90" << std::endl;
    filestr << std::endl;
    filestr << "## Hides catagories from the algorithm list in MantidPlot"
            << std::endl;
    filestr << "#algorithms.catagories.hidden=Muons,Inelastic" << std::endl;
    filestr << std::endl;
    filestr << "## Set the maximum number of coures used to run algorithms over"
            << std::endl;
    filestr << "#MultiThreaded.MaxCores=4" << std::endl;
    filestr << std::endl;
    filestr << "##" << std::endl;
    filestr << "## FACILITY AND INSTRUMENT" << std::endl;
    filestr << "##" << std::endl;
    filestr << std::endl;
    filestr << "## Sets the default facility" << std::endl;
    filestr << "## e.g.: ISIS, SNS, ILL" << std::endl;
    filestr << "default.facility=" << std::endl;
    filestr << std::endl;
    filestr << "## Stes the default instrument" << std::endl;
    filestr << "## e.g. IRIS, HET, NIMROD" << std::endl;
    filestr << "default.instrument=" << std::endl;
    filestr << std::endl;
    filestr << "##" << std::endl;
    filestr << "## DIRECTORIES" << std::endl;
    filestr << "##" << std::endl;
    filestr << std::endl;
    filestr << "## Sets a list of directories (separated by semi colons) to "
               "search for data" << std::endl;
    filestr << "#datasearch.directories=../data;../isis/data" << std::endl;
    filestr << std::endl;
    filestr << "## Set a list (separated by semi colons) of directories to "
               "look for additional Python scripts" << std::endl;
    filestr << "#pythonscripts.directories=../scripts;../docs/MyScripts"
            << std::endl;
    filestr << std::endl;
    filestr << "## Uncomment to enable archive search - ICat and Orbiter"
            << std::endl;
    filestr << "#datasearch.searcharchive=On" << std::endl;
    filestr << std::endl;
    filestr << "## Sets default save directory" << std::endl;
    filestr << "#defaultsave.directory=../data" << std::endl;
    filestr << std::endl;
    filestr << "##" << std::endl;
    filestr << "## LOGGING" << std::endl;
    filestr << "##" << std::endl;
    filestr << std::endl;
    filestr << "## Uncomment to change logging level" << std::endl;
    filestr << "## Default is information" << std::endl;
    filestr << "## Valid values are: error, warning, notice, information, debug"
            << std::endl;
    filestr << "#logging.loggers.root.level=information" << std::endl;
    filestr << std::endl;
    filestr << "## Sets the lowest level messages to be logged to file"
            << std::endl;
    filestr << "## Default is warning" << std::endl;
    filestr << "## Valid values are: error, warning, notice, information, debug"
            << std::endl;
    filestr << "#logging.channels.fileFilterChannel.level=debug" << std::endl;
    filestr << std::endl;
    filestr << "## Sets the file to write logs to" << std::endl;
    filestr << "#logging.channels.fileChannel.path=../mantid.log" << std::endl;
    filestr << std::endl;
    filestr << "##" << std::endl;
    filestr << "## MantidPlot" << std::endl;
    filestr << "##" << std::endl;
    filestr << std::endl;
    filestr << "## Show invisible workspaces" << std::endl;
    filestr << "#MantidOptions.InvisibleWorkspaces=0" << std::endl;
    filestr << "## Re-use plot instances for different plot types" << std::endl;
    filestr << "#MantidOptions.ReusePlotInstances=Off" << std::endl;
    filestr << std::endl;
    filestr << "## Uncomment to disable use of OpenGL to render unwrapped "
               "instrument views" << std::endl;
    filestr << "#MantidOptions.InstrumentView.UseOpenGL=Off" << std::endl;

    filestr.close();
  }
  catch (std::runtime_error &ex) {
    g_log.warning() << "Unable to write out user.properties file to "
                    << getUserPropertiesDir() << m_user_properties_file_name
                    << " error: " << ex.what() << std::endl;
  }
}

/**
 * Provides a default Configuration string to use if the config file cannot be
 * loaded.
 * @returns The string value of default properties
 */
std::string ConfigServiceImpl::defaultConfig() const {
  std::string propFile =
      "# logging configuration"
      "# root level message filter (drop to debug for more messages)"
      "logging.loggers.root.level = debug"
      "# splitting the messages to many logging channels"
      "logging.loggers.root.channel.class = SplitterChannel"
      "logging.loggers.root.channel.channel1 = consoleChannel"
      "logging.loggers.root.channel.channel2 = fileFilterChannel"
      "# output to the console - primarily for console based apps"
      "logging.channels.consoleChannel.class = ConsoleChannel"
      "logging.channels.consoleChannel.formatter = f1"
      "# specfic filter for the file channel raising the level to warning "
      "(drop to debug for debugging)"
      "logging.channels.fileFilterChannel.class= FilterChannel"
      "logging.channels.fileFilterChannel.channel= fileChannel"
      "logging.channels.fileFilterChannel.level= warning"
      "# output to a file (For error capturing and debugging)"
      "logging.channels.fileChannel.class = debug"
      "logging.channels.fileChannel.path = ../logs/mantid.log"
      "logging.channels.fileChannel.formatter.class = PatternFormatter"
      "logging.channels.fileChannel.formatter.pattern = %Y-%m-%d %H:%M:%S,%i "
      "[%I] %p %s - %t"
      "logging.formatters.f1.class = PatternFormatter"
      "logging.formatters.f1.pattern = %s-[%p] %t"
      "logging.formatters.f1.times = UTC";
  return propFile;
}

//-------------------------------
// Public member functions
//-------------------------------

/**
 * Removes the user properties file & loads a fresh configuration
 */
void ConfigServiceImpl::reset() {
  // Remove the current user properties file and write a fresh one
  try {
    Poco::File userFile(getUserFilename());
    userFile.remove();
  }
  catch (Poco::Exception &) {
  }
  createUserPropertiesFile();

  // Now load the original
  const bool append = false;
  const bool updateCaches = true;
  updateConfig(getPropertiesDir() + m_properties_file_name, append,
               updateCaches);
}

/** Updates and existing configuration and restarts the logging
 *  @param filename :: The filename and optionally path of the file to load
 *  @param append ::   If false (default) then any previous configuration is
 * discarded,
 *                  otherwise the new keys are added, and repeated keys will
 * override existing ones.
 *  @param update_caches :: If true(default) then the various property caches
 * are updated
 */
void ConfigServiceImpl::updateConfig(const std::string &filename,
                                     const bool append,
                                     const bool update_caches) {
  loadConfig(filename, append);

  // Ensure that the default save directory makes sense
  /*
  if (!append)
  {
    std::string save_dir = getString("defaultsave.directory");
    if (Poco::trimInPlace(save_dir).size() == 0)
      setString("defaultsave.directory", Poco::Path::home());
  }
  */

  if (update_caches) {
    // Only configure logging once
    configureLogging();
    // Ensure that any relative paths given in the configuration file are
    // relative to the correct directory
    convertRelativeToAbsolute();
    // Configure search paths into a specially saved store as they will be used
    // frequently
    cacheDataSearchPaths();
    appendDataSearchDir(getString("defaultsave.directory"));
    cacheUserSearchPaths();
    cacheInstrumentPaths();
  }
}

/**
 * Save the configuration to the user file
 * @param filename :: The filename for the saved configuration
 * @throw std::runtime_error if the file cannot be opened
 */
void ConfigServiceImpl::saveConfig(const std::string &filename) const {
  // Open and read the user properties file
  std::string updated_file("");

  std::ifstream reader(filename.c_str(), std::ios::in);
  if (reader.bad()) {
    throw std::runtime_error("Error opening user properties file. Cannot save "
                             "updated configuration.");
  }

  std::string file_line(""), output("");
  bool line_continuing(false);
  while (std::getline(reader, file_line)) {
    if (!file_line.empty()) {
      char last = *(file_line.end() - 1);
      if (last == '\\') {
        // If we are not in line continuation mode then need
        // a fresh start line
        if (!line_continuing)
          output = "";
        line_continuing = true;
        output += file_line + "\n";
        continue;
      } else if (line_continuing) {
        output += file_line;
        line_continuing = false;
      } else {
        output = file_line;
      }
    } else {
      output = "";
      updated_file += "\n";
      continue;
    } // end if-else

    // Output is the current line in the file

    // Extract the key from the current line
    std::string key;
    std::string::size_type pos = output.find('=');
    if (pos == std::string::npos) {
      key = output; // If no equals then the entire thing is the key
    } else {
      key = output.substr(0, pos); // Strip the equals to get only the key
    }
    // Now deal with trimming (removes spaces)
    Poco::trimInPlace(key);

    // Find the comments
    std::string::size_type comment = key.find('#');

    // Check if it exists in the service using hasProperty and make sure it
    // isn't a comment
    if (comment == 0) {
      updated_file += output;
    } else if (!hasProperty(key)) {
      // Remove the key from the changed key list
      m_changed_keys.erase(key);
      continue;
    } else {
      // If it does exist make sure the value is current
      std::string value = getString(key, false);
      Poco::replaceInPlace(value, "\\", "\\\\"); // replace single \ with double
      updated_file += key + "=" + value;
      // Remove the key from the changed key list
      m_changed_keys.erase(key);
    }
    updated_file += "\n";
  } // End while-loop

  // Any remaining keys within the changed key store weren't present in the
  // current user properties so append them
  if (!m_changed_keys.empty()) {
    updated_file += "\n";
    std::set<std::string>::iterator key_end = m_changed_keys.end();
    for (std::set<std::string>::iterator key_itr = m_changed_keys.begin();
         key_itr != key_end;) {
      updated_file += *key_itr + "=";
      std::string value = getString(*key_itr, false);
      Poco::replaceInPlace(value, "\\", "\\\\"); // replace single \ with double
      updated_file += value;
      if (++key_itr != key_end) {
        updated_file += "\n";
      }
    }
    m_changed_keys.clear();
  }

  // Write out the new file
  std::ofstream writer(filename.c_str(), std::ios_base::trunc);
  if (writer.bad()) {
    writer.close();
    g_log.error() << "Error writing new user properties file. Cannot save "
                     "current configuration.\n";
    throw std::runtime_error("Error writing new user properties file. Cannot "
                             "save current configuration.");
  }

  writer.write(updated_file.c_str(), updated_file.size());
  writer.close();
}

/** Searches for a string within the currently loaded configuaration values and
 *  returns the value as a string. If the key is one of those that was a
 *possible relative path
 *  then the local store is searched first.
 *
 *  @param keyName :: The case sensitive name of the property that you need the
 *value of.
 *  @param use_cache :: If true, the local cache of directory names is queried
 *first.
 *  @returns The string value of the property, or an empty string if the key
 *cannot be found
 */
std::string ConfigServiceImpl::getString(const std::string &keyName,
                                         bool use_cache) const {
  if (use_cache) {
    std::map<std::string, std::string>::const_iterator mitr =
        m_AbsolutePaths.find(keyName);
    if (mitr != m_AbsolutePaths.end()) {
      return (*mitr).second;
    }
  }
  std::string retVal;
  try {
    retVal = m_pConf->getString(keyName);
    if (retVal == m_removedFlag)
      retVal = "";
  }
  catch (Poco::NotFoundException &) {
    g_log.debug() << "Unable to find " << keyName << " in the properties file"
                  << std::endl;
    retVal = "";
  }
  return retVal;
}

/** Searches for keys within the currently loaded configuaration values and
 *  returns them as strings in a vector.
 *
 *  @param keyName :: The case sensitive name of the property that you need the
 *key for.
 *  @returns The string value of each key within a vector, or an empty vector if
 *there isn't
 *  a key or it couldn't be found.
 */
std::vector<std::string>
ConfigServiceImpl::getKeys(const std::string &keyName) const {
  std::vector<std::string> rawKeys;
  std::vector<std::string> keyVector;
  keyVector.reserve(rawKeys.size());
  try {
    m_pConf->keys(keyName, rawKeys);
    // Work around a limitation of Poco < v1.4 which has no remove functionality
    // so check those that have been marked with the correct flag
    const size_t nraw = rawKeys.size();
    for (size_t i = 0; i < nraw; ++i) {
      const std::string key = rawKeys[i];
      try {
        if (m_pConf->getString(key) == m_removedFlag)
          continue;
      }
      catch (Poco::NotFoundException &) {
      }
      keyVector.push_back(key);
    }
  }
  catch (Poco::NotFoundException &) {
    g_log.debug() << "Unable to find " << keyName << " in the properties file"
                  << std::endl;
    keyVector.clear();
  }
  return keyVector;
}

/**
 * Recursively gets a list of all config options from a given root node.
 *
 * @return Vector containing all config options
 */
void ConfigServiceImpl::getKeysRecursive(const std::string &root,
    std::vector<std::string> &allKeys) const {
  std::vector<std::string> rootKeys = getKeys(root);

  if(rootKeys.empty())
    allKeys.push_back(root);

  for (auto rkIt = rootKeys.begin(); rkIt != rootKeys.end(); ++rkIt) {
    std::string searchString;
    if (root.empty()) {
      searchString = *rkIt;
    } else {
      searchString = root + "." + *rkIt;
    }

    getKeysRecursive(searchString, allKeys);
  }
}

/**
 * Recursively gets a list of all config options.
 *
 * This function is needed as Boost Python does not like calling function with
 * default arguments.
 *
 * @return Vector containing all config options
 */
std::vector<std::string> ConfigServiceImpl::keys() const {
  std::vector<std::string> allKeys;
  getKeysRecursive("", allKeys);
  return allKeys;
}

/** Removes a key from the memory stored properties file and inserts the key
 *into the
 *  changed key list so that when the program calls saveConfig the properties
 *file will
 *  be the same and not contain the key no more
 *
 *  @param rootName :: The key that is to be deleted
 */
void ConfigServiceImpl::remove(const std::string &rootName) const {
  try {
    // m_pConf->remove(rootName) will only work in Poco v >=1.4. Current Ubuntu
    // and RHEL use 1.3.x
    // Simulate removal by marking with a flag value
    m_pConf->setString(rootName, m_removedFlag);
  }
  catch (Poco::NotFoundException &) {
    g_log.debug() << "Unable to find " << rootName << " in the properties file"
                  << std::endl;
  }
  m_changed_keys.insert(rootName);
}

/** Checks to see whether the given key exists.
 *
 *  @param rootName :: The case sensitive key that you are looking to see if
 *exists.
 *  @returns Boolean value denoting whether the exists or not.
 */
bool ConfigServiceImpl::hasProperty(const std::string &rootName) const {
  // Work around a limitation of Poco < v1.4 which has no remove functionality
  return m_pConf->hasProperty(rootName) &&
         m_pConf->getString(rootName) != m_removedFlag;
}

/** Checks to see whether the given file target is an executable one and it
 *exists.
 * This method will expand environment variables found in the given file path.
 *
 *  @param target :: The path to the file you wish to see whether it's an
 *executable.
 *  @returns Boolean value denoting whether the file is an executable or not.
 */
bool ConfigServiceImpl::isExecutable(const std::string &target) const {
  try {
    std::string expTarget = Poco::Path::expand(target);
    Poco::File tempFile = Poco::File(expTarget);

    if (tempFile.exists()) {
      if (tempFile.canExecute())
        return true;
      else
        return false;
    } else
      return false;
  }
  catch (Poco::Exception &) {
    return false;
  }
}

/** Runs a command line string to open a program. The function can take program
 *arguments.
 *  i.e it can load in a file to the program on startup.
 *
 *  This method will expand environment variables found in the given file path.
 *
 *  @param programFilePath :: The directory where the program is located.
 *  @param programArguments :: The arguments that the program can take on
 *startup. For example,
 *  the file to load up.
 */

void ConfigServiceImpl::launchProcess(
    const std::string &programFilePath,
    const std::vector<std::string> &programArguments) const {
  try {
    std::string expTarget = Poco::Path::expand(programFilePath);
    Poco::Process::launch(expTarget, programArguments);
  }
  catch (Poco::SystemException &e) {
    throw std::runtime_error(e.what());
  }
}

/**
 * Set a configuration property. An existing key will have its value updated.
 * @param key :: The key to refer to this property
 * @param value :: The value of the property
 */
void ConfigServiceImpl::setString(const std::string &key,
                                  const std::string &value) {
  // If the value is unchanged (after any path conversions), there's nothing to
  // do.
  const std::string old = getString(key);
  if (value == old)
    return;

  // Ensure we keep a correct full path
  std::map<std::string, bool>::const_iterator itr = m_ConfigPaths.find(key);
  if (itr != m_ConfigPaths.end()) {
    m_AbsolutePaths[key] = makeAbsolute(value, key);
  }

  if (key == "datasearch.directories") {
    cacheDataSearchPaths();
  } else if (key == "usersearch.directories") {
    cacheUserSearchPaths();
  } else if (key == "instrumentDefinition.directory") {
    cacheInstrumentPaths();
  } else if (key == "defaultsave.directory") {
    appendDataSearchDir(value);
  }

  m_pConf->setString(key, value);

  m_notificationCenter.postNotification(new ValueChanged(key, value, old));
  m_changed_keys.insert(key);
}

/** Searches for a string within the currently loaded configuaration values and
 *  attempts to convert the values to the template type supplied.
 *
 *  @param keyName :: The case sensitive name of the property that you need the
 *value of.
 *  @param out ::     The value if found
 *  @returns A success flag - 0 on failure, 1 on success
 */
template <typename T>
int ConfigServiceImpl::getValue(const std::string &keyName, T &out) {
  std::string strValue = getString(keyName);
  int result = Mantid::Kernel::Strings::convert(strValue, out);
  return result;
}

/**
 * Return the full filename of the local properties file.
 * @returns A string containing the full path to the local file.
 */
std::string ConfigServiceImpl::getLocalFilename() const {
#ifdef _WIN32
  return "Mantid.local.properties";
#else
  return "/etc/mantid.local.properties";
#endif
}

/**
 * Return the full filename of the user properties file
 * @returns A string containing the full path to the user file
 */
std::string ConfigServiceImpl::getUserFilename() const {
  return getUserPropertiesDir() + m_user_properties_file_name;
}

/** Searches for the string within the environment variables and returns the
 *  value as a string.
 *
 *  @param keyName :: The name of the environment variable that you need the
 *value of.
 *  @returns The string value of the property
 */
std::string ConfigServiceImpl::getEnvironment(const std::string &keyName) {
  return m_pSysConfig->getString("system.env." + keyName);
}

/** Gets the name of the host operating system
 *
 *  @returns The name pf the OS version
 */
std::string ConfigServiceImpl::getOSName() {
  return m_pSysConfig->getString("system.osName");
}

/** Gets the name of the computer running Mantid
 *
 *  @returns The  name of the computer
 */
std::string ConfigServiceImpl::getOSArchitecture() {
  return m_pSysConfig->getString("system.osArchitecture");
}

/** Gets the name of the operating system Architecture
 *
 * @returns The operating system architecture
 */
std::string ConfigServiceImpl::getComputerName() {
  return m_pSysConfig->getString("system.nodeName");
}

/** Gets the name of the operating system version
 *
 * @returns The operating system version
 */
std::string ConfigServiceImpl::getOSVersion() {
  return m_pSysConfig->getString("system.osVersion");
}

/// @returns true if the file exists and can be read
bool canRead(const std::string &filename) {
  // check for existence of the file
  Poco::File pocoFile(filename);
  if (!pocoFile.exists()) {
    return false;
  }

  // just return if it is readable
  return pocoFile.canRead();
}

/// @returns the value associated with the key.
std::string getValueFromStdOut(const std::string &orig,
                               const std::string &key) {
  size_t start = orig.find(key);
  if (start == std::string::npos) {
    return std::string();
  }
  start += key.size();

  size_t stop = orig.find("\n", start);
  if (stop == std::string::npos) {
    return std::string();
  }

  return Mantid::Kernel::Strings::strip(orig.substr(start, stop - start - 1));
}

/**
 * Gets the name of the operating system version in a human readable form.
 *
 * @returns The operating system desciption
 */
std::string ConfigServiceImpl::getOSVersionReadable() {
  std::string description;

  // read os-release
  static const std::string OS_RELEASE("/etc/os-release");
  if (canRead(OS_RELEASE)) {
    static const std::string PRETTY_NAME("PRETTY_NAME=");

    // open it to see if it has the magic line
    std::ifstream handle(OS_RELEASE.c_str(), std::ios::in);

    // go through the file
    std::string line;
    while (std::getline(handle, line)) {
      if (line.find(PRETTY_NAME) != std::string::npos) {
        if (line.length() > PRETTY_NAME.length() + 1) {
          size_t length = line.length() - PRETTY_NAME.length() - 2;
          description = line.substr(PRETTY_NAME.length() + 1, length);
        }
        break;
      }
    }

    // cleanup
    handle.close();
    if (!description.empty()) {
      return description;
    }
  }

  // read redhat-release
  static const std::string REDHAT_RELEASE("/etc/redhat-release");
  if (canRead(REDHAT_RELEASE)) {
    // open it to see if it has the magic line
    std::ifstream handle(REDHAT_RELEASE.c_str(), std::ios::in);

    // go through the file
    std::string line;
    while (std::getline(handle, line)) {
      if (!line.empty()) {
        description = line;
        break;
      }
    }

    // cleanup
    handle.close();
    if (!description.empty()) {
      return description;
    }
  }

  // try system calls
  std::string cmd;
  std::vector<std::string> args;
#ifdef __APPLE__
  cmd = "sw_vers"; // mac
#elif _WIN32
  cmd = "wmic";              // windows
  args.push_back("os");      // windows
  args.push_back("get");     // windows
  args.push_back("Caption"); // windows
  args.push_back("/value");  // windows
#endif

  if (!cmd.empty()) {
    try {
      Poco::Pipe outPipe, errorPipe;
      Poco::ProcessHandle ph =
          Poco::Process::launch(cmd, args, 0, &outPipe, &errorPipe);
      const int rc = ph.wait();
      // Only if the command returned successfully.
      if (rc == 0) {
        Poco::PipeInputStream pipeStream(outPipe);
        std::stringstream stringStream;
        Poco::StreamCopier::copyStream(pipeStream, stringStream);
        const std::string result = stringStream.str();
#ifdef __APPLE__
        const std::string product_name =
            getValueFromStdOut(result, "ProductName:");
        const std::string product_vers =
            getValueFromStdOut(result, "ProductVersion:");

        description = product_name + " " + product_vers;
#elif _WIN32
        description = getValueFromStdOut(result, "Caption=");
#else
        UNUSED_ARG(result); // only used on mac and windows
#endif
      } else {
        std::stringstream messageStream;
        messageStream << "command \"" << cmd << "\" failed with code: " << rc;
        g_log.debug(messageStream.str());
      }
    } catch (Poco::SystemException &e) {
      g_log.debug("command \"" + cmd + "\" failed");
      g_log.debug(e.what());
    }
  }

  return description;
}

/// @returns The name of the current user as reported by the environment.
std::string ConfigServiceImpl::getUsername() {
  std::string username;

  // mac and favorite way to get username on linux
  try {
    username = m_pSysConfig->getString("system.env.USER");
    if (!username.empty()) {
      return username;
    }
  } catch (Poco::NotFoundException &e) {
    UNUSED_ARG(e); // let it drop on the floor
  }

  // windoze and alternate linux username variable
  try {
    username = m_pSysConfig->getString("system.env.USERNAME");
    if (!username.empty()) {
      return username;
    }
  } catch (Poco::NotFoundException &e) {
    UNUSED_ARG(e); // let it drop on the floor
  }

  // give up and return an empty string
  return std::string();
}

/** Gets the absolute path of the current directory containing the dll
 *
 * @returns The absolute path of the current directory containing the dll
 */
std::string ConfigServiceImpl::getCurrentDir() {
  return m_pSysConfig->getString("system.currentDir");
}

/** Gets the absolute path of the current directory containing the dll. Const
 *version.
 *
 * @returns The absolute path of the current directory containing the dll
 */
std::string ConfigServiceImpl::getCurrentDir() const {
  return m_pSysConfig->getString("system.currentDir");
}

/** Gets the absolute path of the temp directory
 *
 * @returns The absolute path of the temp directory
 */
std::string ConfigServiceImpl::getTempDir() {
  return m_pSysConfig->getString("system.tempDir");
}

/** Gets the absolute path of the appdata directory
*
* @returns The absolute path of the appdata directory
*/
std::string ConfigServiceImpl::getAppDataDir() {
  const std::string applicationName = "mantid";
#if POCO_OS == POCO_OS_WINDOWS_NT
  const std::string vendorName = "mantidproject";
  std::string appdata = std::getenv("APPDATA");
  Poco::Path path(appdata);
  path.makeDirectory();
  path.pushDirectory(vendorName);
  path.pushDirectory(applicationName);
  return path.toString();
#else // linux and mac
  Poco::Path path(Poco::Path::home());
  path.pushDirectory("." + applicationName);
  return path.toString();
#endif
}

/**
 * Get the directory containing the program executable
 * @returns A string containing the path of the directory
 * containing the executable, including a trailing slash
 */
std::string ConfigServiceImpl::getDirectoryOfExecutable() const {
  return Poco::Path(getPathToExecutable()).parent().toString();
}

/**
  * Get the full path to the executing program (i.e. whatever Mantid is embedded
 * in)
  * @returns A string containing the full path the the executable
  */
std::string ConfigServiceImpl::getPathToExecutable() const {
  std::string execpath("");
  const size_t LEN(1024);
  // cppcheck-suppress variableScope
  char pBuf[LEN];

#ifdef _WIN32
  unsigned int bytes = GetModuleFileName(NULL, pBuf, LEN);
#elif defined __linux__
  char szTmp[32];
  sprintf(szTmp, "/proc/%d/exe", getpid());
  ssize_t bytes = readlink(szTmp, pBuf, LEN);
#elif defined __APPLE__
  // Two calls to _NSGetExecutablePath required - first to get size of buffer
  uint32_t bytes(0);
  _NSGetExecutablePath(pBuf, &bytes);
  const int success = _NSGetExecutablePath(pBuf, &bytes);
  if (success < 0)
    bytes = 1025;
#endif

  if (bytes > 0 && bytes < 1024) {
    pBuf[bytes] = '\0';
    execpath = std::string(pBuf);
  }
  return execpath;
}

/**
 * Check if the path is on a network drive
 * @param path :: The path to be checked
 * @return True if the path is on a network drive.
 */
bool ConfigServiceImpl::isNetworkDrive(const std::string &path) {
#ifdef _WIN32
  // if path is relative get the full one
  char buff[MAX_PATH];
  GetFullPathName(path.c_str(), MAX_PATH, buff, NULL);
  std::string fullName(buff);
  size_t i = fullName.find(':');

  // if the full path doesn't contain a drive letter assume it's on the network
  if (i == std::string::npos)
    return true;

  fullName.erase(i + 1);
  fullName += '\\'; // make sure the name has the trailing backslash
  UINT type = GetDriveType(fullName.c_str());
  return DRIVE_REMOTE == type;
#elif defined __linux__
  // This information is only present in the /proc/mounts file on linux. There
  // are no drives on
  // linux only mount locations therefore the test will have to check the path
  // against
  // entries in /proc/mounts to see if the filesystem type is NFS or SMB (any
  // others ????)
  // Each line corresponds to a particular mounted location
  // 1st column - device name
  // 2nd column - mounted location
  // 3rd column - filesystem type commonly ext2, ext3 for hard drives and NFS or
  // SMB for
  //              network locations

  std::ifstream mntfile("/proc/mounts");
  std::string txtread("");
  while (getline(mntfile, txtread)) {
    std::istringstream strm(txtread);
    std::string devname(""), mntpoint(""), fstype("");
    strm >> devname >> mntpoint >> fstype;
    if (!strm)
      continue;
    // I can't be sure that the file system type is always lower case
    std::transform(fstype.begin(), fstype.end(), fstype.begin(), toupper);
    // Skip the current line if the file system isn't a network one
    if (fstype != "NFS" && fstype != "SMB")
      continue;
    // Now we have a line containing a network filesystem and just need to check
    // if the path
    // supplied contains the mount location. There is a small complication in
    // that the mount
    // points within the file have certain characters transformed into their
    // octal
    // representations, for example spaces->040.
    std::string::size_type idx = mntpoint.find("\\0");
    if (idx != std::string::npos) {
      std::string oct = mntpoint.substr(idx + 1, 3);
      strm.str(oct);
      int printch(-1);
      strm.setf(std::ios::oct, std::ios::basefield);
      strm >> printch;
      if (printch != -1) {
        mntpoint = mntpoint.substr(0, idx) + static_cast<char>(printch) +
                   mntpoint.substr(idx + 4);
      }
      // Search for this at the start of the path
      if (path.find(mntpoint) == 0)
        return true;
    }
  }
  return false;
#else
  UNUSED_ARG(path);
  // Not yet implemented for the mac
  return false;
#endif
}

/**
 * Gets the directory that we consider to be the directory containing the
 * Mantid.properties file.
 * Basically, this is the either the directory pointed to by MANTIDPATH or the
 * directory of the current
 * executable if this is not set.
 * @returns The directory to consider as the base directory, including a
 * trailing slash
 */
std::string ConfigServiceImpl::getPropertiesDir() const { return m_strBaseDir; }

/**
 * Return the directory that Mantid should use for writing any files it needs so
 * that
 * this is kept separated to user saved files. A trailing slash is appended
 * so that filenames can more easily be concatenated with this
 * @return the directory that Mantid should use for writing files
 */
std::string ConfigServiceImpl::getUserPropertiesDir() const {
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
const std::vector<std::string> &ConfigServiceImpl::getDataSearchDirs() const {
  return m_DataSearchDirs;
}

/**
 * Set a list of search paths via a vector
 * @param searchDirs :: A list of search directories
 */
void ConfigServiceImpl::setDataSearchDirs(
    const std::vector<std::string> &searchDirs) {
  std::string searchPaths = boost::join(searchDirs, ";");
  setDataSearchDirs(searchPaths);
}

/**
 * Set a list of search paths via a string
 * @param searchDirs :: A string containing a list of search directories
 * separated by a semi colon (;).
 */
void ConfigServiceImpl::setDataSearchDirs(const std::string &searchDirs) {
  setString("datasearch.directories", searchDirs);
}

/**
 *  Adds the passed path to the end of the list of data search paths
 *  the path name must be absolute
 *  @param path :: the absolute path to add
 */
void ConfigServiceImpl::appendDataSearchDir(const std::string &path) {
  if (path.empty())
    return;

  Poco::Path dirPath;
  try {
    dirPath = Poco::Path(path);
    dirPath.makeDirectory();
  }
  catch (Poco::PathSyntaxException &) {
    return;
  }
  if (!isInDataSearchList(dirPath.toString())) {
    std::string newSearchString;
    std::vector<std::string>::const_iterator it = m_DataSearchDirs.begin();
    for (; it != m_DataSearchDirs.end(); ++it) {
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
const std::vector<std::string> &ConfigServiceImpl::getUserSearchDirs() const {
  return m_UserSearchDirs;
}

/**
 * Return the search directories for XML instrument definition files (IDFs)
 * @returns An ordered list of paths for instrument searching
 */
const std::vector<std::string> &
ConfigServiceImpl::getInstrumentDirectories() const {
  return m_InstrumentDirs;
}

/**
 * Return the base search directories for XML instrument definition files (IDFs)
 * @returns a last entry of getInstrumentDirectories
 */
const std::string ConfigServiceImpl::getInstrumentDirectory() const {
  return m_InstrumentDirs[m_InstrumentDirs.size() - 1];
}

/**
 * Fills the internal cache of instrument definition directories
 */
void ConfigServiceImpl::cacheInstrumentPaths() {
  m_InstrumentDirs.clear();
  Poco::Path path(getAppDataDir());
  path.makeDirectory();
  path.pushDirectory("instrument");
  std::string appdatadir = path.toString();
  addDirectoryifExists(appdatadir, m_InstrumentDirs);

#ifndef _WIN32
  std::string etcdatadir = "/etc/mantid/instrument";
  addDirectoryifExists(etcdatadir, m_InstrumentDirs);
#endif

  // Determine the search directory for XML instrument definition files (IDFs)
  std::string directoryName = getString("instrumentDefinition.directory");
  if (directoryName.empty()) {
    // This is the assumed deployment directory for IDFs, where we need to be
    // relative to the
    // directory of the executable, not the current working directory.
    directoryName =
        Poco::Path(getPropertiesDir()).resolve("../instrument").toString();
  }
  addDirectoryifExists(directoryName, m_InstrumentDirs);
}

/**
 * Verifies the directory exists and add it to the back of the directory list if
 * valid
 * @param directoryName the directory name to add
 * @param directoryList the list to add the directory to
 * @returns true if the directory was valid and added to the list
 */
bool ConfigServiceImpl::addDirectoryifExists(
    const std::string &directoryName, std::vector<std::string> &directoryList) {
  try {
    if (Poco::File(directoryName).isDirectory()) {
      directoryList.push_back(directoryName);
      return true;
    } else {
      g_log.information("Unable to locate directory at: " + directoryName);
      return false;
    }
  }
  catch (Poco::PathNotFoundException &) {
    g_log.information("Unable to locate directory at: " + directoryName);
    return false;
  }
  catch (Poco::FileNotFoundException &) {
    g_log.information("Unable to locate directory at: " + directoryName);
    return false;
  }
}

/**
 * Load facility information from instrumentDir/Facilities.xml file if fName
 * parameter
 * is not set
 * @param fName :: An alternative file name for loading facilities information.
 */
void ConfigServiceImpl::updateFacilities(const std::string &fName) {
  clearFacilities();

  std::string instrDir = getString("instrumentDefinition.directory");
  std::string fileName = fName.empty() ? instrDir + "Facilities.xml" : fName;

  // Set up the DOM parser and parse xml file
  Poco::XML::DOMParser pParser;
  Poco::AutoPtr<Poco::XML::Document> pDoc;

  try {
    try {
      pDoc = pParser.parse(fileName);
    } catch (...) {
      throw Kernel::Exception::FileError("Unable to parse file:", fileName);
    }
    // Get pointer to root element
    Poco::XML::Element *pRootElem = pDoc->documentElement();
    if (!pRootElem->hasChildNodes()) {
      throw std::runtime_error("No root element in Facilities.xml file");
    }

    Poco::AutoPtr<Poco::XML::NodeList> pNL_facility =
        pRootElem->getElementsByTagName("facility");
    unsigned long n = pNL_facility->length();

    for (unsigned long i = 0; i < n; ++i) {
      Poco::XML::Element *elem =
          dynamic_cast<Poco::XML::Element *>(pNL_facility->item(i));
      if (elem) {
        m_facilities.push_back(new FacilityInfo(elem));
      }
    }

    if (m_facilities.empty()) {
      throw std::runtime_error("The facility definition file " + fileName +
                               " defines no facilities");
    }

  } catch (std::exception &e) {
    g_log.error(e.what());
  }
}

/// Empty the list of facilities, deleting the FacilityInfo objects in the
/// process
void ConfigServiceImpl::clearFacilities() {
  for (auto it = m_facilities.begin(); it != m_facilities.end(); ++it) {
    delete *it;
  }
  m_facilities.clear();
}

/**
 * Returns instruments with given name
 * @param  instrumentName Instrument name
 * @return the instrument information object
 * @throw NotFoundError if iName was not found
 */
const InstrumentInfo &
ConfigServiceImpl::getInstrument(const std::string &instrumentName) const {

  // Let's first search for the instrument in our default facility
  std::string defaultFacility = ConfigService::Instance().getFacility().name();

  if (!defaultFacility.empty()) {
    try {
      g_log.debug() << "Looking for " << instrumentName << " at "
                    << defaultFacility << "." << std::endl;
      return getFacility(defaultFacility).instrument(instrumentName);
    }
    catch (Exception::NotFoundError &) {
      // Well the instName doesn't exist for this facility
      // Move along, there's nothing to see here...
    }
  }

  // Now let's look through the other facilities
  std::vector<FacilityInfo *>::const_iterator it = m_facilities.begin();
  for (; it != m_facilities.end(); ++it) {
    try {
      g_log.debug() << "Looking for " << instrumentName << " at "
                    << (**it).name() << "." << std::endl;
      return (**it).instrument(instrumentName);
    }
    catch (Exception::NotFoundError &) {
      // Well the instName doesn't exist for this facility...
      // Move along, there's nothing to see here...
    }
  }
  g_log.debug("Instrument " + instrumentName + " not found");
  throw Exception::NotFoundError("Instrument", instrumentName);
}

/** Gets a vector of the facility Information objects
 * @return A vector of FacilityInfo objects
 */
const std::vector<FacilityInfo *> ConfigServiceImpl::getFacilities() const {
  return m_facilities;
}

/** Gets a vector of the facility names
 * @return A vector of the facility Names
 */
const std::vector<std::string> ConfigServiceImpl::getFacilityNames() const {
  auto names = std::vector<std::string>(m_facilities.size());
  auto itFacilities = m_facilities.begin();
  auto itNames = names.begin();
  for (; itFacilities != m_facilities.end(); ++itFacilities, ++itNames) {
    *itNames = (**itFacilities).name();
  }
  return names;
}

/** Get the default facility
 * @return the facility information object
 */
const FacilityInfo &ConfigServiceImpl::getFacility() const {
  std::string defFacility = getString("default.facility");
  if (defFacility.empty()) {
    defFacility = "ISIS";
  }
  return this->getFacility(defFacility);
}

/**
 * Get a facility
 * @param facilityName :: Facility name
 * @return the facility information object
 * @throw NotFoundException if the facility is not found
 */
const FacilityInfo &
ConfigServiceImpl::getFacility(const std::string &facilityName) const {
  if (facilityName.empty())
    return this->getFacility();

  std::vector<FacilityInfo *>::const_iterator it = m_facilities.begin();
  for (; it != m_facilities.end(); ++it) {
    if ((**it).name() == facilityName) {
      return **it;
    }
  }

  throw Exception::NotFoundError("Facilities", facilityName);
}

/**
 * Set the default facility
 * @param facilityName the facility name
 * @throw NotFoundException if the facility is not found
 */
void ConfigServiceImpl::setFacility(const std::string &facilityName) {
  bool found = false;
  // Look through the facilities for a matching one.
  std::vector<FacilityInfo *>::const_iterator it = m_facilities.begin();
  for (; it != m_facilities.end(); ++it) {
    if ((**it).name() == facilityName) {
      // Found the facility
      found = true;
      // So it's safe to set it as our default
      setString("default.facility", facilityName);
    }
  }
  if (found == false) {
    g_log.error("Failed to set default facility to be " + facilityName +
                ". Facility not found");
    throw Exception::NotFoundError("Facilities", facilityName);
  }
}

/**  Add an observer to a notification
 @param observer :: Reference to the observer to add
 */
void
ConfigServiceImpl::addObserver(const Poco::AbstractObserver &observer) const {
  m_notificationCenter.addObserver(observer);
}

/**  Remove an observer
 @param observer :: Reference to the observer to remove
 */
void ConfigServiceImpl::removeObserver(const Poco::AbstractObserver &observer)
    const {
  m_notificationCenter.removeObserver(observer);
}

/*
Checks to see whether the pvplugins.directory variable is set. If it is set, assume
we have built Mantid with ParaView
@return True if paraview is available or not disabled.
*/
bool ConfigServiceImpl::pvPluginsAvailable() const {
  std::string pvpluginsDir = getString("pvplugins.directory");
  return !pvpluginsDir.empty();
}

/**
 * Gets the path to the ParaView plugins
 * @returns A string giving the directory of the ParaView plugins
 */
const std::string ConfigServiceImpl::getPVPluginsPath() const {
  return getString("pvplugins.directory");
}

/*
Gets the system proxy information
@url A url to match the proxy to
@return the proxy information.
*/
Kernel::ProxyInfo &ConfigServiceImpl::getProxy(const std::string &url) {
  if (!m_isProxySet) {
    // set the proxy
    // first check if the proxy is defined in the properties file
    std::string proxyHost;
    int proxyPort;
    if ((getValue("proxy.host", proxyHost) == 1) &&
        (getValue("proxy.port", proxyPort) == 1)) {
      // set it from the config values
      m_proxyInfo = ProxyInfo(proxyHost, proxyPort, true);
    } else {
      // get the system proxy
      Poco::URI uri(url);
      Mantid::Kernel::NetworkProxy proxyHelper;
      m_proxyInfo = proxyHelper.getHttpProxy(uri.toString());
    }
    m_isProxySet = true;
  }
  return m_proxyInfo;
}

/// \cond TEMPLATE
template DLLExport int ConfigServiceImpl::getValue(const std::string &,
                                                   double &);
template DLLExport int ConfigServiceImpl::getValue(const std::string &,
                                                   std::string &);
template DLLExport int ConfigServiceImpl::getValue(const std::string &, int &);
template DLLExport int ConfigServiceImpl::getValue(const std::string &,
                                                   std::size_t &);
/// \endcond TEMPLATE

} // namespace Kernel
} // namespace Mantid
