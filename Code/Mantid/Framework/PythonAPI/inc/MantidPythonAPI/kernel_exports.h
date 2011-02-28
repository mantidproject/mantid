#ifndef MANTIDPYTHONAPI_KERNEL_EXPORTS_H_
#define MANTIDPYTHONAPI_KERNEL_EXPORTS_H_

#include "MantidPythonAPI/PythonInterfaceFunctions.h"

#include "MantidKernel/Exception.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"


namespace Mantid
{
  namespace PythonAPI
  {

    /**
    * A wrapper for the ConfigService Singleton
    *
    * Simple forwards calls onto the real config service. Inheritance cannot be used 
    * as the constructor is private
    */
    struct ConfigServiceWrapper
    {
      /** Get the welcome message
       * @returns The welcome message
       */
      std::string welcomeMessage() const
      {
        return Mantid::welcomeMessage();
      }
      
      /**
       * Access the facilities information
       * @param name :: The name of a facility. If empty the default facility is returned. (default = "")
       * @returns The facility information object
       */
      Kernel::FacilityInfo facility(const std::string & name = "")
      {
        if( name.empty() )
        {
          return Mantid::Kernel::ConfigService::Instance().Facility();
        }
        else
        {
          try
          {
            return Mantid::Kernel::ConfigService::Instance().Facility(name);
          }
          catch(Kernel::Exception::NotFoundError&)
          {
            // Throw a better error to Python users
            throw std::runtime_error("Facility \"" + name + "\" not defined.") ;
          }
        }
      }

      /**
      * Retrieve a setting from the ConfigService
      * @param name :: The name of the property
      * @returns The current value of the property
      */
      std::string getString(const std::string & name) const
      {
        return Mantid::Kernel::ConfigService::Instance().getString(name);
      }

      /**
      * Update a setting in the ConfigService
      * @param name :: The name of the property
      * @param value :: The new value of the property
      */
      void setString(const std::string & name, const std::string & value)
      {
        Mantid::Kernel::ConfigService::Instance().setString(name, value);
      }

      /** @name Special handling for search path manipulation. */
      //@{
      
      /**
       * Get the list of data search directories
       * @returns A list of the data search paths
       */
      boost::python::list getDataSearchDirs() const
      {
	const std::vector<std::string> & paths = Mantid::Kernel::ConfigService::Instance().getDataSearchDirs();
	return Conversions::toPyList(paths);
      }

      /**
       * Replace the current list of data search paths with the given ones
       * @param value :: A semi-colon separated list of paths
       */
      void setDataSearchDirs(const std::string &value)
      {
      	Mantid::Kernel::ConfigService::Instance().setDataSearchDirs(value);
      }

      /**
       * Replace the current list of data search paths with the given ones
       * @param value :: A semi-colon separated list of paths
       */
      void setDataSearchDirs(const boost::python::list &values)
      {
      	Mantid::Kernel::ConfigService::Instance().setDataSearchDirs(Conversions::toStdVector<std::string>(values));
      }      

      /** 
       * Adds the passed path to the end of the list of data search paths
       * @param path :: A path to append
       */
      void appendDataSearchDir(const std::string & path)
      {
	Mantid::Kernel::ConfigService::Instance().appendDataSearchDir(path);
      }
      
      /// Get instrument search directory
      const std::string getInstrumentDirectory() const
      {
	return Mantid::Kernel::ConfigService::Instance().getInstrumentDirectory();
      }
      //@}

      /** 
       * Return the user properties filename
       * @returns A string containing the filename of the user properties file
       */
      std::string getUserFilename() const
      {
	return Mantid::Kernel::ConfigService::Instance().getUserFilename();
      }

      /**
       * Saves and properties changed from the default to the given file
       * @param A filename to write the settings to
       */
      void saveConfig(const std::string &filename) const
      {
	Mantid::Kernel::ConfigService::Instance().saveConfig(filename);
      }

    };

  }
}

#endif //MANTIDPYTHONAPI_KERNEL_EXPORTS_H_
