#ifndef NUMPY_FUNCTIONS_H
#define NUMPY_FUNCTIONS_H
/*
  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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

  File change history is stored at: <https://github.com/mantidproject/mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#ifdef __GNUC__
#pragma GCC system_header
#endif

#include <boost/python/list.hpp>
#include "MantidKernel/WarningSuppressions.h"
GCC_DIAG_OFF(cast - qual)
// See
// http://docs.scipy.org/doc/numpy/reference/c-api.array.html#PY_ARRAY_UNIQUE_SYMBOL
#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>
GCC_DIAG_ON(cast - qual)

/**functions containing numpy macros. We put them in a separate header file to
  *suppress the warning
  *ISO C++ forbids casting between pointer-to-function and pointer-to-object
  */
namespace Mantid {
namespace PythonInterface {
namespace Converters {
namespace Impl {
/// equivalent to macro PyArray_IterNew
PyObject *func_PyArray_IterNew(PyArrayObject *arr);
/// equivalent to macro PyArray_NewFromDescr
PyArrayObject *func_PyArray_NewFromDescr(int datatype, const int ndims,
                                         Py_intptr_t *dims);
}
}
}
}
#endif // NUMPY_FUNCTIONS_H
