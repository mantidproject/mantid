#ifndef MANTID_PYTHONINTERFACE_NUMPYTYPEHANDLER_H_
#define MANTID_PYTHONINTERFACE_NUMPYTYPEHANDLER_H_
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
#include "MantidPythonInterface/kernel/PropertyHandler.h"

//-----------------------------------------------------------------------------
// Forward delcarations
//-----------------------------------------------------------------------------
struct PyArrayObject;

namespace Mantid
{
  namespace PythonInterface
  {
    namespace TypeRegistry
    {
      /**
       * A property handler that deals with translation of numpy arrays
       * etc to/from Mantid algorithm properties
       */
      struct DLLExport NumpyTypeHandler : PythonTypeHandler
      {
        /// Call to set a named property where the value is some container type
        virtual void set(Kernel::IPropertyManager* alg, const std::string &name, boost::python::object value);
        /// Is the given object an instance the handler's type
        virtual bool isInstance(const boost::python::object&) const;
      private:
        /// Handle double-type properties
        void setDoubleArrayProperty(Kernel::IPropertyManager* alg, const std::string &name, PyArrayObject * nparray);
        /// Handle int-type properties
        void setIntNumpyProperty(Kernel::IPropertyManager* alg, const std::string &name,
            const std::type_info & typeInfo, PyArrayObject * nparray);
        /// Handle string property types. Needs to be merged with the vector delegate stuff
        void setStringArrayProperty(Kernel::IPropertyManager* alg, const std::string &name, PyArrayObject * nparray);
      };

    }
  }
}

#endif /* MANTID_PYTHONINTERFACE_NUMPYTYPEHANDLER_H_ */
