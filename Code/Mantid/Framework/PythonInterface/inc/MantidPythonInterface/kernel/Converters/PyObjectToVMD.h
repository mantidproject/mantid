#ifndef MANTID_PYTHONINERFACE_PYOBJECTTOVMD_H_
#define MANTID_PYTHONINERFACE_PYOBJECTTOVMD_H_
/**
    Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
#include "MantidKernel/VMD.h"
#include <boost/python/object.hpp>

namespace Mantid {
namespace PythonInterface {
namespace Converters {
/**
 * Takes a Python object and if it supports
 * indexing and is of length 3 then it will
 * attempt to convert a Kernel::VMD object from
 * it
 */
struct DLLExport PyObjectToVMD {
  PyObjectToVMD(const boost::python::object &p);
  /// Produces a VMD object from the given PyObject
  Kernel::VMD operator()();

private:
  /// A reference to the object
  const boost::python::object &m_obj;
  /// Is the object a wrapped instance of VMD
  bool m_alreadyVMD;
};
}
}
}

#endif /* MANTID_PYTHONINERFACE_PYOBJECTTOVMD_H_ */
