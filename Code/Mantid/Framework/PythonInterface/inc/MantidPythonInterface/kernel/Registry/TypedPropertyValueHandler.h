#ifndef MANTID_PYTHONINTERFACE_TYPEDPROPERTYVALUEHANDLER_H_
#define MANTID_PYTHONINTERFACE_TYPEDPROPERTYVALUEHANDLER_H_
/**
    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include "MantidPythonInterface/kernel/Registry/PropertyValueHandler.h"
#include "MantidPythonInterface/kernel/Registry/DowncastRegistry.h"

#include "MantidPythonInterface/kernel/IsNone.h" // includes object.hpp
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/IPropertyManager.h"

#include <boost/python/converter/arg_from_python.hpp>
#include <boost/python/call_method.hpp>
#include <string>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace Registry
    {
      /**
       * This class provides a templated class object that is able to take a 
       * python object and perform operations with a given C type.
       */
      template<typename ValueType>
      struct DLLExport TypedPropertyValueHandler : public PropertyValueHandler
      {
        /// Type required by TypeRegistry framework
        typedef ValueType HeldType;

        /**
         * Set function to handle Python -> C++ calls and get the correct type
         * @param alg :: A pointer to an IPropertyManager
         * @param name :: The name of the property
         * @param value :: A boost python object that stores the value
         */
        void set(Kernel::IPropertyManager* alg, const std::string &name, const boost::python::object & value) const
        {
          alg->setProperty<ValueType>(name, boost::python::extract<ValueType>(value));
        }
        /**
         * Create a PropertyWithValue from the given python object value
         * @param name :: The name of the property
         * @param defaultValue :: The defaultValue of the property. The object attempts to extract
         * a value of type ValueType from the python object
         * @param validator :: A python object pointing to a validator instance, which can be None.
         * @param direction :: The direction of the property
         * @returns A pointer to a newly constructed property instance
         */
        Kernel::Property * create(const std::string & name, const boost::python::object & defaultValue, 
                                  const boost::python::object & validator, const unsigned int direction) const
        {
          using boost::python::extract;
          const ValueType valueInC = extract<ValueType>(defaultValue)();
          Kernel::Property *valueProp(NULL);
          if( isNone(validator) )
          {
            valueProp = new Kernel::PropertyWithValue<ValueType>(name, valueInC, direction);
          }
          else
          {
            const Kernel::IValidator *propValidator = extract<Kernel::IValidator*>(validator);
            valueProp = new Kernel::PropertyWithValue<ValueType>(name, valueInC, propValidator->clone(), direction);
          }
          return valueProp;
        }

      };

      //
      // Specialization for shared_ptr types that can be set via weak pointers
      //
      template<typename T>
      struct DLLExport TypedPropertyValueHandler<boost::shared_ptr<T> > : public PropertyValueHandler
      {
        /// Type required by TypeRegistry framework
        typedef boost::shared_ptr<T> HeldType;

        /// Convenience typedef
        typedef T PointeeType;
        /// Convenience typedef
        typedef boost::shared_ptr<T> PropertyValueType;

        /**
         * Set function to handle Python -> C++ calls and get the correct type
         * @param alg :: A pointer to an IPropertyManager
         * @param name :: The name of the property
         * @param value :: A boost python object that stores the value
         */
        void set(Kernel::IPropertyManager* alg, const std::string &name, const boost::python::object & value) const
        {
          using namespace boost::python;
          using Registry::DowncastRegistry;

          const auto & entry = DowncastRegistry::retrieve(call_method<std::string>(value.ptr(), "id"));
          alg->setProperty<HeldType>(name, boost::dynamic_pointer_cast<T>(entry.fromPythonAsSharedPtr(value)));
        }

        /**
         * Create a PropertyWithValue from the given python object value
         * @param name :: The name of the property
         * @param defaultValue :: The defaultValue of the property. The object attempts to extract
         * a value of type ValueType from the python object
         * @param validator :: A python object pointing to a validator instance, which can be None.
         * @param direction :: The direction of the property
         * @returns A pointer to a newly constructed property instance
         */
        Kernel::Property * create(const std::string & name, const boost::python::object & defaultValue,
                                  const boost::python::object & validator, const unsigned int direction) const
        {
          using boost::python::extract;
          const PropertyValueType valueInC = extract<PropertyValueType>(defaultValue)();
          Kernel::Property *valueProp(NULL);
          if( isNone(validator) )
          {
            valueProp = new Kernel::PropertyWithValue<PropertyValueType>(name, valueInC, direction);
          }
          else
          {
            const Kernel::IValidator *propValidator = extract<Kernel::IValidator*>(validator);
            valueProp = new Kernel::PropertyWithValue<PropertyValueType>(name, valueInC, propValidator->clone(), direction);
          }
          return valueProp;
        }
      };

    }
  }
}

#endif /* MANTID_PYTHONINTERFACE_TYPEDPROPERTYVALUEHANDLER_H_ */
