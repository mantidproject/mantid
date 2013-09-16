#include "MantidKernel/BoundedValidator.h"
#include "MantidPythonInterface/kernel/IsNone.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/default_call_policies.hpp>

using Mantid::Kernel::BoundedValidator;
using Mantid::Kernel::IValidator;
using namespace boost::python;

namespace
{
  /**
   * Factory function to allow more flexibility in the constructor
   * @param lower An optional lower bound
   * @param upper An optional upper bound
   * @returns A pointer to a new BoundedValidator object
   */
  template<typename T>
  BoundedValidator<T> * createBoundedValidator(object lower = object(), object upper = object())
  {
    BoundedValidator<T> * validator = new BoundedValidator<T>();
    if(!Mantid::PythonInterface::isNone(lower))
    {
      validator->setLower(extract<T>(lower));
    }
    if(!Mantid::PythonInterface::isNone(upper))
    {
      validator->setUpper(extract<T>(upper));
    }
    return validator;
  }

  /// A macro for generating exports for each type
  #define EXPORT_BOUNDEDVALIDATOR(ElementType, prefix) \
    class_<BoundedValidator<ElementType>, bases<IValidator>, \
           boost::noncopyable>(#prefix"BoundedValidator") \
      .def("__init__", make_constructor(&createBoundedValidator<ElementType>, \
                                        default_call_policies(), (arg("lower")=object(), arg("upper")=object())))\
      .def("setLower", &BoundedValidator<ElementType>::setLower, "Set the lower bound") \
      .def("setUpper", &BoundedValidator<ElementType>::setUpper, "Set the upper bound" ) \
      .def("lower", &BoundedValidator<ElementType>::lower, return_value_policy<copy_const_reference>(), \
           "Returns the lower bound") \
      .def("upper", &BoundedValidator<ElementType>::upper, return_value_policy<copy_const_reference>(), \
           "Returns the upper bound" ) \
      .def("setBounds", &BoundedValidator<ElementType>::setBounds, "Set both bounds" ) \
      .def("hasLower", &BoundedValidator<ElementType>::hasLower, "Returns True if a lower bound has been set" ) \
      .def("hasUpper", &BoundedValidator<ElementType>::hasUpper, "Returns True if an upper bound has been set" ) \
    ;

}

void export_BoundedValidator()
{
  EXPORT_BOUNDEDVALIDATOR(double, Float);
  EXPORT_BOUNDEDVALIDATOR(long, Int);
}

