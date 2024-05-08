// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/ArrayBoundedValidator.h"

#include <boost/python/class.hpp>
#include <boost/python/default_call_policies.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::Kernel::ArrayBoundedValidator;
using Mantid::Kernel::IValidator;
using namespace boost::python;

namespace {

/**
 * Factory function to allow more flexibility in the constructor
 * @param lower An optional lower bound
 * @param upper An optional upper bound
 * @param exclusive Optional argument specifiying if the bounds are exclusive
 * @returns A pointer to a new BoundedValidator object
 */
template <typename T>
ArrayBoundedValidator<T> *createExclusiveArrayBoundedValidator(object lower = object(), object upper = object(),
                                                               const bool exclusive = false) {
  auto validator = std::make_unique<ArrayBoundedValidator<T>>();
  if (lower.ptr() != Py_None) {
    validator->setLower(extract<T>(lower));
    validator->setLowerExclusive(exclusive);
  }
  if (upper.ptr() != Py_None) {
    validator->setUpper(extract<T>(upper));
    validator->setUpperExclusive(exclusive);
  }
  return validator.release();
}

#define EXPORT_ARRAYBOUNDEDVALIDATOR(type, prefix)                                                                     \
  class_<ArrayBoundedValidator<type>, bases<IValidator>, boost::noncopyable>(#prefix "ArrayBoundedValidator")          \
      .def(init<type, type>((arg("self"), arg("lowerBound"), arg("upperBound")),                                       \
                            "Construct a validator to ensure each value is in the given range"))                       \
      .def("__init__", make_constructor(&createExclusiveArrayBoundedValidator<type>, default_call_policies(),          \
                                        (arg("lower") = object(), arg("upper") = object(), arg("exclusive") = false))) \
      .def("hasLower", &ArrayBoundedValidator<type>::hasLower, arg("self"),                                            \
           "Return true if a lower bound has been set")                                                                \
      .def("hasUpper", &ArrayBoundedValidator<type>::hasUpper, arg("self"),                                            \
           "Return true if an upper bound has been set")                                                               \
      .def("lower", &ArrayBoundedValidator<type>::lower, arg("self"), "Return the lower bound")                        \
      .def("upper", &ArrayBoundedValidator<type>::upper, arg("self"), "Return the upper bound")                        \
      .def("setLowerExclusive", &ArrayBoundedValidator<type>::setLowerExclusive, (arg("self"), arg("exclusive")),      \
           "Set if the lower bound is exclusive")                                                                      \
      .def("setUpperExclusive", &ArrayBoundedValidator<type>::setUpperExclusive, (arg("self"), arg("exclusive")),      \
           "Set if the upper bound is exclusive")                                                                      \
      .def("setExclusive", &ArrayBoundedValidator<type>::setExclusive, (arg("self"), arg("exclusive")),                \
           "Set if the bounds are exclusive")                                                                          \
      .def("isLowerExclusive", &ArrayBoundedValidator<type>::isLowerExclusive, arg("self"),                            \
           "Return True if the lower bound is exclusive")                                                              \
      .def("isUpperExclusive", &ArrayBoundedValidator<type>::isUpperExclusive, arg("self"),                            \
           "Return True if the upper bound is exclusive")                                                              \
      .def("setLower", &ArrayBoundedValidator<type>::setLower, (arg("self"), arg("lower")), "Set the lower bound")     \
      .def("setUpper", &ArrayBoundedValidator<type>::setUpper, (arg("self"), arg("upper")), "Set the upper bound")     \
      .def("clearLower", &ArrayBoundedValidator<type>::clearLower, arg("self"), "Clear any set lower bound")           \
      .def("clearUpper", &ArrayBoundedValidator<type>::clearUpper, arg("self"), "Clear any set upper bound");
} // namespace

void export_ArrayBoundedValidator() {
  EXPORT_ARRAYBOUNDEDVALIDATOR(double, Float);
  EXPORT_ARRAYBOUNDEDVALIDATOR(int, Int);
}
