#include "MantidAPI/WorkspaceValidators.h"
#include "MantidPythonInterface/kernel/TypedValidatorExportMacro.h"
#include <boost/python/class.hpp>

// This is typed on the ExperimentInfo class
void export_InstrumentValidator()
{
  using namespace Mantid::API;
  using namespace boost::python;
  EXPORT_TYPEDVALIDATOR(ExperimentInfo_sptr);
  class_<InstrumentValidator, bases<Mantid::Kernel::TypedValidator<ExperimentInfo_sptr>>,
          boost::noncopyable
         >("InstrumentValidator", init<>("Checks that the workspace has an instrument defined"))
   ;
}