// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <string>
#include <variant>
#include <vector>

#include <boost/python/extract.hpp>
#include <boost/python/object.hpp>
#include <exception>

namespace Mantid::PythonInterface {

struct PyNativeTypeExtractor {
  using PythonOutputT = std::variant<bool, int, double, std::string>;

  static PythonOutputT convert(const boost::python::object &obj) {
    using namespace boost::python;
    const PyObject *rawptr = obj.ptr();
    PythonOutputT out;

    // This currently  doesn't handle lists, but this could be retrofitted in future work
    if (PyBool_Check(rawptr) == 1) {
      bool val = extract<bool>(obj);
      out = val;
    } else if (PyFloat_Check(rawptr) == 1) {
      double val = extract<double>(obj);
      out = val;
    } else if (PyLong_Check(rawptr) == 1) {
      int val = extract<int>(obj);
      out = val;
    } else if (PyUnicode_Check(rawptr)) {
      std::string val = extract<std::string>(obj);
      out = val;
    } else {
      throw std::invalid_argument("Unrecognised Python type");
    }
    return out;
  }
};

} // namespace Mantid::PythonInterface