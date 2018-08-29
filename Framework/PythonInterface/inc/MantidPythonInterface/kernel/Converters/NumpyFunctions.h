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

#include <boost/python/detail/wrap_python.hpp>

// Forward declare the numpy array types
struct tagPyArrayObject;
using PyArrayObject = tagPyArrayObject;
struct _PyArray_Descr;
using PyArray_Descr = _PyArray_Descr;

/*
 * The numpy API is a C api where pointers to functions and objects are the
 * same size.
 * This is not guaranteed by the C++ standard so macros in the numpy headers,
 * such as PyArray_IterNew produces warnings when compiled with a C++ compiler.
 *
 * The only solution appears to be wrap the calls in our own functions and
 * suppress the warnings.
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
PyArrayObject *func_PyArray_NewFromDescr(const char *datadescr, const int ndims,
                                         Py_intptr_t *dims);
PyArray_Descr *func_PyArray_Descr(const char *datadescr);
} // namespace Impl
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid
#endif // NUMPY_FUNCTIONS_H
