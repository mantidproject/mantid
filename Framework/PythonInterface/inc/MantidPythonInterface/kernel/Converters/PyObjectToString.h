#ifndef MANTID_PYTHONINERFACE_CONVERTERS_PYOBJECTTOSTRING_H_
#define MANTID_PYTHONINERFACE_CONVERTERS_PYOBJECTTOSTRING_H_
/**
    Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
#include "MantidKernel/System.h"
#include <boost/python/object.hpp>

namespace Mantid {
namespace PythonInterface {
namespace Converters {

/**
 * Convert a python object to a string or throw an exception. This will convert
 * unicode strings in python2 via utf8.
 */
DLLExport std::string pyObjToStr(const boost::python::object &value);

} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINERFACE_CONVERTERS_PYOBJECTTOSTRING_H_ */
