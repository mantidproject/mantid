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
  namespace Kernel
  {
    //----------------------------------------------------------------------
    // More forward declarations
    //----------------------------------------------------------------------
    class Logger;

    /** The ConfigService class provides a simple facade to access the Configuration functionality of the Mantid Framework.
        The class gathers information from config files and the system variables.  
        This information is available to all the objects within the framework as well as being used to configure the logging framework.
        This class currently uses the Logging functionality provided through the POCO (portable components library).

        @author Nicholas Draper, Tessella Support Services plc
        @date 15/10/2007

        Copyright &copy; 2007-2010 STFC Rutherford Appleton Laboratory

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

      // Searches for the given environment variable and returns it as a string
      std::string getEnvironment(const std::string& keyName);

      // Getters for properties of the host system
      std::string getOSName();
      std::string getComputerName();
      std::string getOSArchitecture();
      std::string getOSVersion();
      std::string getCurrentDir();
      std::string getTempDir();
      std::string getBaseDir() const;
      std::string getOutputDir() const;

      /// Get the list of search paths
      const std::vector<std::string>& getDataSearchDirs() const;
      /// Get the list of known instrument prefixes for the given facility
      const std::vector<std::string>& getInstrumentPrefixes(const std::string& facility) const;

    private:
      friend struct Mantid::Kernel::CreateUsingNew<ConfigServiceImpl>;

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
      const std::string defaultConfig() const;
      /// Writes out a fresh user properties file
      void createUserPropertiesFile() const;
      /// Convert any relative paths to absolute ones and store them locally so that
      /// if the working directory is altered the paths will not be affected
      void convertRelativeToAbsolute();
      ///Make a relative path or a list of relative paths into an absolute one.
      std::string makeAbsolute(const std::string & dir, const std::string & key) const;
      /// Create the storage of the data search directories
      void cacheDataSearchPaths();
      /// Create the map of facility name to instrument prefix list
      void cacheInstrumentPrefixes();

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
      /// A map of facilities to instruments
      std::map<std::string,std::vector<std::string> > m_instr_prefixes;
    };

    /// Forward declaration of a specialisation of SingletonHolder for AlgorithmFactoryImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef __APPLE__
    inline
#endif
    template class EXPORT_OPT_MANTID_KERNEL Mantid::Kernel::SingletonHolder<ConfigServiceImpl>;
    typedef EXPORT_OPT_MANTID_KERNEL Mantid::Kernel::SingletonHolder<ConfigServiceImpl> ConfigService;

  } // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_CONFIGSERVICE_H_*/
