// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL-3.0 +
#include "MantidPythonInterface/core/PythonObjectTypeValidator.h"

namespace Mantid::PythonInterface {

std::string PythonObjectTypeValidator::check(boost::any const &value) const {
  boost::python::object obj;
  try {
    obj = *(boost::any_cast<boost::python::object const *>(value));
  } catch (boost::bad_any_cast const &) {
    return "Attempting to run a python type validator on an object that is not a python object";
  }
  std::string ret;
  int check = PyObject_IsInstance(obj.ptr(), pythonClass.ptr());
  if (check < 0) {
    // this represents an internal error while checking type
    PyErr_Clear();
    ret = "Failed to check instance type";
  } else if (check == 0) {
    std::string objclassname = extract<std::string>(obj.attr("__class__").attr("__name__"));
    std::string classname = extract<std::string>(pythonClass.attr("__name__"));
    ret = std::string("The passed object is of type ") + objclassname + std::string(" and not of type ") + classname;
  }
  return ret;
}

} // namespace Mantid::PythonInterface
