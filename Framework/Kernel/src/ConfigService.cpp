// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Glob.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/NetworkProxy.h"
#include "MantidKernel/StdoutChannel.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"

#include <Poco/AutoPtr.h>
#include <Poco/Channel.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/Environment.h>
#include <Poco/Exception.h>
#include <Poco/File.h>
#include <Poco/Instantiator.h>
#include <Poco/Logger.h>
#include <Poco/LoggingFactory.h>
#include <Poco/LoggingRegistry.h>
#include <Poco/Path.h>
#include <Poco/Pipe.h>
#include <Poco/PipeStream.h>
#include <Poco/Platform.h>
#include <Poco/Process.h>
#include <Poco/StreamCopier.h>
#include <Poco/String.h>
#include <Poco/URI.h>
#include <Poco/Util/LoggingConfigurator.h>
#include <Poco/Util/PropertyFileConfiguration.h>
#include <Poco/Util/SystemConfiguration.h>
#include <Poco/Version.h>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/optional/optional.hpp>

#include <algorithm>
#include <cctype>
#include <codecvt>
#include <exception>
#include <fstream>
#include <functional>
#include <iostream>
#include <locale>
#include <stdexcept>
#include <utility>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <sys/sysctl.h>
#endif

namespace Mantid {
/**
 * Get the welcome message for Mantid.
 * @returns A string containing the welcome message for Mantid.
 */
std::string welcomeMessage() {
  return "Welcome to Mantid " + std::string(Mantid::Kernel::MantidVersion::version()) +
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
 * @returns vector containing the split path.
 */
std::vector<std::string> splitPath(const std::string &path) {
  std::vector<std::string> splitted;

  if (path.find(';') == std::string::npos) { // don't bother tokenizing
    splitted.emplace_back(path);
  } else {
    int options = Mantid::Kernel::StringTokenizer::TOK_TRIM + Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY;
    Mantid::Kernel::StringTokenizer tokenizer(path, ";,", options);
    auto iend = tokenizer.end();
    for (auto itr = tokenizer.begin(); itr != iend; ++itr) {
      if (!itr->empty()) {
        splitted.emplace_back(*itr);
      }
    }
  }
  return splitted;
}

const std::string LOG_LEVEL_KEY("logging.loggers.root.level");

} // end of anonymous namespace

//-------------------------------
// Private member functions
//-------------------------------

/// Private constructor for singleton class
ConfigServiceImpl::ConfigServiceImpl()
    : m_pConf(nullptr), m_pSysConfig(new Poco::Util::SystemConfiguration()), m_changed_keys(), m_strBaseDir(""),
      m_propertyString(""), m_properties_file_name("Mantid.properties"),
      m_user_properties_file_name("Mantid.user.properties"), m_dataSearchDirs(), m_instrumentDirs(), m_proxyInfo(),
      m_isProxySet(false) {
  // Register StdChannel with Poco
  Poco::LoggingFactory::defaultFactory().registerChannelClass(
      "StdoutChannel", new Poco::Instantiator<Poco::StdoutChannel, Poco::Channel>);

  setBaseDirectory();

  m_configPaths.insert("framework.plugins.directory");
  m_configPaths.insert("mantidqt.plugins.directory");
  m_configPaths.insert("instrumentDefinition.directory");
  m_configPaths.insert("instrumentDefinition.vtpDirectory");
  m_configPaths.insert("groupingFiles.directory");
  m_configPaths.insert("maskFiles.directory");
  m_configPaths.insert("colormaps.directory");
  m_configPaths.insert("requiredpythonscript.directories");
  m_configPaths.insert("pythonscripts.directory");
  m_configPaths.insert("pythonscripts.directories");
  m_configPaths.insert("python.plugins.directories");
  m_configPaths.insert("user.python.plugins.directories");
  m_configPaths.insert("icatDownload.directory");
  m_configPaths.insert("datasearch.directories");
  m_configPaths.insert("python.plugins.manifest");
  m_configPaths.insert("python.templates.directory");

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

  g_log.debug() << "ConfigService created.\n";
  g_log.debug() << "Configured Mantid.properties directory of application as " << getPropertiesDir() << '\n';
  g_log.information() << "This is Mantid version " << MantidVersion::version() << " revision "
                      << MantidVersion::revision() << '\n';
  g_log.information() << "running on " << getComputerName() << " starting "
                      << Types::Core::DateAndTime::getCurrentTime().toFormattedString("%Y-%m-%dT%H:%MZ") << "\n";
  g_log.information() << "Properties file(s) loaded: " << propertiesFilesList << '\n';

  // Assert that the appdata and the instrument subdirectory exists
  std::string appDataDir = getAppDataDir();
  Poco::Path path(appDataDir);
  path.pushDirectory("instrument");
  Poco::File file(path);
  // createDirectories will fail gracefully if it is already present - but will
  // throw an error if it cannot create the directory
  try {
    file.createDirectories();
  } catch (Poco::FileException &fe) {
    g_log.error() << "Cannot create the local instrument cache directory [" << path.toString()
                  << "]. Mantid will not be able to update instrument definitions.\n"
                  << fe.what() << '\n';
  }
  Poco::File vtpDir(getVTPFileDirectory());
  try {
    vtpDir.createDirectories();
  } catch (Poco::FileException &fe) {
    g_log.error() << "Cannot create the local instrument geometry cache directory [" << path.toString()
                  << "]. Mantid will be slower at viewing complex instruments.\n"
                  << fe.what() << '\n';
  }
  // must update the cache of instrument paths
  cacheInstrumentPaths();

  // update the facilities AFTER we have ensured that all of the directories are
  // created and the paths updated
  // if we don't do that first the function below will silently fail without
  // initialising the facilities vector
  // and Mantid will crash when it tries to access them, for example when
  // creating the first time startup screen
  updateFacilities();
}

/** Private Destructor
 *  Prevents client from calling 'delete' on the pointer handed out by Instance
 */
ConfigServiceImpl::~ConfigServiceImpl() {
  Kernel::Logger::shutdown();
  clearFacilities();
}

/**
 * Set the base directory path so we can file the Mantid.properties file.
 *
 * This will search for the base directory that contains the .properties file
 * by checking the following places:
 *  - The current working directory
 *  - The executable directory
 *  - The directory defined by the MANTIDPATH enviroment var
 *  - OSX only: the directory two directories up from the executable (which
 *    is the base on the OSX package.
 *
 */
void ConfigServiceImpl::setBaseDirectory() {
  // Define the directory to search for the Mantid.properties file.
  Poco::File f;

  // First directory: the current working
  m_strBaseDir = Poco::Path::current();
  f = Poco::File(m_strBaseDir + m_properties_file_name);
  if (f.exists())
    return;

  // Check the executable directory to see if it includes a mantid.properties
  // file
  m_strBaseDir = getDirectoryOfExecutable();
  f = Poco::File(m_strBaseDir + m_properties_file_name);
  if (f.exists())
    return;

  // Check the MANTIDPATH environment var
  if (Poco::Environment::has("MANTIDPATH")) {
    // Here we have to follow the convention of the rest of this code and
    // add a trailing slash.
    // Note: adding it to the MANTIDPATH itself will make other parts of the
    // code crash.
#ifdef _WIN32
    // In case Poco returns a Windows long path (prefixed with "\\?\"), we cannot
    // mix forward and back slashes in the path.
    m_strBaseDir = Poco::Environment::get("MANTIDPATH") + "\\";
#else
    m_strBaseDir = Poco::Environment::get("MANTIDPATH") + "/";
#endif
    f = Poco::File(m_strBaseDir + m_properties_file_name);
    if (f.exists())
      return;
  }

#ifdef __APPLE__
  // Finally, on OSX check if we're in the package directory and the .properties
  // file just happens to be two directories up
  auto path = Poco::Path(getDirectoryOfExecutable());
  m_strBaseDir = path.parent().parent().parent().toString();
#endif
}

namespace {
// look for specific keys and throw an exception if one is found
std::string checkForBadConfigOptions(const std::string &filename, const std::string &propertiesString) {
  std::stringstream stream(propertiesString);
  std::stringstream resultPropertiesString;
  std::string line;
  int line_num = 0;
  while (std::getline(stream, line)) {
    line_num += 1; // increment early
    bool is_ok = true;

    // Check for common errors. Empty lines are ok, things that are a key
    // without a value are a critical failure. Forbidden keys are just commented
    // out.
    if (line.empty() || (Kernel::Strings::strip(line)[0] == '#')) {
      // do nothing
    } else if (line.find("FilterChannel") != std::string::npos) {
      is_ok = false;
    }

    // Print warning to error channel and comment out offending line
    if (!is_ok) {
      const auto end = line.find("=");
      g_log.warning() << "Encontered invalid key \"";
      if (end != std::string::npos) {
        g_log.warning() << Kernel::Strings::strip(line.substr(0, end));
      } else {
        g_log.warning() << Kernel::Strings::strip(line);
      }
      g_log.warning() << "\" in " << filename << " on line " << line_num << std::endl;

      // comment out the property
      resultPropertiesString << '#';
    }
    // copy over the line
    resultPropertiesString << line << '\n';
  }
  return resultPropertiesString.str();
}
} // end of anonymous namespace

/** Loads the config file provided.
 *  If the file contains logging setup instructions then these will be used to
 *setup the logging framework.
 *
 *  @param filename :: The filename and optionally path of the file to load
 *  @param append :: If false (default) then any previous configuration is
 *discarded, otherwise the new keys are added, and repeated keys will override
 *existing ones.
 */
void ConfigServiceImpl::loadConfig(const std::string &filename, const bool append) {

  if (!append) {
    // remove the previous property string
    m_propertyString = "";
    m_changed_keys.clear();
  }

  try {
    // slurp in entire file
    std::string temp;
    bool good = readFile(filename, temp);

    // check if we have failed to open the file
    if ((!good) || (temp.empty())) {
      if (filename == getUserPropertiesDir() + m_user_properties_file_name) {
        // write out a fresh file
        createUserPropertiesFile();
      } else {
        throw Exception::FileError("Cannot open file", filename);
      }
    }

    // verify the contents and comment out offending lines
    temp = checkForBadConfigOptions(filename, temp);

    // store the property string
    if ((append) && (!m_propertyString.empty())) {
      m_propertyString = m_propertyString + "\n" + temp;
    } else {
      m_propertyString = temp;
    }
  } catch (std::exception &e) {
    // there was a problem loading the file - it probably is not there
    g_log.error() << "Problem loading the configuration file " << filename << " " << e.what() << '\n';
    g_log.error() << "Mantid is unable to start.\n" << std::endl;
    throw;
  }

  // use the cached property string to initialise the POCO property file
  std::istringstream istr(m_propertyString);
  m_pConf = new Poco::Util::PropertyFileConfiguration(istr);
}

/**
 * Read a file and place its contents into the given string
 * @param filename :: The filename of the file to read
 * @param contents :: The file contents will be placed here
 * @returns A boolean indicating whether opening the file was successful
 */
bool ConfigServiceImpl::readFile(const std::string &filename, std::string &contents) const {
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
    // Configure the logging framework
    Poco::Util::LoggingConfigurator configurator;
#if POCO_VERSION > 0x01090400
    configurator.configure(m_pConf);
#else
    configurator.configure(m_pConf.get());
#endif
  } catch (std::exception &e) {
    std::cerr << "Trouble configuring the logging framework " << e.what() << '\n';
  }
}

/**
 * Make a relative path or a list of relative paths into an absolute one.
 * @param dir :: The directory to convert
 * @param key :: The key variable this relates to
 * @returns A string containing an absolute path by resolving the relative
 * directory with the executable directory
 */
std::string ConfigServiceImpl::makeAbsolute(const std::string &dir, const std::string &key) const {
  if (dir.empty()) {
    // Don't do anything for an empty value
    return dir;
  }
  std::string converted;
  // If we have a list, chop it up and convert each one
  if (dir.find_first_of(";,") != std::string::npos) {
    auto splitted = splitPath(dir);
    auto iend = splitted.cend();
    for (auto itr = splitted.begin(); itr != iend;) {
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
  } catch (Poco::PathSyntaxException &) {
    g_log.warning() << "Malformed path detected in the \"" << key << "\" variable, skipping \"" << dir << "\"\n";
    return "";
  }
  if (is_relative) {
    const std::string propFileDir(getPropertiesDir());
    converted = Poco::Path(propFileDir).resolve(dir).toString();
  } else {
    converted = dir;
  }
  if (Poco::Path(converted).getExtension() != "") {
    converted = Poco::Path(converted).toString();
  } else {
    converted = Poco::Path(converted).makeDirectory().toString();
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
  std::string paths = getString("datasearch.directories", true);
  if (paths.empty()) {
    m_dataSearchDirs.clear();
  } else {
    m_dataSearchDirs = splitPath(paths);
  }
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

  using std::placeholders::_1;
  auto it = std::find_if(m_dataSearchDirs.cbegin(), m_dataSearchDirs.cend(),
                         std::bind(std::equal_to<std::string>(), _1, correctedPath));
  return (it != m_dataSearchDirs.end());
}

/**
 * writes a basic placeholder user.properties file to disk
 * any errors are caught and logged, but not propagated
 */
void ConfigServiceImpl::createUserPropertiesFile() const {
  try {
    std::fstream filestr((getUserPropertiesDir() + m_user_properties_file_name).c_str(), std::fstream::out);

    filestr << "# This file can be used to override any properties for this "
               "installation.\n";
    filestr << "# Any properties found in this file will override any that are "
               "found in the Mantid.Properties file\n";
    filestr << "# As this file will not be replaced with further installations "
               "of Mantid it is a safe place to put \n";
    filestr << "# properties that suit your particular installation.\n";
    filestr << "#\n";
    filestr << "# See here for a list of possible options:\n";
    filestr << "# "
               "http://docs.mantidproject.org/nightly/concepts/PropertiesFile.html"
               "\n\n";
    filestr << "##\n";
    filestr << "## GENERAL\n";
    filestr << "##\n\n";
    filestr << "## Set the maximum number of cores used to run algorithms over\n";
    filestr << "#MultiThreaded.MaxCores=4\n\n";
    filestr << "##\n";
    filestr << "## FACILITY AND INSTRUMENT\n";
    filestr << "##\n\n";
    filestr << "## Sets the default facility\n";
    filestr << "## e.g.: ISIS, SNS, ILL\n";
    filestr << "default.facility=\n\n";
    filestr << "## Sets the default instrument\n";
    filestr << "## e.g. IRIS, HET, NIMROD\n";
    filestr << "default.instrument=\n\n";
    filestr << '\n';
    filestr << "## Sets the Q.convention\n";
    filestr << "## Set to Crystallography for kf-ki instead of default "
               "Inelastic which is ki-kf\n";
    filestr << "#Q.convention=Crystallography\n";
    filestr << "##\n";
    filestr << "## DIRECTORIES\n";
    filestr << "##\n\n";
    filestr << "## Sets a list of directories (separated by semi colons) to "
               "search for data\n";
    filestr << "#datasearch.directories=../data;../isis/data\n\n";
    filestr << "## Set a list (separated by semi colons) of directories to "
               "look for additional Python scripts\n";
    filestr << "#pythonscripts.directories=../scripts;../docs/MyScripts\n\n";
    filestr << "## Uncomment to enable archive search - ICat and Orbiter\n";
    filestr << "#datasearch.searcharchive=On\n\n";
    filestr << "## Sets default save directory\n";
    filestr << "#defaultsave.directory=../data\n\n";
    filestr << "##\n";
    filestr << "## LOGGING\n";
    filestr << "##\n\n";
    filestr << "## Uncomment to change logging level\n";
    filestr << "## Default is information\n";
    filestr << "## Valid values are: error, warning, notice, information, debug\n";
    filestr << "#logging.loggers.root.level=information\n\n";
    filestr << "##\n";
    filestr << "## MantidWorkbench\n";
    filestr << "##\n\n";
    filestr << "## Hides categories from the algorithm list in MantidWorkbench\n";
    filestr << "#algorithms.catagories.hidden=Muons,Inelastic\n\n";
    filestr << "## Show invisible workspaces\n";
    filestr << "#MantidOptions.InvisibleWorkspaces=0\n";
    filestr << "## Re-use plot instances for different plot types\n";
    filestr << "#MantidOptions.ReusePlotInstances=Off\n\n";
    filestr << "## Uncomment to disable use of OpenGL to render unwrapped "
               "instrument views\n";
    filestr << "#MantidOptions.InstrumentView.UseOpenGL=Off\n\n";
    filestr << "## Muon GUI settings\n";
    filestr << "#muon.GUI = \n";

    filestr.close();
  } catch (std::runtime_error &ex) {
    g_log.warning() << "Unable to write out user.properties file to " << getUserPropertiesDir()
                    << m_user_properties_file_name << " error: " << ex.what() << '\n';
  }
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
  } catch (Poco::Exception &) {
  }
  createUserPropertiesFile();

  // Now load the original
  const bool append = false;
  const bool updateCaches = true;
  updateConfig(getPropertiesDir() + m_properties_file_name, append, updateCaches);
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
void ConfigServiceImpl::updateConfig(const std::string &filename, const bool append, const bool update_caches) {
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
    // Configure search paths into a specially saved store as they will be used
    // frequently
    cacheDataSearchPaths();
    appendDataSearchDir(getString("defaultsave.directory"));
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
  std::string updated_file;

  std::ifstream reader(filename.c_str(), std::ios::in);
  if (reader.bad()) {
    throw std::runtime_error("Error opening user properties file. Cannot save "
                             "updated configuration.");
  }

  std::string file_line, output;
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
      updated_file.append(key).append("=").append(value);
      // Remove the key from the changed key list
      m_changed_keys.erase(key);
    }
    updated_file += "\n";
  } // End while-loop

  // Any remaining keys within the changed key store weren't present in the
  // current user properties so append them IF they exist
  if (!m_changed_keys.empty()) {
    updated_file += "\n";
    auto key_end = m_changed_keys.end();
    for (auto key_itr = m_changed_keys.begin(); key_itr != key_end;) {
      // if the key does not have a property, skip it
      if (!hasProperty(*key_itr)) {
        ++key_itr;
        continue;
      }
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
 *  @param pathAbsolute :: If true then any key that looks like it contains
 * a path has this path converted to an absolute path.
 *  @returns The string value of the property, or an empty string if the key
 *cannot be found
 */
std::string ConfigServiceImpl::getString(const std::string &keyName, bool pathAbsolute) const {
  if (m_pConf->hasProperty(keyName)) {
    std::string value = m_pConf->getString(keyName);
    const auto key = m_configPaths.find(keyName);
    if (pathAbsolute && key != m_configPaths.end()) {
      value = makeAbsolute(value, keyName);
    }
    return value;
  }

  g_log.debug() << "Unable to find " << keyName << " in the properties file" << '\n';
  return {};
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
std::vector<std::string> ConfigServiceImpl::getKeys(const std::string &keyName) const {
  std::vector<std::string> rawKeys;
  m_pConf->keys(keyName, rawKeys);
  return rawKeys;
}

/**
 * Recursively gets a list of all config options from a given root node.
 */
void ConfigServiceImpl::getKeysRecursive(const std::string &root, std::vector<std::string> &allKeys) const {
  std::vector<std::string> rootKeys = getKeys(root);

  if (rootKeys.empty())
    allKeys.emplace_back(root);

  for (auto &rootKey : rootKeys) {
    std::string searchString;
    if (root.empty()) {
      searchString.append(rootKey);
    } else {
      searchString.append(root).append(".").append(rootKey);
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
void ConfigServiceImpl::remove(const std::string &rootName) {
  m_pConf->remove(rootName);
  m_changed_keys.insert(rootName);
}

/** Checks to see whether the given key exists.
 *
 *  @param rootName :: The case sensitive key that you are looking to see if
 *exists.
 *  @returns Boolean value denoting whether the exists or not.
 */
bool ConfigServiceImpl::hasProperty(const std::string &rootName) const { return m_pConf->hasProperty(rootName); }

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
      return tempFile.canExecute();
    } else
      return false;
  } catch (Poco::Exception &) {
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

void ConfigServiceImpl::launchProcess(const std::string &programFilePath,
                                      const std::vector<std::string> &programArguments) const {
  try {
    std::string expTarget = Poco::Path::expand(programFilePath);
    Poco::Process::launch(expTarget, programArguments);
  } catch (Poco::SystemException &e) {
    throw std::runtime_error(e.what());
  }
}

/**
 * Set a configuration property. An existing key will have its value updated.
 * @param key :: The key to refer to this property
 * @param value :: The value of the property
 */
void ConfigServiceImpl::setString(const std::string &key, const std::string &value) {
  // If the value is unchanged (after any path conversions), there's nothing to
  // do.
  const std::string old = getString(key);
  if (value == old)
    return;

  // Update the internal value
  m_pConf->setString(key, value);

  // Cache things that are accessed frequently
  if (key == "datasearch.directories") {
    cacheDataSearchPaths();
  } else if (key == "instrumentDefinition.directory") {
    cacheInstrumentPaths();
  } else if (key == "defaultsave.directory") {
    appendDataSearchDir(value);
  } else if (key == "logging.channels.consoleChannel.class") {
    // this key requires reloading logging for it to take effect
    configureLogging();
  } else if (key == LOG_LEVEL_KEY) {
    this->setLogLevel(value);
  }

  m_notificationCenter.postNotification(new ValueChanged(key, value, old));
  m_changed_keys.insert(key);
}

/** Searches for a string within the currently loaded configuration values and
 *  attempts to convert the values to the template type supplied.
 *
 *  @param keyName :: The case sensitive name of the property that you need the
 *value of.
 *  @returns An optional container with the value if found
 */
template <typename T> boost::optional<T> ConfigServiceImpl::getValue(const std::string &keyName) {
  std::string strValue = getString(keyName);
  T output;
  int result = Mantid::Kernel::Strings::convert(strValue, output);

  if (result != 1) {
    return boost::none;
  }

  return boost::optional<T>(output);
}

/** Searches for a string within the currently loaded configuration values and
 *  attempts to convert the values to a boolean value
 *
 *  @param keyName :: The case sensitive name of the property that you need the
 *value of.
 *  @returns An optional container with the value if found
 */
template <> boost::optional<bool> ConfigServiceImpl::getValue(const std::string &keyName) {
  auto returnedValue = getValue<std::string>(keyName);
  if (!returnedValue.is_initialized()) {
    return boost::none;
  }

  auto &configVal = returnedValue.get();

  std::transform(configVal.begin(), configVal.end(), configVal.begin(), ::tolower);

  boost::trim(configVal);

  bool trueString = configVal == "true";
  bool valueOne = configVal == "1";
  bool onOffString = configVal == "on";

  // A string of 1 or true both count
  return trueString || valueOne || onOffString;
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
std::string ConfigServiceImpl::getUserFilename() const { return getUserPropertiesDir() + m_user_properties_file_name; }

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
std::string ConfigServiceImpl::getOSName() { return m_pSysConfig->getString("system.osName"); }

/** Gets the name of the computer running Mantid
 *
 *  @returns The  name of the computer
 */
std::string ConfigServiceImpl::getOSArchitecture() {
  auto osArch = m_pSysConfig->getString("system.osArchitecture");
#ifdef __APPLE__
  if (osArch == "x86_64") {
    // This could be running under Rosetta on an Arm Mac
    // https://developer.apple.com/documentation/apple-silicon/about-the-rosetta-translation-environment
    int ret = 0;
    size_t size = sizeof(ret);
    if (sysctlbyname("sysctl.proc_translated", &ret, &size, nullptr, 0) != -1 && ret == 1) {
      osArch = "arm64_(x86_64)";
    }
  }
#endif
  return osArch;
}

/** Gets the name of the operating system Architecture
 *
 * @returns The operating system architecture
 */
std::string ConfigServiceImpl::getComputerName() { return m_pSysConfig->getString("system.nodeName"); }

/** Gets the name of the operating system version
 *
 * @returns The operating system version
 */
std::string ConfigServiceImpl::getOSVersion() { return m_pSysConfig->getString("system.osVersion"); }

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
std::string getValueFromStdOut(const std::string &orig, const std::string &key) {
  size_t start = orig.find(key);
  if (start == std::string::npos) {
    return std::string();
  }
  start += key.size();

  size_t stop = orig.find('\n', start);
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
  cmd = "wmic";                 // windows
  args.emplace_back("os");      // windows
  args.emplace_back("get");     // windows
  args.emplace_back("Caption"); // windows
  args.emplace_back("/value");  // windows
#endif

#if defined __APPLE__ || defined _WIN32
  try {
    Poco::Pipe outPipe, errorPipe;
    Poco::ProcessHandle ph = Poco::Process::launch(cmd, args, nullptr, &outPipe, &errorPipe);
    const int rc = ph.wait();
    // Only if the command returned successfully.
    if (rc == 0) {
      Poco::PipeInputStream pipeStream(outPipe);
      std::stringstream stringStream;
      Poco::StreamCopier::copyStream(pipeStream, stringStream);
      const std::string result = stringStream.str();
#ifdef __APPLE__
      const std::string product_name = getValueFromStdOut(result, "ProductName:");
      const std::string product_vers = getValueFromStdOut(result, "ProductVersion:");

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
#endif
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
  } catch (const Poco::NotFoundException &e) {
    UNUSED_ARG(e); // let it drop on the floor
  }

  // windoze and alternate linux username variable
  try {
    username = m_pSysConfig->getString("system.env.USERNAME");
    if (!username.empty()) {
      return username;
    }
  } catch (const Poco::NotFoundException &e) {
    UNUSED_ARG(e); // let it drop on the floor
  }

  // give up and return an empty string
  return std::string();
}

/** Gets the absolute path of the current directory containing the dll
 *
 * @returns The absolute path of the current directory containing the dll
 */
std::string ConfigServiceImpl::getCurrentDir() { return m_pSysConfig->getString("system.currentDir"); }

/** Gets the absolute path of the current directory containing the dll. Const
 *version.
 *
 * @returns The absolute path of the current directory containing the dll
 */
std::string ConfigServiceImpl::getCurrentDir() const { return m_pSysConfig->getString("system.currentDir"); }

/** Gets the absolute path of the temp directory
 *
 * @returns The absolute path of the temp directory
 */
std::string ConfigServiceImpl::getTempDir() { return m_pSysConfig->getString("system.tempDir"); }

/** Gets the absolute path of the appdata directory
 *
 * @returns The absolute path of the appdata directory
 */
std::string ConfigServiceImpl::getAppDataDir() {
  const std::string applicationName = "mantid";
#if POCO_OS == POCO_OS_WINDOWS_NT
  const std::string vendorName = "mantidproject";
  wchar_t *w_appdata = _wgetenv(L"APPDATA");
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  std::string appdata = converter.to_bytes(w_appdata);
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
 * @returns A string containing the full path the executable
 */
std::string ConfigServiceImpl::getPathToExecutable() const {
  std::string execpath;
  const size_t LEN(1024);
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
bool ConfigServiceImpl::isNetworkDrive([[maybe_unused]] const std::string &path) {
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
        mntpoint = mntpoint.substr(0, idx) + static_cast<char>(printch) + mntpoint.substr(idx + 4);
      }
      // Search for this at the start of the path
      if (path.find(mntpoint) == 0)
        return true;
    }
  }
  return false;
#else
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
const std::vector<std::string> &ConfigServiceImpl::getDataSearchDirs() const { return m_dataSearchDirs; }

/**
 * Set a list of search paths via a vector
 * @param searchDirs :: A list of search directories
 */
void ConfigServiceImpl::setDataSearchDirs(const std::vector<std::string> &searchDirs) {
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
 *  Appends the passed subdirectory path to the end of each of data
 *  search dirs and adds these new dirs to data search directories
 *  @param subdir :: the subdirectory path to add (relative)
 */
void ConfigServiceImpl::appendDataSearchSubDir(const std::string &subdir) {
  if (subdir.empty())
    return;

  Poco::Path subDirPath;
  try {
    subDirPath = Poco::Path(subdir);
  } catch (Poco::PathSyntaxException &) {
    return;
  }

  if (!subDirPath.isDirectory() || !subDirPath.isRelative()) {
    return;
  }

  auto newDataDirs = m_dataSearchDirs;
  for (const auto &path : m_dataSearchDirs) {
    Poco::Path newDirPath;
    try {
      newDirPath = Poco::Path(path);
      newDirPath.append(subDirPath);
      // only add new path if it isn't already there
      if (std::find(newDataDirs.begin(), newDataDirs.end(), newDirPath.toString()) == newDataDirs.end())
        newDataDirs.emplace_back(newDirPath.toString());
    } catch (Poco::PathSyntaxException &) {
      continue;
    }
  }

  setDataSearchDirs(newDataDirs);
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
  } catch (Poco::PathSyntaxException &) {
    return;
  }
  if (!isInDataSearchList(dirPath.toString())) {
    auto newSearchString = Strings::join(std::begin(m_dataSearchDirs), std::end(m_dataSearchDirs), ";");
    newSearchString.append(";" + path);
    setString("datasearch.directories", newSearchString);
  }
}

/**
 * Sets the search directories for XML instrument definition files (IDFs)
 * @param directories An ordered list of paths for instrument searching
 */
void ConfigServiceImpl::setInstrumentDirectories(const std::vector<std::string> &directories) {
  m_instrumentDirs = directories;
}

/**
 * Return the search directories for XML instrument definition files (IDFs)
 * @returns An ordered list of paths for instrument searching
 */
const std::vector<std::string> &ConfigServiceImpl::getInstrumentDirectories() const { return m_instrumentDirs; }

/**
 * Return the base search directories for XML instrument definition files (IDFs)
 * @returns a last entry of getInstrumentDirectories
 */
const std::string ConfigServiceImpl::getInstrumentDirectory() const { return m_instrumentDirs.back(); }
/**
 * Return the search directory for vtp files
 * @returns a path
 */
const std::string ConfigServiceImpl::getVTPFileDirectory() {
  // Determine the search directory for XML instrument definition files (IDFs)
  std::string directoryName = getString("instrumentDefinition.vtp.directory");

  if (directoryName.empty()) {
    Poco::Path path(getAppDataDir());
    path.makeDirectory();
    path.pushDirectory("instrument");
    path.pushDirectory("geometryCache");
    directoryName = path.toString();
  }
  return directoryName;
}
/**
 * Fills the internal cache of instrument definition directories and creates
 * The %appdata%/mantidproject/mantid or $home/.mantid directory.
 *
 * This will normally contain from Index 0
 * - The download directory (win %appdata%/mantidproject/mantid/instrument)
 *   (linux $home/.mantid/instrument )
 * - The user instrument area /etc/mantid/instrument (not on windows)
 * - The install directory/instrument
 */
void ConfigServiceImpl::cacheInstrumentPaths() {
  m_instrumentDirs.clear();

  Poco::Path path(getAppDataDir());
  path.makeDirectory();
  path.pushDirectory("instrument");
  const std::string appdatadir = path.toString();
  addDirectoryifExists(appdatadir, m_instrumentDirs);

#ifndef _WIN32
  addDirectoryifExists("/etc/mantid/instrument", m_instrumentDirs);
#endif

  // Determine the search directory for XML instrument definition files (IDFs)
  std::string directoryName = getString("instrumentDefinition.directory", true);
  if (directoryName.empty()) {
    // This is the assumed deployment directory for IDFs, where we need to be
    // relative to the
    // directory of the executable, not the current working directory.
    directoryName = Poco::Path(getPropertiesDir()).resolve("../instrument").toString();
  }
  addDirectoryifExists(directoryName, m_instrumentDirs);
}

/**
 * Verifies the directory exists and add it to the back of the directory list if
 * valid
 * @param directoryName the directory name to add
 * @param directoryList the list to add the directory to
 * @returns true if the directory was valid and added to the list
 */
bool ConfigServiceImpl::addDirectoryifExists(const std::string &directoryName,
                                             std::vector<std::string> &directoryList) {
  try {
    if (Poco::File(directoryName).isDirectory()) {
      directoryList.emplace_back(directoryName);
      return true;
    } else {
      g_log.information("Unable to locate directory at: " + directoryName);
      return false;
    }
  } catch (Poco::PathNotFoundException &) {
    g_log.information("Unable to locate directory at: " + directoryName);
    return false;
  } catch (Poco::FileNotFoundException &) {
    g_log.information("Unable to locate directory at: " + directoryName);
    return false;
  }
}

const std::vector<std::string> ConfigServiceImpl::getFacilityFilenames(const std::string &fName) {
  std::vector<std::string> returnPaths;

  // first try the supplied file
  if (!fName.empty()) {
    const Poco::File fileObj(fName);
    if (fileObj.exists()) {
      returnPaths.emplace_back(fName);
      return returnPaths;
    }
  }

  // search all of the instrument directories
  const auto &directoryNames = getInstrumentDirectories();

  // only use downloaded instruments if configured to download
  const std::string updateInstrStr = this->getString("UpdateInstrumentDefinitions.OnStartup");

  auto instrDir = directoryNames.begin();

  // If we are not updating the instrument definitions
  // update the iterator, this means we will skip the folder in HOME and
  // look in the instrument folder in mantid install directory or mantid source
  // code directory
  if (!(updateInstrStr == "1" || updateInstrStr == "on" || updateInstrStr == "On") && directoryNames.size() > 1) {
    instrDir++;
  }

  // look through all the possible files
  for (; instrDir != directoryNames.end(); ++instrDir) {
    Poco::Path p(*instrDir);
    p.append("Facilities.xml");
    std::string filename = p.toString();
    Poco::File fileObj(filename);

    if (fileObj.exists())
      returnPaths.emplace_back(filename);
  }

  if (returnPaths.size() > 0) {
    return returnPaths;
  }

  // getting this far means the file was not found
  std::string directoryNamesList = boost::algorithm::join(directoryNames, ", ");
  throw std::runtime_error("Failed to find \"Facilities.xml\". Searched in " + directoryNamesList);
}

/**
 * Load facility information from instrumentDir/Facilities.xml file if fName
 * parameter is not set.
 *
 * If any of the steps fail, we cannot sensibly recover, because the
 * Facilities.xml file is missing or corrupted.
 *
 * @param fName :: An alternative file name for loading facilities information.
 * @throws std::runtime_error :: If the file is not found or fails to parse
 */
void ConfigServiceImpl::updateFacilities(const std::string &fName) {
  clearFacilities();

  // Try to find the file. If it does not exist we will crash, and cannot read
  // the Facilities file
  const auto fileNames = getFacilityFilenames(fName);
  size_t attemptIndex = 0;
  bool success = false;
  while ((!success) && (attemptIndex < fileNames.size())) {
    const auto &fileName = fileNames[attemptIndex];
    try {
      // Set up the DOM parser and parse xml file
      Poco::AutoPtr<Poco::XML::Document> pDoc;
      try {
        Poco::XML::DOMParser pParser;
        pDoc = pParser.parse(fileName);
      } catch (...) {
        throw Kernel::Exception::FileError("Unable to parse file:", fileName);
      }

      // Get pointer to root element
      Poco::XML::Element *pRootElem = pDoc->documentElement();
      if (!pRootElem->hasChildNodes()) {
        throw std::runtime_error("No root element in Facilities.xml file");
      }

      const Poco::AutoPtr<Poco::XML::NodeList> pNL_facility = pRootElem->getElementsByTagName("facility");
      const size_t n = pNL_facility->length();

      for (unsigned long i = 0; i < n; ++i) {
        const auto *elem = dynamic_cast<Poco::XML::Element *>(pNL_facility->item(i));
        if (elem) {
          m_facilities.emplace_back(new FacilityInfo(elem));
        }
      }

      if (m_facilities.empty()) {
        throw std::runtime_error("The facility definition file " + fileName + " defines no facilities");
      }

      // if we got here we have suceeded and can exit the loop
      success = true;
    } catch (std::runtime_error &ex) {
      // log this failure to load a file
      g_log.error() << "Failed to load the facilities.xml file at " << fileName << "\nIt might be corrupt.  "
                    << ex.what() << "\nWill try to load another version.\n";
      attemptIndex++;
      // move on to the next file index if available
      if (attemptIndex == fileNames.size()) {
        const std::string errorMessage = "No more Facilities.xml files can be found, Mantid will not be "
                                         "able to start, Sorry.  Try reinstalling Mantid.";
        // This is one of the few times that both logging a messge and throwing
        // might make sense
        // as the error reporter tends to swallow the thrown message.
        g_log.error() << errorMessage << "\n";
        // Throw an exception as we have run out of files to try
        throw std::runtime_error(errorMessage);
      }
    }
  }
}

/// Empty the list of facilities, deleting the FacilityInfo objects in the
/// process
void ConfigServiceImpl::clearFacilities() {
  for (auto &facility : m_facilities) {
    delete facility;
  }
  m_facilities.clear();
}

/**
 * Returns instruments with given name
 * @param  instrumentName Instrument name
 * @return the instrument information object
 * @throw NotFoundError if iName was not found
 */
const InstrumentInfo &ConfigServiceImpl::getInstrument(const std::string &instrumentName) const {

  // Let's first search for the instrument in our default facility
  std::string defaultFacility = getFacility().name();

  if (!defaultFacility.empty()) {
    try {
      g_log.debug() << "Looking for " << instrumentName << " at " << defaultFacility << ".\n";
      return getFacility(defaultFacility).instrument(instrumentName);
    } catch (Exception::NotFoundError &) {
      // Well the instName doesn't exist for this facility
      // Move along, there's nothing to see here...
    }
  }

  // Now let's look through the other facilities
  for (auto facility : m_facilities) {
    try {
      g_log.debug() << "Looking for " << instrumentName << " at " << (*facility).name() << ".\n";
      return (*facility).instrument(instrumentName);
    } catch (Exception::NotFoundError &) {
      // Well the instName doesn't exist for this facility...
      // Move along, there's nothing to see here...
    }
  }

  const std::string errMsg = "Failed to find an instrument with this name in any facility: '" + instrumentName + "' -";
  g_log.debug("Instrument " + instrumentName + " not found");
  throw Exception::NotFoundError(errMsg, instrumentName);
}

/** Gets a vector of the facility Information objects
 * @return A vector of FacilityInfo objects
 */
const std::vector<FacilityInfo *> ConfigServiceImpl::getFacilities() const { return m_facilities; }

/** Gets a vector of the facility names
 * @return A vector of the facility Names
 */
const std::vector<std::string> ConfigServiceImpl::getFacilityNames() const {
  auto names = std::vector<std::string>(m_facilities.size());
  std::transform(m_facilities.cbegin(), m_facilities.cend(), names.begin(),
                 [](const FacilityInfo *facility) { return facility->name(); });

  return names;
}

/** Get the default facility
 * @return the facility information object
 */
const FacilityInfo &ConfigServiceImpl::getFacility() const {
  std::string defFacility = getString("default.facility");
  if (defFacility.empty()) {
    defFacility = " ";
  }
  return this->getFacility(defFacility);
}

/**
 * Get a facility
 * @param facilityName :: Facility name
 * @return the facility information object
 * @throw NotFoundException if the facility is not found
 */
const FacilityInfo &ConfigServiceImpl::getFacility(const std::string &facilityName) const {
  if (facilityName.empty())
    return this->getFacility();

  auto facility = std::find_if(m_facilities.cbegin(), m_facilities.cend(),
                               [&facilityName](const auto f) { return f->name() == facilityName; });

  if (facility != m_facilities.cend()) {
    return **facility;
  }

  throw Exception::NotFoundError("Facilities", facilityName);
}

/**
 * Set the default facility
 * @param facilityName the facility name
 * @throw NotFoundException if the facility is not found
 */
void ConfigServiceImpl::setFacility(const std::string &facilityName) {
  const FacilityInfo *foundFacility = nullptr;

  try {
    // Get facility looks up by string - so re-use that to check if the facility
    // is known
    foundFacility = &getFacility(facilityName);
  } catch (const Exception::NotFoundError &) {
    g_log.error("Failed to set default facility to be " + facilityName + ". Facility not found");
    throw;
  }
  assert(foundFacility);
  setString("default.facility", facilityName);

  const auto &associatedInsts = foundFacility->instruments();
  if (associatedInsts.empty()) {
    throw std::invalid_argument("The selected facility has no instruments associated with it");
  }

  // Update the default instrument to be one from this facility
  setString("default.instrument", associatedInsts[0].name());
}

/**  Add an observer to a notification
 @param observer :: Reference to the observer to add
 */
void ConfigServiceImpl::addObserver(const Poco::AbstractObserver &observer) const {
  m_notificationCenter.addObserver(observer);
}

/**  Remove an observer
 @param observer :: Reference to the observer to remove
 */
void ConfigServiceImpl::removeObserver(const Poco::AbstractObserver &observer) const {
  m_notificationCenter.removeObserver(observer);
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
    auto proxyHost = getValue<std::string>("proxy.host");
    auto proxyPort = getValue<int>("proxy.port");

    if (proxyHost.is_initialized() && proxyPort.is_initialized()) {
      // set it from the config values
      m_proxyInfo = ProxyInfo(proxyHost.get(), proxyPort.get(), true);
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

std::string ConfigServiceImpl::getFullPath(const std::string &filename, const bool ignoreDirs,
                                           const int options) const {
  std::string fName = Kernel::Strings::strip(filename);
  g_log.debug() << "getFullPath(" << fName << ")\n";
  // If this is already a full path, nothing to do
  if (Poco::Path(fName).isAbsolute())
    return fName;
  // First try the path relative to the current directory. Can throw in some
  // circumstances with extensions that have wild cards
  try {
    Poco::File fullPath(Poco::Path().resolve(fName));
    if (fullPath.exists() && (!ignoreDirs || !fullPath.isDirectory()))
      return fullPath.path();
  } catch (std::exception &) {
  }

  auto directoryNames = getDataSearchDirs();
  const auto &instrDirectories = getInstrumentDirectories();
  directoryNames.insert(directoryNames.end(), instrDirectories.begin(), instrDirectories.end());
  for (const auto &searchPath : directoryNames) {
    g_log.debug() << "Searching for " << fName << " in " << searchPath << "\n";
// On windows globbing is not working properly with network drives
// for example a network drive containing a $
// For this reason, and since windows is case insensitive anyway
// a special case is made for windows
#ifdef _WIN32
    if (fName.find("*") != std::string::npos) {
#endif
      Poco::Path path(searchPath, fName);
      std::set<std::string> files;
      Kernel::Glob::glob(path, files, options);
      if (!files.empty()) {
        Poco::File matchPath(*files.begin());
        if (ignoreDirs && matchPath.isDirectory()) {
          continue;
        }
        return *files.begin();
      }
#ifdef _WIN32
    } else {
      Poco::Path path(searchPath, fName);
      Poco::File file(path);
      if (file.exists() && !(ignoreDirs && file.isDirectory())) {
        return path.toString();
      }
    }
#endif
  }
  return "";
}

/** Sets the log level priority for all logging channels
 * @param logLevel the integer value of the log level to set, 1=Critical,
 * 7=Debug
 * @param quiet If true then no message regarding the level change is emitted
 */
void ConfigServiceImpl::setLogLevel(int logLevel, bool quiet) {
  Mantid::Kernel::Logger::setLevelForAll(logLevel);
  // update the internal value to keep strings in sync
  m_pConf->setString(LOG_LEVEL_KEY, g_log.getLevelName());

  if (!quiet) {
    g_log.log("logging set to " + Logger::PriorityNames[std::size_t(logLevel)] + " priority",
              static_cast<Logger::Priority>(logLevel));
  }
}

void ConfigServiceImpl::setLogLevel(std::string logLevel, bool quiet) {
  Mantid::Kernel::Logger::setLevelForAll(logLevel);
  // update the internal value to keep strings in sync
  m_pConf->setString(LOG_LEVEL_KEY, g_log.getLevelName());

  if (!quiet) {
    g_log.log("logging set to " + logLevel + " priority", static_cast<Logger::Priority>(g_log.getLevel()));
  }
}

/// \cond TEMPLATE
template DLLExport boost::optional<double> ConfigServiceImpl::getValue(const std::string &);
template DLLExport boost::optional<std::string> ConfigServiceImpl::getValue(const std::string &);
template DLLExport boost::optional<int> ConfigServiceImpl::getValue(const std::string &);
template DLLExport boost::optional<size_t> ConfigServiceImpl::getValue(const std::string &);
#ifdef _MSC_VER
template DLLExport boost::optional<bool> ConfigServiceImpl::getValue(const std::string &);
#endif

/// \endcond TEMPLATE

} // namespace Kernel
} // namespace Mantid
