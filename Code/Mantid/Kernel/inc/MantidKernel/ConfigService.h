#ifndef MANTID_KERNEL_CONFIGSERVICE_H_
#define MANTID_KERNEL_CONFIGSERVICE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>

#include "MantidKernel/DllExport.h"
#include "MantidKernel/SingletonHolder.h"

//----------------------------------------------------------------------
// Forward declaration
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
class Logger;	
	
/** The ConfigService class provides a simple facade to access the Configuration functionality of the Mantid Framework.
	  The class gathers information from config files and the system variables.  
	  This information is available to all the objects within the framework as well as being used to configure the logging framework.
	  This class currently uses the Logging functionality provided through the POCO (portable components library).
    
    @author Nicholas Draper, Tessella Support Services plc
    @date 15/10/2007
    
    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
	  /** Inner templated class to wrap the poco library objects that have protected 
	   *  desctructors and expose them as public.
	   */
	  template<typename T >
	  class WrappedObject : public T
	  {
	  public:
	    /// The template type of class that is being wrapped
	    typedef T element_type;
	    /// Simple constructor
	    WrappedObject() : T()
	    {
	      m_pPtr = static_cast<T*>(this);
	    }

	    /** Constructor with a class to wrap
	     *  @param F The object to wrap
	     */
	    template<typename Field>
	    WrappedObject(Field& F) : T(F)
	    {
	      m_pPtr = static_cast<T*>(this);
	    }

	    /// Copy constructor
	    WrappedObject(const WrappedObject<T>& A) : T(A)
	    {
  				m_pPtr = static_cast<T*>(this);
	    }
		       
	    /// Virtual destructor
	    virtual ~WrappedObject()
	    {}
		       
	    /// Overloaded * operator returns the wrapped object pointer
	    const T& operator*() const { return *m_pPtr; }	    
	    /// Overloaded * operator returns the wrapped object pointer
	    T& operator*() { return m_pPtr; }
	    /// Overloaded -> operator returns the wrapped object pointer
	    const T* operator->() const{ return m_pPtr; }
	    /// Overloaded -> operator returns the wrapped object pointer
	    T* operator->() { return m_pPtr; }

		 private:
		   /// Private pointer to the wrapped class
		   T* m_pPtr;
	  };

	  // Back to the ConfigService class itself...
	  
	public:	
		// Loads a config file
		void loadConfig(const std::string& filename, const bool append=false);
		
		// Searches for a configuration property
		std::string getString(const std::string& keyName);

		// Searches for a configuration property and returns its value
		template<typename T>
		int getValue(const std::string& keyName, T& out);

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

	private:
		friend struct Mantid::Kernel::CreateUsingNew<ConfigServiceImpl>;
	
	  // Private constructors and destructor for singleton class
		ConfigServiceImpl();
		/// Private copy constructor. Prevents singleton being copied.
		ConfigServiceImpl(const ConfigServiceImpl&);
	    
		virtual ~ConfigServiceImpl();

		/// Provies a string of a default configuration
		const std::string defaultConfig() const;

		/// Writes out a fresh user properties file
		void createUserPropertiesFile() const;

    /// the POCO file config object
		WrappedObject<Poco::Util::PropertyFileConfiguration>* m_pConf;
		/// the POCO system Config Object
		WrappedObject<Poco::Util::SystemConfiguration>* m_pSysConfig;

		/// Convert any relative paths to absolute ones and store them locally so that
		/// if the working directory is altered the paths will not be affected
		void convertRelativePaths();

    /// static reference to the logger class
	  Logger& g_log;

	  /// A list of keys that may contain relative paths and need to be altered
	  std::vector<std::string> m_vConfigPaths;
	  
	  /// Local storage for the relative path key/values that have been changed 
	  std::map<std::string, std::string> m_mAbsolutePaths;

    /// The directory that is considered to be the base directory
    std::string m_strBaseDir;

		///The configuration properties in string format
		std::string m_PropertyString;
		
    /// The filename of the Mantid properties file
    const std::string m_properties_file_name;
    /// The filename of the Mantid user properties file
    const std::string m_user_properties_file_name;
	};
	
	///Forward declaration of a specialisation of SingletonHolder for AlgorithmFactoryImpl (needed for dllexport/dllimport) and a typedef for it.
	template class EXPORT_OPT_MANTID_KERNEL Mantid::Kernel::SingletonHolder<ConfigServiceImpl>;
	typedef EXPORT_OPT_MANTID_KERNEL Mantid::Kernel::SingletonHolder<ConfigServiceImpl> ConfigService;

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_CONFIGSERVICE_H_*/
