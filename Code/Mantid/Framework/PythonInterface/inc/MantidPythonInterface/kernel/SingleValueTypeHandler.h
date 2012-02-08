/*
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
#include "MantidPythonInterface/kernel/PythonTypeHandler.h"
#include "MantidPythonInterface/kernel/PropertyMarshal.h"
#include "MantidPythonInterface/kernel/TypeRegistry.h"
#include "MantidKernel/IPropertyManager.h"

#if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#include <boost/python/extract.hpp>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace TypeRegistry
    {

      /**
       * A templated handler that calls the appropriate setProperty method for the type on the given PropertyManager
       * A new typedhandler should be inserted into the type look up map with the
       * DECLARE_SINGLEVALUETYPEHANDLER macro whenever a new class is exported that will be used with PropertyWithValue
       */
      template<typename BaseType, typename DerivedType=BaseType>
      struct DLLExport SingleValueTypeHandler : public PythonTypeHandler
      {
        /**
         * Set function to handle Python -> C++ calls and get the correct type
         * @param alg :: A pointer to an IPropertyManager
         * @param name :: The name of the property
         * @param value :: A boost python object that stores the value
         */
        void set(Kernel::IPropertyManager* alg, const std::string &name, boost::python::object value)
        {
          alg->setProperty<BaseType>(name, boost::python::extract<BaseType>(value));
        }
        /**
         * Is the python object an instance of the Derived template type
         * @param value ::
         * @returns True if it is, false otherwise
         */
        bool isInstance(const boost::python::object & value) const
        {
          // Can we extract the derived type from the object?
          boost::python::extract<DerivedType> extractor(value);
          return extractor.check();
        }
      };

      /**
       * Specialized string version to avoid a current bug where string property values are not
       * assigned polymorphically. This can be removed when the bug is fixed
       */
      template<>
      struct DLLExport SingleValueTypeHandler<std::string> : public PythonTypeHandler
      {
        /**
         * Set function to handle Python -> C++ calls and get the correct type
         * @param alg :: A pointer to an IPropertyManager
         * @param name :: The name of the property
         * @param value :: A boost python object that stores the value
         */
        void set(Kernel::IPropertyManager* alg, const std::string &name, boost::python::object value)
        {
          alg->setPropertyValue(name, boost::python::extract<std::string>(value));
        }
        /**
         * Is the python object an instance of the string template type
         * @param value ::
         * @returns True if it is, false otherwise
         */
        bool isInstance(const boost::python::object & value) const
        {
          // Can we extract the derived type from the object?
          boost::python::extract<std::string> extractor(value);
          return extractor.check();
        }

      };

      /**
       * Specialized integer version to deal with situations where a property
       * is of type double but an integer is passed.
       */
      template<>
      struct DLLExport SingleValueTypeHandler<int> : public PythonTypeHandler
      {
        /**
         * Set function to handle Python -> C++ calls and get the correct type
         * @param alg :: A pointer to an IPropertyManager
         * @param name :: The name of the property
         * @param value :: A boost python object that stores the integer value
         */
        void set(Kernel::IPropertyManager* alg, const std::string &name, boost::python::object value)
        {
          int intValue = boost::python::extract<int>(value)();
          try
          {
            alg->setProperty(name, intValue);
          }
          catch(std::invalid_argument&)
          {
            // Throws this also if the type is wrong. The type could be a double so check first and extract as a double if necessary
            alg->setProperty(name, static_cast<double>(intValue));
          }
        }
        /**
         * Is the python object an instance of the string template type
         * @param value ::
         * @returns True if it is, false otherwise
         */
        bool isInstance(const boost::python::object & value) const
        {
          // Can we extract the derived type from the object?
          boost::python::extract<int> extractor(value);
          return extractor.check();
        }
      };
    }
  }
}

/**
  * A macro to declare typed handlers.
  * @param export_type :: The C++ type that is to be converted
  * @param base_type :: The C++ type that the export_type is to be treated as
  */
#define DECLARE_SINGLEVALUETYPEHANDLER(export_type, base_type) \
  const boost::python::converter::registration *reg = boost::python::converter::registry::query(typeid(export_type));\
  Mantid::PythonInterface::TypeRegistry::registerHandler(reg->get_class_object(), new Mantid::PythonInterface::TypeRegistry::SingleValueTypeHandler<base_type, export_type>());
