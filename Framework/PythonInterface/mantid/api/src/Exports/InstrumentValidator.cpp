// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/InstrumentValidator.h"
#include "MantidPythonInterface/core/TypedValidatorExporter.h"
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
