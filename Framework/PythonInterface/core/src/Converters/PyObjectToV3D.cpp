// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "MantidPythonInterface/core/Converters/PyObjectToV3D.h"
#include <boost/python/extract.hpp>

// See
// http://docs.scipy.org/doc/numpy/reference/c-api.array.html#PY_ARRAY_UNIQUE_SYMBOL
#define PY_ARRAY_UNIQUE_SYMBOL CORE_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

using boost::python::extract;
using boost::python::handle;
using boost::python::len;
using boost::python::object;

namespace Mantid {
namespace PythonInterface {
namespace Converters {
/**
 * Construct the converter object with the given Python object
 * @param p :: A boost::python object that should support
 * the __getitem__ and __len__ protocol or be a wrapped V3D object.
 * Throws std::invalid_argument if not
 * if that is not the case.
 */
PyObjectToV3D::PyObjectToV3D(const object &p) : m_obj(p), m_alreadyV3D(false) {
  // Is it an already wrapped V3D ?
  extract<Kernel::V3D> converter(p);
  if (converter.check()) {
    m_alreadyV3D = true;
    return;
  }
  // Is it a sequence
  try {
    const size_t length = len(p);
    if (length != 3) {
      throw std::invalid_argument("Incorrect length for conversion to V3D");
    }
    // Can we index the object
    p.attr("__getitem__")(0);
  } catch (boost::python::error_already_set &) {
    throw std::invalid_argument(
        std::string(
            "Cannot convert object to V3D. Expected a python sequence found ") +
        p.ptr()->ob_type->tp_name);
  }
}

/**
 * Returns a V3D object from the Python object given
 * to the converter
 * @returns A newly constructed V3D object converted
 * from the PyObject.
 */
Kernel::V3D PyObjectToV3D::operator()() {
  if (m_alreadyV3D) {
    return extract<Kernel::V3D>(m_obj)();
  }
  auto toDouble = [](const object &obj) {
    return extract<double>(object(handle<>(PyNumber_Float(obj.ptr()))))();
  };
  return Kernel::V3D(toDouble(m_obj[0]), toDouble(m_obj[1]),
                     toDouble(m_obj[2]));
}
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid
