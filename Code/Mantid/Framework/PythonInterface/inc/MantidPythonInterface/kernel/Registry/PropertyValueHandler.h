#ifndef MANTID_PYTHONINTERFACE_PROPERTYVALUEHANDLER_H_
#define MANTID_PYTHONINTERFACE_PROPERTYVALUEHANDLER_H_
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
#include "MantidKernel/System.h"
#include <boost/python/object.hpp>
#include <string>

namespace Mantid
{
  namespace Kernel
  {
    // Forward declarations
    class IPropertyManager;
  }
  namespace PythonInterface
  {
      namespace Registry
      {
      /**
       * This class provides a base-class objects that are able to take
       * a python object and set it on an algorithm property.
       *
       * The set function should call the setProperty method once it has the
       * correct C++ type from the Python object
       */
      struct DLLExport PropertyValueHandler
      {
        /// Virtual Destructor
        virtual ~PropertyValueHandler() {};
        /// Set function to handle Python -> C++ calls
        virtual void set(Kernel::IPropertyManager* alg, const std::string &name, boost::python::object value) = 0;
        /// Is the given object a derived type of this objects Type
        virtual bool isDerivedType(const boost::python::object & value) const = 0;
        /// Return the Python type corresponding to this object. May return NULL
        virtual const PyTypeObject * pythonType() const = 0;
      };
    }
  }
}

#endif /* MANTID_PYTHONINTERFACE_PROPERTYVALUEHANDLER_H_ */
