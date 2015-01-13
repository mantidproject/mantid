#ifndef MANTID_PYTHONINTERFACE_PYSEQUENCETOVECTORCONVERTER_H_
#define MANTID_PYTHONINTERFACE_PYSEQUENCETOVECTORCONVERTER_H_
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
#include <boost/python/extract.hpp>
#include <vector>

namespace Mantid {
namespace PythonInterface {
namespace // <anonymous>
    {
/**
 * Extract a C type from a Python object.
 */
template <typename CType> struct ExtractCType {
  /**
   * Calls extract on the Python object using the template type
   * @param value A pointer to the Python object
   * @return The value as a C type
   */
  inline CType operator()(PyObject *value) {
    return boost::python::extract<CType>(value);
  }
};

/**
 * Template specialization to convert a Python object to a C++ std::string
 */
template <> struct ExtractCType<std::string> {
  /**
   * Uses boost lexical cast to convert the type to a string
   * @param value A pointer to the Python object
   * @return The value as a C type
   */
  inline std::string operator()(PyObject *value) {
    return boost::python::extract<std::string>(PyObject_Str(value));
  }
};

} // end <anonymous>

namespace Converters {
/**
 * Converts a Python sequence type to a C++ std::vector, where the element
 * type is defined by the template type
 */
template <typename DestElementType> struct DLLExport PySequenceToVector {
  PySequenceToVector(const boost::python::object &value) : m_obj(value.ptr()) {
    check(value);
  }

  /**
   * Converts the Python object to a C++ vector
   * @return A std::vector<ElementType> containing the values
   * from the Python sequence
   */
  inline const std::vector<DestElementType> operator()() {
    Py_ssize_t length = PySequence_Size(m_obj);
    std::vector<DestElementType> cvector(length);
    if (length == 0)
      return cvector;
    ExtractCType<DestElementType> elementConverter;
    for (Py_ssize_t i = 0; i < length; ++i) {
      PyObject *item = PySequence_Fast_GET_ITEM(m_obj, i);
      DestElementType element = elementConverter(item);
      cvector[i] = element;
    }
    return cvector;
  }

private:
  inline void check(const boost::python::object &obj) {
    if (!PySequence_Check(obj.ptr())) {
      throw std::invalid_argument(
          std::string(
              "PySequenceToVector expects Python sequence type, found ") +
          obj.ptr()->ob_type->tp_name);
    }
  }
  /// Python object to convert
  PyObject *m_obj;
};
}
}
}

#endif /* MANTID_PYTHONINTERFACE_PYSEQUENCETOVECTORCONVERTER_H_ */
