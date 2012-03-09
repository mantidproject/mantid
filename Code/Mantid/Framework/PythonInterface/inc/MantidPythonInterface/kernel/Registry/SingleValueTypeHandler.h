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
#include "MantidPythonInterface/kernel/Registry/TypedPropertyValueHandler.h"
#include "MantidKernel/IPropertyManager.h"

#if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#include <boost/python/extract.hpp>
#include <boost/type_traits/integral_constant.hpp>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace Registry
    {
      namespace
      {
        /// MPL struct to pick out shared_ptr<T> types. General one inherits from false
        template <class T>
        struct is_shared_ptr : boost::false_type {};
        /// Specialization of MPL struct to inherit from true for shared_ptr<T> types
        template <class T>
        struct is_shared_ptr<boost::shared_ptr<T> > : boost::true_type {};
      }

      /**
       * Templated struct to handle property types whose values
       * are a single item .e.g. int, Workspace_sptr etc.
       *
       * @tparam PropertyType :: The PropertyWithValue<> template type
       */
      template<typename PropertyType>
      struct DLLExport SingleValueTypeHandler : public TypedPropertyValueHandler<PropertyType>
      {
        /**
         * Return the PyTypeObject of the DerivedType
         * @returns A PyTypeObject for the given DerivedType
         */
        const PyTypeObject * pythonType() const
        {
          using namespace boost::python;
          const std::type_info & ctype = typeID<PropertyType>(is_shared_ptr<PropertyType>());
          const converter::registration *regEntry = converter::registry::query(ctype);
          return regEntry->get_class_object();
        }
      private:
        /**
         * Return the PyTypeObject of the DerivedType
         * @returns A PyTypeObject for the given DerivedType
         */
        template<typename T>
        const std::type_info& typeID(const boost::true_type &) const
        {
          return typeid(typename T::element_type);
        }
        /**
         * Return the PyTypeObject of the DerivedType
         * @returns A PyTypeObject for the given DerivedType
         */
        template<typename T>
        const std::type_info& typeID(const boost::false_type &) const
        {
          return typeid(T);
        }

      };

    }
  }
}
