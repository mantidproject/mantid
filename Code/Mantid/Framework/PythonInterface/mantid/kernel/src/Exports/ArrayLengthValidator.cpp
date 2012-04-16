#include "MantidKernel/ArrayLengthValidator.h"
#include <boost/python/class.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/copy_const_reference.hpp>

using Mantid::Kernel::ArrayLengthValidator;
using Mantid::Kernel::IValidator;
using namespace boost::python;

namespace
{

  #define EXPORT_LENGTHVALIDATOR(type, prefix) \
    class_<ArrayLengthValidator<type>, bases<IValidator>, \
           boost::noncopyable \
          >(#prefix"ArrayLengthValidator") \
      .def(init<int>(arg("length"), "Constructs a validator verifying that an array is of the exact length given"))\
      .def(init<int,int>((arg("lenmin"),arg("lenmax")), "Constructs a validator verifying that the length of an array is within the range given"))\
      .def("hasLength", &ArrayLengthValidator<type>::hasLength, "Returns true if a single length has been set")\
      .def("hasMinLength", &ArrayLengthValidator<type>::hasMinLength, "Returns true if a minimum length has been set")\
      .def("hasMaxLength", &ArrayLengthValidator<type>::hasMaxLength, "Returns true if a maximum length has been set")\
      .def("getLength", &ArrayLengthValidator<type>::getLength, return_value_policy<copy_const_reference>(), \
           "Returns the set fixed length")\
      .def("getMinLength", &ArrayLengthValidator<type>::getMinLength, return_value_policy<copy_const_reference>(), \
           "Returns the set minimum length")\
      .def("getMaxLength", &ArrayLengthValidator<type>::getMaxLength, return_value_policy<copy_const_reference>(), \
           "Returns the set maximum length")\
      .def("setLength", &ArrayLengthValidator<type>::setLength, "Set the accepted length of an array") \
      .def("clearLength", &ArrayLengthValidator<type>::clearLength, "Clears accepted length of an array") \
      .def("setLengthMin", &ArrayLengthValidator<type>::setLengthMin, "Set the accepted minimum length of an array") \
      .def("setLengthMax", &ArrayLengthValidator<type>::setLengthMax, "Set the accepted maximum length of an array") \
      .def("clearLengthMin", &ArrayLengthValidator<type>::clearLengthMin, "Set the accepted minimum length of an array") \
      .def("clearLengthMax", &ArrayLengthValidator<type>::clearLengthMax, "Set the accepted maximum length of an array") \
  ;
}

void export_ArrayLengthValidator()
{
  EXPORT_LENGTHVALIDATOR(double, Float);
  EXPORT_LENGTHVALIDATOR(long, Int);
  EXPORT_LENGTHVALIDATOR(std::string, String);
}

