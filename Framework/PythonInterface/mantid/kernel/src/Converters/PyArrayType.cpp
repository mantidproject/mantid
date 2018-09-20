//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Converters/PyArrayType.h"

// See
// http://docs.scipy.org/doc/numpy/reference/c-api.array.html#PY_ARRAY_UNIQUE_SYMBOL
#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

namespace Mantid {
namespace PythonInterface {
namespace Converters {

/**
 * Returns a pointer to the PyArray_Type object
 * @return
 */
PyTypeObject *getNDArrayType() { return &PyArray_Type; }
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid
