#ifndef MANTID_PYTHONINTERFACE_CARRAYTONDARRAY_H_
#define MANTID_PYTHONINTERFACE_CARRAYTONDARRAY_H_
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
#include "MantidPythonInterface/kernel/Converters/WrapWithNumpy.h"

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
       * Converter that takes a c array and its size then converts/wraps it into a numpy array.
       *
       * The type of conversion is specified by another struct/class that
       * contains a static member create.
       */
      template<typename ElementType, typename ConversionPolicy>
      struct CArrayToNDArray
      {
        inline PyObject * operator()(const ElementType * carray, const int ndims, Py_intptr_t *dims) const
        {
          // Round about way of calling the wrapNDArray template function that is defined
          // in the cpp file
          typedef typename ConversionPolicy::template apply<ElementType> policy;
          return policy::createFromArray(carray, ndims, dims);
        }
      };

    }
  }
}



#endif /* MANTID_PYTHONINTERFACE_CARRAYTONDARRAY_H_ */
