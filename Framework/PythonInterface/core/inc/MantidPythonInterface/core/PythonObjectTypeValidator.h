// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidPythonInterface/core/DllConfig.h"

#include "MantidKernel/IValidator.h"

#include <Python.h>
#include <boost/python/extract.hpp>
#include <boost/python/object.hpp>

// forward declare
namespace Json {
class Value;
}

namespace {
using namespace boost::python;
object const &validate_python_class(object const &pyclass) {
  if (PyType_Check(pyclass.ptr())) {
    return pyclass;
  } else {
    throw std::invalid_argument("Attempt to construct validator with an object instead of a class type");
  }
}
} // namespace

namespace Mantid::PythonInterface {

using namespace boost::python;
using Mantid::Kernel::IValidator_sptr;

class MANTID_PYTHONINTERFACE_CORE_DLL PythonObjectTypeValidator : public Mantid::Kernel::IValidator {

public:
  PythonObjectTypeValidator() : pythonClass() {};

  PythonObjectTypeValidator(object const &pyclass) : pythonClass(validate_python_class(pyclass)) {}

  ~PythonObjectTypeValidator() = default;

  IValidator_sptr clone() const override { return std::make_shared<PythonObjectTypeValidator>(*this); };

private:
  object pythonClass;

  std::string check(boost::any const &value) const override {
    boost::python::object obj;
    try {
      obj = *(boost::any_cast<boost::python::object const *>(value));
    } catch (...) {
      return "Attempting to run a python type validator on an object that is not a python object";
    }
    if (PyObject_IsInstance(obj.ptr(), pythonClass.ptr())) {
      return "";
    } else {
      std::string objclassname = extract<std::string>(obj.attr("__class__").attr("__name__"));
      std::string classname = extract<std::string>(pythonClass.attr("__name__"));
      return std::string("The passed object is of type ") + objclassname + std::string(" and not of type ") + classname;
    }
  }
};

} // namespace Mantid::PythonInterface
