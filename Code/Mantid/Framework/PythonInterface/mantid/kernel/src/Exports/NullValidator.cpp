#include "MantidKernel/NullValidator.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Kernel::NullValidator;
using Mantid::Kernel::IValidator;
using namespace boost::python;

// clang-format off
void export_NullValidator()
// clang-format on
{
  register_ptr_to_python<boost::shared_ptr<NullValidator>>();

  class_<NullValidator, bases<IValidator>, boost::noncopyable>("NullValidator")
    ;
}

