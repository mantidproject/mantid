#ifndef MANTID_PYTHONINERFACE_CONVERTERS_DATEANDTIME_H_
#define MANTID_PYTHONINERFACE_CONVERTERS_DATEANDTIME_H_
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
#include "MantidKernel/DateAndTime.h"
#include <boost/python/object.hpp>
#include <numpy/ndarraytypes.h>

namespace Mantid {
namespace PythonInterface {
namespace Converters {

/// Convert to numpy's datetime64. This is panda's name for the function.
PyObject *to_datetime64(const Types::Core::DateAndTime &dateandtime);
/// Total nanoseconds since the unix epoch
npy_datetime to_npy_datetime(const Types::Core::DateAndTime &dateandtime);
PyArray_Descr *descr_ns();

boost::shared_ptr<Types::Core::DateAndTime>
to_dateandtime(const boost::python::api::object &value);
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINERFACE_CONVERTERS_DATEANDTIME_H_ */
