#ifndef MANTID_PYTHONINTERFACE_VECTORTONDARRAY_H_
#define MANTID_PYTHONINTERFACE_VECTORTONDARRAY_H_
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
#include "MantidPythonInterface/kernel/Converters/NumpyWrapMode.h"
#include <vector>

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
       * Converter that takes a std::vector and converts it into a
       * numpy array.
       *
       * The type of conversion is specified by another struct/class that
       * contains a static member create.
       */
      template<typename ElementType, typename ConversionPolicy>
      struct VectorToNDArray
      {
        inline PyObject * operator()(const std::vector<ElementType> & cvector) const
        {
          // Round about way of calling the wrapNDArray template function that is defined
          // in the cpp file
          typedef typename ConversionPolicy::template apply<std::vector<ElementType> > policy;
          return policy::create(cvector);
        }
      };

      //-----------------------------------------------------------------------
      // Conversion Policies
      //-----------------------------------------------------------------------
      namespace Impl
      {
        /**
         * Helper functions to keep the numpy arrayobject header out
         * the header file
         */
          /// Clone the data into a new array
          template<typename VectorType>
          PyObject *cloneToNDArray(const VectorType &);
      }

      /**
       * Clone is a policy for VectorToNDArray
       * to wrap the vector in a read-write numpy array
       * that looks at the original data. No copy is performed
       */
      struct Clone
      {
        template<typename ContainerType>
        struct apply
        {
          /**
           * Returns a Numpy array that has a copy of the vectors data
           * @param cvector
           * @return
           */
          static PyObject * create(const ContainerType & cvector)
          {
            return Impl::cloneToNDArray<ContainerType>(cvector);
          }
        };
      };

    }
  }
}


#endif /* MANTID_PYTHONINTERFACE_VECTORTONDARRAY_H_ */
