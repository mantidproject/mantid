#ifndef MANTIDPYTHONAPI_KERNEL_EXPORTS_H_
#define MANTIDPYTHONAPI_KERNEL_EXPORTS_H_

#include <MantidKernel/Exception.h>
#include <MantidKernel/ConfigService.h>
#include <MantidKernel/FacilityInfo.h>

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
      * Retrieve a setting from the ConfigService
      * @param name The name of the property
      * @returns The current value of the property
      */
      std::string getProperty(const std::string & name) const
      {
        return Mantid::Kernel::ConfigService::Instance().getString(name);
      }

      /**
      * Update a setting in the ConfigService
      * @param name The name of the property
      * @param value The new value of the property
      */
      void setProperty(const std::string & name, const std::string & value)
      {
        Mantid::Kernel::ConfigService::Instance().setString(name, value);
      }

      /**
       * Access the facilities information
       * @param name The name of a facility. If empty the default facility is returned. (default = "")
       * @return the facility information object
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
    };

  }
}

#endif //MANTIDPYTHONAPI_KERNEL_EXPORTS_H_
