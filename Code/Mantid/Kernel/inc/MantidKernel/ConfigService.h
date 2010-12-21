#ifndef MANTID_KERNEL_CONFIGSERVICE_H_
#define MANTID_KERNEL_CONFIGSERVICE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllExport.h"
#include "MantidKernel/SingletonHolder.h"
#include <vector>
#include <map>
#include <set>

#include <Poco/Notification.h>
#include <Poco/NotificationCenter.h>
#include <Poco/AutoPtr.h>

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
/// @cond Exclude from doxygen documentation
namespace Poco
{
  namespace Util
  {
    class PropertyFileConfiguration;
    class SystemConfiguration;
  }
}
/// @endcond

namespace Mantid
{

  /// Returns the welcome message for Mantid.
  DLLExport std::string welcomeMessage();

  namespace Kernel
  {
    //----------------------------------------------------------------------
    // More forward declarations
    //----------------------------------------------------------------------
    class Logger;
    class FacilityInfo;

    /** The ConfigService class provides a simple facade to access the Configuration functionality of the Mantid Framework.
        The class gathers information from config files and the system variables.  
        This information is available to all the objects within the framework as well as being used to configure the logging framework.
        This class currently uses the Logging functionality provided through the POCO (portable components library).

        @author Nicholas Draper, Tessella Support Services plc
        @date 15/10/2007

        Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

        This file is part of Mantid.

        Mantid is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 3 of the License, or
        (at your option) any later version.

        Mantid is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.  If not, see <http://www.gnu.org/licenses/>.

        File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
        Code Documentation is available at: <http://doxygen.mantidproject.org>
     */
    class EXPORT_OPT_MANTID_KERNEL ConfigServiceImpl
    {
    public:

      /**
      * This is the base class for POCO Notifications sent out from the Config Service.
      * It does nothing.
      */
      class ConfigServiceNotification : public Poco::Notification
      {
      public:
        /// Empty constructor for ConfigServiceNotification Base Class
        ConfigServiceNotification() : Poco::Notification() {}
      };

      /**
      * This is the class for the notification that is to be sent when a value has been changed in
      * config service.
      */
      class ValueChanged : public ConfigServiceNotification
      {
      public:
        /** Creates the Notification object with the required values
        *   @param name property that has been changed
        *   @param newvalue new value of property
        *   @param prevvalue previous value of property
        */
        ValueChanged(const std::string name, const std::string newvalue, const std::string prevvalue) : ConfigServiceNotification(), m_name(name), m_value(newvalue), m_prev(prevvalue) {}
        /// The name of the user property that has changed, as it appears in the user.properties file
        const std::string & key() const { return this->m_name; } ///< @return The name of the changed the property
        /// The new value for the property
        const std::string & curValue() const { return this->m_value; } ///< @return The new value for the property
        /// The previous value for the property
        const std::string & preValue() const { return this->m_prev; } ///< @return The previous value for the property
      private:
        std::string m_name; ///< The name of the changed the property
        std::string m_value; ///< The new value for the property
        std::string m_prev; ///< The previous value for the property
      };
      
      /// Wipe out the current configuration and load a new one
      void updateConfig(const std::string& filename, const bool append=false, const bool update_caches=true);
      /// Save the configuration to the user file
      void saveConfig(const std::string &filename) const;
      /// Searches for a configuration property
      std::string getString(const std::string& keyName, bool use_cache=true) const;
      /// Sets a configuration property
      void setString(const std::string & keyName, const std::string & keyValue);
      // Searches for a configuration property and returns its value
      template<typename T>
      int getValue(const std::string& keyName, T& out);
      /// Return the user properties filename
      std::string getUserFilename() const;

      /** @name Host information */
      //@{
      /// Searches for the given environment variable and returns it as a string
      std::string getEnvironment(const std::string& keyName);
      /// Returns the OS name
      std::string getOSName();
      /// Returns the computer name
      std::string getComputerName();
      /// Returns the architecture
      std::string getOSArchitecture();
      /// Returns the OS version
      std::string getOSVersion();
      /// Returns the current directory
      std::string getCurrentDir();
      /// Returns the system's temp directory
      std::string getTempDir();
      //@}

      /// Returns the directory where the framework libraries lie
      std::string getBaseDir() const;
      /// Returns a directory to use as a default output directory
      std::string getOutputDir() const;
      /// Get the list of search paths
      const std::vector<std::string>& getDataSearchDirs() const;
      /// Get the list of user search paths
      const std::vector<std::string>& getUserSearchDirs() const;
      /// Get instrument search directory
      const std::string getInstrumentDirectory() const;

      /// Load facility information from instrumentDir/Facilities.xml file 
      void updateFacilities(const std::string& fName = "");
      /// Get the default facility
      const FacilityInfo& Facility()const;
      /// Get a facility
      const FacilityInfo& Facility(const std::string& fName)const;

      /// Add an observer for a notification
      void addObserver(const Poco::AbstractObserver& observer)const;

      /// Remove an observer
      void removeObserver(const Poco::AbstractObserver& observer)const;

    private:
      friend struct Mantid::Kernel::CreateUsingNew<ConfigServiceImpl>;
      /// Handles distribution of Poco signals.
      mutable Poco::NotificationCenter m_notificationCenter;

      // Private constructors and destructor for singleton class
      ConfigServiceImpl();
      /// Private copy constructor. Prevents singleton being copied.
      ConfigServiceImpl(const ConfigServiceImpl&);

      virtual ~ConfigServiceImpl();

      /// Loads a config file
      void loadConfig(const std::string& filename, const bool append=false);
      /// Read a file and place its contents into the given string
      bool readFile(const std::string& filename, std::string & contents) const;
      // Starts up the logging
      void configureLogging();
      /// Provies a string of a default configuration
      std::string defaultConfig() const;
      /// Writes out a fresh user properties file
      void createUserPropertiesFile() const;
      /// Convert any relative paths to absolute ones and store them locally so that
      /// if the working directory is altered the paths will not be affected
      void convertRelativeToAbsolute();
      ///Make a relative path or a list of relative paths into an absolute one.
      std::string makeAbsolute(const std::string & dir, const std::string & key) const;
      /// Create the storage of the data search directories
      void cacheDataSearchPaths();
      /// Create the storage of the user search directories
      void cacheUserSearchPaths();

      // Forward declaration of inner class
      template <class T>
      class WrappedObject;
      /// the POCO file config object
      WrappedObject<Poco::Util::PropertyFileConfiguration>* m_pConf;
      /// the POCO system Config Object
      WrappedObject<Poco::Util::SystemConfiguration>* m_pSysConfig;

      /// reference to the logger class
      Logger& g_log;

      /// A set of property keys that have been changed
      mutable std::set<std::string> m_changed_keys;
      
      /// A map storing string/key pairs where the string denotes a path 
      /// that could be relative in the user properties file
      /// The boolean indicates whether the path needs to exist or not
      std::map<std::string, bool> m_ConfigPaths;
      /// Local storage for the relative path key/values that have been changed 
      std::map<std::string, std::string> m_AbsolutePaths;
      /// The directory that is considered to be the base directory
      std::string m_strBaseDir;
      ///The configuration properties in string format
      std::string m_PropertyString;
      /// The filename of the Mantid properties file
      const std::string m_properties_file_name;
      /// The filename of the Mantid user properties file
      const std::string m_user_properties_file_name;
      /// Store a list of data search paths
      std::vector<std::string> m_DataSearchDirs;
      /// Store a list of user search paths
      std::vector<std::string> m_UserSearchDirs;
      /// A map of facilities to instruments
      std::map<std::string,std::vector<std::string> > m_instr_prefixes;

      /// The list of available facilities
      std::vector<FacilityInfo*> m_facilities;
    };

    /// Forward declaration of a specialisation of SingletonHolder for AlgorithmFactoryImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef __APPLE__
    inline
#endif
    template class EXPORT_OPT_MANTID_KERNEL Mantid::Kernel::SingletonHolder<ConfigServiceImpl>;
    typedef EXPORT_OPT_MANTID_KERNEL Mantid::Kernel::SingletonHolder<ConfigServiceImpl> ConfigService;

    typedef Mantid::Kernel::ConfigServiceImpl::ValueChanged ConfigValChangeNotification;
    typedef const Poco::AutoPtr<Mantid::Kernel::ConfigServiceImpl::ValueChanged>& ConfigValChangeNotification_ptr;

  } // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_CONFIGSERVICE_H_*/
