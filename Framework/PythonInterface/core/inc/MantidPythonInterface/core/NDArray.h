// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidPythonInterface/core/DllConfig.h"

#include <boost/python/object.hpp>

namespace Mantid {
namespace PythonInterface {

// It is important that the numpy/arrayobject header
// does not appear in any of our headers as it
// contains some static definitions that cannot be
// allowed to be defined in other translation units

MANTID_PYTHONINTERFACE_CORE_DLL void importNumpy();

MANTID_PYTHONINTERFACE_CORE_DLL PyTypeObject *ndarrayType();

/**
 * Thin object wrapper around a numpy array. This is intended to take the place
 * of boost::python::numeric::array, which is a dated wrapper containing a bug
 * when used with Python 3 - https://github.com/boostorg/python/issues/75.
 *
 * Only minimal functionality has been ported here.
 */
class MANTID_PYTHONINTERFACE_CORE_DLL NDArray : public boost::python::object {
public:
  static bool check(const boost::python::object &obj);

  NDArray(const boost::python::object &obj);
  BOOST_PYTHON_FORWARD_OBJECT_CONSTRUCTORS(NDArray, boost::python::object);

  Py_intptr_t const *get_shape() const;
  int get_nd() const;
  void *get_data() const;
  char get_typecode() const;

  NDArray astype(char dtype, bool copy = true) const;
};

} // end namespace PythonInterface
} // end namespace Mantid

namespace boost {
namespace python {
namespace converter {
/**
 * Register ndarray as a type that manages a PyObject* internally.
 */
template <> struct MANTID_PYTHONINTERFACE_CORE_DLL object_manager_traits<Mantid::PythonInterface::NDArray> {
  BOOST_STATIC_CONSTANT(bool, is_specialized = true);
  static bool check(PyObject *obj);
  static python::detail::new_reference adopt(PyObject *obj);
  static PyTypeObject const *get_pytype();
};
} // end namespace converter
} // end namespace python
} // end namespace boost
