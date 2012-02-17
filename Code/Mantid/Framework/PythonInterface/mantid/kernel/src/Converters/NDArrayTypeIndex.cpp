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
        template DLLExport struct NDArrayTypeIndex<CType>;

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
    }
  }
}

