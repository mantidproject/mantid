#ifndef MANTID_PYTHONINTERFACE_NUMPYWRAPMODE_H_
#define MANTID_PYTHONINTERFACE_NUMPYWRAPMODE_H_
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
#include <boost/python/detail/prefix.hpp>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace Converters
    {
      /// Enum defining wrapping type for conversion to numpy
      enum NumpyWrapMode { ReadOnly, ReadWrite };

      namespace Impl
      {
        // Forward declare a conversion function. This should be specialized for each
        // container type that is to be wrapped
        template <typename ContainerType> PyObject * wrapWithNDArray(const ContainerType &, const NumpyWrapMode);
      }

      /**
       * WrapReadOnly is a policy for VectorToNDArray
       * to wrap the vector in a read-only numpy array
       * that looks at the original data. No copy is performed
       */
      struct WrapReadOnly
      {
        template<typename ContainerType>
        struct apply
        {
          /**
           * Returns a read-only Numpy array wrapped around an existing vector
           * @param cdata ::
           * @return
           */
          static PyObject * create(const ContainerType & cdata)
          {
            return Impl::wrapWithNDArray(cdata, ReadOnly);
          }
        };
      };

      /**
       * WrapReadWrite is a policy for VectorToNDArray
       * to wrap the vector in a read-write numpy array
       * that looks at the original data. No copy is performed
       */
      struct WrapReadWrite
      {
        template<typename ContainerType>
        struct apply
        {
          /**
           * Returns a read-write Numpy array wrapped around an existing vector
           * @param cvector
           * @return
           */
          static PyObject * create(const ContainerType & cdata)
          {
            return Impl::wrapWithNDArray(cdata, ReadWrite);
          }
        };
      };

    }
  }
}

#endif
