#include "MantidKernel/BoundedValidator.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/default_call_policies.hpp>
#include <boost/python/overloads.hpp>

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
    if(!lower.is_none())
    {
      validator->setLower(extract<T>(lower));
    }
    if(!upper.is_none())
    {
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
  template<typename T>
  BoundedValidator<T> * createexclusiveBoundedValidator(object lower = object(), object upper = object(),
    const bool exclusive = false)
  {
    BoundedValidator<T> * validator = new BoundedValidator<T>();
    if(lower.ptr() != Py_None)
    {
      validator->setLower(extract<T>(lower), exclusive);
    }
    if(upper.ptr() != Py_None)
    {
      validator->setUpper(extract<T>(upper), exclusive);
    }
    return validator;
  }

  // Python overloads for optional function args
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(setLower_Overload, setLower, 1, 2);
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(setUpper_Overload, setUpper, 1, 2);
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(setBounds_Overload, setBounds, 2, 3);

  /// A macro for generating exports for each type
  #define EXPORT_BOUNDEDVALIDATOR(ElementType, prefix) \
    class_<BoundedValidator<ElementType>, bases<IValidator>, \
           boost::noncopyable>(#prefix"BoundedValidator") \
      .def("__init__", make_constructor(&createBoundedValidator<ElementType>, \
                                        default_call_policies(), (arg("lower")=object(), arg("upper")=object())))\
      .def("__init__", make_constructor(&createexclusiveBoundedValidator<ElementType>, \
                                        default_call_policies(), (arg("lower")=object(), arg("upper")=object(),arg("exclusive")=false)))\
      .def("setLower", &BoundedValidator<ElementType>::setLower, "Set the lower bound") \
      .def("setLower", &BoundedValidator<ElementType>::setLower, \
                                        setLower_Overload("Set the lower bound", (arg("lower")=object(), arg("exclusive")=false))) \
      .def("setUpper", &BoundedValidator<ElementType>::setUpper, "Set the upper bound" ) \
      .def("setUpper", &BoundedValidator<ElementType>::setUpper, \
                                        setUpper_Overload("Set the upper bound" , (arg("upper")=object(), arg("exclusive")=false))) \
      .def("lower", &BoundedValidator<ElementType>::lower, return_value_policy<copy_const_reference>(), \
           "Returns the lower bound") \
      .def("upper", &BoundedValidator<ElementType>::upper, return_value_policy<copy_const_reference>(), \
           "Returns the upper bound" ) \
      .def("setBounds", &BoundedValidator<ElementType>::setBounds, "Set both bounds" ) \
      .def("setBounds", &BoundedValidator<ElementType>::setBounds, \
                                        setBounds_Overload( "Set both bounds", (arg("lower")=object(), arg("upper")=object(), arg("exclusive")=false))) \
      .def("hasLower", &BoundedValidator<ElementType>::hasLower, "Returns True if a lower bound has been set" ) \
      .def("hasUpper", &BoundedValidator<ElementType>::hasUpper, "Returns True if an upper bound has been set" ) \
      .def("isLowerExclusive", &BoundedValidator<ElementType>::isLowerexclusive, "Returns True if the lower bound is exclusive" ) \
      .def("isUpperExclusive", &BoundedValidator<ElementType>::isUpperexclusive, "Returns True if the upper bound is exclusive" ) \
    ;
}

void export_BoundedValidator()
{
  EXPORT_BOUNDEDVALIDATOR(double, Float);
  EXPORT_BOUNDEDVALIDATOR(long, Int);
}

