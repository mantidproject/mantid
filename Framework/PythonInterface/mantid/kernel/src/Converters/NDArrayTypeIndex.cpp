//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Converters/NDArrayTypeIndex.h"

#include <boost/python/detail/prefix.hpp> // Safe include of Python.h
#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include "MantidTypes/Core/DateAndTime.h"
#include <numpy/arrayobject.h>

#include <boost/python/type_id.hpp>
#include <stdexcept>

namespace Mantid {
namespace PythonInterface {
namespace Converters {
/// Macro to define mappings between the CType and Numpy enum
#define DEFINE_TYPE_MAPPING(CType, NDTypeNum, NDTypeCode)                      \
  template <> int NDArrayTypeIndex<CType>::typenum = NDTypeNum;                \
  template <> char NDArrayTypeIndex<CType>::typecode = NDTypeCode;             \
  template struct NDArrayTypeIndex<CType>;

DEFINE_TYPE_MAPPING(int, NPY_INT, NPY_INTLTR)
DEFINE_TYPE_MAPPING(long, NPY_LONG, NPY_LONGLTR)
DEFINE_TYPE_MAPPING(long long, NPY_LONGLONG, NPY_LONGLONGLTR)
DEFINE_TYPE_MAPPING(Mantid::Types::Core::DateAndTime, NPY_INT64,
                    NPY_DATETIMELTR)
DEFINE_TYPE_MAPPING(unsigned int, NPY_UINT, NPY_UINTLTR)
DEFINE_TYPE_MAPPING(unsigned long, NPY_ULONG, NPY_ULONGLTR)
DEFINE_TYPE_MAPPING(unsigned long long, NPY_ULONGLONG, NPY_ULONGLONGLTR)
DEFINE_TYPE_MAPPING(bool, NPY_BOOL, NPY_BOOLLTR)
DEFINE_TYPE_MAPPING(double, NPY_DOUBLE, NPY_DOUBLELTR)
DEFINE_TYPE_MAPPING(float, NPY_FLOAT, NPY_CFLOATLTR)
}
} // namespace PythonInterface
} // namespace Mantid
