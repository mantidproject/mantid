#ifndef MANTID_PYTHONINTERFACE_SEQUENCETYPEHANDLER_H_
#define MANTID_PYTHONINTERFACE_SEQUENCETYPEHANDLER_H_
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
#include "MantidPythonInterface/kernel/Registry/TypedPropertyValueHandler.h"

namespace Mantid
{
  namespace PythonInterface
  {
    namespace Registry
    {
      /**
       * A specialisation of PropertyValueHander to handle coercing a Python
       * value into a C++ sequence/array property. The template type ContainerType
       * should contain a type called value_type indicating the element type.
       */
      template<typename ContainerType>
      struct DLLExport SequenceTypeHandler : TypedPropertyValueHandler<ContainerType>
      {

        /// Call to set a named property where the value is some container type
        void set(Kernel::IPropertyManager* alg, const std::string &name,
                 const boost::python::object & value) const;

        /// Call to create a name property where the value is some container type
        Kernel::Property *create(const std::string &name, const boost::python::object &defaultValue,
                                 const boost::python::object &validator, const unsigned int direction) const;
//        /**
//         * Return the PyTypeObject of the DerivedType
//         * @returns A PyTypeObject for the given DerivedType
//         */
//        const PyTypeObject * pythonType() const
//        {
//          return &PyList_Type;
//        }
       };

    }
  }
}

#endif /* MANTID_PYTHONINTERFACE_SEQUENCETYPEHANDLER_H_ */
