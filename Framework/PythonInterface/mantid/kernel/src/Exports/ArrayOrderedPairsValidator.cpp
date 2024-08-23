// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/ArrayOrderedPairsValidator.h"
#include <boost/python/class.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::Kernel::ArrayOrderedPairsValidator;
using Mantid::Kernel::IValidator;
using namespace boost::python;

namespace {
template <typename TYPE> ArrayOrderedPairsValidator<TYPE> *createArrayOrderedPairsValidator() {
  return new ArrayOrderedPairsValidator<TYPE>();
}

#define EXPORT_PAIRSVALIDATOR(type, prefix)                                                                            \
  class_<ArrayOrderedPairsValidator<type>, bases<IValidator>, boost::noncopyable>(#prefix                              \
                                                                                  "ArrayOrderedPairsValidator")        \
      .def("__init__", make_constructor(&createArrayOrderedPairsValidator<type>, default_call_policies()));
} // namespace
void export_ArrayOrderedPairsValidator() {
  EXPORT_PAIRSVALIDATOR(double, Float);
  EXPORT_PAIRSVALIDATOR(int, Int);
}
