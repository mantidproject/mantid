// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/ArrayLengthValidator.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::Kernel::ArrayLengthValidator;
using Mantid::Kernel::IValidator;
using namespace boost::python;

namespace {

#define EXPORT_LENGTHVALIDATOR(type, prefix)                                                                           \
  class_<ArrayLengthValidator<type>, bases<IValidator>, boost::noncopyable>(#prefix "ArrayLengthValidator")            \
      .def(init<int>((arg("self"), arg("length")), "Constructs a validator verifying that "                            \
                                                   "an array is of the exact length given"))                           \
      .def(init<int, int>((arg("self"), arg("lenmin"), arg("lenmax")),                                                 \
                          "Constructs a validator verifying that the length "                                          \
                          "of an array is within the range given"))                                                    \
      .def("hasLength", &ArrayLengthValidator<type>::hasLength, arg("self"),                                           \
           "Returns true if a single length has been set")                                                             \
      .def("hasMinLength", &ArrayLengthValidator<type>::hasMinLength, arg("self"),                                     \
           "Returns true if a minimum length has been set")                                                            \
      .def("hasMaxLength", &ArrayLengthValidator<type>::hasMaxLength, arg("self"),                                     \
           "Returns true if a maximum length has been set")                                                            \
      .def("getLength", &ArrayLengthValidator<type>::getLength, return_value_policy<copy_const_reference>(),           \
           arg("self"), "Returns the set fixed length")                                                                \
      .def("getMinLength", &ArrayLengthValidator<type>::getMinLength, return_value_policy<copy_const_reference>(),     \
           arg("self"), "Returns the set minimum length")                                                              \
      .def("getMaxLength", &ArrayLengthValidator<type>::getMaxLength, return_value_policy<copy_const_reference>(),     \
           arg("self"), "Returns the set maximum length")                                                              \
      .def("setLength", &ArrayLengthValidator<type>::setLength, (arg("self"), arg("length")),                          \
           "Set the accepted length of an array")                                                                      \
      .def("clearLength", &ArrayLengthValidator<type>::clearLength, arg("self"), "Clears accepted length of an array") \
      .def("setLengthMin", &ArrayLengthValidator<type>::setLengthMin, (arg("self"), arg("minimum length")),            \
           "Set the accepted minimum length of an array")                                                              \
      .def("setLengthMax", &ArrayLengthValidator<type>::setLengthMax, (arg("self"), arg("maximum length")),            \
           "Set the accepted maximum length of an array")                                                              \
      .def("clearLengthMin", &ArrayLengthValidator<type>::clearLengthMin, arg("self"),                                 \
           "Set the accepted minimum length of an array")                                                              \
      .def("clearLengthMax", &ArrayLengthValidator<type>::clearLengthMax, arg("self"),                                 \
           "Set the accepted maximum length of an array");
} // namespace

void export_ArrayLengthValidator() {
  EXPORT_LENGTHVALIDATOR(double, Float);
  EXPORT_LENGTHVALIDATOR(int, Int);
  EXPORT_LENGTHVALIDATOR(std::string, String);
}
