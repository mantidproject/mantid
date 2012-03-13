#include "MantidKernel/IValidator.h"
#include <boost/python/class.hpp>

using Mantid::Kernel::IValidator;
using namespace boost::python;

void export_IValidator()
{
  class_<IValidator, boost::noncopyable>("IValidator", no_init)
  ;
}

