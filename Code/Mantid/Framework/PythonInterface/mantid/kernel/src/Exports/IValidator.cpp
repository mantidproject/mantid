#include "MantidKernel/IValidator.h"
#include <boost/python/class.hpp>

using Mantid::Kernel::IValidator;
using namespace boost::python;

namespace
{
  /// Export an IValidator base class
  #define EXPORT_IVALIDATOR(ElementType,Suffix) \
    class_<IValidator<ElementType>, boost::noncopyable>("IValidator_"#Suffix, no_init) \
      .def("isValid", &IValidator<ElementType>::isValid, "Returns an empty string if the given value is valid. " \
           "Otherwise a user-level error is returned.") \
    ;
}

void export_IValidators()
{
  EXPORT_IVALIDATOR(double,double);
  EXPORT_IVALIDATOR(int,double);
  EXPORT_IVALIDATOR(std::string,std_string);
}

