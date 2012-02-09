#ifndef MANTID_PYTHONINTERFACE_TYPEREGISTRY_H_
#define MANTID_PYTHONINTERFACE_TYPEREGISTRY_H_
/**
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

namespace Mantid
{
  namespace PythonInterface
  {
    /**
     * This namespace stores a lookup between
     * a PyTypeObject and a C++ TypeHandler
     */
    namespace TypeRegistry
    {
      //-----------------------------------------------------------------------
      // Forward declarations
      //-----------------------------------------------------------------------
      struct PythonTypeHandler;

      /// Inserts a new property handler
      DLLExport void registerHandler(PyTypeObject* typeObject, PythonTypeHandler* handler);
      /// Get a TypeHandler, throws if one does not exist
      DLLExport PythonTypeHandler *getHandler(PyTypeObject* typeObject);
      /// Get an upcasted type object for the given object
      DLLExport PyTypeObject * getDerivedType(boost::python::object value);
    }
  }
}


#endif /* MANTID_PYTHONINTERFACE_TYPEREGISTRY_H_*/
