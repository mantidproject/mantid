#ifndef MANTID_PYTHONINTERFACE_NDARRAYTOVECTORCONVERTER_H_
#define MANTID_PYTHONINTERFACE_NDARRAYTOVECTORCONVERTER_H_
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
#include "MantidKernel/System.h"
#include <boost/python/object.hpp>
#include <vector>

namespace Mantid {
namespace PythonInterface {
namespace Converters {
/**
 * Converts a Python sequence type to a C++ std::vector, where the vector
 * element
 * type is defined by the template type
 */
template <typename DestElementType> struct DLLExport NDArrayToVector {
  /// Constructor
  NDArrayToVector(const boost::python::object &value);
  /// Do the conversion
  const std::vector<DestElementType> operator()();

private:
  /// Check the array is of the correct type and coerce it if not
  void typeCheck();
  /// Pointer to ndarray object
  boost::python::object m_arr;
};
}
}
}

#endif /* MANTID_PYTHONINTERFACE_NDARRAYTOVECTORCONVERTER_H_ */
