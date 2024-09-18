// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/BoundedValidator.h"
#include "MantidPythonInterface/core/IsNone.h"
#include <boost/python/class.hpp>
#include <boost/python/default_call_policies.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/overloads.hpp>

using Mantid::Kernel::BoundedValidator;
using Mantid::Kernel::IValidator;
using namespace boost::python;

namespace {
/**
 * Factory function to allow more flexibility in the constructor
 * @param lower An optional lower bound
 * @param upper An optional upper bound
 * @returns A pointer to a new BoundedValidator object
 */
template <typename T> BoundedValidator<T> *createBoundedValidator(object lower = object(), object upper = object()) {
  auto validator = new BoundedValidator<T>();
  if (!Mantid::PythonInterface::isNone(lower)) {
    validator->setLower(extract<T>(lower));
  }
  if (!Mantid::PythonInterface::isNone(upper)) {
    validator->setUpper(extract<T>(upper));
  }
  return validator;
}

/**
 * Factory function to allow more flexibility in the constructor
 * @param lower An optional lower bound
 * @param upper An optional upper bound
 * @param exclusive Optional argument specifiying if the bounds are exclusive
 * @returns A pointer to a new BoundedValidator object
 */
template <typename T>
BoundedValidator<T> *createExclusiveBoundedValidator(object lower = object(), object upper = object(),
                                                     const bool exclusive = false) {
  auto validator = new BoundedValidator<T>();
  if (lower.ptr() != Py_None) {
    validator->setLower(extract<T>(lower));
    validator->setLowerExclusive(exclusive);
  }
  if (upper.ptr() != Py_None) {
    validator->setUpper(extract<T>(upper));
    validator->setUpperExclusive(exclusive);
  }
  return validator;
}

/// A macro for generating exports for each type
#define EXPORT_BOUNDEDVALIDATOR(ElementType, prefix)                                                                   \
  class_<BoundedValidator<ElementType>, bases<IValidator>, boost::noncopyable>(#prefix "BoundedValidator")             \
      .def("__init__", make_constructor(&createBoundedValidator<ElementType>, default_call_policies(),                 \
                                        (arg("lower") = object(), arg("upper") = object())))                           \
      .def("__init__", make_constructor(&createExclusiveBoundedValidator<ElementType>, default_call_policies(),        \
                                        (arg("lower") = object(), arg("upper") = object(), arg("exclusive") = false))) \
      .def("setLower", &BoundedValidator<ElementType>::setLower, (arg("self"), arg("lower")), "Set the lower bound")   \
      .def("setUpper", &BoundedValidator<ElementType>::setUpper, (arg("self"), arg("upper")), "Set the upper bound")   \
      .def("setLowerExclusive", &BoundedValidator<ElementType>::setLowerExclusive, (arg("self"), arg("exclusive")),    \
           "Sets if the lower bound is exclusive")                                                                     \
      .def("setUpperExclusive", &BoundedValidator<ElementType>::setUpperExclusive, (arg("self"), arg("exclusive")),    \
           "Sets if the upper bound is exclsuive")                                                                     \
      .def("setExclusive", &BoundedValidator<ElementType>::setExclusive, (arg("self"), arg("exclusive")),              \
           "Sets both bounds to be inclusive/exclusive")                                                               \
      .def("lower", &BoundedValidator<ElementType>::lower, arg("self"), "Returns the lower bound")                     \
      .def("upper", &BoundedValidator<ElementType>::upper, arg("self"), "Returns the upper bound")                     \
      .def("setBounds", &BoundedValidator<ElementType>::setBounds, (arg("self"), arg("lower"), arg("upper")),          \
           "Set both bounds")                                                                                          \
      .def("hasLower", &BoundedValidator<ElementType>::hasLower, arg("self"),                                          \
           "Returns True if a lower bound has been set")                                                               \
      .def("hasUpper", &BoundedValidator<ElementType>::hasUpper, arg("self"),                                          \
           "Returns True if an upper bound has been set")                                                              \
      .def("isLowerExclusive", &BoundedValidator<ElementType>::isLowerExclusive, arg("self"),                          \
           "Returns True if the lower bound is exclusive")                                                             \
      .def("isUpperExclusive", &BoundedValidator<ElementType>::isUpperExclusive, arg("self"),                          \
           "Returns True if the upper bound is exclusive");
} // namespace

void export_BoundedValidator() {
  EXPORT_BOUNDEDVALIDATOR(double, Float);
  EXPORT_BOUNDEDVALIDATOR(int, Int);
}
