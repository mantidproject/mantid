#ifndef MANTID_PYTHONINTERFACE_WRAPPERHELPERS_H_
#define MANTID_PYTHONINTERFACE_WRAPPERHELPERS_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/System.h"
#include <boost/python/wrapper.hpp>
#include <stdexcept>

namespace Mantid {
namespace PythonInterface {
namespace Environment {
/**
This namespace contains helper functions for classes that are overridden in
Python

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
/// Checks whether the given object's type dictionary contains the named
/// attribute.
bool DLLExport typeHasAttribute(PyObject *obj, const char *attr);
/// An overload for the above taking a wrapper reference
bool DLLExport
    typeHasAttribute(const boost::python::detail::wrapper_base &wrapper,
                     const char *attr);

/**
 * Defines an exception for an undefined attribute
 */
class UndefinedAttributeError : std::runtime_error {
public:
  /// Construct the exception
  UndefinedAttributeError() : std::runtime_error("") {}
};
}
}
}

#endif // MANTID_PYTHONINTERFACE_WRAPPERHELPERS_H_
