// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "MantidPythonInterface/core/Converters/PyObjectToVMD.h"
#include "MantidKernel/WarningSuppressions.h"
#include <boost/python/extract.hpp>

// See
// http://docs.scipy.org/doc/numpy/reference/c-api.array.html#PY_ARRAY_UNIQUE_SYMBOL
#define PY_ARRAY_UNIQUE_SYMBOL CORE_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

using boost::python::extract;
using boost::python::len;
using boost::python::object;

GNU_DIAG_OFF("strict-aliasing")

namespace Mantid {
namespace PythonInterface {
namespace Converters {
/**
 * Construct the converter object with the given Python object
 * @param p :: A boost::python object that should support
 * the __getitem__ and __len__ protocol or be a wrapped VMD object.
 * Throws std::invalid_argument if not
 * if that is not the case.
 */
PyObjectToVMD::PyObjectToVMD(const object &p) : m_obj(p), m_alreadyVMD(false) {
  // Is it an already wrapped VMD ?
  extract<Kernel::VMD> converter(p);
  if (converter.check()) {
    m_alreadyVMD = true;
    return;
  }
  // Is it a sequence
  try {
    const size_t length = len(p);
    if (length < 3) {
      throw std::invalid_argument("Must be > 2 for conversion to VMD");
    }
    // Can we index the object
    p.attr("__getitem__")(0);
  } catch (boost::python::error_already_set &) {
    throw std::invalid_argument(
        std::string("Cannot convert object to VMD. "
                    "Expected a python sequence found: ") +
        p.ptr()->ob_type->tp_name);
  }
}

/**
 * Returns a VMD object from the Python object given
 * to the converter
 * @returns A newly constructed VMD object converted
 * from the PyObject.
 */
Kernel::VMD PyObjectToVMD::operator()() {
  if (m_alreadyVMD) {
    return extract<Kernel::VMD>(m_obj)();
  }
  const size_t length = len(m_obj);
  Kernel::VMD ret(length);
  for (size_t i = 0; i < length; ++i) {
    ret[i] = extract<float>(m_obj[i])();
  }
  return ret;
}
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid
