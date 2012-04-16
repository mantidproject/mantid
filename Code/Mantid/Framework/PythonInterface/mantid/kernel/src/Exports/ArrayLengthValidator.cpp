#include "MantidKernel/ArrayLengthValidator.h"
#include <boost/python/class.hpp>

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
    ;

}

void export_ArrayLengthValidator()
{
  EXPORT_LENGTHVALIDATOR(double, Float);
  EXPORT_LENGTHVALIDATOR(int, Int);
  EXPORT_LENGTHVALIDATOR(std::string, String);
}

