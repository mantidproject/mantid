#ifndef MANTID_CONFIGSVC_H_
#define MANTID_CONFIGSVC_H_
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "System.h"
#include <string>


//forward declaration
namespace Poco
{
	namespace Util
	{
		class PropertyFileConfiguration;
		class SystemConfiguration;
	}
}

namespace Mantid
{
/** @class ConfigSvc ConfigSvc.h Kernel/ConfigSvc.h

    The ConfigSvc class provides a simple facade to access the Configuration functionality of the Mantid Framework.
	The class gathers information from config files and the system varaibles.  
	This information is available to all the objects within the framework as well as being used to configure the logging framework.
	This class currently uses the Logging functionality provided through the POCO (portable components library).
    
    @author Nicholas Draper, Tessella Support Services plc
    @date 15/10/2007
    
    Copyright &copy; 2007 ???RAL???

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
*/
	class DLLExport ConfigSvc
	{
		 /// Inner templated class to wrap the poco library objects that have protected desctructors and expose them as public
		 template<typename T >
		 class WrappedObject : public T
		 {
		 public:
			   ///The template type of class that is being wrapped
			   typedef T element_type;
			   ///simple constructor
			   WrappedObject()
			   {
				   m_pPtr = static_cast<T*>(this);
			   }

			   ///consturctor with a class to wrap
			   /// @param Field the class to wrap
			   template<typename Field>
			   WrappedObject(Field& F) : T(F)
			   {
				   m_pPtr = static_cast<T*>(this);
			   }

			   ///copy constructor
			   WrappedObject(const WrappedObject<T>& A) : T(A)
			   {
					m_pPtr = static_cast<T*>(this);
			   }
		       
			   ///virtual destructor
			   virtual ~WrappedObject()
			   {}
		       
			   ///overloaded * operator returns the wrapped object pointer
			   const T& operator*() const { return *m_pPtr; }
			 
			   ///overloaded * operator returns the wrapped object pointer
			   T& operator*() { return m_pPtr; }

			   ///overloaded -> operator returns the wrapped object pointer
			   const T* operator->() const{ return m_pPtr; }
			   ///overloaded -> operator returns the wrapped object pointer
			   T* operator->() { return m_pPtr; }

		 private:
			   /// private pointer to the wrapped class
			   T* m_pPtr;
		 };


	public:	
		/// A static method which retrieves the single instance of the ConfigSvc
		///
		/// @returns A pointer to the instance
		static ConfigSvc* Instance();

		/// Loads the config file provided, any previous configuration is discarded.
		/// If the file contains logging setup instructions then these will be used to setup the logging framework.
		///
		/// @param filename The filename and optionally path of the file to load
		void loadConfig(const std::string& filename);
		
		/// Searches for the string within the currently loaded configuaration values and returns to value as a string.
		///
		/// @param keyName The case sensitive name of the property that you need the value of.
		/// @returns the string value of the property
		std::string getString(const std::string& keyName);

		/// Searches for the string within the currently loaded configuaration values and attempts to convert the values to the template type supplied.
		///
		/// @param keyName The case sensitive name of the property that you need the value of.
		/// @param out The value if found
		/// @returns A success flag - 0 on failure 1 on success
		template<typename T>
		int getValue(const std::string& keyName, T& out);

		/// Searches for the string within the environment variables and returns to value as a string.
		///
		/// @param keyName The name of the environment variable that you need the value of.
		/// @returns the string value of the property
		std::string getEnvironment(const std::string& keyName);

		/// Gets the name of the operation system
		///
		/// @returns the string value of the operation system version
		std::string getOSName();

		/// Gets the name of the computer running Mantid
		///
		/// @returns the string value of the name of the computer
		std::string getComputerName();
			
		/// Gets the name of the operation system Architecture
		///
		/// @returns the string value of the operation system Architecture
		std::string getOSArchitecture();

/*		Removed as the use of these throughs a debug assertion about an invlid heap pointer
		File dbgheap.c
		Expression _CrtIsValidHeapPointer(pUserData)

		/// Gets the name of the operation system version
		///
		/// @returns the string value of the operation system version
		std::string getOSVersion();	

		/// Gets the path of the current directory
		///
		/// @returns the string value of the path of the current directory
		std::string getCurrentDir();
		
		/// Gets the path of the home directory
		///
		/// @returns the string value of the path of the home directory
		std::string getHomeDir();
		
		/// Gets the path of the temp directory
		///
		/// @returns the string value of the path of the temp directory
		std::string getTempDir();
*/

		/// Destructor
		/// Prevents client from calling 'delete' on the pointer handed 
		/// out by Instance
		virtual ~ConfigSvc();
	private:
		/// Private Constructor for singleton class
		ConfigSvc();
	    
		/// Private copy constructor
		/// Prevents singleton being copied
		ConfigSvc(const ConfigSvc&) {}
	    
		/// the POCO file config object
		WrappedObject<Poco::Util::PropertyFileConfiguration>* m_pConf;
		/// the POCO system Config Object
		WrappedObject<Poco::Util::SystemConfiguration>* m_pSysConfig;

		/// Pointer to the factory instance
		static ConfigSvc* m_instance;

	};

} // namespace Mantid

#endif /*MANTID_CONFIGSVC_H_*/
