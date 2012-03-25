#include "MantidKernel/BoundedValidator.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>

using Mantid::Kernel::BoundedValidator;
using Mantid::Kernel::IValidator;
using namespace boost::python;

namespace
{
  /// A macro for generating exports for each type
  #define EXPORT_BOUNDEDVALIDATOR(ElementType) \
    class_<BoundedValidator<ElementType>, bases<IValidator>, \
           boost::noncopyable>("BoundedValidator_"#ElementType) \
      .def(init<ElementType,ElementType>()) \
      .def("setLower", &BoundedValidator<ElementType>::setLower, "Set the lower bound") \
      .def("setUpper", &BoundedValidator<ElementType>::setUpper, "Set the upper bound" ) \
      .def("lower", &BoundedValidator<ElementType>::lower, return_value_policy<copy_const_reference>(), \
           "Returns the lower bound") \
      .def("upper", &BoundedValidator<ElementType>::upper, return_value_policy<copy_const_reference>(), \
           "Returns the upper bound" ) \
      .def("setBounds", &BoundedValidator<ElementType>::setBounds, "Set both bounds" ) \
      .def("hasLower", &BoundedValidator<ElementType>::hasLower, "Returns True if a lower bound has been set" ) \
      .def("hasUpper", &BoundedValidator<ElementType>::hasLower, "Returns True if an upper bound has been set" ) \
    ;

}

void export_BoundedValidator()
{
  EXPORT_BOUNDEDVALIDATOR(double);
  EXPORT_BOUNDEDVALIDATOR(int);
  EXPORT_BOUNDEDVALIDATOR(long);
}

