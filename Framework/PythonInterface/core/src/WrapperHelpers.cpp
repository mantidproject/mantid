// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/core/WrapperHelpers.h"

using namespace boost::python;

namespace Mantid::PythonInterface {

/** Checks whether the given object's type dictionary contains the named
 *attribute.
 *
 * Usually boost::python::get_override is used for this check but if the
 * object's overridden function is declared on a superclass of the wrapped
 * class then get_override always returns true, regardless of whether the
 * method has been overridden in Python.
 *
 * An example is the algorithm hierachy. We export the IAlgorithm interface
 * with the name method attach to it. If a class in Python does not
 * override the name method then get_override still claims that the
 * override exists because it has found the IAlgorithm one
 * @param obj :: A pointer to a PyObject
 * @param attr :: A string containin the attribute name
 * @returns True if the type dictionary contains the attribute
 */
bool typeHasAttribute(PyObject *obj, const char *attr) {
  PyObject *cls_dict = obj->ob_type->tp_dict;
  object key(handle<>(to_python_value<char const *&>()(attr)));
  return PyDict_Contains(cls_dict, key.ptr()) > 0;
}

/** Same as above but taking a wrapper reference instead
 * @param wrapper :: A reference to a Wrapper object
 * @param attr :: A string containin the attribute name
 * @returns True if the type dictionary contains the attribute
 */
bool typeHasAttribute(const boost::python::detail::wrapper_base &wrapper, const char *attr) {
  using namespace boost::python::detail;
  return typeHasAttribute(wrapper_base_::get_owner(wrapper), attr);
}

} // namespace Mantid::PythonInterface
