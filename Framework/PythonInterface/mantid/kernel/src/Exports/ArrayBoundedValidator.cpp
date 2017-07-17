#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidPythonInterface/kernel/IsNone.h"
#include <boost/python/class.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/make_constructor.hpp>



using Mantid::Kernel::ArrayBoundedValidator;
using Mantid::Kernel::IValidator;
using namespace boost::python;

namespace {
    /**
    * Factory function to allow more flexibility in the constructor
    * @param lower An optional lower bound
    * @param upper An optional upper bound
    * @returns A pointer to a new ArrayBoundedValidator object
    */
    template <typename T>
    ArrayBoundedValidator<T> *createArrayBoundedValidator(object lower = object(),
        object upper = object()) {
        auto validator = new ArrayBoundedValidator<T>();
        if (!Mantid::PythonInterface::isNone(lower)) {
            validator->setLower(extract<T>(lower));
        }
        if (!Mantid::PythonInterface::isNone(upper)) {
            validator->setUpper(extract<T>(upper));
        }
        return validator;
    }

#define EXPORT_ARRAYBOUNDEDVALIDATOR(type, prefix)                             \
  class_<ArrayBoundedValidator<type>, bases<IValidator>, boost::noncopyable>(  \
      #prefix "ArrayBoundedValidator")                                         \
      .def("__init__",                                                         \
           make_constructor(                                                   \
               &createArrayBoundedValidator<type>, default_call_policies(),    \
               (arg("lower") = object(), arg("upper") = object())))            \
      .def("hasLower", &ArrayBoundedValidator<type>::hasLower, arg("self"),    \
           "Returns true if a lower bound has been set")                       \
      .def("hasUpper", &ArrayBoundedValidator<type>::hasUpper, arg("self"),    \
           "Returns true if an upper bound has been set")                      \
      .def("lower", &ArrayBoundedValidator<type>::lower, arg("self"),          \
           return_value_policy<copy_const_reference>(),                        \
           "Returns the lower bound")                                          \
      .def("upper", &ArrayBoundedValidator<type>::upper, arg("self"),          \
           return_value_policy<copy_const_reference>(),                        \
           "Returns the upper bound")                                          \
      .def("setLower", &ArrayBoundedValidator<type>::setLower,                 \
           (arg("self"), arg("lower")), "Sets the lower bound")                \
      .def("setUpper", &ArrayBoundedValidator<type>::setUpper,                 \
           (arg("self"), arg("upper")), "Sets the upper bound")                \
      .def("clearLower", &ArrayBoundedValidator<type>::clearLower,             \
           arg("self"), "Clear any set lower bound")                           \
      .def("clearUpper", &ArrayBoundedValidator<type>::clearUpper,             \
           arg("self"), "Clear any set upper bound");
}
 
void export_ArrayBoundedValidator() {
  EXPORT_ARRAYBOUNDEDVALIDATOR(double, Float);
  EXPORT_ARRAYBOUNDEDVALIDATOR(long, Int);

}

