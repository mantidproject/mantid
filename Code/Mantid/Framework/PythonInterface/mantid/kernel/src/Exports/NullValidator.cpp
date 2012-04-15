#include "MantidKernel/NullValidator.h"
#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"
#include <boost/python/class.hpp>

using Mantid::Kernel::NullValidator;
using Mantid::Kernel::IValidator;
using namespace boost::python;

void export_NullValidator()
{
  REGISTER_SHARED_PTR_TO_PYTHON(NullValidator);

  class_<NullValidator, bases<IValidator>, boost::noncopyable>("NullValidator")
    ;
}

