#include "MantidKernel/IValidator.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Kernel::IValidator;
using namespace boost::python;

// clang-format off
void export_IValidator()
// clang-format on
{
  register_ptr_to_python<boost::shared_ptr<IValidator>>();

  class_<IValidator, boost::noncopyable>("IValidator", no_init)
    ;
}

