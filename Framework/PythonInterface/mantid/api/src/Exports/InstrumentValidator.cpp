#include "MantidAPI/InstrumentValidator.h"
#include "MantidPythonInterface/kernel/TypedValidatorExporter.h"
#include <boost/python/class.hpp>

using namespace Mantid::API;
using Mantid::PythonInterface::TypedValidatorExporter;
using namespace boost::python;

// This is typed on the ExperimentInfo class
void export_InstrumentValidator() {
  TypedValidatorExporter<ExperimentInfo_sptr>::define(
      "ExperimentInfoValidator");

  class_<InstrumentValidator,
         bases<Mantid::Kernel::TypedValidator<ExperimentInfo_sptr>>,
         boost::noncopyable>(
      "InstrumentValidator",
      init<>("Checks that the workspace has an instrument defined"));
}
