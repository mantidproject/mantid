// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/OrientedLatticeValidator.h"
#include "MantidPythonInterface/core/TypedValidatorExporter.h"
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
