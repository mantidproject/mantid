#ifndef MANTID_PYTHONINTERFACE_KERNEL_ISNONE_H_
#define MANTID_PYTHONINTERFACE_KERNEL_ISNONE_H_
/**
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
#include <boost/python/object.hpp>

/**
 * Defines a helper function to check whether an object is of Python
 * type None.
 *
 * The boost::python::object method is_none was only
 * added in version 1.43 and we still support some versions
 * prior to this
 */
namespace Mantid {
namespace PythonInterface {

/**
 * @param ptr A * to a raw PyObject
 * @returns true if the given object is of type None
 */
inline bool isNone(PyObject *ptr) { return (ptr == Py_None); }

/**
 * @param obj A const reference to boost python object wrapper
 * @returns true if the given boost python object is of type None
 */
inline bool isNone(const boost::python::object &obj) {
#ifdef BOOST_PYTHON_OBJECT_HAS_IS_NONE
  return obj.is_none();
#else
  return isNone(obj.ptr());
#endif
}
}
}

#endif /* #ifndef MANTID_PYTHONINTERFACE_KERNEL_ISNONE_H_ */
