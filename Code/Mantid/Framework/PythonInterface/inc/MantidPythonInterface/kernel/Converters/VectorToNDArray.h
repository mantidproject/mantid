#ifndef MANTID_PYTHONINTERFACE_VECTORTONDARRAY_H_
#define MANTID_PYTHONINTERFACE_VECTORTONDARRAY_H_
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

#include <boost/python/detail/prefix.hpp>
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
       * Converter that takes a std::vector and converts it into a flat numpy array.
       *
       * The type of conversion is specified by another struct/class that
       * contains a static member create.
       */
      template<typename ElementType, typename ConversionPolicy>
      struct VectorToNDArray
      {
        /**
         * Converts a cvector to a numpy array
         * @param cdata :: A const reference to a vector
         * @returns A new PyObject that wraps the vector in a numpy array
         */
        inline PyObject * operator()(const std::vector<ElementType> & cdata) const
        {
          // Hand off the work to the conversion policy
          typedef typename ConversionPolicy::template apply<ElementType> policy;
          return policy::create1D(cdata);
        }
      };

    }
  }
}


#endif /* MANTID_PYTHONINTERFACE_VECTORTONDARRAY_H_ */
