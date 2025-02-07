// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
inline bool isNone(const PyObject *ptr) { return (ptr == Py_None); }

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
} // namespace PythonInterface
} // namespace Mantid
