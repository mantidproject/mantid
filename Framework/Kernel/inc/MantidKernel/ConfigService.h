// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/ProxyInfo.h"
#include "MantidKernel/SingletonHolder.h"
#include <boost/optional/optional.hpp>

#include <map>
#include <set>
#include <string>
#include <vector>

#include <Poco/Notification.h>
#include <Poco/NotificationCenter.h>

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
/// @cond Exclude from doxygen documentation
namespace Poco {
class AbstractObserver;
class Channel;
template <class C> class AutoPtr;
namespace Util {
class PropertyFileConfiguration;
class SystemConfiguration;
} // namespace Util
} // namespace Poco
/// @endcond

namespace Mantid {

/// Returns the welcome message for Mantid.
MANTID_KERNEL_DLL std::string welcomeMessage();

namespace Kernel {
//----------------------------------------------------------------------
// More forward declarations
//----------------------------------------------------------------------
class FacilityInfo;
class InstrumentInfo;

/** The ConfigService class provides a simple facade to access the Configuration
   functionality of the Mantid Framework.
    The class gathers information from config files and the system variables.
    This information is available to all the objects within the framework as
   well as being used to configure the logging framework.
    This class currently uses the Logging functionality provided through the
   POCO (portable components library).

    @author Nicholas Draper, Tessella Support Services plc
    @date 15/10/2007
 */
class MANTID_KERNEL_DLL ConfigServiceImpl final {
public:
  /**
   * This is the base class for POCO Notifications sent out from the Config
   * Service.
   * It does nothing.
   */
  class ConfigServiceNotification : public Poco::Notification {
  public:
    /// Empty constructor for ConfigServiceNotification Base Class
    ConfigServiceNotification() : Poco::Notification() {}
  };

  /**
   * This is the class for the notification that is to be sent when a value has
   * been changed in
   * config service.
   */
  class ValueChanged : public ConfigServiceNotification {
  public:
    /** Creates the Notification object with the required values
     *   @param name :: property that has been changed
     *   @param newvalue :: new value of property
     *   @param prevvalue :: previous value of property
     */
    ValueChanged(const std::string &name, const std::string &newvalue, const std::string &prevvalue)
        : ConfigServiceNotification(), m_name(name), m_value(newvalue), m_prev(prevvalue) {}
    /// The name of the user property that has changed, as it appears in the
    /// user.properties file
    const std::string &key() const { return this->m_name; } ///< @return The name of the changed the property
    /// The new value for the property
    const std::string &curValue() const { return this->m_value; } ///< @return The new value for the property
    /// The previous value for the property
    const std::string &preValue() const { return this->m_prev; } ///< @return The previous value for the property
  private:
    std::string m_name;  ///< The name of the changed the property
    std::string m_value; ///< The new value for the property
    std::string m_prev;  ///< The previous value for the property
  };

  /// Setup the base directory
  void setBaseDirectory();
  /// Reset to "factory" settings. Removes current user properties
  void reset();
  /// Wipe out the current configuration and load a new one
  void updateConfig(const std::string &filename, const bool append = false, const bool update_caches = true);
  /// Save the configuration to the user file
  void saveConfig(const std::string &filename) const;
  /// Searches for a configuration property
  std::string getString(const std::string &keyName, bool pathAbsolute = true) const;
  /// Searches for a key in the configuration property
  std::vector<std::string> getKeys(const std::string &keyName) const;
  /// Returns a list of all full keys in the config
  std::vector<std::string> keys() const;
  /// Removes the value from a selected keyName
  void remove(const std::string &rootName);
  /// Checks to see whether a key has a value assigned to it
  bool hasProperty(const std::string &rootName) const;
  /// Checks to see whether the target passed is an executable file
  bool isExecutable(const std::string &target) const;
  /// Launches a process i.e opening a program
  void launchProcess(const std::string &programFilePath, const std::vector<std::string> &programArguments) const;
  /// Sets a configuration property
  void setString(const std::string &key, const std::string &value);
  // Searches for a configuration property and returns its value
  template <typename T> boost::optional<T> getValue(const std::string &keyName);
  /// Return the local properties filename.
  std::string getLocalFilename() const;
  /// Return the user properties filename
  std::string getUserFilename() const;

  /** @name Host information */
  //@{
  /// Searches for the given environment variable and returns it as a string
  std::string getEnvironment(const std::string &keyName);
  /// Returns the OS name
  std::string getOSName();
  /// Returns the computer name
  std::string getComputerName();
  /// Returns the architecture
  std::string getOSArchitecture();
  /// Returns the OS version
  std::string getOSVersion();
  /// Returns a human readable version of the OS version
  std::string getOSVersionReadable();
  /// Returns the username
  std::string getUsername();
  /// Returns the current directory
  std::string getCurrentDir();
  /// Returns the current directory
  std::string getCurrentDir() const;
  /// Returns the system's temp directory
  std::string getTempDir();
  /// Returns the system's appdata directory
  std::string getAppDataDir();
  // Return the executable path
  std::string getDirectoryOfExecutable() const;
  // Return the full path to the executable
  std::string getPathToExecutable() const;
  // Check if the path is on a network drive
  bool isNetworkDrive([[maybe_unused]] const std::string &path);
  //@}

  /// Returns the directory where the Mantid.properties file is found.
  std::string getPropertiesDir() const;
  /// Returns a directory to use to write out Mantid information. Needs to be
  /// writable
  std::string getUserPropertiesDir() const;

  /** @name Search paths handling */
  //@{
  /// Get the list of search paths
  const std::vector<std::string> &getDataSearchDirs() const;
  /// Set a list of search paths via a vector
  void setDataSearchDirs(const std::vector<std::string> &searchDirs);
  /// Set a list of search paths via a string
  void setDataSearchDirs(const std::string &searchDirs);
  /// Adds the passed path to the end of the list of data search paths
  void appendDataSearchDir(const std::string &path);
  /// Appends subdirectory to each of the specified data search directories
  void appendDataSearchSubDir(const std::string &subdir);
  /// Sets instrument directories
  void setInstrumentDirectories(const std::vector<std::string> &directories);
  /// Get instrument search directories
  const std::vector<std::string> &getInstrumentDirectories() const;
  /// Get instrument search directory
  const std::string getInstrumentDirectory() const;
  /// get the vtp file directory
  const std::string getVTPFileDirectory();
  //@}

  /// Load facility information from instrumentDir/Facilities.xml file
  void updateFacilities(const std::string &fName = "");
  /// Get the list of facilities
  const std::vector<FacilityInfo *> getFacilities() const;
  /// Get the list of facility names
  const std::vector<std::string> getFacilityNames() const;
  /// Get the default facility
  const FacilityInfo &getFacility() const;
  /// Get a facility
  const FacilityInfo &getFacility(const std::string &facilityName) const;
  /// Set the default facility
  void setFacility(const std::string &facilityName);
  /// Sets the log level priority for all log channels
  void setLogLevel(int logLevel, bool quiet = false);
  /// Sets the log level priority for all log channels
  void setLogLevel(std::string const &logLevel, bool quiet = false);
  // return the string name for the log level
  std::string getLogLevel();

  /// Look for an instrument
  const InstrumentInfo &getInstrument(const std::string &instrumentName = "") const;

  /// Add an observer for a notification
  void addObserver(const Poco::AbstractObserver &observer) const;

  /// Remove an observer
  void removeObserver(const Poco::AbstractObserver &observer) const;

  // Starts up the logging
  void configureLogging();

  /// Gets the proxy for the system
  Kernel::ProxyInfo &getProxy(const std::string &url);

  std::string getFullPath(const std::string &filename, const bool ignoreDirs, const int options) const;

private:
  friend struct Mantid::Kernel::CreateUsingNew<ConfigServiceImpl>;
  /// Handles distribution of Poco signals.
  mutable Poco::NotificationCenter m_notificationCenter;

  // Private constructors and destructor for singleton class
  ConfigServiceImpl();
  /// Private copy constructor. Prevents singleton being copied.
  ConfigServiceImpl(const ConfigServiceImpl &);

  virtual ~ConfigServiceImpl();

  /// Loads a config file
  void loadConfig(const std::string &filename, const bool append = false);
  /// Read a file and place its contents into the given string
  bool readFile(const std::string &filename, std::string &contents) const;

  /// Writes out a fresh user properties file
  void createUserPropertiesFile() const;
  /// Make a relative path or a list of relative paths into an absolute one.
  std::string makeAbsolute(const std::string &dir, const std::string &key) const;
  /// Create the storage of the data search directories
  void cacheDataSearchPaths();
  /// Create the storage of the instrument directories
  void cacheInstrumentPaths();
  /// Returns true if the path is in the data search list
  bool isInDataSearchList(const std::string &path) const;
  /// Empty the list of facilities, deleting the FacilityInfo objects in the
  /// process
  void clearFacilities();
  /// Determine the name of the facilities file to use
  const std::vector<std::string> getFacilityFilenames(const std::string &fName);
  /// Verifies the directory exists and add it to the back of the directory list
  /// if valid
  bool addDirectoryifExists(const std::string &directoryName, std::vector<std::string> &directoryList);
  /// Returns a list of all keys under a given root key
  void getKeysRecursive(const std::string &root, std::vector<std::string> &allKeys) const;

  /// the POCO file config object
  Poco::AutoPtr<Poco::Util::PropertyFileConfiguration> m_pConf;
  /// the POCO system Config Object
  Poco::AutoPtr<Poco::Util::SystemConfiguration> m_pSysConfig;

  /// A set of property keys that have been changed
  mutable std::set<std::string> m_changed_keys;

  /// The directory that is considered to be the base directory
  std::string m_strBaseDir;
  /// The configuration properties in string format
  std::string m_propertyString;
  /// The filename of the Mantid properties file
  const std::string m_properties_file_name;
  /// The filename of the Mantid user properties file
  const std::string m_user_properties_file_name;
  /// Store a list of data search paths
  std::vector<std::string> m_dataSearchDirs;
  /// Store a list of instrument directory paths
  std::vector<std::string> m_instrumentDirs;

  /// The list of available facilities
  std::vector<FacilityInfo *> m_facilities;

  /// List of config paths that may be relative
  std::set<std::string> m_configPaths;

  /// local cache of proxy details
  Kernel::ProxyInfo m_proxyInfo;
  /// whether the proxy has been populated yet
  bool m_isProxySet;
};

EXTERN_MANTID_KERNEL template class MANTID_KERNEL_DLL Mantid::Kernel::SingletonHolder<ConfigServiceImpl>;
using ConfigService = Mantid::Kernel::SingletonHolder<ConfigServiceImpl>;

using ConfigValChangeNotification = Mantid::Kernel::ConfigServiceImpl::ValueChanged;
using ConfigValChangeNotification_ptr = const Poco::AutoPtr<Mantid::Kernel::ConfigServiceImpl::ValueChanged> &;

} // namespace Kernel
} // namespace Mantid
