#include "MantidKernel/IValidator.h"
#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"
#include <boost/python/class.hpp>

using Mantid::Kernel::IValidator;
using namespace boost::python;

void export_IValidator()
{
  REGISTER_SHARED_PTR_TO_PYTHON(IValidator);

  class_<IValidator, boost::noncopyable>("IValidator", no_init)
    ;
}

