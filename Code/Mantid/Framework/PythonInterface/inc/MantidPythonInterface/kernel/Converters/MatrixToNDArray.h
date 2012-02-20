#ifndef MANTID_PYTHONINTERFACE_MATRIXTONDARRAY_H_
#define MANTID_PYTHONINTERFACE_MATRIXTONDARRAY_H_
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
#include "MantidKernel/Matrix.h"
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
        inline PyObject * operator()(const Kernel::Matrix<ElementType> & cmatrix) const
        {
          typedef typename ConversionPolicy::template apply<ElementType> policy;
          return policy::create(cmatrix);
        }
      };

      //-----------------------------------------------------------------------
      // Conversion Policies
      //-----------------------------------------------------------------------
      namespace Impl
      {
        enum WrapMode { ReadOnly, ReadWrite };
        /**
         * Helper functions to keep the numpy arrayobject header out
         * the header file
         */
        /// Wrap a numpy array around existing data
        template<typename ElementType>
        PyObject *wrapWithNDArray(const Kernel::Matrix<ElementType>&, const WrapMode);
      }

      /**
       * WrapReadOnly is a policy for VectorToNDArray
       * to wrap the vector in a read-only numpy array
       * that looks at the original data. No copy is performed
       */
      struct WrapReadOnly
      {
        template<typename ElementType>
        struct apply
        {
          /**
           * Returns a read-only Numpy array wrapped around an existing vector
           * @param cvector
           * @return
           */
          static PyObject * create(const Kernel::Matrix<ElementType> & cmatrix)
          {
            return Impl::wrapWithNDArray(cmatrix, Impl::ReadOnly);
          }
        };
      };

      /**
       * WrapReadWrite is a policy for MatrixToNDArray
       * to wrap the vector in a read-write numpy array
       * that looks at the original data. No copy is performed
       */
      struct WrapReadWrite
      {
        template<typename ElementType>
        struct apply
        {
          /**
           * Returns a read-write Numpy array wrapped around an existing vector
           * @param cvector
           * @return
           */
          static PyObject * create(const Kernel::Matrix<ElementType> & cmatrix)
          {
            return Impl::wrapWithNDArray(cmatrix, Impl::ReadWrite);
          }
        };
      };

    }
  }
}

#endif /// MANTID_PYTHONINTERFACE_MATRIXTONDARRAY_H_
