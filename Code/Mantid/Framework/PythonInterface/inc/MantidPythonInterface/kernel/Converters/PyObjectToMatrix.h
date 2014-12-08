#ifndef MANTID_PYTHONINERFACE_PYOBJECTTOMATRIX_H_
#define MANTID_PYTHONINERFACE_PYOBJECTTOMATRIX_H_
/**
Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include "MantidKernel/System.h"
#include "MantidKernel/Matrix.h"
#include <boost/python/object.hpp>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace Converters
    {
      /**
       * Takes a Python object and if it supports
       * indexing and is two dimensional it attempts to
       * convert it to a Kernel::Matrix object. Note, this
       * currently only suuports Matrix<double>
       */
      struct DLLExport PyObjectToMatrix
      {
        PyObjectToMatrix(const boost::python::object & p);
        /// Produces a V3D object from the given PyObject
        Kernel::Matrix<double> operator()();
      private:
        /// A reference to the object
        const boost::python::object & m_obj;
        /// Is the object a wrapped instance of Matrix<double>
        bool m_alreadyMatrix;
      };
    }

  }
}

#endif /* MANTID_PYTHONINERFACE_PYOBJECTTOMATRIX_H_ */
