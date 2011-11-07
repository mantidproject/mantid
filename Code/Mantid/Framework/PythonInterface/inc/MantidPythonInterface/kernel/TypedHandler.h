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
#include "MantidPythonInterface/kernel/PropertyMarshal.h"

namespace Mantid
{
  namespace PythonInterface
  {
    namespace PropertyMarshal
    {

      /**
       * A templated handler that calls the appropriate setProperty method for the type on the given PropertyManager
       * A new typedhandler should be inserted into the type look up map with the
       * DECLARE_TYPEHANDLER macro whenever a new class is exported that will be used with PropertyWithValue
       */
      template<typename BaseType, typename DerivedType=BaseType>
      struct DLLExport TypedHandler : public PropertyHandler
      {
        /// Set function to handle Python -> C++ calls and get the correct type
        void set(Kernel::IPropertyManager* alg, const std::string &name, boost::python::object value)
        {
          alg->setProperty<BaseType>(name, boost::python::extract<BaseType>(value));
        }
        /// Return a reference tp the type_info of the derived type
        const std::type_info & typeInfo() const
        {
          return typeid(DerivedType);
        }
      };

      /**
       * Specialized string version to avoid a current bug where string property values are not
       * assigned polymorphically. This can be removed when the bug is fixed
       */
      template<>
      struct DLLExport TypedHandler<std::string> : public PropertyHandler
      {
        /// Set function to handle Python -> C++ calls and get the correct type
        void set(Kernel::IPropertyManager* alg, const std::string &name, boost::python::object value)
        {
          alg->setPropertyValue(name, boost::python::extract<std::string>(value));
        }
        /// Return a reference to the type_info of the derived type
        const std::type_info & typeInfo() const
        {
          return typeid(std::string);
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
#define DECLARE_TYPEHANDLER(export_type, base_type) \
  const boost::python::converter::registration *reg = boost::python::converter::registry::query(typeid(export_type));\
  Mantid::PythonInterface::PropertyMarshal::registerHandler(reg->get_class_object(), new Mantid::PythonInterface::PropertyMarshal::TypedHandler<base_type, export_type>());
