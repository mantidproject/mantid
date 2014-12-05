#ifndef MANTID_PYTHONINTERFACE_MATRIXTONDARRAY_H_
#define MANTID_PYTHONINTERFACE_MATRIXTONDARRAY_H_
/**
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
#include "MantidKernel/System.h"
#include "MantidKernel/Matrix.h"
#include "MantidPythonInterface/kernel/Converters/WrapWithNumpy.h"
#include <boost/python/detail/prefix.hpp>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace Converters
    {
      //-----------------------------------------------------------------------
      // Converter implementation
      //-----------------------------------------------------------------------
      /**
       * Converter that takes a Mantid Matrix and converts it into a
       * numpy array.
       *
       * The type of conversion is specified by another struct/class that
       * contains a static member create.
       */
      template<typename ElementType, typename ConversionPolicy>
      struct DLLExport MatrixToNDArray
      {
        /**
         * Operator to convert a matrix to a numpy array
         * @param cmatrix :: A reference to matrix
         * @returns A new PyObject* that points to a numpy array
         */
        inline PyObject * operator()(const Kernel::Matrix<ElementType> & cmatrix) const
        {
          const std::pair<size_t,size_t> matrixDims = cmatrix.size();
          Py_intptr_t dims[2] = { static_cast<Py_intptr_t>(matrixDims.first), static_cast<Py_intptr_t>(matrixDims.second) };
          typedef typename ConversionPolicy::template apply<ElementType> policy;
          return policy::createFromArray(&(cmatrix[0][0]), 2, dims);
        }
      };

    }
  }
}

#endif /// MANTID_PYTHONINTERFACE_MATRIXTONDARRAY_H_
