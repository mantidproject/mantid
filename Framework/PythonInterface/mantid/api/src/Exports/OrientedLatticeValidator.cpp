#include "MantidAPI/OrientedLatticeValidator.h"
#include "MantidPythonInterface/kernel/TypedValidatorExporter.h"
#include <boost/python/class.hpp>

using namespace Mantid::API;
using Mantid::PythonInterface::TypedValidatorExporter;
using namespace boost::python;

// This is typed on the ExperimentInfo class
void export_OrientedLatticeValidator() {
  TypedValidatorExporter<ExperimentInfo_sptr>::define(
      "OrientedLatticeValidator");

  class_<OrientedLatticeValidator,
         bases<Mantid::Kernel::TypedValidator<ExperimentInfo_sptr>>,
         boost::noncopyable>(
      "OrientedLatticeValidator",
      init<>("Checks that the workspace has an orientation matrix defined"));
}
