#ifndef MANTID_PYTHONINTERFACE_NDARRAYTYPEINDEX_H_
#define MANTID_PYTHONINTERFACE_NDARRAYTYPEINDEX_H_
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
#include <boost/python/detail/prefix.hpp> // Safe include of Python.h

//#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
//#define NO_ARRAY_IMPORT
//#include <numpy/ndarrayobject.h> 

namespace Mantid
{
  namespace PythonInterface
  {
    namespace Numpy
    {

      /**
       * Defines a mapping between C++ type given by
       * the template parameter and numpy type enum
       * NPY_TYPES.
       *
       * There is no general definition, only specialized
       * versions are defined. Each specialization should
       * contain a static const NPY_TYPES definition giving
       * the result of the mapping
       */
      template<typename T>
      struct NDArrayTypeIndex
      {};

      /// Macro to define mappings between the CType and Numpy enum
      #define DEFINE_TYPE_MAPPING(CType, NDTypeNum) \
         template<>\
         struct NDArrayTypeIndex<CType>\
         {\
           static int typenum() { return NDTypeNum; }\
         };

      DEFINE_TYPE_MAPPING(int16_t, NPY_INT16);
      DEFINE_TYPE_MAPPING(uint16_t, NPY_UINT16);
      DEFINE_TYPE_MAPPING(int32_t, NPY_INT32);
      DEFINE_TYPE_MAPPING(uint32_t, NPY_UINT32);
      DEFINE_TYPE_MAPPING(int64_t, NPY_INT64);
#ifdef __APPLE__
      DEFINE_TYPE_MAPPING(unsigned long, NPY_ULONG);
#endif
      DEFINE_TYPE_MAPPING(uint64_t, NPY_UINT64);
      DEFINE_TYPE_MAPPING(double, NPY_DOUBLE);
      // Not needed outside here
      #undef DEFINE_TYPE_MAPPING

    }
  }
}


#endif /* MANTID_PYTHONINTERFACE_NDARRAYTYPEINDEX_H_*/
