#ifndef MANTID_PYTHONINTERFACE_TYPEDPROPERTYVALUEHANDLER_H_
#define MANTID_PYTHONINTERFACE_TYPEDPROPERTYVALUEHANDLER_H_
/**
    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include "MantidPythonInterface/kernel/Registry/PropertyValueHandler.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/IPropertyManager.h"
#include <boost/python/object.hpp>
#include <boost/python/extract.hpp>
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
        /**
         * Set function to handle Python -> C++ calls and get the correct type
         * @param alg :: A pointer to an IPropertyManager
         * @param name :: The name of the property
         * @param value :: A boost python object that stores the value
         */
        void set(Kernel::IPropertyManager* alg, const std::string &name, const boost::python::object & value)
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
          if( validator.is_none() )
          {
            valueProp = new Kernel::PropertyWithValue<ValueType>(name, valueInC, direction);
          }
          else
          {
            const Kernel::IValidator<ValueType> * propValidator = extract<Kernel::IValidator<ValueType> *>(validator);
            valueProp = new Kernel::PropertyWithValue<ValueType>(name, valueInC, propValidator->clone(), direction);
          }
          return valueProp;
        }
        /// Is the given object a derived type of this objects Type
        bool checkExtract(const boost::python::object & value) const
        {
          boost::python::extract<ValueType> extractor(value);
          return extractor.check();
        }
      };
    }
  }
}

#endif /* MANTID_PYTHONINTERFACE_TYPEDPROPERTYVALUEHANDLER_H_ */
