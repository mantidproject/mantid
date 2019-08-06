// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_TYPEDVALIDATOREXPORTER_H_
#define MANTID_PYTHONINTERFACE_TYPEDVALIDATOREXPORTER_H_

#include "MantidKernel/TypedValidator.h"
#include <boost/python/class.hpp>

namespace Mantid {
namespace PythonInterface {
/**
 * Declares a simple static struct to export a TypedValidator to Python
 * @tparam HeldType The type held within the validator
 */
template <typename Type> struct TypedValidatorExporter {
  static void define(const char *pythonClassName) {
    using namespace boost::python;
    using Mantid::Kernel::IValidator;
    using Mantid::Kernel::TypedValidator;

    class_<TypedValidator<Type>, bases<IValidator>, boost::noncopyable>(
        pythonClassName, no_init)
        .def("isValid", &IValidator::isValid<Type>, (arg("self"), arg("value")),
             "Returns an empty string if the value is considered valid, "
             "otherwise a string defining the error is returned.");
  }
};

#define EXPORT_TYPEDVALIDATOR(Type)
} // namespace PythonInterface
} // namespace Mantid

#endif // MANTID_PYTHONINTERFACE_TYPEDVALIDATOREXPORTER_H_
