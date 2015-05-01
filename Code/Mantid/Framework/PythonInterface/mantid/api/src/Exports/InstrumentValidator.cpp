#include "MantidAPI/WorkspaceValidators.h"
#include "MantidPythonInterface/kernel/TypedValidatorExporter.h"
#include <boost/python/class.hpp>

using namespace Mantid::API;
using Mantid::PythonInterface::TypedValidatorExporter;
using namespace boost::python;


// This is typed on the ExperimentInfo class
// clang-format off
void export_InstrumentValidator()
// clang-format on
{
  TypedValidatorExporter<ExperimentInfo_sptr>::define("ExperimentInfoValidator");

  class_<InstrumentValidator, bases<Mantid::Kernel::TypedValidator<ExperimentInfo_sptr> >,
          boost::noncopyable
         >("InstrumentValidator", init<>("Checks that the workspace has an instrument defined"))
   ;
}
