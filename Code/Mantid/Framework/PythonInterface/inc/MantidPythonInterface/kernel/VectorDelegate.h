#ifndef MANTID_PYTHONINTERFACE_VECTORDELEGATE_H_
#define MANTID_PYTHONINTERFACE_VECTORDELEGATE_H_
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
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/System.h"

#include <boost/python/object.hpp>

#include <string>
#include <vector>

//-----------------------------------------------------------------------------
// Forward declaration
//-----------------------------------------------------------------------------
struct PyArrayObject;

namespace Mantid
{
  namespace PythonInterface
  {
    namespace VectorDelegate
    {
      /// Check the object contains a type that is some container of values
      DLLExport std::string isSequenceType(PyObject* value);
      /// Convert a Python type into a C++ std vector using the template element type
      template<typename ElementType>
      DLLExport const std::vector<ElementType> toStdVector(PyObject *value);
      /// Convert a numpy array to a std::vector
      template<typename VectorElementType, typename NumpyType>
      DLLExport const std::vector<VectorElementType> toStdVectorFromNumpy(PyArrayObject *value);
    }
  }
}

#endif /* MANTID_PYTHONINTERFACE_VECTORDELEGATE_H_ */
