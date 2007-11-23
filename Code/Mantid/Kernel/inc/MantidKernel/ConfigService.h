#ifndef MANTID_KERNEL_CONFIGSERVICE_H_
#define MANTID_KERNEL_CONFIGSERVICE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>
#include <string>

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
/** @class ConfigService ConfigService.h Kernel/ConfigService.h

    The ConfigService class provides a simple facade to access the Configuration functionality of the Mantid Framework.
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
	class DLLExport ConfigService
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
	  // Returns the single instance of the service
		static ConfigService* Instance();

		// Loads a config file
		void loadConfig(const std::string& filename);
		
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
		std::string getHomeDir();
		std::string getTempDir();

	private:
	  // Private constructors and destructor for singleton class
		ConfigService();
		/// Private copy constructor. Prevents singleton being copied.
		ConfigService(const ConfigService&) {}
	    
    virtual ~ConfigService();

    /// the POCO file config object
		WrappedObject<Poco::Util::PropertyFileConfiguration>* m_pConf;
		/// the POCO system Config Object
		WrappedObject<Poco::Util::SystemConfiguration>* m_pSysConfig;

		/// Pointer to the factory instance
		static ConfigService* m_instance;

	};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_CONFIGSERVICE_H_*/
