//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Converters/NDArrayTypeIndex.h"

#include <boost/python/detail/prefix.hpp> // Safe include of Python.h
#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

#include <boost/python/type_id.hpp>
#include <stdexcept>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace Converters
    {
      /// Macro to define mappings between the CType and Numpy enum
      #define DEFINE_TYPE_MAPPING(CType, NDTypeNum) \
        template<> \
        int NDArrayTypeIndex<CType>::typenum = NDTypeNum;\
        template DLLExport struct NDArrayTypeIndex<CType>;\


      DEFINE_TYPE_MAPPING(int, NPY_INT)
      DEFINE_TYPE_MAPPING(long, NPY_LONG)
      DEFINE_TYPE_MAPPING(long long, NPY_LONGLONG)
      DEFINE_TYPE_MAPPING(unsigned int, NPY_UINT)
      DEFINE_TYPE_MAPPING(unsigned long, NPY_ULONG)
      DEFINE_TYPE_MAPPING(unsigned long long, NPY_ULONGLONG)
      DEFINE_TYPE_MAPPING(bool, NPY_BOOL)
      DEFINE_TYPE_MAPPING(double, NPY_DOUBLE)
      DEFINE_TYPE_MAPPING(float, NPY_FLOAT)
    }
  }
}

